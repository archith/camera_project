/*  
fac97.c programmed by Ivan Wang
AC97 program for audio 2004/8/17 09:29am
2004/12/21 06:28pm  add emulation for 8KHz
2004/12/28 01:51pm  for ahb dma
*/
#include <linux/config.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/stddef.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <asm/system.h>
#include <linux/interrupt.h>
#include <asm/arch/hardware.h>
#include <asm/dma.h>
#include <asm/arch/cpe/cpe.h>
#include <asm/arch/cpe_int.h>
#include <asm/arch/ahb_dma.h>
#include <asm/arch/ssp.h>
#include "sound_config.h"

#define SIMULATE_8KHZ_MONO_INPUT

// AC97 codec tag
#define TAG_COMMAND                 0xe000
#define TAG_DATA                    0x9800

// Winbond W83972D
#define AC97_RESET					0x00			// RESET CODEC TO DEFAULT					
#define AC97_MASTER_VOLUME			0x02			// LINE OUT VOLUME
#define AC97_HEADPHONE_VOLUME		0x04
#define AC97_MASTER_VOLUME_MONO		0x06
#define AC97_PC_BEEP_VOLUME			0x0A
#define AC97_PHONE_VOLUME			0x0C
#define AC97_MIC_VOLUME				0x0E			// MICROPHONE VOLUME/ AGC
#define AC97_LINE_IN_VOLUME			0x10			// LINE IN VOLUME
#define AC97_CD_VOLUME				0x12
#define AC97_VIDEO_VOLUME			0x14
#define AC97_AUX_VOLUME				0x16
#define AC97_PCM_OUT_VOL			0x18
#define AC97_RECORD_SELECT			0x1A			// SELECT LINE IN OR MICROPHONE
#define AC97_RECORD_GAIN			0x1C
#define AC97_RECORD_GAIN_MIC    	0x1E
#define AC97_GENERAL_PURPOSE		0x20
#define AC97_CONTROL_3D				0x22
#define AC97_POWERDOWN_CTRL_STAT	0x26			// POWER MANAGEMENT
#define AC97_EXTENDED_AUDIO_ID		0x28
#define AC97_EXTENDED_AUDIO_CTRL	0x2A			// BIT 0 must be set to 1 for variable rate audio
#define AC97_AUDIO_DAC_RATE			0x2C			// 16 bit unsigned is sample rate in hertz
#define AC97_AUDIO_ADC_RATE			0x32			// 16 bit unsigned is sample rate in hertz
#define AC97_ENHANCED_FUNCTION		0x5E			
#define AC97_VENDOR_ID1				0x7c			
#define AC97_VENDOR_ID2				0x7e



/***************************************************************************
    AC97 Driver
 **************************************************************************/
static int fac97_open(int dev, int mode);
static void fac97_close(int dev);
static void fac97_output_block(int dev, unsigned long buf, int count, int intrflag);
static void fac97_start_input(int dev, unsigned long buf, int count, int intrflag);
//static int fac97_audio_ioctl(int dev, unsigned int cmd, caddr_t arg);
static int fac97_prepare_for_input(int dev, int bsize, int bcount);
static int fac97_prepare_for_output(int dev, int bsize, int bcount);
static void fac97_halt(int dev);
static void fac97_halt_input(int dev);
static void fac97_halt_output(int dev);
static void fac97_trigger(int dev_id, int state);
static int fac97_set_speed(int dev, int speed);
static unsigned int fac97_set_bits(int dev, unsigned int arg);
static short fac97_set_channels(int dev, short arg);
void fac97_interrupt_handler(int irq, void *dev_id, struct pt_regs *dummy);
void fac97_init_hw(int);

static struct address_info fac97_cfg;

typedef struct
{
    unsigned int    io_base;
    unsigned int    io_base_phys;
    unsigned char   name[20];
    unsigned int    irq;
    unsigned int    recBuffer_Phy;
    unsigned int    playBuffer_Phy;
    unsigned int    RemainRecCount;	
    unsigned int    RemainPlayCount;
    unsigned int    first_time;
    unsigned int    audio_mode;		
    unsigned int    open_mode;		
    unsigned int    dev_no;
	unsigned int    sclk_div;
	unsigned int    fpclk;
	ahb_dma_data_t  *priv;
} fac97_info;


typedef struct fac97_port_info
{
    int             open_mode;
    int             speed;
    unsigned char   speed_bits;
    int             channels;
    int             audio_format;
    unsigned char   format_bits;
} fac97_port_info;


static struct audio_driver fac97_audio_driver =
{
    owner:          THIS_MODULE,
    open:           fac97_open,
    close:          fac97_close,
    output_block:   fac97_output_block,
    start_input:    fac97_start_input,
//        ioctl:          fac97_audio_ioctl,
    prepare_for_input:      fac97_prepare_for_input,
    prepare_for_output:     fac97_prepare_for_output,
    halt_io:        fac97_halt,
    halt_input:     fac97_halt_input,
    halt_output:    fac97_halt_output,
    trigger:        fac97_trigger,
    set_speed:      fac97_set_speed,
    set_bits:       fac97_set_bits,
    set_channels:   fac97_set_channels
};

