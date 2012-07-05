/*
 *******************************************************************************
 * Porting to Linux on 20041115
 * Author: Chris Lee
 * Version: 0.2
 * History:
 *          0.1 new creation
 *          0.2 Porting to meet the style of linux dma
 *          0.3 modify dma usage to virtual irq of dma interrupt
 * Todo:
 *******************************************************************************
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/hdreg.h>	/* HDIO_GETGEO */
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/arch/cpe_int.h>
#include "sd.h"
#ifdef CONFIG_CPE_SD_DMA
#include <asm/arch/apb_dma.h>
#endif


/*------------------------------------------------------------------------------
 * Predefine for block device
 */
#define MAJOR_NR			sd_major	/* force definitions on in blk.h */
static int sd_major;					/* must be declared before including blk.h */
#define SD_SHIFT			4			/* max 16 partitions  */
#define DEVICE_NAME			"cpesd" 	/* name for messaging */
#define DEVICE_REQUEST		sd_request
#define DEVICE_NR(device)	(MINOR(device) >> SD_SHIFT)
//#define DEVICE_INTR		sd_intrptr	/* pointer to the bottom half */
#define DEVICE_NO_RANDOM				/* no entropy to contribute */
#define DEVICE_OFF(d)					/* do-nothing */

#include <linux/blk.h>
#include <linux/blkpg.h> /* blk_ioctl() */

/*------------------------------------------------------------------------------
 * Macro definition
 */
#define SDC_W_REG(offset, value)	outl(value, CPE_SD_VA_BASE + offset)
#define SDC_R_REG(offset)			inl(CPE_SD_VA_BASE + offset)


/*------------------------------------------------------------------------------
 * Global variable
 */
static int major = SD_MAJOR;
int sd_size;

/* The following items are obtained through kmalloc() in sd_module_init() */
sd_dev_t *sd_devices = NULL;
int *sd_sizes = NULL;
int *sd_max_sectors = NULL;

struct block_device_operations sd_fops;
/* our genhd structure */
struct gendisk sd_gendisk = {
	major:			0,				/* Major number assigned later */
	major_name:		DEVICE_NAME,	/* Name of the major device */
	minor_shift:	SD_SHIFT,	/* Shift to get device number */
	max_p:			1 << SD_SHIFT,	/* Number of partitions */
	fops:			&sd_fops		/* Block dev operations */
};

sd_card_t sd_card_info;
int sector_offset;
struct hd_struct *sd_partitions = NULL;

#ifdef CONFIG_CPE_SD_DMA /* only used for dma mode */
dma_addr_t dma_buf;
wait_queue_head_t sd_dma_queue;
apb_dma_data_t *priv = NULL;
apb_dma_parm_t parm;
#endif

static uint first_run = 0;
uint sd_err_code;

#define FILE_FORMAT_HARD_DISK_LIKE	0
#define FILE_FORMAT_FLOPPY_LIKE		1
#define FILE_FORMAT_UNIVERSAL		2
#define FILE_FORMAT_UNKNOW			3
#define FILE_FORMAT_RESERVED		4

#define K 1000

uint TAAC_TimeUnitTable[] =	{ // unit is ns
	1, 10, 100, 1 * K, 10 * K, 100 * K, 1 * K * K, 10 * K * K
};

uint TRANS_SPEED_RateUintTable[] = {
	100 * K, 1 * K * K, 10 * K * K, 100 * K * K
};

uint TRANS_SPEED_TimeValueTable_u[] = { // unit=1/10
	0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80
};

uint VDD_CURR_MIN_Table_u[] = { // unit=1/10
	5, 10, 50, 100, 250, 350, 600, 1000
};

uint VDD_CURR_MAX_Table_u[] = {
	1, 5, 10, 25, 35, 45, 80, 200
};

uint TAAC_TimeValueTable_u[] = { // unit=1/10
	0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80
};


/*------------------------------------------------------------------------------
 * Local declaration of function
 */
int sd_revalidate(kdev_t i_rdev);
int sd_card_setup(void);
int sd_check_err(uint status);
int sd_get_scr(sd_card_t *info, uint *scr);
int sd_set_transfer_state(sd_card_t *info);
uint sd_block_size_convert(uint size);
int sd_read_sector(sd_card_t *info, uint addr, uint count, unchar *buf);


/*------------------------------------------------------------------------------
 * Local function
 */

/*
 * SD host controller operation
 */
int sdc_send_cmd(uint cmd, uint arg, uint *rsp)
{
	int i;
	uint status, count = 0;

	/* clear command relative bits of status register */
	SDC_W_REG(SDC_CLEAR_REG, SDC_STATUS_REG_RSP_CRC_FAIL | SDC_STATUS_REG_RSP_TIMEOUT | SDC_STATUS_REG_RSP_CRC_OK| SDC_STATUS_REG_CMD_SEND);
	/* write argument to arugument register if necessary */
	SDC_W_REG(SDC_ARGU_REG, arg);
	/* send command */
	SDC_W_REG(SDC_CMD_REG, cmd | SDC_CMD_REG_CMD_EN);

	/* wait for the CMD_SEND bit of status register is set */
	while (count++ < SDC_GET_STATUS_RETRY_COUNT) {
		status = SDC_R_REG(SDC_STATUS_REG);
		if (!(cmd & SDC_CMD_REG_NEED_RSP)) {
			/* if this command does not need response, wait command sent flag */
			if (status & SDC_STATUS_REG_CMD_SEND) {
				/* clear command sent bit */
				SDC_W_REG(SDC_CLEAR_REG, SDC_STATUS_REG_CMD_SEND);
				sd_err_code = ERR_NO_ERROR;
				return TRUE;
			}
		} else {
			/* if this command needs response */
			if (status & SDC_STATUS_REG_RSP_TIMEOUT) {
				/* clear response timeout bit */
				SDC_W_REG(SDC_CLEAR_REG, SDC_STATUS_REG_RSP_TIMEOUT);
				sd_err_code = ERR_RSP_TIMEOUT_ERROR;
				printk("%s() ERR_RSP_TIMEOUT_ERROR\n", __func__);
				return FALSE;
			} else if (status & SDC_STATUS_REG_RSP_CRC_FAIL) {
				/* clear response fail bit */
				SDC_W_REG(SDC_CLEAR_REG, SDC_STATUS_REG_RSP_CRC_FAIL);
				sd_err_code = ERR_RSP_CRC_ERROR;
				printk("%s() ERR_RSP_CRC_ERROR\n", __func__);
				return FALSE;
			} else if (status & SDC_STATUS_REG_RSP_CRC_OK) {
				/* clear response OK bit */
				SDC_W_REG(SDC_CLEAR_REG, SDC_STATUS_REG_RSP_CRC_OK);
				/* if it is long response */
				if (cmd & SDC_CMD_REG_LONG_RSP)
					for (i = 0; i < 4; i++, rsp++)
						*rsp = SDC_R_REG(SDC_RESPONSE0_REG + (i * 4));
				else
					*rsp = SDC_R_REG(SDC_RESPONSE0_REG);
				sd_err_code = ERR_NO_ERROR;
				return TRUE;
			}
		}
	}
	sd_err_code = ERR_SEND_COMMAND_TIMEOUT;
	P_DEBUG("%s() ERR_SEND_COMMAND_TIMEOUT\n", __func__);
	return FALSE;
}

int sdc_check_tx_ready(void)
{
	uint status;
	int count = 0;

	while (count++ < SDC_GET_STATUS_RETRY_COUNT) {
		status = SDC_R_REG(SDC_STATUS_REG);
		if (status & SDC_STATUS_REG_FIFO_UNDERRUN) {
			/* clear FIFO underrun bit */
			SDC_W_REG(SDC_CLEAR_REG, SDC_STATUS_REG_FIFO_UNDERRUN);
			return TRUE;
		} else if (status & SDC_STATUS_REG_DATA_TIMEOUT) {
			/* clear data timeout bit */
			printk("Wait Write FIFO TimeOut\n");
			SDC_W_REG(SDC_CLEAR_REG, SDC_STATUS_REG_DATA_TIMEOUT);
			sd_err_code = ERR_DATA_TIMEOUT_ERROR;
			return FALSE;
		}
	}
	sd_err_code = ERR_WAIT_UNDERRUN_TIMEOUT;
	P_DEBUG("%s() ERR_WAIT_UNDERRUN_TIMEOUT\n", __func__);
	return FALSE;
}

int sdc_check_rx_ready(void)
{
	uint status;
	int count = 0;

	while (count++ < SDC_GET_STATUS_RETRY_COUNT) {
		status = SDC_R_REG(SDC_STATUS_REG);
		if (status & SDC_STATUS_REG_FIFO_OVERRUN) {
			/* clear FIFO overrun bit */
			SDC_W_REG(SDC_CLEAR_REG, SDC_STATUS_REG_FIFO_OVERRUN);
			return TRUE;
		} else if (status & SDC_STATUS_REG_DATA_TIMEOUT) {
			/* clear data timeout bit */
			printk("Wait Read FIFO TimeOut\n");
			SDC_W_REG(SDC_CLEAR_REG, SDC_STATUS_REG_DATA_TIMEOUT);
			sd_err_code = ERR_DATA_TIMEOUT_ERROR;
			return FALSE;
		}
	}
	sd_err_code = ERR_WAIT_OVERRUN_TIMEOUT;
	P_DEBUG("%s() ERR_WAIT_OVERRUN_TIMEOUT\n", __func__);
	return FALSE;
}

