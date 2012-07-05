#ifndef _SD_H_
#define _SD_H_

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

//#define SD_DEBUG
#define DELAY_FOR_DMA_READ

#ifdef SD_DEBUG
	#define P_DEBUG(fmt, args...) printk(KERN_ALERT "SD: " fmt, ## args)
#else
	#define P_DEBUG(a...)
#endif
#define P_DEBUGG(a...)

#define MAX_READ_SECTOR_NR	16
#define MAX_WRITE_SECTOR_NR	16

#define SD_MAJOR		6 /* default major number, if zero, it means dynamic allocate */
#define SD_DEVS			1 /* number of disks */
#define SD_RAHEAD		2 /* number of sectors */
#define SD_BLKSIZE		1024 /* block size */
#define SD_SECTOR_SIZE	512 /* sector size */
#define SD_DUMMY_SIZE	(256*1024*1024) // for sake of hotswap/hotplug

typedef struct _sd_dev_t {
   int size;
   int usage;
   //struct timer_list timer;
   spinlock_t lock;
   struct semaphore sema;  // synchronization
   int card_state;
} sd_dev_t;

//---------SD Card State
#define SD_CARD_REMOVE	0
#define SD_CARD_INSERT	1
#define SD_CARD_WORK	2

/* so far, SD controller support 3.2-3.3 VDD */
#define SDC_OCR 0x00FF8000

/* sd controller register */
#define SDC_CMD_REG						0x00000000
#define SDC_ARGU_REG					0x00000004
#define SDC_RESPONSE0_REG				0x00000008
#define SDC_RESPONSE1_REG				0x0000000C
#define SDC_RESPONSE2_REG				0x00000010
#define SDC_RESPONSE3_REG				0x00000014
#define SDC_RSP_CMD_REG					0x00000018
#define SDC_DATA_CTRL_REG				0x0000001C
#define SDC_DATA_TIMER_REG				0x00000020
#define SDC_DATA_LEN_REG				0x00000024
#define SDC_STATUS_REG					0x00000028
#define SDC_CLEAR_REG					0x0000002C
#define SDC_INT_MASK_REG				0x00000030
#define SDC_POWER_CTRL_REG				0x00000034
#define SDC_CLOCK_CTRL_REG				0x00000038
#define SDC_BUS_WIDTH_REG				0x0000003C
#define SDC_DATA_WINDOW_REG				0x00000040

/* bit mapping of command register */
#define SDC_CMD_REG_INDEX				0x0000003F
#define SDC_CMD_REG_NEED_RSP			0x00000040
#define SDC_CMD_REG_LONG_RSP			0x00000080
#define SDC_CMD_REG_APP_CMD				0x00000100
#define SDC_CMD_REG_CMD_EN				0x00000200
#define SDC_CMD_REG_SDC_RST				0x00000400

/* bit mapping of response command register */
#define SDC_RSP_CMD_REG_INDEX			0x0000003F
#define SDC_RSP_CMD_REG_APP				0x00000040

/* bit mapping of data control register */
#define SDC_DATA_CTRL_REG_BLK_SIZE		0x0000000F
#define SDC_DATA_CTRL_REG_DATA_WRITE	0x00000010
#define SDC_DATA_CTRL_REG_DATA_READ		0x00000000
#define SDC_DATA_CTRL_REG_DMA_EN		0x00000020
#define SDC_DATA_CTRL_REG_DATA_EN		0x00000040

/* bit mapping of status/clear/mask register */
#define SDC_STATUS_REG_RSP_CRC_FAIL		0x00000001
#define SDC_STATUS_REG_DATA_CRC_FAIL	0x00000002
#define SDC_STATUS_REG_RSP_TIMEOUT		0x00000004
#define SDC_STATUS_REG_DATA_TIMEOUT		0x00000008
#define SDC_STATUS_REG_RSP_CRC_OK		0x00000010
#define SDC_STATUS_REG_DATA_CRC_OK		0x00000020
#define SDC_STATUS_REG_CMD_SEND			0x00000040
#define SDC_STATUS_REG_DATA_END			0x00000080
#define SDC_STATUS_REG_FIFO_UNDERRUN	0x00000100
#define SDC_STATUS_REG_FIFO_OVERRUN		0x00000200
#define SDC_STATUS_REG_CARD_CHANGE		0x00000400
#define SDC_STATUS_REG_CARD_INSERT		0x00000800
#define SDC_STATUS_REG_CARD_LOCK		0x00001000