#define             FAC97_BUFFER_NUM    6   //null,null,null,data0,data1,data2
#define             FAC97_BUFFER_START  3   //null,null,null,data0,data1,data2
//#define             FAC97_BUFFER_SIZE   0x4000
//#define             FAC97_BUFFER_SIZE   0xc000    //for 48 to 8khz simulation
#define             FAC97_BUFFER_SIZE   0x30000      //for 48-8khz and mono mode
#define             FAC97_NULL_SIZE     0xf00

static fac97_info       fac97_dev_info[MAX_AUDIO_DEV];
spinlock_t              fac97_lock;
static int              nr_fac97_devs=0;
static unsigned int     fac97_buf_idx;
static unsigned int     fac97_copy_idx;
static unsigned char    *fac97_buf[FAC97_BUFFER_NUM];   
static unsigned int     fac97_buf_phys[FAC97_BUFFER_NUM];
static unsigned int     va_start,pa_start;


/*
Copy action from => to with byte size (bsize)
*/
void fac97_do_copy_action(fac97_port_info *portc,unsigned int to,unsigned int bsize)
{
    int             i;
    unsigned int    from=(unsigned int)fac97_buf[fac97_copy_idx];
    
    if((from==0)||(to==0)||(bsize==0))
        return;
//printk("from:0x%x to:0x%x sz:0x%x\n",from,to,bsize);
#ifdef SIMULATE_8KHZ_MONO_INPUT
    if(portc->speed==8000)
    {
        if(portc->channels==1)
        {
            for(i=0;i<(bsize/48);i++)
                *(unsigned short *)(to+(i*2))=(unsigned short)((*(unsigned int *)(from+(i*48)))>>4);
        }
        else
        {
            for(i=0;i<(bsize/48);i++)
            {
                *(unsigned short *)(to+(i*4))=(unsigned short)((*(unsigned int *)(from+(i*48)))>>4);
                *(unsigned short *)(to+(i*4+2))=(unsigned short)((*(unsigned int *)(from+(i*48)+4))>>4);
            }
        }
    }
    else
#endif
    {
        for (i=0;i<bsize*2;i=i+4)
           *(unsigned short *)(to+(i/2))=(unsigned short)((*(unsigned int *)(from+i))>>4);
    }
    if(fac97_copy_idx==(FAC97_BUFFER_NUM-1))
        fac97_copy_idx=FAC97_BUFFER_START;
    else
        fac97_copy_idx++;
}

static void fac97_init_dma(fac97_info *dev_info,unsigned int llp_cnt)
{
//printk("init_dma\n");
    fac97_buf_idx=FAC97_BUFFER_START;
    fac97_copy_idx=FAC97_BUFFER_START;
    
    /* init ahb dma */    
    dev_info->priv=ahb_dma_alloc();
    if(dev_info->priv==0)
    {
        printk("alloca memory fail!\n");
        return;
    }


//printk("dev_info->priv=0x%x\n",dev_info->priv);
    dev_info->priv->base=CPE_AHBDMA_VA_BASE;
    dev_info->priv->llp_master=AHBDMA_MASTER_0;
    dev_info->priv->src_data_master=AHBDMA_MASTER_0;
    dev_info->priv->dest_data_master=AHBDMA_MASTER_0;
    dev_info->priv->llp_count=llp_cnt*(FAC97_BUFFER_NUM-FAC97_BUFFER_START);
//printk("init dma count=%d\n",dev_info->priv->llp_count);
    dev_info->priv->channel=PMU_SSP_DMA_CHANNEL;
    dev_info->priv->hw_handshake=1;
    ahb_dma_init(dev_info->priv);


//printk("ahb_dma_init\n");
}

static void dma_interrupt_handler(int irq, void *dev_id, struct pt_regs *dummy)
{
	fac97_info *dev_info= (fac97_info *)audio_devs[(int)dev_id]->devc;
	fac97_port_info      *portc = (fac97_port_info *) audio_devs[(int)dev_id]->portc;
    //printk("<0x%x>",*(volatile unsigned int *)0xf9040148);
    ahbdma_clear_int(dev_info->priv);
    if (dev_info->audio_mode & PCM_ENABLE_INPUT)
    {
        fac97_do_copy_action(portc,(unsigned int)__va(dev_info->recBuffer_Phy),(unsigned int)dev_info->RemainRecCount);
        DMAbuf_inputintr((int)dev_id);
    }
	else if (dev_info->audio_mode & PCM_ENABLE_OUTPUT)
        DMAbuf_outputintr((int)dev_id, 1);
}