int sdc_check_data_crc(void)
{
	uint status;
	int count = 0;

	while (count++ < SDC_GET_STATUS_RETRY_COUNT) {
		status = SDC_R_REG(SDC_STATUS_REG);
		if (status & SDC_STATUS_REG_DATA_CRC_OK) {
			P_DEBUGG("%s : receive data ok, status=0x%x\n", __func__, status);
			/* clear data CRC OK bit */
			SDC_W_REG(SDC_CLEAR_REG, SDC_STATUS_REG_DATA_CRC_OK);
			return TRUE;
		} else if (status & SDC_STATUS_REG_DATA_CRC_FAIL) {
			/* clear data CRC fail bit */
			SDC_W_REG(SDC_CLEAR_REG, SDC_STATUS_REG_DATA_CRC_FAIL);
			sd_err_code = ERR_DATA_CRC_ERROR;
			printk("%s() ERR_DATA_CRC_ERROR\n", __func__);
			return FALSE;
		} else if (status & SDC_STATUS_REG_DATA_TIMEOUT) {
			/* clear data timeout bit */
			SDC_W_REG(SDC_CLEAR_REG, SDC_STATUS_REG_DATA_TIMEOUT);
			sd_err_code = ERR_DATA_TIMEOUT_ERROR;
			printk("%s() ERR_DATA_TIMEOUT_ERROR\n", __func__);
			return FALSE;
		}
	}
	P_DEBUG("%s() ERR_WAIT_DATA_CRC_TIMEOUT, status=0x%x\n", __func__, status);
	sd_err_code = ERR_WAIT_DATA_CRC_TIMEOUT;
	return FALSE;
}

int sdc_check_data_end(void)
{
	uint status;
	int count = 0;

	while (count++ < SDC_GET_STATUS_RETRY_COUNT) {
		status = SDC_R_REG(SDC_STATUS_REG);
		if (status & SDC_STATUS_REG_DATA_END) {
			SDC_W_REG(SDC_CLEAR_REG, SDC_STATUS_REG_DATA_END);
			return TRUE;
		} else if (status & SDC_STATUS_REG_DATA_TIMEOUT) {
			/* clear data timeout bit */
			SDC_W_REG(SDC_CLEAR_REG, SDC_STATUS_REG_DATA_TIMEOUT);
			sd_err_code = ERR_DATA_TIMEOUT_ERROR;
			printk("%s() ERR_DATA_TIMEOUT_ERROR\n", __func__);
			return FALSE;
		}
	}
	sd_err_code = ERR_WAIT_TRANSFER_END_TIMEOUT;
	P_DEBUG("%s() ERR_WAIT_TRANSFER_END_TIMEOUT\n", __func__);
	return FALSE;
}

int sdc_set_bus_width_cmd(sd_card_t *info, uint width)
{
	uint status;

	/* send CMD55 to indicate to the card that the next command is an application specific command */
	if (!sdc_send_cmd(SD_APP_CMD | SDC_CMD_REG_NEED_RSP, (((uint)info->RCA) << 16), &status))
		return FALSE;
	if (!sd_check_err(status))
		return FALSE;
	/* send ACMD6 to set bus width */
	if (!sdc_send_cmd(SD_SET_BUS_WIDTH_CMD | SDC_CMD_REG_APP_CMD | SDC_CMD_REG_NEED_RSP, width, &status))
		return FALSE;
	if (!sd_check_err(status))
		return FALSE;

	return TRUE;
}

int sdc_set_bus_width(sd_card_t *info)
{
	uint width;

	/* if it is not SD card, it does not support wide bus */
	if (info->CardType != MEMORY_CARD_TYPE_SD)
		return TRUE;
	/* get SCR register */
	if (!sd_get_scr(info, (uint *) &info->SCR))
		return FALSE;
	/* if host controller does not support wide bus, return */
	if ((SDC_R_REG(SDC_BUS_WIDTH_REG) & SDC_WIDE_BUS_SUPPORT) != SDC_WIDE_BUS_SUPPORT)
		return TRUE;
	if (!sd_set_transfer_state(info))
		return FALSE;
	if (info->SCR.SD_BUS_WIDTH & SD_SCR_4_BIT_BIT)
		width = SD_BUS_WIDTH_4_BIT;
	else
		width = SD_BUS_WIDTH_1_BIT;
	if (!sdc_set_bus_width_cmd(info, width))
		return FALSE;
	if (width == SD_BUS_WIDTH_1_BIT)
		SDC_W_REG(SDC_BUS_WIDTH_REG, SDC_BUS_WIDTH_REG_SINGLE_BUS);
	else
		SDC_W_REG(SDC_BUS_WIDTH_REG, SDC_BUS_WIDTH_REG_WIDE_BUS);

	return TRUE;
}

uint sdc_set_bus_clock(sd_card_t *info, uint clock)
{
	uint div = 0, reg;

	while (clock < (info->SysFrequency / (2 * (div + 1))))
		div++;
	/* write clock divided */
	reg = SDC_R_REG(SDC_CLOCK_CTRL_REG);
	reg &= ~SDC_CLOCK_REG_CLK_DIV;
	reg += div & SDC_CLOCK_REG_CLK_DIV;
	SDC_W_REG(SDC_CLOCK_CTRL_REG, reg);

	return info->SysFrequency / (2 * (div + 1));
}

int sdc_set_block_size(uint size)
{
	uint status;

	if (!sdc_send_cmd(SD_SET_BLOCKLEN_CMD | SDC_CMD_REG_NEED_RSP, size, &status))
		return FALSE;
	if (!sd_check_err(status))
		return FALSE;
	return TRUE;
}

void sdc_set_card_type(int type)
{
	uint reg;

	reg = SDC_R_REG(SDC_CLOCK_CTRL_REG);
	reg &= ~SDC_CLOCK_REG_CARD_TYPE;

	if (type == MEMORY_CARD_TYPE_SD)
		reg |= SDC_CARD_TYPE_SD;
	else
		reg |= SDC_CARD_TYPE_MMC;

	SDC_W_REG(SDC_CLOCK_CTRL_REG, reg);
}

int sdc_read_block(sd_card_t *info, uint size, uint *buf)
{
	uint count, i;

#ifdef CONFIG_CPE_SD_DMA
	if ((info->DMAEnable) && (size >= (SDC_READ_FIFO_LEN * 4))) {
		P_DEBUG("%s() DMA Read\n", __func__);
		P_DEBUG("dma_buf = %d\n", dma_buf);
		//prepare parameter for add dma entry
		parm.src = CPE_SD_BASE + SDC_DATA_WINDOW_REG;	// given source phy addr
		if (dma_buf)
			parm.dest = dma_buf;						// given dest phy addr
		else
			parm.dest = __pa(buf);
		parm.width = APBDMA_WIDTH_32BIT;				// data width of transfer
		parm.req_num = SDC_APBDMA_CHAL;	   	        // hardware request number
		parm.sctl = APBDMA_CTL_FIX;						// source address increment
		parm.dctl = APBDMA_CTL_INC16;					// destination address increment
		parm.stype = APBDMA_TYPE_APB;					// indicate source device, AHB or APB
		parm.dtype = APBDMA_TYPE_AHB;					// indicate destinate device, AHB of APB
		parm.size = size >> 4;							// transfer count, transer size is parm.size*parm.sctl
		parm.burst = TRUE;								// yes/no burst
		parm.irq = APBDMA_TRIGGER_IRQ;					// trigger irq enable/disable

		apb_dma_add(priv,&parm); // call add function
 	    apb_dma_start(priv); // start dma
		interruptible_sleep_on(&sd_dma_queue);
		P_DEBUG("DMA Read interruptible_sleep_on()\n");
	}
	else
#endif
	{
		while (size > 0) {
			if (!sdc_check_rx_ready()) {
				printk("error...........\n");
				return FALSE;
			}
			/* read data from FIFO */
			if (size >= (SDC_READ_FIFO_LEN << 2))
				count = SDC_READ_FIFO_LEN;
			else
				count = size >> 2;
			/* read data from FIFO */
			for (i = 0; i < count; i++, buf++)
				*buf = SDC_R_REG(SDC_DATA_WINDOW_REG);
			size -= (count << 2);
		}
	}
	return sdc_check_data_crc();
}

int sdc_write_block(sd_card_t *info, uint size, uint *buf)
{
	uint count, i;

#ifdef CONFIG_CPE_SD_DMA
	if (info->DMAEnable) {
		P_DEBUG("%s : DMA Write\n", __func__);
		if (dma_buf)
			parm.src = dma_buf;
		else
			parm.src = __pa(buf);
		parm.dest = CPE_SD_BASE + SDC_DATA_WINDOW_REG;
		parm.width = APBDMA_WIDTH_32BIT;
		parm.req_num = SDC_APBDMA_CHAL;
		parm.sctl = APBDMA_CTL_INC16;
		parm.dctl = APBDMA_CTL_FIX;
		parm.stype = APBDMA_TYPE_AHB;
		parm.dtype = APBDMA_TYPE_APB;
		parm.size = size >> 4;
		parm.burst = TRUE;
		parm.irq = APBDMA_TRIGGER_IRQ;

		apb_dma_add(priv,&parm);
		apb_dma_start(priv);
		interruptible_sleep_on(&sd_dma_queue);
		P_DEBUG("DMA Read sleep_on()\n");
	}
	else
#endif /* #ifdef CONFIG_CPE_SD_DMA */
	{
		while (size > 0) {
			if (!sdc_check_tx_ready())
				return FALSE;
			/* read data from FIFO */
			if (size >= (SDC_WRITE_FIFO_LEN << 2))
				count = SDC_WRITE_FIFO_LEN;
			else
				count = (size >> 2) ;
			/* read data from FIFO */
			for (i = 0; i < count; i++, buf++)
				SDC_W_REG(SDC_DATA_WINDOW_REG, *buf);
			size -= (count << 2);
		}
	}
	return sdc_check_data_crc();
}