#define SDC_CARD_INSERT					0x0
#define SDC_CARD_REMOVE					SDC_STATUS_REG_CARD_INSERT

/* bit mapping of power control register */
#define SDC_POWER_REG_POWER_ON			0x00000010
#define SDC_POWER_REG_POWER_BITS		0x0000000F

/* bit mapping of clock control register */
#define SDC_CLOCK_REG_CARD_TYPE			0x00000080
#define SDC_CLOCK_REG_CLK_DIV			0x0000007F

/* card type */
#define SDC_CARD_TYPE_SD				SDC_CLOCK_REG_CARD_TYPE
#define SDC_CARD_TYPE_MMC				0x0

/* bit mapping of bus width register */
#define SDC_BUS_WIDTH_REG_SINGLE_BUS	0x00000001
#define SDC_BUS_WIDTH_REG_WIDE_BUS		0x00000004
#define SDC_WIDE_BUS_SUPPORT			0x00000008

/* data window register */
#define SDC_READ_FIFO_LEN				4
#define SDC_WRITE_FIFO_LEN				4

/* card type, sd or mmc */
#define MEMORY_CARD_TYPE_SD				0
#define MEMORY_CARD_TYPE_MMC			1

/********************************************************************/
/* SYSTEM ERROR_CODE */
/********************************************************************/
#define ERR_NO_ERROR						0x00000000

/* general error */
#define ERR_CARD_NOT_EXIST					0x00000001
#define ERR_OUT_OF_VOLF_RANGE				0x00000002
#define ERR_SD_PARTITIAL_READ_ERROR			0x00000004
#define ERR_SD_PARTITIAL_WRITE_ERROR		0x00000008

#define ERR_SD_CARD_IS_LOCK					0x00000010

/* command error */
#define ERR_DATA_CRC_ERROR					0x00000100
#define ERR_RSP_CRC_ERROR					0x00000200
#define ERR_DATA_TIMEOUT_ERROR				0x00000400
#define ERR_RSP_TIMEOUT_ERROR				0x00000800

#define ERR_WAIT_OVERRUN_TIMEOUT			0x00001000
#define ERR_WAIT_UNDERRUN_TIMEOUT			0x00002000
#define ERR_WAIT_DATA_CRC_TIMEOUT			0x00004000
#define ERR_WAIT_TRANSFER_END_TIMEOUT		0x00008000

#define ERR_SEND_COMMAND_TIMEOUT			0x00010000

/* sd error */
#define ERR_SD_CARD_IS_BUSY					0x00100000
#define ERR_CID_REGISTER_ERROR				0x00200000
#define ERR_CSD_REGISTER_ERROR				0x00400000

/* sd card status error */
#define ERR_SD_CARD_STATUS_ERROR			0x01000000

#define SD_SCR_1_BIT_BIT				0x0001
#define SD_SCR_4_BIT_BIT				0x0004

/********************************************************************/
/* The bit mapping of SD Status register */
/********************************************************************/
#define SD_STATUS_OUT_OF_RANGE				0x80000000
#define SD_STATUS_ADDRESS_ERROR				0x40000000
#define SD_STATUS_BLOCK_LEN_ERROR			0x20000000
#define SD_STATUS_ERASE_SEQ_ERROR			0x10000000
#define SD_STATUS_ERASE_PARAM				0x08000000
#define SD_STATUS_WP_VIOLATION				0x04000000
#define SD_STATUS_CARD_IS_LOCK				0x02000000
#define SD_STATUS_LOCK_UNLOCK_FILED			0x01000000
#define SD_STATUS_COM_CRC_ERROR				0x00800000
#define SD_STATUS_ILLEGAL_COMMAND			0x00400000
#define SD_STATUS_CARD_ECC_FAILED			0x00200000
#define SD_STATUS_CC_ERROR					0x00100000
#define SD_STATUS_ERROR						0x00080000
#define SD_STATUS_UNDERRUN					0x00040000
#define SD_STATUS_OVERRUN					0x00020000
#define SD_STATUS_CID_CSD_OVERWRITE			0x00010000
#define SD_STATUS_WP_ERASE_SKIP				0x00008000
#define SD_STATUS_CARD_ECC_DISABLE			0x00004000
#define SD_STATUS_ERASE_RESET				0x00002000
#define SD_STATUS_CURRENT_STATE				0x00001E00
#define SD_STATUS_READY_FOR_DATA			0x00000100
#define SD_STATUS_APP_CMD					0x00000020
#define SD_STATUS_AKE_SEQ_ERROR				0x00000008
#define SD_STATUS_ERROR_BITS (SD_STATUS_ADDRESS_ERROR | SD_STATUS_BLOCK_LEN_ERROR | SD_STATUS_ERASE_SEQ_ERROR | SD_STATUS_ERASE_PARAM | SD_STATUS_WP_VIOLATION | SD_STATUS_LOCK_UNLOCK_FILED | SD_STATUS_CARD_ECC_FAILED | SD_STATUS_CC_ERROR | SD_STATUS_ERROR | SD_STATUS_UNDERRUN | SD_STATUS_OVERRUN | SD_STATUS_CID_CSD_OVERWRITE | SD_STATUS_WP_ERASE_SKIP | SD_STATUS_AKE_SEQ_ERROR)
#define SD_STATUS_CURRENT_STATE_LOC			9