void fac97_interrupt_handler(int irq, void *dev_id, struct pt_regs *dummy)
{
    fac97_info      *dev_info=(fac97_info *) audio_devs[(int)dev_id]->devc;  

    printk("<<%d>>",irq);
    *(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL2)=0;
    ahb_dma_reset(dev_info->priv);
    printk("***SSP underrun error status:0x%x from IRQ %d\n",
        *(unsigned int *)(dev_info->io_base+SSP_INT_STATUS),irq);
    dev_info->first_time=1;
}

inline void fill_ssp_buffer(unsigned int base)
{
    int i;
    *(volatile unsigned int *)(base+SSP_CONTROL2)=(SSP_TXFCLR|SSP_RXFCLR);
    mdelay(100);
    for(i=0;i<16;i++)    
        *(volatile unsigned int *)(base+SSP_DATA)=0;
}

/**********************************************
    PLAY
 *********************************************/
inline void fac97_trigger_play(fac97_info *dev_info)
{
    ahb_dma_parm_t parm;
//printk("<t2:0x%x>",*(volatile unsigned int *)0xf9040148);
    unsigned int    dmacnt=0,dmasz=0,cnum=0,i=0;

    dmacnt=((dev_info->RemainPlayCount)*2)/4;
    cnum=(dmacnt+AHBDMA_MAX_DMA_SZ-1)/AHBDMA_MAX_DMA_SZ;   //number of llp per dma data
    dmasz=dmacnt/cnum;  //size of dma per llp item
    if(dev_info->first_time)
    {
//printk("<first:0x%x 0x%x 0x%x 0x%x ptr=0x%x>\n",(unsigned int)fac97_buf_phys[0],(unsigned int)fac97_buf_phys[1],(unsigned int)fac97_buf_phys[2],(unsigned int)dev_info->playBuffer_Phy,*(volatile unsigned int *)0xf9040148);
        if(!dev_info->priv)
            fac97_init_dma(dev_info,cnum);
        /* clear SSP irq status */
        while(*(volatile unsigned int*)(dev_info->io_base+SSP_INT_STATUS)&0x3)
            ;
        fill_ssp_buffer(dev_info->io_base);
        dev_info->first_time=0;
        /* null data 0*/
        parm.src=(unsigned int)fac97_buf_phys[0];
        parm.dest=dev_info->io_base_phys+SSP_DATA;
        parm.sw=AHBDMA_WIDTH_32BIT;
        parm.dw=AHBDMA_WIDTH_32BIT;
        parm.sctl=AHBDMA_CTL_FIX;
        //parm.sctl=AHBDMA_CTL_INC;
        parm.dctl=AHBDMA_CTL_FIX;
        parm.size=FAC97_NULL_SIZE;
        parm.irq=AHBDMA_NO_TRIGGER_IRQ;
        ahb_dma_add(dev_info->priv,&parm);
        
        /* null data 1*/
        parm.src=(unsigned int)fac97_buf_phys[1];
        parm.irq=AHBDMA_TRIGGER_IRQ;
        ahb_dma_add(dev_info->priv,&parm);
        
        /* null data 2*/
        parm.src=(unsigned int)fac97_buf_phys[2];
        ahb_dma_add(dev_info->priv,&parm);
        
        /* real data */
        for(i=0;i<cnum;i++)
        {
            parm.src=(unsigned int)dev_info->playBuffer_Phy+(i*dmasz*4);    //32 bit
            parm.sctl=AHBDMA_CTL_INC;
            parm.size=dmasz;
            if(i==(cnum-1))
                parm.irq=AHBDMA_TRIGGER_IRQ;
            else
                parm.irq=AHBDMA_NO_TRIGGER_IRQ;
            ahb_dma_add(dev_info->priv,&parm);
        }
        
        ahb_dma_start(dev_info->priv);
        *(volatile unsigned int *)(dev_info->io_base+SSP_INT_CONTROL)=(SSP_FIFO_THOD|SSP_TXDMAEN|SSP_TFURIEN);
        //*(volatile unsigned int *)(dev_info->io_base+SSP_INT_CONTROL)=(SSP_FIFO_THOD|SSP_TXDMAEN);
        *(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL2)=(SSP_SSPEN|SSP_TXDOE);
    }
    else
    {
        /* real data */
        for(i=0;i<cnum;i++)
        { 
            parm.src=(unsigned int)dev_info->playBuffer_Phy+(i*dmasz*4);    //32 bit
            parm.dest=dev_info->io_base_phys+SSP_DATA;
            parm.sw=AHBDMA_WIDTH_32BIT;
            parm.dw=AHBDMA_WIDTH_32BIT;
            parm.sctl=AHBDMA_CTL_INC;
            parm.dctl=AHBDMA_CTL_FIX;
            parm.size=dmasz;
            if(i==(cnum-1))
                parm.irq=AHBDMA_TRIGGER_IRQ;
            else
                parm.irq=AHBDMA_NO_TRIGGER_IRQ;
            ahb_dma_add(dev_info->priv,&parm);
        }
    }
}
/**********************************************
    RECORD
 *********************************************/
