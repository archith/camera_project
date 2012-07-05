/* 
fi2s.c programmed by Ivan Wang
I2S program for audio 2004/8/23 06:53pm
2004/11/30 05:23pm  : support Mono mode
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

                   

/***************************************************************************
    I2S Driver
 **************************************************************************/
static int fi2s_open(int dev, int mode);
static void fi2s_close(int dev);
static void fi2s_output_block(int dev, unsigned long buf, int count, int intrflag);
static void fi2s_start_input(int dev, unsigned long buf, int count, int intrflag);
//static int fi2s_audio_ioctl(int dev, unsigned int cmd, caddr_t arg);
static int fi2s_prepare_for_input(int dev, int bsize, int bcount);
static int fi2s_prepare_for_output(int dev, int bsize, int bcount);
static void fi2s_halt(int dev);
static void fi2s_halt_input(int dev);
static void fi2s_halt_output(int dev);
static void fi2s_trigger(int dev_id, int state);
static int fi2s_set_speed(int dev, int speed);
static unsigned int fi2s_set_bits(int dev, unsigned int arg);
static short fi2s_set_channels(int dev, short arg);
void fi2s_interrupt_handler(int irq, void *dev_id, struct pt_regs *dummy);
void fi2s_init_hw(int);

static struct address_info fi2s_cfg;

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
} fi2s_info;


typedef struct fi2s_port_info
{
    int             open_mode;
    int             speed;
    unsigned char   speed_bits;
    int             channels;
    int             audio_format;
    unsigned char   format_bits;
} fi2s_port_info;


static struct audio_driver fi2s_audio_driver =
{
    owner:          THIS_MODULE,
    open:           fi2s_open,
    close:          fi2s_close,
    output_block:   fi2s_output_block,
    start_input:    fi2s_start_input,
//        ioctl:          fi2s_audio_ioctl,
    prepare_for_input:      fi2s_prepare_for_input,
    prepare_for_output:     fi2s_prepare_for_output,
    halt_io:        fi2s_halt,
    halt_input:     fi2s_halt_input,
    halt_output:    fi2s_halt_output,
    trigger:        fi2s_trigger,
    set_speed:      fi2s_set_speed,
    set_bits:       fi2s_set_bits,
    set_channels:   fi2s_set_channels
};

//Playing: null,null,null,data0,data1,data2
//Recording: null,null,null,data0,data1,data2
#define             FI2S_BUFFER_NUM    6   //null,null,null,data0,data1,data2
#define             FI2S_BUFFER_START  3   //null,null,null,data0,data1,data2
#define             FI2S_BUFFER_SIZE   0x6000
#define             FI2S_NULL_SIZE     0x800

static fi2s_info        fi2s_dev_info[MAX_AUDIO_DEV];
static spinlock_t       fi2s_lock;
static int              nr_fi2s_devs=0;
static unsigned int     fi2s_buf_idx;
static unsigned int     fi2s_copy_idx;
static unsigned char    *fi2s_buf[FI2S_BUFFER_NUM];   
static unsigned int     fi2s_buf_phys[FI2S_BUFFER_NUM];
static unsigned int     va_start,pa_start;


/*
Copy action from => to with byte size (bsize)
*/
static inline void fi2s_do_copy_action(fi2s_port_info *portc,unsigned int to,unsigned int bsize)
{
    unsigned int    from=(unsigned int)fi2s_buf[fi2s_copy_idx];

    if((from==0)||(to==0)||(bsize==0))
        return;
//printk("copy from 0x%x to 0x%x with 0x%x\n",from,to,bsize);
    /* handle for mono record */
    if(portc->channels==1)
    {
        unsigned int i;
        for(i=0;i<bsize/2;i=i+2) //16 bit count
            *(unsigned short *)(to+i)=*(unsigned short *)(from+i*2);
    }
    else
        memcpy((char *)to,(char *)from,bsize);
        
    if(fi2s_copy_idx==(FI2S_BUFFER_NUM-1))
        fi2s_copy_idx=FI2S_BUFFER_START;
    else
        fi2s_copy_idx++;
}