void sdc_config_transfer(sd_card_t *SDCard, uint len, uint size, uint rw, uint timeout)
{
	/* write timeout */
	SDC_W_REG(SDC_DATA_TIMER_REG, timeout * 2);
	/* set data length */
	SDC_W_REG(SDC_DATA_LEN_REG, len);
	/* set data block */
	if (SDCard->DMAEnable) {
		P_DEBUG("%s() transfer DMA mode\n", __func__);
		SDC_W_REG(SDC_DATA_CTRL_REG, sd_block_size_convert(size) | SDC_DATA_CTRL_REG_DMA_EN | rw | SDC_DATA_CTRL_REG_DATA_EN);
	} else {
		P_DEBUG("%s() transfer nonDMA mode\n", __func__);
		SDC_W_REG(SDC_DATA_CTRL_REG, sd_block_size_convert(size) | rw | SDC_DATA_CTRL_REG_DATA_EN);
	}
}

void sdc_reset(void)
{
	uint ret;

	/* reset host interface */
	SDC_W_REG(SDC_CMD_REG, SDC_CMD_REG_SDC_RST);

	/* loop, until the reset bit is clear */
	do {
		ret = SDC_R_REG(SDC_CMD_REG);
	} while ((ret & SDC_CMD_REG_SDC_RST) != 0);

	udelay(10000);
}

/*
 * SD card operation
 */
void sd_endian_change(uint *dt, int len)
{
	uint ul;

	for(; len > 0; len--, dt++)	{
		ul = *dt;
		((unchar *)dt)[0] = ((unchar *)&ul)[3];
		((unchar *)dt)[1] = ((unchar *)&ul)[2];
		((unchar *)dt)[2] = ((unchar *)&ul)[1];
		((unchar *)dt)[3] = ((unchar *)&ul)[0];
	}
}

int sd_get_ocr(sd_card_t *info, uint hocr, uint *cocr)
{
	uint status;
	int count = 0;

	do {
		if (info->CardType == MEMORY_CARD_TYPE_SD) {
			/* send CMD55 to indicate to the card that the next command is an application specific command */
			if (!sdc_send_cmd(SD_APP_CMD | SDC_CMD_REG_NEED_RSP, ((uint) info->RCA) << 16, &status))
				return FALSE;
			if (!sd_check_err(status))
				return FALSE;
			/* send ACMD41 to get OCR register */
			if (!sdc_send_cmd(SD_APP_OP_COND | SDC_CMD_REG_APP_CMD | SDC_CMD_REG_NEED_RSP, (uint) hocr, (uint *) cocr))
				return FALSE;
		} else {
			/* send CMD1 to get OCR register */
			if (!sdc_send_cmd(SD_MMC_OP_COND | SDC_CMD_REG_NEED_RSP, (uint) hocr, (uint *) cocr))
				return FALSE;
		}
		if (count++ > SD_CARD_GET_OCR_RETRY_COUNT) {
			sd_err_code = ERR_SD_CARD_IS_BUSY;
			printk("%s : ERR_SD_CARD_IS_BUSY\n", __func__);
			return FALSE;
		}
	} while ((*cocr & SD_OCR_BUSY_BIT) != SD_OCR_BUSY_BIT);

	return TRUE;
}

int sd_get_scr(sd_card_t *info, uint *scr)
{
	uint status;

	if (!sd_set_transfer_state(info))
		return FALSE;
	if (!sdc_set_block_size(8))
		return FALSE;
	sdc_config_transfer(info, 8, 8, SDC_DATA_CTRL_REG_DATA_READ, 0xFFFFFFFF);
	/* send CMD55 to indicate to the card that the next command is an application specific command */
	if (!sdc_send_cmd(SD_APP_CMD | SDC_CMD_REG_NEED_RSP, ((uint) info->RCA) << 16, &status))
		return FALSE;
	if (!sd_check_err(status))
		return FALSE;
	/* send ACMD51 to get SCR */
	if (!sdc_send_cmd(SD_SEND_SCR_CMD | SDC_CMD_REG_APP_CMD | SDC_CMD_REG_NEED_RSP, 0, &status))
		return FALSE;
	if (!sd_check_err(status))
		return FALSE;
	if (!sdc_read_block(info, 8, (uint *) scr))
		return FALSE;
	if (!sdc_check_data_end())
		return FALSE;
	sd_endian_change(scr, 2);

	return TRUE;
}

static int sd_get_mbr(struct inode *inode)
{
	int ret = 0, i;
	unchar *buf = NULL;

	P_DEBUG("--> %s\n", __func__);
	sector_offset = 0;
#ifdef CONFIG_CPE_SD_DMA
	buf = consistent_alloc(GFP_KERNEL|GFP_DMA, SD_SECTOR_SIZE, &dma_buf);
#else
	buf = kmalloc(SD_SECTOR_SIZE, GFP_KERNEL);
#endif
	if (!buf) {
		printk("kmalloc failed");
		return -1;
	}
	for (i = 0; i < SD_SECTOR_SIZE; i++)
		*(buf+i) = 0xAA;
	ret = sd_read_sector(&sd_card_info, 0, 1, buf);
#ifdef CONFIG_CPE_SD_DMA
	//dma_buf = 0; /* chris modified */
#endif
	if (ret <= 0) {
		printk("sd_read_sector() error!\n");
		ret = -1;
		goto getMBR_error;
	}
	if (buf[510] != 0x55 || buf[511] != 0xaa) {
		printk("Unformat Card\n");
		goto getMBR_error;
	}
	sector_offset = (*(buf+0x1C6)) | (*(buf+0x1C7))<<8 | (*(buf+0x1C8))<<16 | (*(buf+0x1C9))<<24;

        /*device->dev_size = (*(buf+0x1CA))    |
			   (*(buf+0x1CB))<<8 |
			   (*(buf+0x1CC))<<16|
			   (*(buf+0x1CD))<<24;
        device->sect_num = device->dev_size;*/ /* only for testing */

	if (buf[446] != 0x00 && buf[446] != 0x80)
		sector_offset=0;
	if (buf[450] != 0x01 && buf[450] != 0x04 && buf[450] != 0x06)
		sector_offset=0;
	if (sector_offset != 0)
		sd_gendisk.part[MINOR(inode->i_rdev)].nr_sects = (*(buf+0x1CA)) | (*(buf+0x1CB))<<8 | (*(buf+0x1CC))<<16 | (*(buf+0x1CD))<<24;
	printk("Total sector number is 0x%lx\n", sd_gendisk.part[MINOR (inode->i_rdev)].nr_sects);
	printk("lba_sec_offet is 0x%x\n", sector_offset);

getMBR_error:
#ifdef CONFIG_CPE_SD_DMA
	consistent_free(buf, 512, dma_buf);
	dma_buf = 0;
#else
	kfree(buf);
#endif
	P_DEBUG("<-- %s\n", __func__);
	return ret;
}

int sd_check_err(uint status)
{
	if (status & SD_STATUS_ERROR_BITS) {
		sd_err_code = ERR_SD_CARD_STATUS_ERROR;
		printk("%s() ERR_SD_CARD_STATUS_ERROR %X\n", __func__, status);
		return FALSE;
	}
	sd_err_code = ERR_NO_ERROR;
	return TRUE;
}

int sd_get_card_state(sd_card_t *info, uint *ret)
{
	uint status;

	/* send CMD13 to get card status */
	if (!sdc_send_cmd(SD_SEND_STATUS_CMD | SDC_CMD_REG_NEED_RSP, ((uint) info->RCA) << 16, &status))
		return FALSE;
	if (!sd_check_err(status))
		return FALSE;
	*ret = (status & SD_STATUS_CURRENT_STATE) >> SD_STATUS_CURRENT_STATE_LOC;
	return TRUE;
}

int sd_operation_complete(sd_card_t *info, uint finish)
{
	uint state;
	int count = 0;

	while (count++ < SD_CARD_WAIT_OPERATION_COMPLETE_RETRY_COUNT) {
		if (!sd_get_card_state(info, &state))
			return FALSE;
		if (state == finish)
			return TRUE;
	}
	P_DEBUG("%s() error\n", __func__);
	return FALSE;
}

int sd_stop_transmission(void)
{
	uint status;

	/* send CMD12 to stop transmission */
	if (!sdc_send_cmd(SD_STOP_TRANSMISSION_CMD | SDC_CMD_REG_NEED_RSP, 0, &status))
		return FALSE;

	if (!sd_check_err(status))
		return FALSE;

	return TRUE;
}

int sd_set_card_standby(sd_card_t *info)
{
	uint state;
	int count = 0;

	while (count++ < SD_CARD_STATE_CHANGE_RETRY_COUNT) {
		if (!sd_get_card_state(info, &state))
			return FALSE;

		switch (state) {
		case SD_IDLE_STATE:
		case SD_READY_STATE:
		case SD_IDENT_STATE:
			printk("%s() error\n", __func__);
			return FALSE;
		case SD_DIS_STATE:
			return sd_operation_complete(info, SD_STBY_STATE);
		case SD_TRAN_STATE:
			if (!sdc_send_cmd(SD_SELECT_CARD_CMD, 0, NULL))
				return FALSE;
			break;
		case SD_DATA_STATE:
			if (sd_operation_complete(info, SD_TRAN_STATE))
				return TRUE;
			if (sd_err_code != ERR_NO_ERROR)
				return FALSE;
			if (!sd_stop_transmission())
				return FALSE;
			break;
		case SD_RCV_STATE:
			if (sd_operation_complete(info, SD_TRAN_STATE))
				return TRUE;
			if (sd_err_code != ERR_NO_ERROR)
				return FALSE;
			if (!sd_stop_transmission())
				return FALSE;
			break;
		case SD_PRG_STATE:
			if (!sd_operation_complete(info, SD_TRAN_STATE))
				return FALSE;
			break;
		case SD_STBY_STATE:
			return TRUE;
		}
	}
	P_DEBUG("%s() error\n", __func__);
	return FALSE;
}