inline void fac97_trigger_record(fac97_info *dev_info)
{
    ahb_dma_parm_t parm;
    unsigned int    dmacnt=0,dmasz=0,cnum=0,i=0;
//printk("<fac97_trigger_record>");

    dmacnt=(dev_info->RemainRecCount)/4;
    cnum=(dmacnt+AHBDMA_MAX_DMA_SZ-1)/AHBDMA_MAX_DMA_SZ;   //number of llp per dma data
    dmasz=dmacnt/cnum;  //size of dma per llp item

//printk("dmacnt=%d cnum=%d dmasz=%d\n",dmacnt,cnum,dmasz);

    //printk("<fac97_trigger_record>");
    if(dev_info->first_time)
    {
        //printk("<first>\n");
        /* clear SSP irq status */
        if(!dev_info->priv)
            fac97_init_dma(dev_info,cnum);
            
        while(*(volatile unsigned int*)(dev_info->io_base+SSP_INT_STATUS)&0x3)
            ;
        *(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL2)=(SSP_TXFCLR|SSP_RXFCLR);
        dev_info->first_time=0;
        fac97_buf_idx=FAC97_BUFFER_START;

        /* null data 0*/
        parm.src=dev_info->io_base_phys+SSP_DATA;
        parm.dest=(unsigned int)fac97_buf_phys[0];
        parm.sw=AHBDMA_WIDTH_32BIT;
        parm.dw=AHBDMA_WIDTH_32BIT;
        parm.sctl=AHBDMA_CTL_FIX;
        parm.dctl=AHBDMA_CTL_INC;
        parm.size=FAC97_NULL_SIZE;
        parm.irq=AHBDMA_NO_TRIGGER_IRQ;
        ahb_dma_add(dev_info->priv,&parm);
        
        /* real data 1*/
        for(i=0;i<cnum;i++)
        {
            parm.size=dmasz;
            parm.dest=(unsigned int)fac97_buf_phys[fac97_buf_idx]+(i*dmasz*4);    //32 bit
            if(i==(cnum-1))
                parm.irq=AHBDMA_TRIGGER_IRQ;
            else
                parm.irq=AHBDMA_NO_TRIGGER_IRQ;
            ahb_dma_add(dev_info->priv,&parm);
        }
        fac97_buf_idx++;

        /* real data 2*/
        for(i=0;i<cnum;i++)
        {        
            parm.dest=(unsigned int)fac97_buf_phys[fac97_buf_idx]+(i*dmasz*4);    //32 bit
            if(i==(cnum-1))
                parm.irq=AHBDMA_TRIGGER_IRQ;
            else
                parm.irq=AHBDMA_NO_TRIGGER_IRQ;
            ahb_dma_add(dev_info->priv,&parm);
        }
        fac97_buf_idx++;
        
        /* real data 3 */
        for(i=0;i<cnum;i++)
        {        
            parm.dest=(unsigned int)fac97_buf_phys[fac97_buf_idx]+(i*dmasz*4);    //32 bit
            if(i==(cnum-1))
                parm.irq=AHBDMA_TRIGGER_IRQ;
            else
                parm.irq=AHBDMA_NO_TRIGGER_IRQ;
            ahb_dma_add(dev_info->priv,&parm);
        }
        fac97_buf_idx++;

        /* start fac97 */
        ahb_dma_start(dev_info->priv);
        *(volatile unsigned int *)(dev_info->io_base+SSP_INT_CONTROL)=(SSP_FIFO_THOD|SSP_RXDMAEN|SSP_RFURIEN);
        *(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL2)=(SSP_SSPEN|SSP_TXDOE);
    }
    else
    {
        for(i=0;i<cnum;i++)
        {
            parm.src=dev_info->io_base_phys+SSP_DATA;
            parm.dest=(unsigned int)fac97_buf_phys[fac97_buf_idx]+(i*dmasz*4); //32 bit
            parm.sw=AHBDMA_WIDTH_32BIT;
            parm.dw=AHBDMA_WIDTH_32BIT;
            parm.sctl=AHBDMA_CTL_FIX;
            parm.dctl=AHBDMA_CTL_INC;
            parm.size=dmasz;
            if(i==(cnum-1))
                parm.irq=AHBDMA_TRIGGER_IRQ;
            else
                parm.irq=AHBDMA_NO_TRIGGER_IRQ;
            ahb_dma_add(dev_info->priv,&parm);
        } 
        fac97_buf_idx++;
    }
    if(fac97_buf_idx==FAC97_BUFFER_NUM)
        fac97_buf_idx=FAC97_BUFFER_START;
}