static void fi2s_init_dma(fi2s_info *dev_info,unsigned int llp_cnt)
{
    fi2s_buf_idx=FI2S_BUFFER_START;
    fi2s_copy_idx=FI2S_BUFFER_START;
    
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
    dev_info->priv->llp_count=llp_cnt*(FI2S_BUFFER_NUM-FI2S_BUFFER_START);
//printk("init dma count=%d\n",dev_info->priv->llp_count);
    dev_info->priv->channel=PMU_SSP_DMA_CHANNEL;
    dev_info->priv->hw_handshake=1;
    ahb_dma_init(dev_info->priv);
//printk("ahb_dma_init\n");
}


static void dma_interrupt_handler(int irq, void *dev_id, struct pt_regs *dummy)
{
	fi2s_info           *dev_info= (fi2s_info *)audio_devs[(int)dev_id]->devc;
	fi2s_port_info      *portc = (fi2s_port_info *) audio_devs[(int)dev_id]->portc;
	
    ahbdma_clear_int(dev_info->priv);
    //printk("<%d,0x%x,0x%x>",irq,dev_info->audio_mode,*(volatile unsigned int *)0xf9040148);

    if(dev_info->audio_mode&PCM_ENABLE_INPUT)
    {
        fi2s_do_copy_action(portc,(unsigned int)__va(dev_info->recBuffer_Phy),(unsigned int)dev_info->RemainRecCount);
        DMAbuf_inputintr((int)dev_id);
    }
	else if(dev_info->audio_mode&PCM_ENABLE_OUTPUT)
        DMAbuf_outputintr((int)dev_id, 1);
}

void fi2s_interrupt_handler(int irq, void *dev_id, struct pt_regs *dummy)
{
    fi2s_info      *dev_info=(fi2s_info *) audio_devs[(int)dev_id]->devc;  

    printk("<<%d>>",irq);
    *(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL2)=0;
    ahb_dma_reset(dev_info->priv);
    printk("***SSP underrun error status:0x%x from IRQ %d\n",
        *(unsigned int *)(dev_info->io_base+SSP_INT_STATUS),irq);
    //printk("==>0x%x\n",*(unsigned int *)0xfb080014);
    dev_info->first_time=1;
}

static inline void fill_ssp_buffer(unsigned int base)
{
    int i;
    *(volatile unsigned int *)(base+SSP_CONTROL2)=(SSP_TXFCLR|SSP_RXFCLR);
    udelay(100);
    for(i=0;i<16;i++)    
        *(volatile unsigned int *)(base+SSP_DATA)=0;
}

/**********************************************
    PLAY
 *********************************************/