uint two_power(uint n)
{
	uint pow = 1;

	for (; n > 0; n--)
		pow <<= 1;
	return pow;
}

int sd_csd_parse(sd_csd_t *csd, uint *csd_word)
{
	sd_csd_bit_t *csd_bit;
	uint mult, blocks, len;

	if ((csd_word[0] & 0x00000001) != 1) {
		sd_err_code = ERR_CSD_REGISTER_ERROR;
		printk("%s() ERR_CSD_REGISTER_ERROR\n", __func__);
		return FALSE;
	}

	csd_bit = (sd_csd_bit_t *) csd_word;
	csd->CSDStructure = csd_bit->CSD_STRUCTURE;
	csd->MMCSpecVersion = csd_bit->MMC_SPEC_VERS;
	csd->TAAC_u = TAAC_TimeValueTable_u[csd_bit->TAAC_TimeValue] * TAAC_TimeUnitTable[csd_bit->TAAC_TimeUnit] / 10;
	csd->NSAC_u = csd_bit->NSAC * 100;
	csd->TransferSpeed = TRANS_SPEED_RateUintTable[csd_bit->TRAN_SPEED_RateUnit] * TRANS_SPEED_TimeValueTable_u[csd_bit->TRAN_SPEED_TimeValue] / 10;
	csd->CardCmdClass = csd_bit->CCC;
	csd->ReadBlockLength = two_power(csd_bit->READ_BL_LEN);
	csd->ReadBlockPartial = csd_bit->READ_BL_PARTIAL;
	csd->WriteBlockMisalign = csd_bit->WRITE_BLK_MISALIGN;
	csd->ReadBlockMisalign = csd_bit->READ_BLK_MISALIGN;
	csd->DSRImplemant = csd_bit->DSR_IMP;
   	mult = 1 << (csd_bit->C_SIZE_MULT + 2);
   	blocks = ((csd_bit->C_SIZE_1 | (csd_bit->C_SIZE_2 << 2)) + 1) * mult;
   	len = 1 << (csd_bit->READ_BL_LEN);
   	csd->BlockNumber = blocks;
	csd->MemorySize = blocks * len;
	csd->VDDReadMin_u = VDD_CURR_MIN_Table_u[csd_bit->VDD_R_CURR_MIN];
	csd->VDDReadMax_u = VDD_CURR_MAX_Table_u[csd_bit->VDD_R_CURR_MAX];
	csd->VDDWriteMin_u = VDD_CURR_MIN_Table_u[csd_bit->VDD_W_CURR_MIN];
	csd->VDDWriteMax_u = VDD_CURR_MAX_Table_u[csd_bit->VDD_W_CURR_MAX];
	csd->EraseBlkEnable = csd_bit->ERASE_BLK_ENABLE;
	csd->EraseSectorSize = csd_bit->ERASE_SECTOR_SIZE + 1;
	csd->WriteProtectGroupSize = csd_bit->WP_GRP_SIZE + 1;
	csd->WriteProtectGroupEnable = csd_bit->WP_GRP_ENABLE;
	csd->WriteSpeedFactor = two_power(csd_bit->R2W_FACTOR);
	csd->WriteBlockLength = two_power(csd_bit->WRITE_BL_LEN);
	csd->WriteBlockPartial = csd_bit->WRITE_BL_PARTIAL;
	csd->CopyFlag = csd_bit->COPY;
	csd->PermanentWriteProtect = csd_bit->PERM_WRITE_PROTECT;
	csd->TemporaryWriteProtect = csd_bit->TMP_WRITE_PROTECT;

	if (csd_bit->FILE_FORMAT_GRP == 0)
		csd->FileFormat = csd_bit->FILE_FORMAT;
	else
		csd->FileFormat = FILE_FORMAT_RESERVED;

	return TRUE;
}

int sd_cid_parse(sd_cid_t *cid, uint *cid_word)
{
	unchar *ptr;
	int i;

	if ((cid_word[0] & 0x00000001) != 1)
	{
		sd_err_code = ERR_CID_REGISTER_ERROR;
		printk("%s() ERR_CID_REGISTER_ERROR\n", __func__);
		return FALSE;
	}

	cid->ManufacturerID = (cid_word[3] & 0xFF000000) >> 24;
	cid->ApplicationID = (cid_word[3] & 0x00FFFF00) >> 8;

	ptr = (unchar *) cid_word;
	ptr += 15 - 3;
	for (i = 0; i < 6; i++, ptr--)
		cid->ProductName[i] = *ptr;
	cid->ProductName[6] = '\0'	;

	cid->ProductRevisionLow = (cid_word[1] & 0x00F00000) >> 20;
	cid->ProductRevisionHigh = (cid_word[1] & 0x000F0000) >> 16;
	cid->ProductSerialNumber = ((cid_word[1] & 0x0000FFFF) << 16) + ((cid_word[0] & 0xFFFF0000) >> 16);
	cid->ManufactureMonth = ((cid_word[0] & 0x00000F00) >> 8);
	cid->ManufactureYear = ((cid_word[0] & 0x0000F000) >> 12) + SD_DEFAULT_YEAR_CODE;

	return TRUE;
}

uint sd_read_timeout_cycle(uint clock, sd_csd_t *csd)
{
	uint ret, total, per;

	per = 1000000000 / clock;
	total = (csd->TAAC_u + (csd->NSAC_u * 100 * per)) * 100;

	if (total > (100 * 1000 * 1000))
		total = 100 * 1000 * 1000;
	ret = total / per;

	return ret;
}

uint sd_block_size_convert(uint size)
{
	uint ret = 0;

	while (size >= 2) {
		size >>= 1;
		ret++;
	}
	return ret;
}

int sd_select_card(sd_card_t *info)
{
	uint status;

	/* send CMD7 with valid RCA to select */
	if (!sdc_send_cmd(SD_SELECT_CARD_CMD | SDC_CMD_REG_NEED_RSP, ((uint)info->RCA) << 16, &status))
		return FALSE;
	if (!sd_check_err(status))
		return FALSE;
	return TRUE;
}

int sd_set_transfer_state(sd_card_t *info)
{
	uint state;
	int count = 0;

	while (count++ < SD_CARD_STATE_CHANGE_RETRY_COUNT) {
		if (!sd_get_card_state(info, &state))
			return FALSE;

		switch (state) {
		case SD_IDLE_STATE:
		case SD_READY_STATE:
		case SD_IDENT_STATE:
			printk("%s() error\n", __func__);
			return FALSE;
		case SD_DIS_STATE:
			if (!sd_operation_complete(info, SD_STBY_STATE))
				return FALSE;
			break;
		case SD_TRAN_STATE:
			return TRUE;
		case SD_DATA_STATE:
			if (sd_operation_complete(info, SD_TRAN_STATE))
				return TRUE;
			if (sd_err_code != ERR_NO_ERROR)
				return FALSE;
			if (!sd_stop_transmission())
				return FALSE;
			break;
		case SD_RCV_STATE:
			if (sd_operation_complete(info, SD_TRAN_STATE))
				return TRUE;
			if (sd_err_code != ERR_NO_ERROR)
				return FALSE;
			if (!sd_stop_transmission())
				return FALSE;
			break;
		case SD_PRG_STATE:
			if (!sd_operation_complete(info, SD_TRAN_STATE))
				return FALSE;
			break;
		case SD_STBY_STATE:
			if (!sd_select_card(info))
				return FALSE;
		}
	}

	P_DEBUG("%s() error\n", __func__);
	return FALSE;
}

uint sd_write_timeout_cycle(uint clock, sd_csd_t *CSD)
{
	uint ret, total, pre;

	pre = 1000000000 / clock;
	total = CSD->WriteSpeedFactor * 100 * (CSD->TAAC_u + (CSD->NSAC_u * 100 * pre));

	if (total > (100 * 1000 * 1000))
		total = 100 * 1000 * 1000;
	ret = total / pre;

	return ret;
}

int sd_card_identify(sd_card_t *info)
{
	uint rca, status, cid[4];

	/* reset all cards */
	if (!sdc_send_cmd(SD_GO_IDLE_STATE_CMD, 0, NULL))
		return FALSE;
	/* Do operating voltage range validation */
	/* get OCR register */
	if (!sd_get_ocr(info, SDC_OCR, (uint *) &info->OCR))
		return FALSE;
	/* ckeck the operation conditions */
	if ((info->OCR & SDC_OCR) == 0) {
		sd_err_code = ERR_OUT_OF_VOLF_RANGE;
		return FALSE;
	}

	/* send CMD2 to get CID register */
	if (!sdc_send_cmd(SD_ALL_SEND_CID_CMD | SDC_CMD_REG_NEED_RSP | SDC_CMD_REG_LONG_RSP, 0, cid))
		return FALSE;
	if (info->CardType == MEMORY_CARD_TYPE_SD) {
		/* send CMD3 to get RCA register */
		if (!sdc_send_cmd(SD_SEND_RELATIVE_ADDR_CMD | SDC_CMD_REG_NEED_RSP, 0, &rca))
			return FALSE;
		info->RCA = (ushort) (rca >> 16);
	} else {
		/* so far, we only support one interface, so we can give RCA any value */
		info->RCA = 0x1;
		/* send CMD3 to set RCA register */
		if (!sdc_send_cmd(SD_SEND_RELATIVE_ADDR_CMD | SDC_CMD_REG_NEED_RSP, (info->RCA << 16), &status))
			return FALSE;
		if (!sd_check_err(status))
			return FALSE;
	}

	return TRUE;
}