static void fac97_trigger(int dev_id, int state)
{
    fac97_port_info         *portc = (fac97_port_info *) audio_devs[dev_id]->portc;
    fac97_info              *dev_info=(fac97_info *) audio_devs[(int)dev_id]->devc;
    unsigned long           flags;

    state&=dev_info->audio_mode;
    spin_lock_irqsave(&fac97_lock,flags);
    if((portc->open_mode&OPEN_WRITE)&&(state&PCM_ENABLE_OUTPUT))
        fac97_trigger_play(dev_info);
    else if((portc->open_mode&OPEN_READ)&&(state&PCM_ENABLE_INPUT))
        fac97_trigger_record(dev_info);
    else
        printk("No trigger action:0x%x,0x%x\n",portc->open_mode,state);
    spin_unlock_irqrestore(&fac97_lock,flags);
}


//affect fragment size
static int fac97_set_speed(int dev, int speed)
{
	fac97_port_info *portc=(fac97_port_info *)audio_devs[dev]->portc;
#ifdef SIMULATE_8KHZ_MONO_INPUT
	if(speed==8000)
	    portc->speed=8000;
#endif    
    if(speed==48000)
        portc->speed=48000;
	return portc->speed;
}

//affect fragment size
static unsigned int fac97_set_bits(int dev, unsigned int arg)
{
    fac97_port_info *portc=(fac97_port_info *) audio_devs[dev]->portc;
    portc->audio_format=AFMT_S16_LE;
    return portc->audio_format;
}

//affect fragment size
static short fac97_set_channels(int dev, short arg)
{
    fac97_port_info *portc = (fac97_port_info *) audio_devs[dev]->portc;
//printk("arg=%d channel=%d\n",arg,portc->channels);
    if((arg==1)||(arg==2))
        portc->channels=arg;
    return portc->channels;
}


void fac97_write_codec(unsigned int base,unsigned int reg,unsigned int data)
{
    unsigned int txbuf[2];

    txbuf[0]=(reg<<12);
    txbuf[1]=data<<4;
	 
	*(volatile unsigned int *)(base+SSP_CONTROL2)=(SSP_TXFCLR|SSP_RXFCLR);
    mdelay(10);
	*(volatile unsigned int *)(base+SSP_ACLINK_SLOT_VALID)=TAG_COMMAND;
    mdelay(10);

    while(((*(volatile unsigned int*)(base+SSP_STATUS)&SSP_TFVE)>>12)!=0)
        ;
    *(volatile unsigned int *)(base+SSP_DATA)=txbuf[0];
	mdelay(10);
    *(volatile unsigned int *)(base+SSP_DATA)=txbuf[1];
	mdelay(10);
	*(volatile unsigned int *)(base+SSP_CONTROL2)=(SSP_SSPEN|SSP_TXDOE);
	mdelay(10);
    while(((*(volatile unsigned int*)(base+SSP_STATUS)&SSP_TFVE)>>12)!=0)
        ;
    mdelay(10);
	*(volatile unsigned int *)(base+SSP_CONTROL2)=0;
    mdelay(10);
}


static void fac97_init_codec_rec(unsigned int base)
{
    //printk("fac97_init_codec_rec\n");
	/* AC97 cold reset */
    *(volatile unsigned int *)(base+SSP_CONTROL2)=0x20;
    mdelay(20);
    fac97_write_codec(base,AC97_RESET,0);
    mdelay(20);
    fac97_write_codec(base,AC97_MASTER_VOLUME, 0x8000);
	fac97_write_codec(base,AC97_LINE_IN_VOLUME, 0x0808);
#if 1 //MIC function
	fac97_write_codec(base,AC97_MIC_VOLUME, 0x0008);    //for MAC-in
	fac97_write_codec(base,AC97_RECORD_SELECT, 0x0505); //for Line-in and MIC-in
	fac97_write_codec(base,AC97_RECORD_GAIN_MIC, 0x0008);  
#else
    fac97_write_codec(base,AC97_RECORD_SELECT, 0x0404); //Line-in only
#endif
	fac97_write_codec(base,AC97_PCM_OUT_VOL, 0x8000);
	fac97_write_codec(base,AC97_RECORD_GAIN, 0x0808);
    *(volatile unsigned int *)(base+SSP_ACLINK_SLOT_VALID)=TAG_DATA;
    /* clear SSP irq status */
    while(*(volatile unsigned int*)(base+SSP_INT_STATUS)&0x3)
        ;
}


static void fac97_init_codec_play(unsigned int base)
{
    //printk("fac97_init_codec_play\n");
	/* AC97 cold reset */
    *(volatile unsigned int *)(base+SSP_CONTROL2)=0x20;
    mdelay(20);
    fac97_write_codec(base,AC97_RESET,0);
    mdelay(20);
    fac97_write_codec(base,AC97_MASTER_VOLUME, 0);
	fac97_write_codec(base,AC97_PCM_OUT_VOL, 0x0808);
	fac97_write_codec(base,AC97_RECORD_GAIN, 0x8000);
    *(volatile unsigned int *)(base+SSP_ACLINK_SLOT_VALID)=TAG_DATA;
    /* clear SSP irq status */
    while(*(volatile unsigned int*)(base+SSP_INT_STATUS)&0x3)
        ;
}