/********************************************************************/
/* SD command response type */
/********************************************************************/
#define SD_NO_RESPONSE				0
#define SD_RESPONSE_R1				1
#define SD_RESPONSE_R1b				2
#define SD_RESPONSE_R2				3
#define SD_RESPONSE_R3				4
#define SD_RESPONSE_R6				5

/********************************************************************/
/* SD command */
/********************************************************************/
#define SD_GO_IDLE_STATE_CMD		0
#define SD_MMC_OP_COND				1
#define SD_ALL_SEND_CID_CMD			2
#define SD_SEND_RELATIVE_ADDR_CMD	3
#define SD_SET_DSR_CMD				4
#define SD_SET_BUS_WIDTH_CMD		6
#define SD_SELECT_CARD_CMD			7
#define SD_SEND_CSD_CMD				9
#define SD_SEND_CID_CMD				10
#define SD_STOP_TRANSMISSION_CMD	12
#define SD_SEND_STATUS_CMD			13
#define SD_GO_INACTIVE_STATE_CMD	15
#define SD_SET_BLOCKLEN_CMD			16
#define SD_READ_SINGLE_BLOCK_CMD	17
#define SD_READ_MULTIPLE_BLOCK_CMD	18
#define SD_WRITE_SINGLE_BLOCK_CMD	24
#define SD_WRITE_MULTIPLE_BLOCK_CMD	25
#define SD_PROGRAM_CSD_CMD			27
#define SD_ERASE_SECTOR_START_CMD	32
#define SD_ERASE_SECTOR_END_CMD		33
#define SD_ERASE_CMD				38
#define SD_APP_OP_COND				41
#define SD_LOCK_UNLOCK_CMD			42
#define SD_SEND_SCR_CMD				51
#define SD_APP_CMD					55
#define SD_GET_CMD					56

/* retry count */
//#define SD_CARD_GET_OCR_RETRY_COUNT	0x1000
#define SD_CARD_GET_OCR_RETRY_COUNT	0x1000
#define SD_CARD_WAIT_OPERATION_COMPLETE_RETRY_COUNT	8000
#define SD_CARD_STATE_CHANGE_RETRY_COUNT 10000
#define SD_CARD_WAIT_TRANSFER_STATE_RETRY_COUNT 10000
#define SDC_GET_STATUS_RETRY_COUNT 0x100000

/* sd card standby state */
#define SD_IDLE_STATE				0
#define SD_READY_STATE				1
#define SD_IDENT_STATE				2
#define SD_STBY_STATE				3
#define SD_TRAN_STATE				4
#define SD_DATA_STATE				5
#define SD_RCV_STATE				6
#define SD_PRG_STATE				7
#define SD_DIS_STATE				8

#define SD_BUS_WIDTH_1_BIT			0
#define SD_BUS_WIDTH_4_BIT			2

/********************************************************************/
/* SD card OCR register */
/********************************************************************/
#define SD_OCR_BUSY_BIT				0x80000000

/********************************************************************/
/* SD CID register */
/********************************************************************/
#define SD_DEFAULT_MONTH_CODE		1
#define SD_DEFAULT_YEAR_CODE		2000
#define MAX_MULTI_BLOCK_NUM			126

typedef struct _sd_cid_t
{
	uint ManufacturerID;
	uint ApplicationID;
	unchar ProductName[7];
	uint ProductRevisionHigh;
	uint ProductRevisionLow;
	uint ProductSerialNumber;
	uint ManufactureMonth;
	uint ManufactureYear;
} sd_cid_t;

/********************************************************************/
/* SD CSD register */
/********************************************************************/
#define SD_CSD_STRUCTURE_1_0		0
#define SD_CSD_STRUCTURE_1_1		1