int sd_card_init(sd_card_t *info)
{
	uint clock;

	P_DEBUG("--> %s\n", __func__);
	if ((SDC_R_REG(SDC_STATUS_REG) & SDC_STATUS_REG_CARD_INSERT) != SDC_CARD_INSERT)
		return FALSE;
	sd_err_code = ERR_NO_ERROR;
	/* At first, set card type to SD */
	info->CardType = MEMORY_CARD_TYPE_SD;
	/* set memory card type */
	sdc_set_card_type(info->CardType);
	/* start card idenfication process */
	if (!sd_card_identify(info)) {
		printk("this is not SD card\n");
		sd_err_code = ERR_NO_ERROR;
		info->CardType = MEMORY_CARD_TYPE_MMC;
		/* set memory card type */
		sdc_set_card_type(info->CardType);
		if (!sd_card_identify(info))
			return FALSE;
	}

	/* get CSD */
	if (!sd_set_card_standby(info))
		return FALSE;
	/* send CMD9 to get CSD register */
	if (!sdc_send_cmd(SD_SEND_CSD_CMD | SDC_CMD_REG_NEED_RSP | SDC_CMD_REG_LONG_RSP, ((uint) info->RCA) << 16, info->CSDWord))
		return FALSE;
	sd_csd_parse(&info->CSD, info->CSDWord);

	if (info->CSD.ReadBlockLength != SD_SECTOR_SIZE) {
		printk("Sector size is mis-matched (SD CSD report=0x%X,SD_SECTOR_SIZE=0x%X)\n", info->CSD.ReadBlockLength, SD_SECTOR_SIZE);
		return FALSE;
	}

	/* get CID */
	/* send CMD10 to get CID register */
	if (!sdc_send_cmd(SD_SEND_CID_CMD | SDC_CMD_REG_NEED_RSP | SDC_CMD_REG_LONG_RSP, ((uint) info->RCA) << 16, info->CIDWord))
		return FALSE;
	sd_cid_parse(&info->CID, info->CIDWord);

	/* Set card bus clock. sdc_set_bus_clock() will give the real card bus clock has been set. */
	clock = sdc_set_bus_clock(info, info->CSD.TransferSpeed);
	info->ReadAccessTimoutCycle = sd_read_timeout_cycle(clock, &(info->CSD));
	info->WriteAccessTimoutCycle = sd_write_timeout_cycle(clock, &(info->CSD));
	/* set bus width */
	if (!sdc_set_bus_width(info))
		return FALSE;

	/* check write protect */
	info->WriteProtect = ((SDC_R_REG(SDC_STATUS_REG) & SDC_STATUS_REG_CARD_LOCK) == SDC_STATUS_REG_CARD_LOCK) ? TRUE : FALSE;
	info->ActiveState = TRUE;
	P_DEBUG("<-- %s\n", __func__);

	return TRUE;
}

int sd_card_insert(sd_card_t *info)
{
	P_DEBUG("--> %s\n", __func__);
	/* reset host interface controller */
	sdc_reset();
	if (!sd_card_init(info)) {
		printk("root initialize failed\n");
		return FALSE;
	}
	/* set interrupt mask register */
	SDC_W_REG(SDC_INT_MASK_REG, SDC_STATUS_REG_CARD_CHANGE);
	P_DEBUG("<-- %s\n", __func__);

	return TRUE;
}

void sd_free(sd_dev_t *dev)
{ 
	//john release_region(CPE_SD_VA_BASE, 0x48); /* return ioport */
	free_irq(IRQ_SDC, dev); /* return irq */
#ifdef CONFIG_CPE_SD_DMA
	//john release_region(priv->channel_base, 0x10);
	apb_dma_free(priv);
	free_irq(VIRQ_SD_APB_DMA, NULL);
#endif
}

int sd_card_remove(sd_card_t *info)
{
	sd_err_code = ERR_NO_ERROR;

	info->ActiveState = FALSE;
	info->WriteProtect = FALSE;
	info->RCA = 0;
	/* reset host interface controller */
	sdc_reset();
	/* set interrupt mask register */
	SDC_W_REG(SDC_INT_MASK_REG, SDC_STATUS_REG_CARD_CHANGE);
	sd_err_code = ERR_CARD_NOT_EXIST;

	return TRUE;
}

void sd_hotswap_interrupt_handler(int irq, void *dev_id, struct pt_regs *dummy)
{
	uint status;
	sd_dev_t *dev = dev_id;

	P_DEBUG("--> %s, irq=%d\n", __func__, irq);
	/* When the card is inserted or removed, we must delay a short time to make sure */
	/* the SDC_STATUS_REG_CARD_INSERT bit of status register is stable */
	udelay(1000);
	status = SDC_R_REG(SDC_STATUS_REG);
	if ((status & SDC_STATUS_REG_CARD_CHANGE) == SDC_STATUS_REG_CARD_CHANGE) {
		SDC_W_REG(SDC_CLEAR_REG, SDC_STATUS_REG_CARD_CHANGE);
		if ((status & SDC_STATUS_REG_CARD_INSERT) == SDC_CARD_INSERT) {
			dev->card_state = SD_CARD_INSERT;
			printk("Card Insert\n");
			sd_card_insert(&sd_card_info);
		} else {
			dev->card_state = SD_CARD_REMOVE;
			printk("Card Remove\n");
			sd_card_remove(&sd_card_info);
		}
	}
	P_DEBUGG("card state=%d\n", dev->card_state);
	P_DEBUG("<-- %s\n", __func__);
}

#ifdef CONFIG_CPE_SD_DMA
void sd_dma_interrupt_handler(int irq, void *dev_id, struct pt_regs *dummy)
{
	u32 ret;

	P_DEBUG("--> %s\n", __func__);
    ret = apbdma_get_status(priv);

	if (ret &INT_ERROR)
	   printk("APB-DMA for SD has someyhing wrong\n");

	if (ret) 
	{
		//INFO("dma transfer finish\n");
		apbdma_clear_int(priv);
		wake_up_interruptible(&sd_dma_queue);
	}

	P_DEBUG("<-- %s\n", __func__);
}
#endif

/*------------------------------------
 * Block-driver specific functions
 */
/*
 * Find the device for this request.
 */
static sd_dev_t *sd_locate_device(const struct request *req)
{
	int devno;
	sd_dev_t *dev;

	P_DEBUG("--> %s\n", __func__);
	/* Check if the minor number is in range */
	devno = DEVICE_NR(req->rq_dev);
	P_DEBUGG("minor=%d\n", devno);
	if (devno >= SD_DEVS) {
		static int count = 0;

		if (count++ < 5) /* print the message at most five times */
			P_DEBUG("request for unknown device\n");
		return NULL;
	}
	dev = sd_devices + devno;
	P_DEBUGG("card_state=%d\n", dev->card_state);
	P_DEBUG("<-- %s\n", __func__);
	return dev;
}

#if 0
int sd_card_check_exist(sd_card_t *info)
{
	/* if card is not exist */
	if ((SDC_R_REG(SDC_STATUS_REG) & SDC_STATUS_REG_CARD_INSERT) != SDC_CARD_INSERT) {
		sd_card_remove(info);
		return FALSE;
	}
	/* if card is not active */
	if (!info->ActiveState)
	{
		return sd_card_insert(info);
	}
	return TRUE;
}
#endif

void sd_reset_host_controller(void)
{
	uint clock, mask, width;

	/* read register */
	clock = SDC_R_REG(SDC_CLOCK_CTRL_REG);
	width = SDC_R_REG(SDC_BUS_WIDTH_REG);
	mask = SDC_R_REG(SDC_INT_MASK_REG);
	/* reset host interface */
	sdc_reset();
	/* restore register */
	SDC_W_REG(SDC_CLOCK_CTRL_REG, clock);
	SDC_W_REG(SDC_BUS_WIDTH_REG, width);
	SDC_W_REG(SDC_INT_MASK_REG, mask);
}

int sd_read_single_block(sd_card_t *info, uint addr, uint size, uint timeout, unchar *buf)
{
	uint status;

	if (!sdc_set_block_size(size))
		return FALSE;

	sdc_config_transfer(info, size, size, SDC_DATA_CTRL_REG_DATA_READ, timeout);

	if (!sdc_send_cmd(SD_READ_SINGLE_BLOCK_CMD | SDC_CMD_REG_NEED_RSP, addr, &status))
		return FALSE;
	if (!sd_check_err(status))
		return FALSE;

#ifdef DELAY_FOR_DMA_READ
	if (first_run==0) {
		udelay(10000);
		first_run=1;
	}
#endif
	if (!sdc_read_block(info, size, (uint *) buf))
		return FALSE;

	if (sd_err_code != ERR_NO_ERROR) {
		printk("%s() error=0x%X\n", __func__, sd_err_code);
		sd_reset_host_controller();
		return FALSE;
	} else {
		if (!sdc_check_data_end()) {
			sd_stop_transmission();
			printk("%s()2 error=0x%X\n", __func__, sd_err_code);
			return FALSE;
		}
	}

	return TRUE;
}

int sd_write_single_block(sd_card_t *info, uint addr, uint size, uint timeout, unchar *buf)
{
	uint status;

	if (!sdc_set_block_size(size))
		return FALSE;

	sdc_config_transfer(info, size, size, SDC_DATA_CTRL_REG_DATA_WRITE, timeout);

	if (!sdc_send_cmd(SD_WRITE_SINGLE_BLOCK_CMD | SDC_CMD_REG_NEED_RSP, addr, &status))
		return FALSE;
	if (!sd_check_err(status))
		return FALSE;

	if (!sdc_write_block(info, size, (uint *) buf))
		return FALSE;
	if (sd_err_code != ERR_NO_ERROR) {
		printk("%s() error=0x%X\n", __func__, sd_err_code);
		sd_reset_host_controller();
		return FALSE;
	} else {
		if (!sdc_check_data_end()) {
			sd_stop_transmission();
			printk("%s()2  error=0x%X\n", __func__, sd_err_code);
			return FALSE;
		}
	}

	return TRUE;
}