static int fac97_open(int dev, int mode)
{
    fac97_info          *dev_info = (fac97_info *) audio_devs[dev]->devc;
    fac97_port_info     *portc = (fac97_port_info *) audio_devs[dev]->portc;
    unsigned long       flags;
    
    if (dev < 0 || dev >= num_audiodevs)
    	return -ENXIO;    

    //init buffer
    memset(fac97_buf[0],0,FAC97_BUFFER_SIZE*FAC97_BUFFER_NUM);
    
    spin_lock_irqsave(&fac97_lock,flags);
    if(portc->open_mode||(dev_info->open_mode&mode))
    {
		spin_unlock_irqrestore(&fac97_lock,flags);
        return -EBUSY;
    }
        
    dev_info->audio_mode = 0;
    dev_info->open_mode |= mode;    
    dev_info->recBuffer_Phy = 0;
    dev_info->playBuffer_Phy = 0;
    dev_info->RemainRecCount = 0;
    dev_info->RemainPlayCount = 0;    
    dev_info->fpclk=0;
    dev_info->first_time=1;
    portc->open_mode = mode;
    portc->speed=48000;
    dev_info->priv=0;
    fac97_buf_idx=FAC97_BUFFER_START;
    fac97_copy_idx=FAC97_BUFFER_START;
#if 0
    /* init ahb dma */    
    dev_info->priv=ahb_dma_alloc();
//printk("dev_info->priv=0x%x\n",dev_info->priv);
    dev_info->priv->base=CPE_AHBDMA_VA_BASE;
    dev_info->priv->llp_master=AHBDMA_MASTER_0;
    dev_info->priv->src_data_master=AHBDMA_MASTER_0;
    dev_info->priv->dest_data_master=AHBDMA_MASTER_0;
    dev_info->priv->llp_count=FAC97_BUFFER_START;
    dev_info->priv->channel=PMU_SSP_DMA_CHANNEL;
    dev_info->priv->hw_handshake=1;
    ahb_dma_init(dev_info->priv);
#endif
    spin_unlock_irqrestore(&fac97_lock,flags);
    return 0;
}

static void fac97_halt_input(int dev)
{
    unsigned long   flags;
    fac97_info      *dev_info = (fac97_info *) audio_devs[dev]->devc;

    spin_lock_irqsave(&fac97_lock,flags);	
	dev_info->audio_mode &= ~PCM_ENABLE_OUTPUT;
	spin_unlock_irqrestore(&fac97_lock,flags);
}


static void fac97_halt_output(int dev)
{
	fac97_info *dev_info=(fac97_info *)audio_devs[dev]->devc;
	unsigned long flags;

	spin_lock_irqsave(&fac97_lock,flags);
	dev_info->audio_mode &= ~PCM_ENABLE_OUTPUT;
	spin_unlock_irqrestore(&fac97_lock,flags);
}	

static void fac97_halt(int dev)
{
    fac97_info      *dev_info=(fac97_info *)audio_devs[dev]->devc;
    fac97_port_info *portc=(fac97_port_info *) audio_devs[dev]->portc;
    
    //printk("fac97_halt\n");
    *(volatile unsigned int *)(dev_info->io_base+SSP_INT_CONTROL)&=0xff00;
    *(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL2)=0;
    if (portc->open_mode & OPEN_WRITE)
		fac97_halt_output(dev);
    if (portc->open_mode & OPEN_READ)
		fac97_halt_input(dev);
    dev_info->audio_mode = 0;
}

static void fac97_close(int dev)
{
    unsigned long   flags;
    fac97_info        *dev_info = (fac97_info *) audio_devs[dev]->devc;
    fac97_port_info   *portc = (fac97_port_info *) audio_devs[dev]->portc;
	//printk("fac97_close\n");
    spin_lock_irqsave(&fac97_lock,flags);
    if(dev_info->priv)
        ahb_dma_free(dev_info->priv);
    fac97_halt(dev);
    dev_info->audio_mode = 0;
    dev_info->open_mode &= ~portc->open_mode;
    portc->open_mode = 0;
    /* SSP state machine reset */
    //printk("ssp state machine reset\n");
    *(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL2)=0x40;
    //fac97_write_codec(dev_info->io_base,AC97_RESET,0);
    mdelay(1);    
    spin_unlock_irqrestore(&fac97_lock,flags);       
}