#define SD_CSD_SPEC_VERS_1_0_1_2	0
#define SD_CSD_SPEC_VERS_1_4		1
#define SD_CSD_SPEC_VERS_2_1		2

#define SD_TAAC_TIME_UINT_BITS		0x07
#define SD_TAAC_TIME_VALUE_BITS		0x78

typedef struct _sd_csd_t
{
	uint CSDStructure;
	uint MMCSpecVersion;
 	uint TAAC_u;
	uint NSAC_u;
	uint TransferSpeed;
	uint CardCmdClass;
	uint ReadBlockLength;
	uint ReadBlockPartial;
	uint WriteBlockMisalign;
	uint ReadBlockMisalign;
	uint DSRImplemant;
	uint BlockNumber;
	uint MemorySize;
	uint VDDReadMin_u;
	uint VDDReadMax_u;
	uint VDDWriteMin_u;
	uint VDDWriteMax_u;
	uint EraseBlkEnable;
	uint EraseSectorSize;
	uint WriteProtectGroupSize;
	uint WriteProtectGroupEnable;
	uint WriteSpeedFactor;
	uint WriteBlockLength;
	unchar WriteBlockPartial;
	unchar CopyFlag;
	unchar PermanentWriteProtect;
	unchar TemporaryWriteProtect;
	unchar FileFormat;
} sd_csd_t;

typedef struct _sd_csd_bit_t
{
	uint NotUsed:1;
	uint CRC:7;
	uint MMCardReserved1:2;
	uint FILE_FORMAT:2;
	uint TMP_WRITE_PROTECT:1;
	uint PERM_WRITE_PROTECT:1;
	uint COPY:1;
	uint FILE_FORMAT_GRP:1;

	uint Reserved2:5;
	uint WRITE_BL_PARTIAL:1;
	uint WRITE_BL_LEN:4;
	uint R2W_FACTOR:3;
	uint MMCardReserved0:2;
	uint WP_GRP_ENABLE:1;

	uint WP_GRP_SIZE:7;
	uint ERASE_SECTOR_SIZE:7;
	uint ERASE_BLK_ENABLE:1;
	uint C_SIZE_MULT:3;
	uint VDD_W_CURR_MAX:3;
	uint VDD_W_CURR_MIN:3;
	uint VDD_R_CURR_MAX:3;
	uint VDD_R_CURR_MIN:3;

	uint C_SIZE_1:2;
	uint C_SIZE_2:10; // divide its into 2, 10bits

	uint Reserved1:2;
	uint DSR_IMP:1;
	uint READ_BLK_MISALIGN:1;
	uint WRITE_BLK_MISALIGN:1;
	uint READ_BL_PARTIAL:1;

	uint READ_BL_LEN:4;
	uint CCC:12;

	uint TRAN_SPEED_RateUnit:3;
	uint TRAN_SPEED_TimeValue:4;
	uint TRAN_SPEED_Reserved:1;

	uint NSAC:8;

	uint TAAC_TimeUnit:3;
	uint TAAC_TimeValue:4;
	uint TAAC_Reserved:1;

	uint Reserved0:2;
	uint MMC_SPEC_VERS:4;
	uint CSD_STRUCTURE:2;
} sd_csd_bit_t;

/********************************************************************/
/* SD SCR register */
/********************************************************************/
typedef struct _sd_scr_t
{
	uint Reserved:16;
	uint SD_BUS_WIDTH:4;
	uint SD_SECURITY:3;
	uint DATA_STAT_AFTER_ERASE:1;
	uint SD_SPEC:4;
	uint SCR_STRUCTURE:4;

	uint ManufacturerReserved;
} sd_scr_t;

/********************************************************************/
/* sd card structure */
/********************************************************************/
typedef struct _sd_card_t
{
	/* host interface configuration */
	uint IOAddr;			/* host controller register base address */
	uint DMAEnable;
	uint DMAChannel;

	uint CardType;

	/* card register */
	uint OCR;

	uint CIDWord[4];
	sd_cid_t CID;

	uint CSDWord[4];
	sd_csd_t CSD;

	ushort RCA;
	sd_scr_t SCR;

	/* access time out */
	uint ReadAccessTimoutCycle;
	uint WriteAccessTimoutCycle;

	/* Drive Name */
	uint Drive;

	/* system configurations */
	uint SysFrequency;

	/* card status */
	int ActiveState;
	int WriteProtect;
} sd_card_t;

#endif