int sd_read_multiple_block(sd_card_t *info, uint addr, uint count, uint size, uint timeout, unchar *buf)
{
	uint err, status;

	if (!sdc_set_block_size(size))
		return FALSE;

	sdc_config_transfer(info, count * size, size, SDC_DATA_CTRL_REG_DATA_READ, timeout);

	if (!sdc_send_cmd(SD_READ_MULTIPLE_BLOCK_CMD | SDC_CMD_REG_NEED_RSP, addr, &status))
		return FALSE;
	if (!sd_check_err(status))
		return FALSE;

#ifdef DELAY_FOR_DMA_READ
	if (first_run==0) {
		udelay(10000);
		first_run=1;
	}
#endif
	while (count > 0) {
		if (!sdc_read_block(info, size, (uint *) buf))
			return FALSE;
		count--;
		buf += size;
	}

	if (sd_err_code != ERR_NO_ERROR) {
		err = sd_err_code;
		sd_stop_transmission();
		sd_reset_host_controller();
		sd_err_code |= err;
		printk("%s() error=0x%X\n", __func__, sd_err_code);
		return FALSE;
	} else {
		if (!sdc_check_data_end()) {
			err = sd_err_code;
			sd_stop_transmission();
			sd_err_code |= err;
			printk("%s()2 error=0x%X\n", __func__, sd_err_code);
			return FALSE;
		}
		if (!sd_stop_transmission())
			return FALSE;
	}

	return TRUE;
}

int sd_write_multiple_block(sd_card_t *info, uint addr, uint count, uint size, uint timeout, unchar *buf)
{
	uint ErrorCode, status;

	if(!sdc_set_block_size(size))
		return FALSE;

	sdc_config_transfer(info, count * size, size, SDC_DATA_CTRL_REG_DATA_WRITE, timeout);

	if (!sdc_send_cmd(SD_WRITE_MULTIPLE_BLOCK_CMD | SDC_CMD_REG_NEED_RSP, addr, &status))
		return FALSE;
	if (!sd_check_err(status))
		return FALSE;

	while (count > 0) {
		if (!sdc_write_block(info, size, (uint *) buf))
			return FALSE;
		count--;
		buf += size;
	}

	if (sd_err_code != ERR_NO_ERROR)
	{
		ErrorCode = sd_err_code;
		sd_stop_transmission();
		sd_reset_host_controller();
		sd_err_code |= ErrorCode;
		printk("%s() error=0x%X\n", __func__, sd_err_code);
		return FALSE;
	} else {
		if (!sdc_check_data_end()) {
			ErrorCode = sd_err_code;
			sd_stop_transmission();
			sd_err_code |= ErrorCode;
			printk("%s()2 error=0x%X\n", __func__, sd_err_code);
			return FALSE;
		}
		if (!sd_stop_transmission())
			return FALSE;
	}

	return TRUE;
}

int sd_wait_transfer_state(sd_card_t *info)
{
	uint state;
	int count = 0;

	while (count++ < SD_CARD_WAIT_TRANSFER_STATE_RETRY_COUNT) {
		if (!sd_get_card_state(info, &state))
			return FALSE;

		switch (state) {
		case SD_IDLE_STATE:
		case SD_READY_STATE:
		case SD_IDENT_STATE:
		case SD_DIS_STATE:
		case SD_STBY_STATE:
			printk("%s() error\n", __func__);
			return FALSE;
		case SD_TRAN_STATE:
			return TRUE;
		case SD_DATA_STATE:
		case SD_RCV_STATE:
		case SD_PRG_STATE:
			break;
		}
	}
	sd_err_code = ERR_SD_CARD_IS_BUSY;
	P_DEBUG("%s() ERR_SD_CARD_IS_BUSY\n", __func__);
	return FALSE;
}

/***************************************************************************
SD Card Read/Write/Erase Function
***************************************************************************/
int sd_read_sector(sd_card_t *info, uint addr, uint count, unchar *buf)
{
	int cnt;
	uint start;

	if (count > MAX_READ_SECTOR_NR) {
		P_DEBUG("Readable Block Number Per Commands is 0x%X\n",MAX_READ_SECTOR_NR);
		return FALSE;
	}

	sd_err_code = ERR_NO_ERROR;

#if 0
	if (!sd_card_check_exist(info)) {
		printk("SD card not exist!!\n");
		return FALSE;
	}
#else
	if (!info->ActiveState) {
		P_DEBUG("%s : SD card not active!!\n", __func__);
		return FALSE;
	}
#endif

	if (!sd_set_transfer_state(info))
		return FALSE;

	start = addr * info->CSD.ReadBlockLength;
	cnt = (int) count;

	while (cnt > 0) {
		if (cnt > 1) {
			if (!sd_read_multiple_block(info, start, (cnt > MAX_MULTI_BLOCK_NUM) ? MAX_MULTI_BLOCK_NUM : cnt,
					info->CSD.ReadBlockLength, info->ReadAccessTimoutCycle, buf))
				return FALSE;
		} else {
			if (!sd_read_single_block(info, start, info->CSD.ReadBlockLength, info->ReadAccessTimoutCycle, buf))
				return FALSE;
			return TRUE;
		}

		if (!sd_wait_transfer_state(info))
			return FALSE;

		cnt -= MAX_MULTI_BLOCK_NUM;
		start += MAX_MULTI_BLOCK_NUM * info->CSD.ReadBlockLength;
		buf += MAX_MULTI_BLOCK_NUM * info->CSD.ReadBlockLength;
	}

	return TRUE;
}

int sd_write_sector(sd_card_t *info, uint addr, uint count, unchar *buf)
{
	int cnt;
	uint start;

	if (count > MAX_WRITE_SECTOR_NR) {
		P_DEBUG("Writable Block Number Per Commands is 0x%X\n",MAX_WRITE_SECTOR_NR);
		return FALSE;
	}

	sd_err_code = ERR_NO_ERROR;

#if 0
	if (!sd_card_check_exist(info))
		return FALSE;
#else
	if (!info->ActiveState) {
		P_DEBUG("%s : SD card not active!!\n", __func__);
		return FALSE;
	}
#endif

	if (info->WriteProtect) {
		sd_err_code = ERR_SD_CARD_IS_LOCK;
		printk("Write Protected!!\n");
		return FALSE;
	}
	if (!sd_set_transfer_state(info))
		return FALSE;

	start = addr * info->CSD.WriteBlockLength;
	cnt = (int) count;

	while (cnt > 0) {
		if (cnt > 1) {
			if (!sd_write_multiple_block(info, start, (cnt > MAX_MULTI_BLOCK_NUM) ? MAX_MULTI_BLOCK_NUM : cnt,
					info->CSD.WriteBlockLength, info->WriteAccessTimoutCycle, buf))
				return FALSE;
		} else {
			if (!sd_write_single_block(info, start, info->CSD.WriteBlockLength, info->WriteAccessTimoutCycle, buf))
				return FALSE;
			return TRUE;
		}

		if (!sd_wait_transfer_state(info))
			return FALSE;

		cnt -= MAX_MULTI_BLOCK_NUM;
		start += MAX_MULTI_BLOCK_NUM * info->CSD.ReadBlockLength;
		buf += MAX_MULTI_BLOCK_NUM * info->CSD.ReadBlockLength;
	}

	return TRUE;
}

/*----------------------------------------------
 * Perform an actual transfer.
 */
static int sd_transfer(sd_dev_t *device, const struct request *req)
{
	int size, minor = MINOR(req->rq_dev), status;
	unchar *ptr = req->buffer;
	uint start = req->sector;
	uint count = req->current_nr_sectors;

	P_DEBUGG("%s() cmd=0x%X start=0x%lX count=0x%lX\n", __func__, req->cmd, req->sector, req->current_nr_sectors);
	size = req->current_nr_sectors * SD_SECTOR_SIZE;
	/* Make sure that the transfer fits within the device. */
	if (req->sector + req->current_nr_sectors > sd_partitions[minor].nr_sects) {
		static int count = 0;

		if (count++ < 5)
			printk("request past end of partition\n");
		return 0;
	}
	switch (req->cmd) {
	case READ:
		status = sd_read_sector(&sd_card_info, start+sector_offset, count, ptr);
		if (status <= 0)
			printk ("Read Error\n");
		return status;
	case WRITE:
		status =sd_write_sector(&sd_card_info, start+sector_offset, count, ptr);
		if (status <= 0)
			printk ("Write Error\n");
		return status;
	default:
		/* can't happen */
		return 0;
	}
}