inline void fi2s_trigger_play(fi2s_info *dev_info)
{
    ahb_dma_parm_t  parm;
    unsigned int    dmacnt=0,dmasz=0,cnum=0,i=0;
    
    dmacnt=(dev_info->RemainPlayCount)/2;
    cnum=(dmacnt+AHBDMA_MAX_DMA_SZ-1)/AHBDMA_MAX_DMA_SZ;   //number of llp per dma data
    dmasz=dmacnt/cnum;  //size of dma per llp item

    if(dmasz>FI2S_BUFFER_SIZE)//dma size cant larger than buffer size
    {
        printk("Not support dma size 0x%x\n",dmacnt);
        return;
    }
    
    if(dev_info->first_time)
    {
        if(!dev_info->priv)
            fi2s_init_dma(dev_info,cnum);

        /* clear SSP irq status */
        while(*(volatile unsigned int*)(dev_info->io_base+SSP_INT_STATUS)&0x3)
            ;
        fill_ssp_buffer(dev_info->io_base);
        dev_info->first_time=0;

        /* null data 0*/
        parm.src=(unsigned int)fi2s_buf_phys[0];
        parm.dest=dev_info->io_base_phys+SSP_DATA;
        parm.sw=AHBDMA_WIDTH_16BIT;
        parm.dw=AHBDMA_WIDTH_16BIT;
        parm.sctl=AHBDMA_CTL_FIX;
        parm.dctl=AHBDMA_CTL_FIX;
        parm.size=FI2S_NULL_SIZE;
        parm.irq=AHBDMA_NO_TRIGGER_IRQ;
        ahb_dma_add(dev_info->priv,&parm);
        
        /* null data 1*/
        parm.src=(unsigned int)fi2s_buf_phys[1];
        parm.irq=AHBDMA_TRIGGER_IRQ;
        ahb_dma_add(dev_info->priv,&parm);
        
        /* null data 2*/
        parm.src=(unsigned int)fi2s_buf_phys[2];
        ahb_dma_add(dev_info->priv,&parm);
        
        /* real data */
        for(i=0;i<cnum;i++)
        {
            parm.src=(unsigned int)dev_info->playBuffer_Phy+(i*dmasz*2);
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
        *(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL2)=(SSP_SSPEN|SSP_TXDOE);
    }
    else
    {
//printk("<again>");
        /* real data */
        for(i=0;i<cnum;i++)
        {
            parm.src=(unsigned int)dev_info->playBuffer_Phy+(i*dmasz*2);
            parm.dest=dev_info->io_base_phys+SSP_DATA;
            parm.sw=AHBDMA_WIDTH_16BIT;
            parm.dw=AHBDMA_WIDTH_16BIT;
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
static inline void fi2s_trigger_record(fi2s_info *dev_info)
{
    ahb_dma_parm_t  parm;
    unsigned int    dmacnt=0,dmasz=0,cnum=0,i=0;
    
    dmacnt=(dev_info->RemainRecCount)/2;
    cnum=(dmacnt+AHBDMA_MAX_DMA_SZ-1)/AHBDMA_MAX_DMA_SZ;   //number of llp per dma data
    dmasz=dmacnt/cnum;  //size of dma per llp item
//    printk("dmasz=0x%x cnum=%d dmacnt=0x%x\n",dmasz,cnum,dmacnt);
    
    if(dmasz>FI2S_BUFFER_SIZE)//dma size cant larger than buffer size
    {
        printk("Not support dma size 0x%x\n",dmacnt);
        return;
    }
        
    //printk("<fi2s_trigger_record>");
    if(dev_info->first_time)
    {
//printk("<first:0x%x 0x%x 0x%x>",empty_data_phys0,empty_data_phys1,empty_data_phys2);
        /* init dma here for the fragment size */
        if(!dev_info->priv)
            fi2s_init_dma(dev_info,cnum);

        /* clear SSP irq status */
        while(*(volatile unsigned int*)(dev_info->io_base+SSP_INT_STATUS)&0x3)
            ;
        *(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL2)=(SSP_TXFCLR|SSP_RXFCLR);
        dev_info->first_time=0;
        fi2s_buf_idx=FI2S_BUFFER_START;
        
        /* null data 0*/
        parm.src=dev_info->io_base_phys+SSP_DATA;
        parm.dest=(unsigned int)fi2s_buf_phys[0];
        parm.sw=AHBDMA_WIDTH_16BIT;
        parm.dw=AHBDMA_WIDTH_16BIT;
        parm.sctl=AHBDMA_CTL_FIX;
        parm.dctl=AHBDMA_CTL_INC;
        parm.size=AHBDMA_MAX_DMA_SZ;
        parm.irq=AHBDMA_NO_TRIGGER_IRQ;
        ahb_dma_add(dev_info->priv,&parm);
        
        /* real data 1*/
        for(i=0;i<cnum;i++)
        {
            parm.size=dmasz;
            parm.dest=(unsigned int)fi2s_buf_phys[fi2s_buf_idx]+(i*dmasz*2);    //16 bit
            if(i==(cnum-1))
                parm.irq=AHBDMA_TRIGGER_IRQ;
            else
                parm.irq=AHBDMA_NO_TRIGGER_IRQ;
            ahb_dma_add(dev_info->priv,&parm);
        }
        fi2s_buf_idx++;
        
        /* real data 2*/
        for(i=0;i<cnum;i++)
        {        
            parm.dest=(unsigned int)fi2s_buf_phys[fi2s_buf_idx]+(i*dmasz*2);    //16 bit
            if(i==(cnum-1))
                parm.irq=AHBDMA_TRIGGER_IRQ;
            else
                parm.irq=AHBDMA_NO_TRIGGER_IRQ;
            ahb_dma_add(dev_info->priv,&parm);
        }
        fi2s_buf_idx++;
        
        /* real data 3 */
        for(i=0;i<cnum;i++)
        {        
            parm.dest=(unsigned int)fi2s_buf_phys[fi2s_buf_idx]+(i*dmasz*2);    //16 bit
            if(i==(cnum-1))
                parm.irq=AHBDMA_TRIGGER_IRQ;
            else
                parm.irq=AHBDMA_NO_TRIGGER_IRQ;
            ahb_dma_add(dev_info->priv,&parm);
        }
        fi2s_buf_idx++;
         
        /* start fi2s */
        ahb_dma_start(dev_info->priv);
        *(volatile unsigned int *)(dev_info->io_base+SSP_INT_CONTROL)=(SSP_FIFO_THOD|SSP_RXDMAEN|SSP_RFURIEN);
        *(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL2)=(SSP_SSPEN|SSP_TXDOE);
    }
    else
    {
//printk("<again>");
        /* real data */
        for(i=0;i<cnum;i++)
        {
            parm.src=dev_info->io_base_phys+SSP_DATA;
            parm.dest=(unsigned int)fi2s_buf_phys[fi2s_buf_idx]+(i*dmasz*2);    //16 bit
            parm.sw=AHBDMA_WIDTH_16BIT;
            parm.dw=AHBDMA_WIDTH_16BIT;
            parm.sctl=AHBDMA_CTL_FIX;
            parm.dctl=AHBDMA_CTL_INC;
            parm.size=dmasz;
            if(i==(cnum-1))
                parm.irq=AHBDMA_TRIGGER_IRQ;
            else
                parm.irq=AHBDMA_NO_TRIGGER_IRQ;
            ahb_dma_add(dev_info->priv,&parm);
        } 
        fi2s_buf_idx++;
    }

    if(fi2s_buf_idx==FI2S_BUFFER_NUM)
        fi2s_buf_idx=FI2S_BUFFER_START;
}

static void fi2s_trigger(int dev_id, int state)
{
    fi2s_port_info         *portc = (fi2s_port_info *) audio_devs[dev_id]->portc;
    fi2s_info              *dev_info=(fi2s_info *) audio_devs[(int)dev_id]->devc;
    unsigned long           flags;

//printk("<t>");
    state&=dev_info->audio_mode;
    spin_lock_irqsave(&fi2s_lock,flags);
    if((portc->open_mode&OPEN_WRITE)&&(state&PCM_ENABLE_OUTPUT))
        fi2s_trigger_play(dev_info);
    else if((portc->open_mode&OPEN_READ)&&(state&PCM_ENABLE_INPUT))
        fi2s_trigger_record(dev_info);
    else
        printk("No trigger action:0x%x,0x%x\n",portc->open_mode,state);
    spin_unlock_irqrestore(&fi2s_lock,flags);
}

static int pmu_set_mclk(fi2s_info *dev_info,unsigned int speed)
{
    unsigned int pmu_val;

    /* set clock */
    pmu_val=*(volatile unsigned int *)(CPE_PMU_VA_BASE+0x34);
	pmu_val&=0xfff0ffff;

    switch (speed)
    {
        case 8000:
            dev_info->fpclk=2048000;
            pmu_val|=(0x0<<16);        
            break;
        case 16000:
            dev_info->fpclk=4096000;
            pmu_val|=(0x2<<16);        
            break;
        case 32000:
            dev_info->fpclk=8192000;
            pmu_val|=(0x4<<16);        
            break;
        case 44100:
            dev_info->fpclk=11289600;
            pmu_val|=(0x5<<16);        
            break;
        case 48000:
            dev_info->fpclk=12288000;
            pmu_val|=(0x6<<16);        
            break;
        default: //44.1KHz
            dev_info->fpclk=11289600;
            pmu_val|=(0x5<<16);        
    }
	*(unsigned int *)(CPE_PMU_VA_BASE+0x34)=pmu_val;
	return 1;
}


//affect fragment size
static int fi2s_set_speed(int dev, int speed)
{
	fi2s_port_info  *portc=(fi2s_port_info *)audio_devs[dev]->portc;
	fi2s_info       *dev_info=(fi2s_info *) audio_devs[dev]->devc;

    //printk("fi2s_set_speed=0x%x 0x%x clk=%d\n",portc->channels,speed,dev_info->fpclk);
    
    if(speed==0)
        speed=portc->speed;
    
    pmu_set_mclk(dev_info,speed);
#if 0
    dev_info->sclk_div = (dev_info->fpclk)/(2*portc->channels*16*speed)-1;
    printk("dev_info->sclk_div=%d dev_info->fpclk=%d\n",dev_info->sclk_div,dev_info->fpclk);
    portc->speed=(dev_info->fpclk)/((2 *(portc->channels)*16)*(dev_info->sclk_div+1));
#else //fix to stereo mode 
    dev_info->sclk_div = (dev_info->fpclk)/(2*2*16*speed)-1;
    //printk("dev_info->sclk_div=%d dev_info->fpclk=%d\n",dev_info->sclk_div,dev_info->fpclk);
    portc->speed=(dev_info->fpclk)/((2*2*16)*(dev_info->sclk_div+1));
#endif
	*(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL1)=(dev_info->sclk_div)|0xf0000;
    //printk("set speed to %d\n",portc->speed);
	return portc->speed;
}

//affect fragment size
static unsigned int fi2s_set_bits(int dev, unsigned int arg)
{
    fi2s_port_info *portc=(fi2s_port_info *) audio_devs[dev]->portc;
    portc->audio_format=AFMT_S16_LE;
    return portc->audio_format;
}

//affect fragment size
static short fi2s_set_channels(int dev, short arg)
{
    fi2s_port_info *portc = (fi2s_port_info *) audio_devs[dev]->portc;
    fi2s_info          *dev_info = (fi2s_info *) audio_devs[dev]->devc;

    //printk("***     set channels=%d\n",arg);

    if(arg==2)
        portc->channels=2;
    else if(arg==1) //set to mono
    {
        portc->channels=1;
        *(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL0)=
            //(*(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL0)&0xfffffff3)|0x8;
            (*(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL0)&0xfffffff3)|0xc; //simulate to stereo
    }
    //else
    //   portc->channels=1;
   
    //printk("set channel to %d\n",portc->channels);
    return portc->channels;
}


static void fi2s_init_codec_rec(unsigned int base)
{
/* nothing to do */
    return;
}


static void fi2s_init_codec_play(unsigned int base)
{
/* nothing to do */
    return;
}

static int fi2s_open(int dev, int mode)
{
    fi2s_info          *dev_info = (fi2s_info *) audio_devs[dev]->devc;
    fi2s_port_info     *portc = (fi2s_port_info *) audio_devs[dev]->portc;
    unsigned long       flags;
//printk("open\n");
    if (dev < 0 || dev >= num_audiodevs)
    	return -ENXIO;    
    //init buffer
    memset(fi2s_buf[0],0,FI2S_BUFFER_SIZE*FI2S_BUFFER_NUM);
    
    spin_lock_irqsave(&fi2s_lock,flags);
    if(portc->open_mode||(dev_info->open_mode&mode))
    {
		spin_unlock_irqrestore(&fi2s_lock,flags);
        return -EBUSY;
    }
        
    dev_info->audio_mode = 0;
    dev_info->open_mode |= mode;    
    dev_info->recBuffer_Phy = 0;
    dev_info->playBuffer_Phy = 0;
    dev_info->RemainRecCount = 0;
    dev_info->RemainPlayCount = 0;    
    dev_info->first_time=1;
    portc->open_mode = mode;
    portc->speed=44100; //set default to 44.1KHz
    dev_info->priv=0;
    spin_unlock_irqrestore(&fi2s_lock,flags);
    return 0;
}

static void fi2s_halt_input(int dev)
{
    unsigned long   flags;
    fi2s_info      *dev_info = (fi2s_info *) audio_devs[dev]->devc;

    spin_lock_irqsave(&fi2s_lock,flags);	
	dev_info->audio_mode &= ~PCM_ENABLE_OUTPUT;
	spin_unlock_irqrestore(&fi2s_lock,flags);
}


static void fi2s_halt_output(int dev)
{
	fi2s_info *dev_info=(fi2s_info *)audio_devs[dev]->devc;
	unsigned long flags;

	spin_lock_irqsave(&fi2s_lock,flags);
	*(volatile unsigned int *)(dev_info->io_base+SSP_INT_CONTROL)&=0xff00;
	dev_info->audio_mode &= ~PCM_ENABLE_OUTPUT;
	spin_unlock_irqrestore(&fi2s_lock,flags);
}	

static void fi2s_halt(int dev)
{
    fi2s_info      *dev_info=(fi2s_info *)audio_devs[dev]->devc;
    fi2s_port_info *portc=(fi2s_port_info *) audio_devs[dev]->portc;
    
    *(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL2)=0;
    if (portc->open_mode & OPEN_WRITE)
		fi2s_halt_output(dev);
    if (portc->open_mode & OPEN_READ)
		fi2s_halt_input(dev);
    dev_info->audio_mode = 0;
}

static void fi2s_close(int dev)
{
    unsigned long   flags;
    fi2s_info        *dev_info = (fi2s_info *) audio_devs[dev]->devc;
    fi2s_port_info   *portc = (fi2s_port_info *) audio_devs[dev]->portc;
	//printk("fi2s_close\n");
    spin_lock_irqsave(&fi2s_lock,flags);
    if (dev_info->priv)
       ahb_dma_free(dev_info->priv);
    //printk("ahb_dma_free\n");
    fi2s_halt(dev);
    dev_info->audio_mode = 0;
    dev_info->open_mode &= ~portc->open_mode;
    portc->open_mode = 0;
    /* SSP state machine reset */
    *(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL2)=0x40;
    spin_unlock_irqrestore(&fi2s_lock,flags);       
}


static void fi2s_output_block(int dev, unsigned long buf, int count, int intrflag)
{
    fi2s_port_info  *portc = (fi2s_port_info *) audio_devs[dev]->portc;
    unsigned long   flags;
    fi2s_info      *dev_info = (fi2s_info *) audio_devs[dev]->devc;   
    unsigned int    *play_buf;
    unsigned int    virt_addr;

    //printk("out_blk:0x%x,cnt=0x%x\n",(int)buf,(int)count);    
    spin_lock_irqsave(&fi2s_lock,flags);

 	dev_info->audio_mode |= PCM_ENABLE_OUTPUT;
    play_buf=(unsigned int *)fi2s_buf[fi2s_buf_idx];
    virt_addr=(unsigned int)__va(buf);

    if(portc->channels==1)
    {
        int i;
        unsigned short *dbuf=(unsigned short *)play_buf;
        unsigned short *sbuf=(unsigned short *)virt_addr;
        //memcpy(play_buf,(unsigned int *)virt_addr,count);
        for(i=0;i<count/2;i++) //16 bit count
        {
            //printf("\r0x%x",dbuf+2*i);
            *(dbuf+2*i)=*(sbuf+i);
            *(dbuf+2*i+1)=*(sbuf+i);
        }
        count=count*2;
    }
    else
        memcpy(play_buf,(unsigned int*)virt_addr,count);
//printk("va=0x%x pa=0x%x idx=%d\n",(unsigned int)play_buf,(unsigned int)fi2s_buf_phys[fi2s_buf_idx],fi2s_buf_idx);

    dev_info->playBuffer_Phy=(unsigned int)fi2s_buf_phys[fi2s_buf_idx];

    if(fi2s_buf_idx==FI2S_BUFFER_NUM-1)
        fi2s_buf_idx=FI2S_BUFFER_START;
    else
        fi2s_buf_idx++;
    dev_info->RemainPlayCount=count;

	spin_unlock_irqrestore(&fi2s_lock,flags);
}


static void fi2s_start_input(int dev, unsigned long buf, int count, int intrflag)
{
    unsigned long       flags;
    fi2s_info           *dev_info = (fi2s_info *) audio_devs[dev]->devc;
    fi2s_port_info      *portc = (fi2s_port_info *) audio_devs[dev]->portc;

    spin_lock_irqsave(&fi2s_lock,flags);
 	dev_info->audio_mode |= PCM_ENABLE_INPUT;
 	//printk("start_input:0x%x/0x%x sz:0x%x\n",(int)buf,(unsigned int)__va(buf),(int)count);
	
	dev_info->recBuffer_Phy = buf;
	//dev_info->RemainRecCount = count; 
	//mono => stereo for ivan
	if(portc->channels==1)
	    dev_info->RemainRecCount = count*2; //mono => record by stereo
	else
	    dev_info->RemainRecCount = count;
    //printk("dev_info->RemainRecCount =0x%x count=0x%x channel=%x\n",dev_info->RemainRecCount,count,portc->channels);
	spin_unlock_irqrestore(&fi2s_lock,flags);
}


static int fi2s_prepare_for_input(int dev, int bsize, int bcount)
{
    fi2s_info      *dev_info = (fi2s_info *) audio_devs[dev]->devc;
    unsigned long   flags;
    
    //printk("fi2s_prepare_for_input,bsize\n");
    spin_lock_irqsave(&fi2s_lock,flags);
    fi2s_init_codec_rec(dev_info->io_base);
    //flib_set_codec(dev_info->io_base);
	audio_devs[dev]->dmap_in->flags |= DMA_NODMA;
    fi2s_halt_input(dev);    
    spin_unlock_irqrestore(&fi2s_lock,flags);
	return 0;
}


static int fi2s_prepare_for_output(int dev, int bsize, int bcount)
{
    fi2s_info      *dev_info = (fi2s_info *) audio_devs[dev]->devc;
    unsigned long   flags;
    //printk("fi2s_prepare_for_output\n");
    spin_lock_irqsave(&fi2s_lock,flags);   
    fi2s_init_codec_play(dev_info->io_base);
    audio_devs[dev]->dmap_out->flags |= DMA_NODMA;        
	fi2s_halt_output(dev);
	spin_unlock_irqrestore(&fi2s_lock,flags);
	return 0;
}

void fi2s_pmu_init(void)
{
    //select AC97 pin
    *(volatile unsigned int *)(CPE_PMU_VA_BASE + 0x28)&=(~(1<<3));
    //using AHB dma
    *(volatile unsigned int *)(CPE_PMU_VA_BASE+0xbc)=(0x8|PMU_SSP_DMA_CHANNEL);
}

static void fi2s_init_hw(int dev)
{
    fi2s_info          *dev_info = (fi2s_info *) audio_devs[dev]->devc;
    
    /* set PMU to select AC97 pin (only A320 have PMU) */   
    fi2s_pmu_init();

	/* set control 0 */
	*(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL0)=0x311c;
	/* clear control 1 */
	*(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL1)=0;
	/* int control */
	*(volatile unsigned int *)(dev_info->io_base+SSP_INT_CONTROL)=SSP_FIFO_THOD;
    
    /* SSP state machine reset */
    *(volatile unsigned int *)(dev_info->io_base+SSP_CONTROL2)=0x40;
        
}

static int __init fi2s_init(void)
{
	fi2s_info      *dev_info = &fi2s_dev_info[nr_fi2s_devs];
	fi2s_port_info *portc = NULL;
    char            dev_name[100];
    int             my_dev,i;
    
    printk(KERN_INFO "Faraday I2S OSS driver(2004/12/2)\n");
    spin_lock_init(&fi2s_lock);
    
    /* address info */
    fi2s_cfg.io_base=CPE_SSP2_VA_BASE;
    fi2s_cfg.irq=IRQ_SSP2;
	fi2s_cfg.dma=-1;			// playback
	fi2s_cfg.dma2=-1;			// record	
	
	/* device info */
    dev_info->irq = fi2s_cfg.irq;
    dev_info->io_base= fi2s_cfg.io_base;
    dev_info->io_base_phys=CPE_SSP2_BASE;
    dev_info->open_mode = 0;

    sprintf(dev_name,"fi2s Sound System(i2s module)");
    conf_printf2(dev_name, dev_info->io_base, dev_info->irq, -1, -1);
    
    portc = (fi2s_port_info *) kmalloc(sizeof(fi2s_port_info), GFP_KERNEL);
    if(portc==NULL)
		return -1;

    if((my_dev = sound_install_audiodrv(AUDIO_DRIVER_VERSION,dev_name,&fi2s_audio_driver,
                                        sizeof(struct audio_driver),DMA_NODMA,AFMT_S16_LE,
                                        dev_info,-1,-1)) < 0)
    {
            kfree(portc);
            portc=NULL;
            return -1;
    }
    printk(KERN_INFO "dsp%d: Faraday I2S OSS driver(2004/12/28)\n",my_dev);
    audio_devs[my_dev]->portc = portc;
    audio_devs[my_dev]->mixer_dev = -1;
    audio_devs[my_dev]->d->owner = THIS_MODULE;

    memset((char *) portc, 0, sizeof(*portc));
    nr_fi2s_devs++;
    
    fi2s_init_hw(my_dev);
    dev_info->dev_no = my_dev;
    
    strcpy(dev_info->name,"FI2S A320 APB DMA");    
    cpe_int_set_irq(VIRQ_SSP_AHB_DMA, LEVEL, H_ACTIVE);
    if(request_irq(VIRQ_SSP_AHB_DMA,dma_interrupt_handler,SA_INTERRUPT,dev_info->name,(void *)my_dev)<0)
    {
		printk(KERN_WARNING "Faraday I2S: Unable to allocate IRQ %d\n",IRQ_CPE_AHB_DMA);
		kfree(portc);
		return -1;
	}
	cpe_int_set_irq(dev_info->irq,LEVEL,H_ACTIVE);
    if(request_irq(dev_info->irq,fi2s_interrupt_handler,SA_INTERRUPT,dev_info->name,(void *)my_dev)<0)
    {
		printk(KERN_WARNING "Faraday I2S: Unable to allocate IRQ %d\n",dev_info->irq);
		kfree(portc);
		return -1;
	}	
	//allocate buffer 
	va_start=(unsigned int)consistent_alloc(GFP_DMA|GFP_KERNEL,FI2S_BUFFER_SIZE*FI2S_BUFFER_NUM,(dma_addr_t *)&pa_start);
	for(i=0;i<FI2S_BUFFER_NUM;i++)
	{
		fi2s_buf[i]=(unsigned char *)(va_start+(i*FI2S_BUFFER_SIZE));
		fi2s_buf_phys[i]=pa_start+(i*FI2S_BUFFER_SIZE);
		//printk("fi2s_buf[%d]=0x%x phys=0x%x\n",i,(unsigned int)fi2s_buf[i],(unsigned int)fi2s_buf_phys[i]);
	}
	memset(fi2s_buf[0],0,FI2S_BUFFER_SIZE*FI2S_BUFFER_NUM);
	fi2s_cfg.slots[0]=my_dev;
	return 0;
}


static void fi2s_unload(struct address_info *hw_config)
{
    int             devid=hw_config->slots[0];
    fi2s_port_info *portc;
    
    portc=(fi2s_port_info *)audio_devs[devid]->portc;
    kfree(portc);
    
	consistent_free((void *)va_start,FI2S_BUFFER_SIZE,(dma_addr_t)pa_start);
    sound_unload_audiodev(hw_config->slots[0]);
    release_region(hw_config->io_base, 4);
}

static void __exit cleanup_fi2s(void)
{
    struct address_info *hw_config=&fi2s_cfg;
    fi2s_unload(hw_config);
    sound_unload_audiodev(hw_config->slots[0]);    
    release_region(hw_config->io_base, 4);
}

module_init(fi2s_init);
module_exit(cleanup_fi2s);

MODULE_LICENSE("GPL");