static void fac97_output_block(int dev, unsigned long buf, int count, int intrflag)
{
    unsigned long   flags;
    fac97_info      *dev_info = (fac97_info *) audio_devs[dev]->devc;
    int             i;
    unsigned int    *play_buf;
    unsigned int    virt_addr;
   
    //printk("out_blk:0x%x,cnt=0x%x\n",(int)buf,(int)count);    
    
    spin_lock_irqsave(&fac97_lock,flags);

 	dev_info->audio_mode |= PCM_ENABLE_OUTPUT;
    play_buf=(unsigned int *)fac97_buf[fac97_buf_idx];
    virt_addr=(unsigned int)__va(buf);
    
#if 1
    for(i=0;i<count;i=i+2)
        play_buf[(i/2)]=(unsigned int)((*(unsigned short *)(virt_addr+i))<<4);
#endif
//printk("va=0x%x pa=0x%x idx=%d cnt=0x%x ptr=0x%x\n",(unsigned int)play_buf,(unsigned int)fac97_buf_phys[fac97_buf_idx],fac97_buf_idx,count,*(volatile unsigned int *)0xf9040148);

    dev_info->playBuffer_Phy=(unsigned int)fac97_buf_phys[fac97_buf_idx];

    if(fac97_buf_idx==FAC97_BUFFER_NUM-1)
        fac97_buf_idx=FAC97_BUFFER_START;
    else
        fac97_buf_idx++;
        
    dev_info->RemainPlayCount=count;
//printk("playcount=0x%x\n",count);
	spin_unlock_irqrestore(&fac97_lock,flags);
}


static void fac97_start_input(int dev, unsigned long buf, int count, int intrflag)
{
    unsigned long       flags;
    fac97_info          *dev_info = (fac97_info *) audio_devs[dev]->devc;
    fac97_port_info     *portc = (fac97_port_info *) audio_devs[dev]->portc;
    
    spin_lock_irqsave(&fac97_lock,flags);
 	dev_info->audio_mode |= PCM_ENABLE_INPUT;
 	//printk("start_input:0x%x sz:0x%x\n",(int)buf,(int)count);
	dev_info->recBuffer_Phy = buf;
#ifdef SIMULATE_8KHZ_MONO_INPUT
    if(portc->speed==8000)
    {
        if(portc->channels==1)
	        dev_info->RemainRecCount=count*6*2*2; //mono => record by stereo
	    else
	        dev_info->RemainRecCount=count*6*2;
    }
    else
#endif
	    dev_info->RemainRecCount = count*2;
	spin_unlock_irqrestore(&fac97_lock,flags);
}


static int fac97_prepare_for_input(int dev, int bsize, int bcount)
{
    fac97_info      *dev_info = (fac97_info *) audio_devs[dev]->devc;
    unsigned long   flags;
    
    //printk("\nfac97_prepare_for_input\n");
    
    spin_lock_irqsave(&fac97_lock,flags);
    fac97_init_codec_rec(dev_info->io_base);
    //flib_set_codec(dev_info->io_base);
	audio_devs[dev]->dmap_in->flags |= DMA_NODMA;
    fac97_halt_input(dev);
    spin_unlock_irqrestore(&fac97_lock,flags);
	return 0;
}


static int fac97_prepare_for_output(int dev, int bsize, int bcount)
{
    fac97_info      *dev_info = (fac97_info *) audio_devs[dev]->devc;
    unsigned long   flags;
    
    //printk("\nfac97_prepare_for_output\n");
    
    spin_lock_irqsave(&fac97_lock,flags);   
    fac97_init_codec_play(dev_info->io_base);
    audio_devs[dev]->dmap_out->flags |= DMA_NODMA;        
	fac97_halt_output(dev);
	spin_unlock_irqrestore(&fac97_lock,flags);
	return 0;
}

void fac97_pmu_init(void)
{
    //select AC97 pin
    *(volatile unsigned int *)(CPE_PMU_VA_BASE+0x28)|=((1<<13)|(1<<3));
    //using AHB dma
    *(volatile unsigned int *)(CPE_PMU_VA_BASE+0xbc)=(0x8|PMU_SSP_DMA_CHANNEL);
}

void fac97_init_hw(int dev)
{
    fac97_info          *dev_info = (fac97_info *) audio_devs[dev]->devc;
    unsigned int        pmu_val;
    
    /* set PMU to select AC97 pin */   
    fac97_pmu_init();
    
    /* set clock */
    pmu_val=*(volatile unsigned int *)(CPE_PMU_VA_BASE+0x34);
	pmu_val&=0xfff0ffff;
	dev_info->fpclk=12288000;
	pmu_val|=(0x6<<16);
	*(unsigned int *)(CPE_PMU_VA_BASE+0x34)=pmu_val;
	
	/* set control 0 */
	*(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL0)=0x400c;
	/* clear control 1 */
	*(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL1)=0;
	/* int control */
	*(volatile unsigned int *)(dev_info->io_base+SSP_INT_CONTROL)=SSP_FIFO_THOD;
	
	/* AC97 cold reset */
    *(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL2)=0x20;
    mdelay(30);
    //fac97_init_codec(dev_info->io_base);
    
    /* SSP state machine reset */
    *(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL2)=0x40;
        
}