#ifdef SD_DEBUG
uint sd_dev_info(void)
{
	sd_csd_t *CSD;
	sd_cid_t *CID;

	P_DEBUG("============SDCard=====================================\n");
#if 0
	if (!sd_card_check_exist(&sd_card_info))
	{
		P_DEBUG("SD Card does not exist!!!");
		return FALSE;
	}
#else
	if (!sd_card_info.ActiveState) {
		P_DEBUG("%s : SD card not active!!\n", __func__);
		return FALSE;
	}
#endif

	/* print OCR, RCA register */
	P_DEBUG("OCR>> 0x%08X RCA>> 0x%04X\n", (uint) sd_card_info.OCR, sd_card_info.RCA);
	/* print CID register */
	P_DEBUG("CID>> 0x%08X 0x%08X 0x%08X 0x%08X\n", sd_card_info.CIDWord[0], sd_card_info.CIDWord[1], sd_card_info.CIDWord[2], sd_card_info.CIDWord[3]);
	CID = &(sd_card_info.CID);
	P_DEBUG("      MID:0x%02X OID:0x%04X PNM:%s PRV:%d.%d PSN:0x%08X\n", CID->ManufacturerID, CID->ApplicationID, CID->ProductName,
		CID->ProductRevisionHigh, CID->ProductRevisionLow, CID->ProductSerialNumber);
	P_DEBUG("      MDT:%d/%d\n", CID->ManufactureMonth, CID->ManufactureYear);
	/* print CSD register */
	P_DEBUG("CSD>> 0x%08X 0x%08X 0x%08X 0x%08X\n", sd_card_info.CSDWord[0], sd_card_info.CSDWord[1], sd_card_info.CSDWord[2], sd_card_info.CSDWord[3]);
	CSD = &(sd_card_info.CSD);
	P_DEBUG("      CSDStructure:%d Spec.Version:%d\n", CSD->CSDStructure, CSD->MMCSpecVersion);
	P_DEBUG("      TAAC:%dns NSAC:%d clock cycles\n", CSD->TAAC_u, CSD->NSAC_u);
	P_DEBUG("      TransferSpeed:%d bit/s CardCommandClass:0x%03X\n", CSD->TransferSpeed, CSD->CardCmdClass);
	P_DEBUG("      ReadBlLen:%d ReadBlPartial:%X WriteBlkMisalign:%X ReadBlkMisalign:%X\n", CSD->ReadBlockLength, CSD->ReadBlockPartial, CSD->WriteBlockMisalign, CSD->ReadBlockMisalign);
	P_DEBUG("      DSP:%X BlockNumber:%d MemorySize:%d \n", CSD->DSRImplemant, CSD->BlockNumber, CSD->MemorySize);
	P_DEBUG("      VDD_R_MIN:%d/10mA VDD_R_MAX:%dmA\n", (uint) CSD->VDDReadMin_u,  (uint) CSD->VDDReadMax_u);
	P_DEBUG("      VDD_W_MIN:%d/10mA VDD_W_MAX:%dmA\n", (uint) CSD->VDDWriteMin_u, (uint) CSD->VDDWriteMax_u);
	P_DEBUG("      EraseBlkEnable:%d EraseSectorSize:%d WpGrpSize:%d WpGrpEnable:%X\n", CSD->EraseBlkEnable, CSD->EraseSectorSize, CSD->WriteProtectGroupSize, CSD->WriteProtectGroupEnable);
	P_DEBUG("      WriteSpeedFactor:%d WriteBlLen:%d WriteBlPartial:%X\n", CSD->WriteSpeedFactor, CSD->WriteBlockLength, CSD->WriteBlockPartial);
	P_DEBUG("      Copy:%X PermWrProtect:%X TmpWrProtect:%X FileFormat:%X\n", CSD->CopyFlag, CSD->PermanentWriteProtect, CSD->TemporaryWriteProtect, CSD->FileFormat);
	P_DEBUG("      ReadTimoutCycle:0x%08X WriteTimoutCycle:0x%08X\n", sd_card_info.ReadAccessTimoutCycle, sd_card_info.WriteAccessTimoutCycle);
	/* print SCR register */
	P_DEBUG("SCR>> 0x%08X 0x%08X \n", *(((uint *) &sd_card_info.SCR)), *(((uint *) &sd_card_info.SCR) + 1));
	P_DEBUG("      SCR_STRUCTURE:%d, SD_SPEC:%d, Data_status_after_erase:%d\n", sd_card_info.SCR.SCR_STRUCTURE, sd_card_info.SCR.SD_SPEC, sd_card_info.SCR.DATA_STAT_AFTER_ERASE);
	P_DEBUG("      sd_security:%d, SD_BUS_WIDTH:%X\n", sd_card_info.SCR.SD_SECURITY, sd_card_info.SCR.SD_BUS_WIDTH);

	return TRUE;
}
#endif

int sd_card_setup()
{
	uint sd_card_size;
	int i;

	P_DEBUG("--> %s\n", __func__);
	first_run = 0;
	sd_err_code = ERR_NO_ERROR;

	sd_card_info.ActiveState = FALSE;
	sd_card_info.WriteProtect = FALSE;
	sd_card_info.IOAddr = CPE_SD_BASE;
#ifdef CONFIG_CPE_SD_DMA
	sd_card_info.DMAEnable = TRUE;
#else
	sd_card_info.DMAEnable = FALSE;
#endif
	sd_card_info.DMAChannel = PMU_SD_APBDMA_CHANNEL;
	sd_card_info.SysFrequency = CONFIG_SYS_CLK/2;
	sd_card_info.RCA = 0;
	sd_card_info.Drive = 'S';

	if (!sd_card_insert(&sd_card_info))
		return FALSE;
#ifdef SD_DEBUG
	if (!sd_dev_info())
		return FALSE;
#endif
	sd_card_size = sd_card_info.CSD.MemorySize;
	printk("card size=%d mb\n", sd_card_size/1024/1024);

	for (i = 0; i < SD_DEVS; i++) {
		sd_size = sd_card_size / SD_BLKSIZE;		//unit is block, not bytes
		sd_sizes[i << SD_SHIFT] = sd_size;		//paulong 20030604
		sd_partitions[i << SD_SHIFT].nr_sects =sd_size * (SD_BLKSIZE / SD_SECTOR_SIZE);
		P_DEBUG ("%s() %d-th device, size=%d blks(blks=%d),nr_sects=%ld\n", __func__, i, sd_size, SD_BLKSIZE, sd_partitions[i << SD_SHIFT].nr_sects);
		//sd_devices[i].card_state = SD_CARD_WORK;
		sema_init(&(sd_devices[i].sema), 1); // add by Charles Tsai*/
	}
	P_DEBUG("<-- %s\n", __func__);
	return TRUE;
}

/*
 * Driver stuff
 */
/*------------------------------------
 * The ioctl implementation
 */
int sd_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int size;
	struct hd_geometry geo;

	P_DEBUG ("ioctl 0x%x 0x%lx\n", cmd, arg);
	switch (cmd) {
	case BLKGETSIZE:
		/* Return the device size, expressed in sectors */
		size = sd_gendisk.part[MINOR (inode->i_rdev)].nr_sects;
		__copy_to_user ((long *) arg, &size, sizeof (long));
		return 0;
	case BLKFLSBUF: /* flush */
		return blk_ioctl(inode->i_rdev, cmd, arg);
	case BLKRAGET: /* return the readahead value */
		return blk_ioctl(inode->i_rdev, cmd, arg);
	case BLKRASET: /* set the readahead value */
		if (!capable (CAP_SYS_RAWIO))
			return -EACCES;
		if (arg > 0xff)
				return -EINVAL; /* limit it */
		read_ahead[MAJOR (inode->i_rdev)] = arg;
		return 0;
	case BLKRRPART:	/* re-read partition table */
		return sd_revalidate (inode->i_rdev);
	case HDIO_GETGEO:
		/*
		 * get geometry: we have to fake one...  trim the size to a
		 * multiple of 64 (32k): tell we have 16 sectors, 4 heads,
		 * whatever cylinders. Tell also that data starts at sector. 4.
		 */
		geo.cylinders = (sd_gendisk.part[MINOR (inode->i_rdev)].nr_sects/4)/8;  /* ?? only for test */
		geo.heads = 4;
		geo.sectors = 8;
		geo.start = 0;
		__copy_to_user ((void *) arg, &geo, sizeof (geo));
		return 0;
	default:
		/*
		 * For ioctls we don't understand, let the block layer handle them.
		 */
		return blk_ioctl (inode->i_rdev, cmd, arg);
	}

	return -ENOTTY; /* unknown command */
}

void sd_request(request_queue_t *q)
{
	sd_dev_t *dev;
	struct request *req;
	int ret;

	P_DEBUG("--> %s\n", __func__);
	/* Locate the device */
	dev = sd_locate_device(blkdev_entry_next_request(&(q->queue_head)));
	if (!dev) {
		printk("sd_locate_device error\n");
		return;
	}
 	/* process the request quueue */
	while (!list_empty(&q->queue_head)) {
		req = blkdev_entry_next_request(&(q->queue_head));
		blkdev_dequeue_request(req);
		spin_unlock_irq(&io_request_lock);
		//spin_lock(&dev->lock);
		down_interruptible(&(dev->sema));
		do {
			ret = sd_transfer(dev, req);
		} while (end_that_request_first(req, ret, DEVICE_NAME));
		up(&(dev->sema));
		//spin_unlock(&dev->lock);
		spin_lock_irq(&io_request_lock);
		end_that_request_last(req);
	}
	P_DEBUG("<-- %s\n", __func__);
}

/*-----------------------------------------
 * Support for removable devices
 */
int sd_check_change(kdev_t i_rdev)
{
	int minor = DEVICE_NR(i_rdev);
	sd_dev_t *dev = sd_devices + minor;

	P_DEBUG("--> %s\n", __func__);
	P_DEBUG("minor=%d\n", minor);
	if (minor >= SD_DEVS) /* paranoid */
		return 0;
	P_DEBUG("check change for dev %d\n", minor);
	if (dev->usage) {
		P_DEBUG("disk not change\n");
		P_DEBUG("<-- %s\n", __func__);
		return 0; /* still valid */
	}
	P_DEBUG("disk changed\n");
	P_DEBUG("<-- %s\n", __func__);
	return 1; /* expired */
}

/*--------------------------------
 * Note no locks taken out here.  In a worst case scenario, we could drop
 * a chunk of system memory.  But that should never happen, since validation
 * happens at open or mount time, when locks are held.
 */
int sd_revalidate(kdev_t i_rdev)
{
	sd_dev_t *dev = sd_devices + DEVICE_NR(i_rdev);

	P_DEBUG("--> %s\n", __func__);
	P_DEBUGG("card state=%s\n", dev->card_state == SD_CARD_INSERT ? "INSERT" : dev->card_state == SD_CARD_WORK ? "WORK" : "REMOVE");

	if (dev->usage == 0) {
		if (sd_card_setup() != TRUE) {
			dev->card_state = SD_CARD_REMOVE;
			return -1;
		} else {
#ifdef CONFIG_CPE_SD_DMA
			if (sd_card_info.DMAEnable) {
				/* require apb dma resource */
				priv = apb_dma_alloc();
				priv->base = CPE_APBDMA_VA_BASE; // APB DMA register virtual base addr
				priv->channel = PMU_SD_APBDMA_CHANNEL; // DMA channel number
				if (!apb_dma_init(priv)) {
					printk("APB DMA initial fail\n");
					sd_free(dev);
					return -1;
				}

				//john
				///* require io port address for apb dma channel */
				//if (request_region(priv->channel_base, 0x10, "PMU_SD_APBDMA_CHANNEL") == NULL) {
				//	printk("request io port of APB DMA %d fail\n", PMU_SD_APBDMA_CHANNEL);
				//	sd_free(dev);
				//	return -1;
				//}

				/* require irq for apb dma channel */
				cpe_int_set_irq (VIRQ_SD_APB_DMA, LEVEL, H_ACTIVE);
				if (request_irq(VIRQ_SD_APB_DMA, sd_dma_interrupt_handler, SA_INTERRUPT, "APB DMA controller", NULL) != 0) {
					printk("APB DMA interrupt request failed, IRQ=%d\n", VIRQ_SD_APB_DMA);
					sd_free(dev);
					return -1;
				} else
					P_DEBUG("APB DMA interrupt request success, IRQ=%d\n", VIRQ_SD_APB_DMA);


				init_waitqueue_head(&sd_dma_queue);
			}
#endif
			// SDC interrupt, currently only for HotSwap
			P_DEBUG("Request SDC IRQ=%d\n", IRQ_SDC);
			cpe_int_set_irq (IRQ_SDC, LEVEL, H_ACTIVE);
			if (request_irq(IRQ_SDC, sd_hotswap_interrupt_handler, SA_INTERRUPT, "SD controller", dev) != 0) {
				printk("Unable to allocate SDC IRQ=0x%X\n", IRQ_SDC);
				sd_free(dev);
				return -1;
			}

			//john
			///* require io port address for sd controller */
			//if (request_region(CPE_SD_VA_BASE, 0x48, "SD Controller") == NULL) {
			//	printk("request io port of sd controller fail\n");
			//	sd_free(dev);
			//	return -1;
			//}

			dev->card_state = SD_CARD_WORK;
		}
	}
	P_DEBUG("<-- %s\n", __func__);

	return 0;
}

/*
 * Device open and close
 */
int sd_open(struct inode *inode, struct file *filp)
{
	sd_dev_t *dev; /* device information */
	int num = DEVICE_NR(inode->i_rdev);

	P_DEBUG("--> %s\n", __func__);
	P_DEBUG("device no=%d\n", num);
	if (num >= SD_DEVS)
		return -ENODEV;

    if ((SDC_R_REG(SDC_STATUS_REG) & SDC_STATUS_REG_CARD_INSERT) != SDC_CARD_INSERT)
       return -ENOMEDIUM;

	dev = sd_devices + num;

	//spin_lock(&dev->lock);
	if (!dev->usage)
		check_disk_change(inode->i_rdev);
	if (sd_get_mbr(inode) < 0) {
		//spin_unlock(&dev->lock);
		sd_free(dev);
		return -ENOMEDIUM;
	}
	blk_size[MAJOR_NR][num] = sd_gendisk.part[MINOR(inode->i_rdev)].nr_sects / 2;
	dev->usage++;
	MOD_INC_USE_COUNT;
	//spin_unlock(&dev->lock);
	P_DEBUG("<-- %s\n", __func__);
	return 0; /* success */
}

int sd_release(struct inode *inode, struct file *filp)
{
	sd_dev_t *dev = sd_devices + DEVICE_NR(inode->i_rdev);

	printk(" + sd_release : umount SD\n");

	P_DEBUG("--> %s\n", __func__);
	//spin_lock(&dev->lock);
	dev->usage--;
	if (!dev->usage) {
		/* but flush it right now */
		fsync_dev(inode->i_rdev);
		invalidate_buffers(inode->i_rdev);
		sd_free(dev);
	}
	MOD_DEC_USE_COUNT;
	//spin_unlock(&dev->lock);
	P_DEBUG("<-- %s\n", __func__);

	return 0;
}

/*--------------------------------------
 * The file operations
 */
struct block_device_operations sd_fops = {
	open:				sd_open,
	release:			sd_release,
	ioctl:				sd_ioctl,
	revalidate:			sd_revalidate,
	check_media_change:	sd_check_change
};

/*
 * module stuff
 */
int sd_module_init(void)
{
	int result, i;
	sd_dev_t *dev = NULL;

#ifdef CONFIG_CPE_SD_DMA
	printk("Faraday CPE SD controller Driver (DMA mode)\n");
#else
	printk("Faraday CPE SD controller Driver (nonDMA mode)\n");
#endif
	sd_major = major;
	sd_size = SD_DUMMY_SIZE / SD_BLKSIZE;

	result = register_blkdev(sd_major, DEVICE_NAME, &sd_fops);
	if (result < 0)	{
		printk("Can't get major %d\n", sd_major);
		return result;
	}
	if (sd_major == 0)
		sd_major = result; /* dynamic */
	P_DEBUG("SD Major Number = %d\n", sd_major);
	printk("make node with 'mknod cpesda b %d 0'\n", sd_major);
	major = sd_major; /* Use 'major' later on to save typing */
	sd_gendisk.major = major; /* was unknown at load time */

	sd_devices = kmalloc(SD_DEVS * sizeof(sd_dev_t), GFP_KERNEL);
	if (!sd_devices)
		goto fail_malloc;
	memset(sd_devices, 0, SD_DEVS * sizeof(sd_dev_t));

	for (i = 0; i < SD_DEVS; i++) {
		dev = sd_devices + i;
		/* data and usage remain zeroed */
		dev->size = SD_BLKSIZE * sd_size;
		dev->usage = 0;
		spin_lock_init(&dev->lock);
		dev->card_state = SD_CARD_REMOVE;
	}
	read_ahead[major] = SD_RAHEAD;
	result = -ENOMEM; /* for the possible errors */

	sd_max_sectors = kmalloc((SD_DEVS << SD_SHIFT) * sizeof(int), GFP_KERNEL);
	if (!sd_max_sectors)
		goto fail_malloc;
	/* Start with zero-sized partitions, and correctly sized units */
	memset(sd_max_sectors, 0, (SD_DEVS << SD_SHIFT) * sizeof(int));
	for (i = 0; i < SD_DEVS; i++)
		sd_max_sectors[i << SD_SHIFT] = MAX_READ_SECTOR_NR;
	max_sectors[MAJOR_NR] = sd_max_sectors;
	result = -ENOMEM;

	sd_sizes = kmalloc((SD_DEVS << SD_SHIFT) * sizeof(int), GFP_KERNEL);
	if (!sd_sizes)
		goto fail_malloc;

	/* Start with zero-sized partitions, and correctly sized units */
	memset(sd_sizes, 0, (SD_DEVS << SD_SHIFT) * sizeof(int));
	for (i = 0; i < SD_DEVS; i++)
		sd_sizes[i << SD_SHIFT] = sd_size;
	blk_size[MAJOR_NR] = sd_gendisk.sizes = sd_sizes;

	/* Allocate the partitions array. */
	sd_partitions = kmalloc((SD_DEVS << SD_SHIFT) * sizeof(struct hd_struct), GFP_KERNEL);
	if (!sd_partitions)
		goto fail_malloc;
	memset(sd_partitions, 0, (SD_DEVS << SD_SHIFT) * sizeof(struct hd_struct));
	sd_gendisk.part = sd_partitions;
	sd_gendisk.nr_real = SD_DEVS;

	add_gendisk(&sd_gendisk);

	blk_init_queue(BLK_DEFAULT_QUEUE(major), sd_request);

	return 0; /* succeed */

fail_malloc:
	read_ahead[major] = 0;
	if (sd_max_sectors)
		kfree(sd_max_sectors);
	max_sectors[major] = NULL;
	if (sd_sizes)
		kfree(sd_sizes);
	if (sd_partitions)
		kfree(sd_partitions);
	blk_size[major] = NULL;
	if (sd_devices)
		kfree(sd_devices);
	unregister_blkdev(major, DEVICE_NAME);
	return result;
}

void sd_module_cleanup(void)
{
	int i;

	P_DEBUG("--> %s\n", __func__);

	/* flush it all and reset all the data structures */
	for (i = 0; i < (SD_DEVS << SD_SHIFT); i++)
		fsync_dev(MKDEV(sd_major, i));	/* flush the devices */
	/* unregister the device now to avoid further operations during cleanup */
	unregister_blkdev(major, DEVICE_NAME);

	blk_cleanup_queue(BLK_DEFAULT_QUEUE(major));
	read_ahead[major] = 0;

	kfree(sd_max_sectors);
	max_sectors[major] = NULL;
	kfree(blk_size[major]);	/* which is gendisk->sizes as well */
	blk_size[major] = NULL;
	kfree(sd_gendisk.part);
	kfree(blksize_size[major]);
	blksize_size[major] = NULL;
	del_gendisk(&sd_gendisk);
	kfree(sd_devices);
	P_DEBUG("<-- %s\n", __func__);
}

module_init(sd_module_init);
module_exit(sd_module_cleanup);