static int __init fac97_init(void)
{
	fac97_info      *dev_info = &fac97_dev_info[nr_fac97_devs];
	fac97_port_info *portc = NULL;
    char            dev_name[100];
    int             my_dev,i;
    
    spin_lock_init(&fac97_lock);
    
    /* address info */
    fac97_cfg.io_base=CPE_SSP2_VA_BASE;
    fac97_cfg.irq=IRQ_SSP2;
	fac97_cfg.dma=-1;			// playback
	fac97_cfg.dma2=-1;			// record	
	
	/* device info */
    dev_info->irq = fac97_cfg.irq;
    dev_info->io_base= fac97_cfg.io_base;
    dev_info->io_base_phys=CPE_SSP2_BASE;
    dev_info->open_mode = 0;
 	dev_info->priv=0;

    sprintf(dev_name,"fac97 Sound System(ac97 module)");
    conf_printf2(dev_name, dev_info->io_base, dev_info->irq, -1, -1);
    
    portc = (fac97_port_info *) kmalloc(sizeof(fac97_port_info), GFP_KERNEL);
    if(portc==NULL)
		return -1;

    if((my_dev = sound_install_audiodrv(AUDIO_DRIVER_VERSION,dev_name,&fac97_audio_driver,
                                        sizeof(struct audio_driver),DMA_NODMA,AFMT_S16_LE,
                                        dev_info,-1,-1)) < 0)
    {
            kfree(portc);
            portc=NULL;
            return -1;
    }
    
    printk(KERN_INFO "dsp%d: Faraday AC97 OSS driver(2004/12/2)\n",my_dev);
    
    audio_devs[my_dev]->portc = portc;
    audio_devs[my_dev]->mixer_dev = -1;
    audio_devs[my_dev]->d->owner = THIS_MODULE;

    memset((char *) portc, 0, sizeof(*portc));
    nr_fac97_devs++;
    
    fac97_init_hw(my_dev);
    dev_info->dev_no = my_dev;
    
    strcpy(dev_info->name,"FAC97 dma/error");
    cpe_int_set_irq(VIRQ_SSP_AHB_DMA, LEVEL, H_ACTIVE);
    if(request_irq(VIRQ_SSP_AHB_DMA,dma_interrupt_handler,SA_INTERRUPT,dev_info->name,(void *)my_dev)<0)
    {
		printk(KERN_WARNING "Faraday AC97 dma: Unable to allocate IRQ %d\n",IRQ_CPE_AHB_DMA);
		kfree(portc);
		return -1;
	} 
	//printk("dev_info->irq=%d\n",dev_info->irq);
	cpe_int_set_irq(dev_info->irq,LEVEL,H_ACTIVE);
    if(request_irq(dev_info->irq,fac97_interrupt_handler,SA_INTERRUPT,dev_info->name,(void *)my_dev)<0)
    {
		printk(KERN_WARNING "Faraday AC97 error: Unable to allocate IRQ %d\n",dev_info->irq);
		kfree(portc);
		return -1;
	}	
	//allocate buffer 
	va_start=(unsigned int)consistent_alloc(GFP_DMA|GFP_KERNEL,FAC97_BUFFER_SIZE*FAC97_BUFFER_NUM,(dma_addr_t *)&pa_start);
	
	for(i=0;i<FAC97_BUFFER_NUM;i++)
	{
		fac97_buf[i]=(unsigned char *)(va_start+(i*FAC97_BUFFER_SIZE));
		fac97_buf_phys[i]=pa_start+(i*FAC97_BUFFER_SIZE);
		//printk("fac97_buf[%d]=0x%x phys=0x%x\n",i,(unsigned int)fac97_buf[i],(unsigned int)fac97_buf_phys[i]);
	}
	memset(fac97_buf[0],0,FAC97_BUFFER_SIZE*FAC97_BUFFER_NUM);
	fac97_cfg.slots[0]=my_dev;
	return 0;
}


void fac97_unload(struct address_info *hw_config)
{
    int             devid=hw_config->slots[0];
    fac97_port_info *portc;
    
    portc=(fac97_port_info *)audio_devs[devid]->portc;
    kfree(portc);
    
	consistent_free((void *)va_start,FAC97_BUFFER_SIZE,(dma_addr_t)pa_start);

    sound_unload_audiodev(hw_config->slots[0]);
    release_region(hw_config->io_base, 4);
}

static void __exit cleanup_fac97(void)
{
    struct address_info *hw_config=&fac97_cfg;
    fac97_unload(hw_config);
    sound_unload_audiodev(hw_config->slots[0]);    
    release_region(hw_config->io_base, 4);
}

module_init(fac97_init);
module_exit(cleanup_fac97);

MODULE_LICENSE("GPL");

