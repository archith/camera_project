#include <linux/module.h>
#include <linux/config.h> 
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/rtnetlink.h>

#include <asm/io.h>
#include <asm/dma.h>
#include <asm/byteorder.h>

#include <net/irda/irda.h>
#include <net/irda/irmod.h>
#include <net/irda/wrapper.h>
#include <net/irda/irda_device.h>

#include <asm/arch-cpe/hardware.h>
//#include "faraday.h"
#include "cpe_serial.h"
#include "irda.h"
#include "faraday_ir.h"

#undef  CONFIG_USE_INTERNAL_TIMER  /* Just cannot make that timer work */
#define CONFIG_USE_W977_PNP        /* Currently needed */
#define PIO_MAX_SPEED       115200 

static char *driver_name = "faraday_ir";
static int  qos_mtt_bits = 0x07;   /* 1 ms or more */

#define CHIP_IO_EXTENT 	65

//static unsigned int io[] = { 0x180, ~0, ~0, ~0 };
static unsigned int io[] = { CPE_IRDA_BASE, 0, 0, 0 };


static unsigned int irq1[] = { CPE_IRDA_INT1  , 0, 0, 0 };
static unsigned int irq2[] = { CPE_IRDA_INT2  , 0, 0, 0 };

static unsigned int dma[] = { CPE_IRDA_DMA, 0, 0, 0 };
//static unsigned int efbase[] = { W977_EFIO_BASE, W977_EFIO2_BASE };
//static unsigned int efio = W977_EFIO_BASE;

static struct faraday_ir *dev_self[] = { NULL, NULL, NULL, NULL};

/* Some prototypes */
static int  faraday_ir_open(int i, unsigned int iobase, unsigned int irq1, unsigned int irq2, 
                          unsigned int dma);
static int  faraday_ir_close(struct faraday_ir *self);
static int  faraday_ir_probe(int iobase, int irq, int dma);
static int  faraday_ir_dma_receive(struct faraday_ir *self); 
static int  faraday_ir_dma_receive_complete(struct faraday_ir *self);
static int  faraday_ir_hard_xmit(struct sk_buff *skb, struct net_device *dev);
static int  faraday_ir_pio_write(int iobase, __u8 *buf, int len, int fifo_size);
static void faraday_ir_dma_write(struct faraday_ir *self, int iobase);
static void faraday_ir_change_speed(struct faraday_ir *self, __u32 speed);
static void faraday_ir_interrupt(int irq, void *dev_id, struct pt_regs *regs);
static int  faraday_ir_is_receiving(struct faraday_ir *self);

static int  faraday_ir_net_init(struct net_device *dev);
static int  faraday_ir_net_open(struct net_device *dev);
static int  faraday_ir_net_close(struct net_device *dev);
static int  faraday_ir_net_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
static struct net_device_stats *faraday_ir_net_get_stats(struct net_device *dev);

//-----------------------------------------
/*
// Chip specific info 
typedef struct {
	int cfg_base;         	// Config register IO base 
    int sir_base;     	  	// SIR IO base 
	int fir_base;         	// FIR IO base 
	int mem_base;         	// Shared memory base 
    int sir_ext;      		// Length of SIR iobase 
	int fir_ext;          	// Length of FIR iobase 
    int irq, irq2;    		// Interrupts used 
    int dma, dma2;    		// DMA channel(s) used 
    int fifo_size;    		// FIFO size 
    int irqflags;     		// interrupt flags (ie, SA_SHIRQ|SA_INTERRUPT) 
	int direction;        	// Link direction, used by some FIR drivers 
	int enabled;          	// Powered on? 
	int suspended;        	// Suspended by APM 
	__u32 speed;          	// Currently used speed 
	__u32 new_speed;      	// Speed we must change to when Tx is finished 
	int dongle_id;        	// Dongle or transceiver currently used 
} chipio_t;
*/
void DumpIR(struct faraday_ir *self)
{
	
	//chipio_t io;               /* IrDA controller information */
	//iobuff_t tx_buff;          // Transmit buffer 
	//iobuff_t rx_buff;          // Receive buffer 

	P_DEBUG("new_speed=0x%lX\n",self->new_speed);
	P_DEBUG("flag=0x%lX\n",self->flags);
	
	P_DEBUG("io.cfg_base=0x%lX\n",self->io.cfg_base);
	P_DEBUG("io.sir_base=0x%lX\n",self->io.sir_base);
	P_DEBUG("io.fir_base=0x%lX\n",self->io.fir_base);
	P_DEBUG("io.mem_base=0x%lX\n",self->io.mem_base);
	P_DEBUG("io.sir_ext=0x%lX\n",self->io.sir_ext);
	P_DEBUG("io.fir_ext=0x%lX\n",self->io.fir_ext);
	P_DEBUG("io.irq=0x%lX\n",self->io.irq);
	P_DEBUG("io.irq2=0x%lX\n",self->io.irq2);
	P_DEBUG("io.dma=0x%lX\n",self->io.dma);
	P_DEBUG("io.dma2=0x%lX\n",self->io.dma2);	
	P_DEBUG("io.fifo_size=0x%lX\n",self->io.fifo_size);
	P_DEBUG("io.irqflags=0x%lX\n",self->io.irqflags);
	P_DEBUG("io.direction=0x%lX\n",self->io.direction);
	P_DEBUG("io.enabled=0x%lX\n",self->io.enabled);
	P_DEBUG("io.suspended=0x%lX\n",self->io.suspended);
	P_DEBUG("io.speed=0x%lX\n",self->io.speed);
	P_DEBUG("io.new_speed=0x%lX\n",self->io.new_speed);
	P_DEBUG("io.dongle_id=0x%lX\n",self->io.dongle_id);
	
}
//--------------------------------------
void DumpIRReg(struct faraday_ir *self)
{
	u32 iobase=self->io.fir_base;
	u32 i;
	
	P_DEBUG("--------------DumpIRReg(base=0x%X)-----------------\n",iobase);
	for(i=iobase;i<(self->io.fir_ext+iobase);i+=4)
		{
			P_DEBUG("reg(0x%X)--0x%X\n",i,INW(i));
		}
	P_DEBUG("\n");	
	
}
//----------------------------------------
void DumpBuff(char *buff,u32 count)
{
	u32 i;
	for(i=0;i<count;i++)
		printk("0x%X ",buff[i]);
	printk("\n");	
}
//---------------------------------------
/*
 * Function faraday_ir_open (iobase, irq)
 *
 *    Open driver instance
 *
 */
int faraday_ir_open(int i, unsigned int iobase, 
			unsigned int irq1, unsigned int irq2,
		  	unsigned int dma)
{
	struct net_device *dev;
        struct faraday_ir *self;
	void *ret;
	int err;

	P_DEBUG( __FUNCTION__ "()\n");

	if (faraday_ir_probe(iobase, irq1, dma) == -1)
		return -1;

	/*
	 *  Allocate new instance of the driver
	 */
	self = kmalloc(sizeof(struct faraday_ir), GFP_KERNEL);
	if (self == NULL) {
		printk( KERN_ERR "IrDA: Can't allocate memory for "
			"IrDA control block!\n");
		return -ENOMEM;
	}
	memset(self, 0, sizeof(struct faraday_ir));
   
	/* Need to store self somewhere */
	dev_self[i] = self;

	/* Initialize IO */
	self->io.fir_base  = iobase;
    self->io.irq       = irq1;
    self->io.irq2      = irq2;
    self->io.fir_ext   = CHIP_IO_EXTENT;
    self->io.dma       = dma;
    //self->io.fifo_size = 32;
    self->io.fifo_size = 64;		//paulong test test

	/* Lock the port that we need */
	ret = REQUEST_IO_REGION(self->io.fir_base, self->io.fir_ext, driver_name);
	if (!ret) { 
		P_DEBUG( __FUNCTION__ "(), can't get iobase of 0x%03x\n",
		      self->io.fir_base);
		/* faraday_ir_cleanup( self);  */
		return -ENODEV;
	}
		
	/* Initialize QoS for this device */
	irda_init_max_qos_capabilies(&self->qos);
	
	/* The only value we must override it the baudrate */

	/* FIXME: The HP HDLS-1100 does not support 1152000! */
	self->qos.baud_rate.bits = IR_9600|IR_19200|IR_38400|IR_57600|IR_115200;
		/*|IR_576000|IR_1152000|(IR_4000000 << 8)*/	//paulong test test

	/* The HP HDLS-1100 needs 1 ms according to the specs */
	self->qos.min_turn_time.bits = qos_mtt_bits;
	irda_qos_bits_to_value(&self->qos);
	
	self->flags = IFF_FIR|IFF_MIR|IFF_SIR|IFF_DMA|IFF_PIO;

	/* Max DMA buffer size needed = (data_size + 6) * (window_size) + 6; */
	//self->rx_buff.truesize = 14384; 	//paulong test test
	//self->tx_buff.truesize = 4000;	//paulong test test
	self->rx_buff.truesize = RX_BUFF_SIZE; 	//paulong test test
	self->tx_buff.truesize = TX_BUFF_SIZE;	//paulong test test
	
	/* Allocate memory if needed */
	self->rx_buff.head = (__u8 *) kmalloc(self->rx_buff.truesize,
					      GFP_KERNEL|GFP_DMA);
	if (self->rx_buff.head == NULL)
		return -ENOMEM;

	memset(self->rx_buff.head, 0, self->rx_buff.truesize);
	
	self->tx_buff.head = (__u8 *) kmalloc(self->tx_buff.truesize, 
					      GFP_KERNEL|GFP_DMA);
	if (self->tx_buff.head == NULL) {
		kfree(self->rx_buff.head);
		return -ENOMEM;
	}
	memset(self->tx_buff.head, 0, self->tx_buff.truesize);

	self->rx_buff.in_frame = FALSE;
	self->rx_buff.state = OUTSIDE_FRAME;
	self->tx_buff.data = self->tx_buff.head;
	self->rx_buff.data = self->rx_buff.head;
	
	if (!(dev = dev_alloc("irda%d", &err))) {
		ERROR(__FUNCTION__ "(), dev_alloc() failed!\n");
		return -ENOMEM;
	}
	dev->priv = (void *) self;
	self->netdev = dev;

	faraday_ir_change_speed(self,9600);		//paulong test test
	
	/* Override the network functions we need to use */
	dev->init            = faraday_ir_net_init;
	dev->hard_start_xmit = faraday_ir_hard_xmit;
	dev->open            = faraday_ir_net_open;
	dev->stop            = faraday_ir_net_close;
	dev->do_ioctl        = faraday_ir_net_ioctl;
	dev->get_stats	     = faraday_ir_net_get_stats;

	rtnl_lock();
	err = register_netdevice(dev);
	rtnl_unlock();
	if (err) {
		ERROR(__FUNCTION__ "(), register_netdevice() failed!\n");
		return -1;
	}
	MESSAGE("IrDA: Registered device %s\n", dev->name);
		
	return 0;
}
//------------------------------
/*
 * Function faraday_ir_close (self)
 *
 *    Close driver instance
 *
 */
static int faraday_ir_close(struct faraday_ir *self)
{
	int iobase;

	P_DEBUG( __FUNCTION__ "()\n");

        iobase = self->io.fir_base;

/*#ifdef CONFIG_USE_W977_PNP
	// enter PnP configuration mode 
	w977_efm_enter(efio);

	w977_select_device(W977_DEVICE_IR, efio);

	// Deactivate device 
	w977_write_reg(0x30, 0x00, efio);

	w977_efm_exit(efio);
#endif // CONFIG_USE_W977_PNP */

	/* Remove netdevice */
	if (self->netdev) {
		rtnl_lock();
		unregister_netdevice(self->netdev);
		rtnl_unlock();
	}

	/* Release the PORT that this driver is using */
	P_DEBUG( __FUNCTION__ "(), Releasing Region %03x\n", 
	      self->io.fir_base);
	RELEASE_IO_REGION(self->io.fir_base, self->io.fir_ext);

	if (self->tx_buff.head)
		kfree(self->tx_buff.head);
	
	if (self->rx_buff.head)
		kfree(self->rx_buff.head);

	kfree(self);

	return 0;
}
//------------------------------
int faraday_ir_probe( int iobase, int irq, int dma)
{
	u8 test;
	P_DEBUG( __FUNCTION__ "() 1\n");
	
	P_DEBUG( __FUNCTION__ "() iobase=0x%lX IO_ADDRESS1(iobase)=0x%lX\n",iobase,IO_ADDRESS1(iobase));
	P_DEBUG( __FUNCTION__ "() IO_ADDRESS1=0x%lX IO_BASE1=0x%lX\n",IO_ADDRESS1(0),IO_BASE);
	test=INB(iobase+SERIAL_FMLSR);
	P_DEBUG( __FUNCTION__ "() 2\n");
	if(test==0xc3)
		{
			printf("Faraday IRDA driver loaded. \n"	);
			return 0;
		}
	else
		return -1;
/*
  	int version;
	int i;
  	
 	for (i=0; i < 2; i++) {
 		P_DEBUG(  __FUNCTION__ "()\n");
#ifdef CONFIG_USE_W977_PNP
 		// Enter PnP configuration mode 
		w977_efm_enter(efbase[i]);
  
 		w977_select_device(W977_DEVICE_IR, efbase[i]);
  
 		// Configure PnP port, IRQ, and DMA channel 
 		w977_write_reg(0x60, (iobase >> 8) & 0xff, efbase[i]);
 		w977_write_reg(0x61, (iobase) & 0xff, efbase[i]);
  
 		w977_write_reg(0x70, irq, efbase[i]);

 		w977_write_reg(0x74, dma, efbase[i]);   

 		w977_write_reg(0x75, 0x04, efbase[i]);  // Disable Tx DMA 
  	
 		// Set append hardware CRC, enable IR bank selection 
 		w977_write_reg(0xf0, APEDCRC|ENBNKSEL, efbase[i]);
  
 		// Activate device 
 		w977_write_reg(0x30, 0x01, efbase[i]);
  
 		w977_efm_exit(efbase[i]);
#endif // CONFIG_USE_W977_PNP 
  		// Disable Advanced mode 
  		switch_bank(iobase, SET2);
  		//OUTB1(iobase+2, 0x00);  
 
 		// Turn on UART (global) interrupts 
 		switch_bank(iobase, SET0);
  		//OUTB1(HCR_EN_IRQ, iobase+HCR);
  	
  		// Switch to advanced mode 
  		switch_bank(iobase, SET2);
  		//OUTB1(INB1(iobase+ADCR1) | ADCR1_ADV_SL, iobase+ADCR1);
  
  		// Set default IR-mode 
  		switch_bank(iobase, SET0);
  		//OUTB1(HCR_SIR, iobase+HCR);
  
  		// Read the Advanced IR ID 
  		switch_bank(iobase, SET3);
  		version = INB1(iobase+AUID);
  	
  		// Should be 0x1? 
  		if (0x10 == (version & 0xf0)) {
 			efio = efbase[i];
 
 			// Set FIFO size to 32 
 			switch_bank(iobase, SET2);
 			//OUTB1(ADCR2_RXFS32|ADCR2_TXFS32, iobase+ADCR2);	
 	
 			// Set FIFO threshold to TX17, RX16 
 			switch_bank(iobase, SET0);	
 			//OUTB1(UFR_RXTL|UFR_TXTL|UFR_TXF_RST|UFR_RXF_RST|
			     UFR_EN_FIFO,iobase+UFR);
 
 			// Receiver frame length 
 			switch_bank(iobase, SET4);
			//OUTB1(2048 & 0xff, iobase+6);
			//OUTB1((2048 >> 8) & 0x1f, iobase+7);

			// 
			 // Init HP HSDL-1100 transceiver. 
			 // 
			 //Set IRX_MSL since we have 2 * receive paths IRRX, 
			 //and IRRXH. Clear IRSL0D since we want IRSL0 * to 
			 //be a input pin used for IRRXH 
			 //
			 //  IRRX  pin 37 connected to receiver 
			 //  IRTX  pin 38 connected to transmitter
			 //  FIRRX pin 39 connected to receiver      (IRSL0) 
			 //  CIRRX pin 40 connected to pin 37
			 
			switch_bank(iobase, SET7);
			//OUTB1(0x40, iobase+7);
			
			MESSAGE("Faraday (IR) driver loaded. "
				"Version: 0x%02x\n", version);
			
			return 0;
		} else {
			// Try next extented function register address 
			P_DEBUG(  __FUNCTION__ "(), Wrong chip version");
		}
  	}   	
	return -1;*/
}
//------------------------------
void faraday_ir_change_speed(struct faraday_ir *self, __u32 speed)
{
	int ir_mode = SERIAL_MDR_SIR;//HCR_SIR;
	int iobase; 
	//__u8 set;

	P_DEBUG( __FUNCTION__ "() speed=%ld\n",speed);
	iobase = self->io.fir_base;

	/* Update accounting for new speed */
	self->io.speed = speed;

	/* Save current bank */
	//set = INB1(iobase+SSR);

	/* Disable interrupts */
	//switch_bank(iobase, SET0);
	//OUTB1(0, iobase+ICR);
	fLib_ResetSerialInt(iobase);

	/* Select Set 2 */
	//switch_bank(iobase, SET2);
	//OUTB1(0x00, iobase+ABHL);

	switch (speed) {
	case 9600:   
	case 19200:  
	case 38400:  
	case 57600:  
	case 115200:
				ir_mode=SERIAL_MDR_SIR;
				P_DEBUG( __FUNCTION__ "(), handling baud of %d\n",speed);
				fLib_SetSIRSpeed(iobase,speed);
				 break;
	/*case 576000:		not support MIR paulong test test
		ir_mode = HCR_MIR_576;
		P_DEBUG( __FUNCTION__ "(), handling baud of 576000\n");
		break;
	case 1152000:
		ir_mode = HCR_MIR_1152;
		P_DEBUG( __FUNCTION__ "(), handling baud of 1152000\n");
		break;*/
	case 4000000:
		//ir_mode = HCR_FIR;
		ir_mode=SERIAL_MDR_FIR;
		P_DEBUG( __FUNCTION__ "(), handling baud of 4000000\n");
		break;
	default:		//SIR 9600
		ir_mode=SERIAL_MDR_SIR;
		fLib_SetSIRSpeed(iobase,9600);
		P_DEBUG( __FUNCTION__ "(), unknown baud rate of %d\n", speed);
		break;
	}

	/* Set speed mode */
	//switch_bank(iobase, SET0);
	//OUTB1(ir_mode, iobase+HCR);
	fLib_SetIRMode(iobase,ir_mode);

	/* set FIFO size to 32 */
	//switch_bank(iobase, SET2);
	//OUTB1(ADCR2_RXFS32|ADCR2_TXFS32, iobase+ADCR2);	
		
	/* set FIFO threshold to TX17, RX16 */
	//switch_bank(iobase, SET0);
	//OUTB1(0x00, iobase+UFR);        /* Reset */
	//OUTB1(UFR_EN_FIFO, iobase+UFR); /* First we must enable FIFO */
	//OUTB1(0xa7, iobase+UFR);
	// fixed to 64 on 20030825 paulong
	fLib_SetFIFO(iobase);
	
	netif_wake_queue(self->netdev);
		
	/* Enable some interrupts so we can receive frames */
	//switch_bank(iobase, SET0);
/*#ifdef USE_IR_DMA	
	if (speed > PIO_MAX_SPEED) {
		OUTB1(ICR_EFSFI, iobase+ICR);
		faraday_ir_dma_receive(self);
	} 
	else
#endif	//USE_IR_DMA	*/
	//	OUTB1(ICR_ERBRI, iobase+ICR);
	fLib_EnableSerialInt(iobase,SERIAL_IER_DR);
    	
	/* Restore SSR */
	//OUTB1(set, iobase+SSR);
}
//--------------------------------------------------------
/*
 * Function faraday_ir_hard_xmit (skb, dev)
 *
 *    Sets up a DMA transfer to send the current frame.
 *
 */
int faraday_ir_hard_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct faraday_ir *self;
	__s32 speed;
	int iobase;
	//__u8 set;
	//int mtt;
	u32 actual;
	
	self = (struct faraday_ir *) dev->priv;

	
	iobase = self->io.fir_base;

	P_DEBUG( __FUNCTION__ "(%ld), skb->len=%d\n", jiffies, 
		   (int) skb->len);
//#ifdef P_DEBUG	
//	DumpIRReg(self);	
//#endif	
	/* Lock transmit buffer */
	netif_stop_queue(dev);
	
	/* Check if we need to change the speed */
	speed = irda_get_next_speed(skb);
	if ((speed != self->io.speed) && (speed != -1)) {
		/* Check for empty frame */
		if (!skb->len) {
			faraday_ir_change_speed(self, speed); 
			dev_kfree_skb(skb);
			return 0;
		} else
			self->new_speed = speed;
	}

	/* Save current set */
	//set = INB1(iobase+SSR);

/*#ifdef USE_IR_DMA	
	// Decide if we should use PIO or DMA transfer 
  if (self->io.speed > PIO_MAX_SPEED) 
	{
		self->tx_buff.data = self->tx_buff.head;
		memcpy(self->tx_buff.data, skb->data, skb->len);
		self->tx_buff.len = skb->len;
		
		mtt = irda_get_mtt(skb);
//#ifdef CONFIG_USE_INTERNAL_TIMER
	        //if (mtt > 50) {
			// Adjust for timer resolution 
			//mtt /= 1000+1;

			// Setup timer 
			//switch_bank(iobase, SET4);
			//OUTB1(mtt & 0xff, iobase+TMRL);
			//OUTB1((mtt >> 8) & 0x0f, iobase+TMRH);
			
			// Start timer 
			//OUTB1(IR_MSL_EN_TMR, iobase+IR_MSL);
			//self->io.direction = IO_XMIT;
			
			// Enable timer interrupt 
			//switch_bank(iobase, SET0);
			//OUTB1(ICR_ETMRI, iobase+ICR);
		//} else {
//#endif
			P_DEBUG(__FUNCTION__ "(%ld), mtt=%d\n", jiffies, mtt);
			if (mtt)
				udelay(mtt);

			// Enable DMA interrupt 
			//switch_bank(iobase, SET0);
	 		OUTB1(ICR_EDMAI, iobase+ICR);
	     	faraday_ir_dma_write(self, iobase);
//#ifdef CONFIG_USE_INTERNAL_TIMER
		//}
//#endif
	}
   else //if (self->io.speed > PIO_MAX_SPEED) 
#endif //USE_IR_DMA		*/
	{
		self->tx_buff.data = self->tx_buff.head;
		self->tx_buff.len = async_wrap_skb(skb, self->tx_buff.data, 
						   self->tx_buff.truesize);
#ifdef P_DEBUG					
		P_DEBUG("skb->data=0x%X,skb->len=0x%X\n",skb->data,skb->len);	   
		DumpBuff(skb->data,skb->len);
		P_DEBUG("self->tx_buff.data=0x%X,self->tx_buff.len=0x%X\n",self->tx_buff.data,self->tx_buff.len);	   
		DumpBuff(self->tx_buff.data,self->tx_buff.len);
#endif		
		/* Add interrupt on tx low level (will fire immediately) */
		//switch_bank(iobase, SET0);
		//OUTB1(ICR_ETXTHI, iobase+ICR);
		P_DEBUG("IER=0x%X, IIR=0x%X, LCR=0x%X \n",INW(iobase+SERIAL_IER),INW(iobase+SERIAL_IIR),INW(iobase+SERIAL_LCR));
		//fLib_EnableSerialInt(iobase,SERIAL_IER_TE);			
	
		/* Write data left in transmit buffer */
		actual = faraday_ir_pio_write(self->io.fir_base, 
					    self->tx_buff.data, 
					    self->tx_buff.len, 
					    self->io.fifo_size);

		self->tx_buff.data += actual;
		self->tx_buff.len  -= actual;
		
		self->io.direction = IO_XMIT;

			self->stats.tx_bytes+=actual;
			self->stats.tx_packets++;

			if()
			netif_wake_queue(self->netdev);
			
		}
	}
	}
	dev_kfree_skb(skb);

	// Restore set register 
	//OUTB1(set, iobase+SSR);

	return 0;
}
/*#ifdef USE_IR_DMA	
//
// Function faraday_ir_dma_write (self, iobase)
// 
//     Send frame using DMA
// 
// 
static void faraday_ir_dma_write(struct faraday_ir *self, int iobase)
{
	__u8 set;

        P_DEBUG( __FUNCTION__ "(), len=%d\n", self->tx_buff.len);

	// Save current set 
	set = INB1(iobase+SSR);

	// Disable DMA 
	switch_bank(iobase, SET0);
	OUTB1(INB1(iobase+HCR) & ~HCR_EN_DMA, iobase+HCR);

	// Choose transmit DMA channel   
	switch_bank(iobase, SET2);
	OUTB1(ADCR1_D_CHSW|ADCR1_ADV_SL, iobase+ADCR1);	//ADCR1_DMA_F|

	setup_dma(self->io.dma, self->tx_buff.data, self->tx_buff.len, 
		  DMA_MODE_WRITE);	

	self->io.direction = IO_XMIT;
	
	// Enable DMA 
 	switch_bank(iobase, SET0);

	OUTB1(INB1(iobase+HCR) | HCR_EN_DMA | HCR_TX_WT, iobase+HCR);


	// Restore set register 
	OUTB1(set, iobase+SSR);
}


//
//  Function faraday_ir_dma_xmit_complete (self)
// 
//     The transfer of a frame in finished. So do the necessary things
// 
//     
// 
void faraday_ir_dma_xmit_complete(struct faraday_ir *self)
{
	int iobase;
	__u8 set;

	P_DEBUG( __FUNCTION__ "(%ld)\n", jiffies);

	ASSERT(self != NULL, return;);

	iobase = self->io.fir_base;

	// Save current set 
	set = INB1(iobase+SSR);

	// Disable DMA 
	switch_bank(iobase, SET0);
	OUTB1(INB1(iobase+HCR) & ~HCR_EN_DMA, iobase+HCR);
	
	// Check for underrrun! 
	if (INB1(iobase+AUDR) & AUDR_UNDR) {
		P_DEBUG( __FUNCTION__ "(), Transmit underrun!\n");
		
		self->stats.tx_errors++;
		self->stats.tx_fifo_errors++;

		// Clear bit, by writing 1 to it 
		OUTB1(AUDR_UNDR, iobase+AUDR);
	} else
		self->stats.tx_packets++;

	
	if (self->new_speed) {
		faraday_ir_change_speed(self, self->new_speed);
		self->new_speed = 0;
	}

	// Unlock tx_buff and request another frame 
	// Tell the network layer, that we want more frames 
	netif_wake_queue(self->netdev);
	
	// Restore set 
	OUTB1(set, iobase+SSR);
}



//
// Function faraday_ir_dma_receive (self)
//
//    Get ready for receiving a frame. The device will initiate a DMA
//    if it starts to receive a frame.
// 
// 
int faraday_ir_dma_receive(struct faraday_ir *self) 
{
	int iobase;
	__u8 set;

	ASSERT(self != NULL, return -1;);

	P_DEBUG( __FUNCTION__ "\n");

	iobase= self->io.fir_base;

	// Save current set 
	set = INB1(iobase+SSR);

	// Disable DMA 
	switch_bank(iobase, SET0);
	OUTB1(INB1(iobase+HCR) & ~HCR_EN_DMA, iobase+HCR);

	// Choose DMA Rx, DMA Fairness, and Advanced mode 
	switch_bank(iobase, SET2);
	OUTB1((INB1(iobase+ADCR1) & ~ADCR1_D_CHSW)|ADCR1_ADV_SL,	//|ADCR1_DMA_F
	     iobase+ADCR1);

	self->io.direction = IO_RECV;
	self->rx_buff.data = self->rx_buff.head;


	setup_dma(self->io.dma, self->rx_buff.data, self->rx_buff.truesize, 
		  DMA_MODE_READ);

	// 
	// Reset Rx FIFO. This will also flush the ST_FIFO, it's very 
	// important that we don't reset the Tx FIFO since it might not
	//  be finished transmitting yet
	 
	switch_bank(iobase, SET0);
	OUTB1(UFR_RXTL|UFR_TXTL|UFR_RXF_RST|UFR_EN_FIFO, iobase+UFR);
	self->st_fifo.len = self->st_fifo.tail = self->st_fifo.head = 0;
	
	// Enable DMA 
	switch_bank(iobase, SET0);

	OUTB1(INB1(iobase+HCR) | HCR_EN_DMA, iobase+HCR);

	// Restore set 
	OUTB1(set, iobase+SSR);

	return 0;
}

//
//  Function faraday_ir_receive_complete (self)
// 
//    Finished with receiving a frame
//
 
int faraday_ir_dma_receive_complete(struct faraday_ir *self)
{
	struct sk_buff *skb;
	struct st_fifo *st_fifo;
	int len;
	int iobase;
	__u8 set;
	__u8 status;

	P_DEBUG( __FUNCTION__ "\n");

	st_fifo = &self->st_fifo;

	iobase = self->io.fir_base;

	/ Save current set 
	set = INB1(iobase+SSR);
	
	iobase = self->io.fir_base;

	// Read status FIFO 
	switch_bank(iobase, SET5);
	while ((status = INB1(iobase+FS_FO)) & FS_FO_FSFDR) {
		st_fifo->entries[st_fifo->tail].status = status;
		
		st_fifo->entries[st_fifo->tail].len  = INB1(iobase+RFLFL);
		st_fifo->entries[st_fifo->tail].len |= INB1(iobase+RFLFH) << 8;
		
		st_fifo->tail++;
		st_fifo->len++;
	}
	
	while (st_fifo->len) {
		// Get first entry 
		status = st_fifo->entries[st_fifo->head].status;
		len    = st_fifo->entries[st_fifo->head].len;
		st_fifo->head++;
		st_fifo->len--;

		// Check for errors 
		if (status & FS_FO_ERR_MSK) {
			if (status & FS_FO_LST_FR) {
				// Add number of lost frames to stats 
				self->stats.rx_errors += len;	
			} else {
				// Skip frame 
				self->stats.rx_errors++;
				
				self->rx_buff.data += len;
				
				if (status & FS_FO_MX_LEX)
					self->stats.rx_length_errors++;
				
				if (status & FS_FO_PHY_ERR) 
					self->stats.rx_frame_errors++;
				
				if (status & FS_FO_CRC_ERR) 
					self->stats.rx_crc_errors++;
			}
			// The errors below can be reported in both cases 
			if (status & FS_FO_RX_OV)
				self->stats.rx_fifo_errors++;
			
			if (status & FS_FO_FSF_OV)
				self->stats.rx_fifo_errors++;
			
		} else {
			// Check if we have transferred all data to memory 
			switch_bank(iobase, SET0);
			if (INB1(iobase+USR) & USR_RDR) {
#ifdef CONFIG_USE_INTERNAL_TIMER
				// Put this entry back in fifo 
				st_fifo->head--;
				st_fifo->len++;
				st_fifo->entries[st_fifo->head].status = status;
				st_fifo->entries[st_fifo->head].len = len;
				
				// Restore set register 
				OUTB1(set, iobase+SSR);
			
				return FALSE; 	// I'll be back! 
#else
				udelay(80); // Should be enough!? 
#endif
			}
						
			skb = dev_alloc_skb(len+1);
			if (skb == NULL)  {
				printk(KERN_INFO __FUNCTION__ 
				       "(), memory squeeze, dropping frame.\n");
				// Restore set register 
				OUTB1(set, iobase+SSR);

				return FALSE;
			}
			
			//  Align to 20 bytes 
			skb_reserve(skb, 1); 
			
			// Copy frame without CRC 
			if (self->io.speed < 4000000) {
				skb_put(skb, len-2);
				memcpy(skb->data, self->rx_buff.data, len-2);
			} else {
				skb_put(skb, len-4);
				memcpy(skb->data, self->rx_buff.data, len-4);
			}

			// Move to next frame 
			self->rx_buff.data += len;
			self->stats.rx_packets++;
			
			skb->dev = self->netdev;
			skb->mac.raw  = skb->data;
			skb->protocol = htons(ETH_P_IRDA);
			netif_rx(skb);
		}
	}
	// Restore set register 
	OUTB1(set, iobase+SSR);

	return TRUE;
}


#endif //#ifdef USE_IR_DMA	*/
//-------------------------------------
/*
 * Function faraday_ir_pio_write (iobase, buf, len, fifo_size)
 *
 *    
 *
 */
static int faraday_ir_pio_write(int iobase, __u8 *buf, int len, int fifo_size)
{
	int actual = 0;
	//__u8 set;
	
	P_DEBUG( __FUNCTION__ "(buf=0x%X len=0x%X fifo_size=0x%X)\n",buf, len, fifo_size);

	/* Save current bank */
	//set = INB1(iobase+SSR);

	//switch_bank(iobase, SET0);
	//if (!(INB1_p(iobase+USR) & USR_TSRE)) {
	if((fLib_SerialIntIdentification(iobase)& SERIAL_IIR_TE)!=0)
	{
		
		P_DEBUG( __FUNCTION__  "(), warning, FIFO not empty yet!\n");

		//fifo_size -= 17;  paulong test test 17 should be fifo triggle level
		P_DEBUG( __FUNCTION__ " %d bytes left in tx fifo\n", fifo_size);
	}
	
#ifdef P_DEBUG						   
		DumpBuff(buf,len);
#endif	
	// Fill FIFO with current frame 
	while ((fifo_size-- > 0) && (actual < len)) {
		// Transmit next byte 
		//OUTB1(buf[actual++], iobase+TBR);		
		OUTW(iobase + SERIAL_THR,buf[actual++]);
	}
        
	P_DEBUG( __FUNCTION__ "(), fifo_size %d ; %d sent of %d\n", 
		   fifo_size, actual, len);

	/* Restore bank */
	//OUTB1(set, iobase+SSR);

	return actual;
}
//------------------------------------------------
/*
 * Function pc87108_pio_receive (self)
 *
 *    Receive all data in receiver FIFO
 *
 */
static void faraday_ir_pio_receive(struct faraday_ir *self) 
{
	__u8 byte = 0x00;
	int iobase;	
	//UINT32 status;

	P_DEBUG( __FUNCTION__ "()\n");

	ASSERT(self != NULL, return;);
	
	iobase = self->io.fir_base;
	
	/*  Receive all characters in Rx FIFO */
	do {
		//byte = INB1(iobase+RBR);
		byte = INW(iobase + SERIAL_RBR);
		async_unwrap_char(self->netdev, &self->stats, &self->rx_buff, 
				  byte);
	//} while (INB1(iobase+USR) & USR_RDR); // Data available 		
	}while((INW(iobase+SERIAL_LSR)&SERIAL_LSR_DR)!=0);
	
}
//------------------------------------------------------------
/*
 * Function faraday_ir_sir_interrupt (self, eir)
 *
 *    Handle SIR interrupt
 *
 */
static __u8 faraday_ir_sir_interrupt(struct faraday_ir *self, int iir)
{
	int actual;
	__u8 new_icr = 0;
	//__u8 set;
	int iobase;

	P_DEBUG( __FUNCTION__ "(), iir=%#x\n", iir);
	
	iobase = self->io.fir_base;
	//....................
	/* Transmit FIFO low on data */
	//if (iir & ISR_TXTH_I) {
	/*
	if (iir & SERIAL_IIR_TE)
	{
		// Write data left in transmit buffer 
		actual = faraday_ir_pio_write(self->io.fir_base, 
					    self->tx_buff.data, 
					    self->tx_buff.len, 
					    self->io.fifo_size);

		self->tx_buff.data += actual;
		self->tx_buff.len  -= actual;
		
		self->io.direction = IO_XMIT;

		// Check if finished 
		if (self->tx_buff.len > 0) {
			//new_icr |= ICR_ETXTHI;
			new_icr |= SERIAL_IIR_TE;
		} else {
			//set = INB1(iobase+SSR);
			//switch_bank(iobase, SET0);
			//OUTB1(AUDR_SFEND, iobase+AUDR); test test
			//OUTB1(set, iobase+SSR); 

			self->stats.tx_packets++;

			// Feed me more packets 
			netif_wake_queue(self->netdev);
			new_icr |= SERIAL_IIR_TE;
		}
	}*/
	//....................
	/*
	// Check if transmission has completed 
	if (iir & ISR_TXEMP_I) {		
		// Check if we need to change the speed? 
		if (self->new_speed) {
			P_DEBUG( __FUNCTION__ 
				   "(), Changing speed!\n");
			faraday_ir_change_speed(self, self->new_speed);
			self->new_speed = 0;
		}

		// Turn around and get ready to receive some data 
		self->io.direction = IO_RECV;
		new_icr |= ICR_ERBRI;
	}*/
	//....................
	/* Rx FIFO threshold or timeout */
	//if (iir & ISR_RXTH_I) {
	if (iir & SERIAL_IIR_DR)
	{
		faraday_ir_pio_receive(self);
		// Keep receiving 
		new_icr |= SERIAL_IIR_DR;
	}
	if (iir & SERIAL_IIR_TIMEOUT)
	{
		faraday_ir_pio_receive(self);
		// Keep receiving 
		new_icr |= SERIAL_IIR_TIMEOUT;
	}
	return new_icr;
}


/*#ifdef USE_IR_FIR
//
// Function pc87108_fir_interrupt (self, eir)
//
//    Handle MIR/FIR interrupt


static __u8 faraday_ir_fir_interrupt(struct faraday_ir *self, int iir)
{
	__u8 new_icr = 0;
	__u8 set;
	int iobase;

	P_DEBUG( __FUNCTION__ "(), iir=%#x\n", iir);
	
	iobase = self->io.fir_base;
	set = INB1(iobase+SSR);
	
	// End of frame detected in FIFO 
	if (iir & (ISR_FEND_I|ISR_FSF_I)) {
		if (faraday_ir_dma_receive_complete(self)) {
			
			// Wait for next status FIFO interrupt 
			new_icr |= ICR_EFSFI;
		} else {
			// DMA not finished yet 

			// Set timer value, resolution 1 ms 
			switch_bank(iobase, SET4);
			OUTB1(0x01, iobase+TMRL); // 1 ms 
			OUTB1(0x00, iobase+TMRH);

			// Start timer 
			OUTB1(IR_MSL_EN_TMR, iobase+IR_MSL);

			new_icr |= ICR_ETMRI;
		}
	}
	// Timer finished 
	if (iir & ISR_TMR_I) {
		// Disable timer 
		switch_bank(iobase, SET4);
		OUTB1(0, iobase+IR_MSL);

		// Clear timer event 
		// switch_bank(iobase, SET0); 
		//OUTB1(ASCR_CTE, iobase+ASCR); 

		// Check if this is a TX timer interrupt 
		if (self->io.direction == IO_XMIT) {
			faraday_ir_dma_write(self, iobase);

			new_icr |= ICR_EDMAI;
		} else {
			// Check if DMA has now finished 
			faraday_ir_dma_receive_complete(self);

			new_icr |= ICR_EFSFI;
		}
	}	
	// Finished with DMA 
	if (iir & ISR_DMA_I) {
		faraday_ir_dma_xmit_complete(self);

		// Check if there are more frames to be transmitted 
		// if (irda_device_txqueue_empty(self)) { 
		
		// Prepare for receive 
		//  Netwinder Tx DMA likes that we do this anyway 
		 
		faraday_ir_dma_receive(self);
		new_icr = ICR_EFSFI;
	       // } 
	}
	
	// Restore set 
	OUTB1(set, iobase+SSR);

	return new_icr;
}
#endif*/
//---------------------------------------------
/*
 * Function faraday_ir_interrupt (irq, dev_id, regs)
 *
 *    An interrupt from the chip has arrived. Time to do some work
 *
 */
static void faraday_ir_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	struct net_device *dev = (struct net_device *) dev_id;
	struct faraday_ir *self;
	//__u8 set, ier, iir;
	__u8 ier, iir;
	int iobase;

	P_DEBUG( __FUNCTION__ "(), irq=%#x\n", irq);
	if (!dev) {
		printk(KERN_WARNING "%s: irq %d for unknown device.\n", 
			driver_name, irq);
		return;
	}
	self = (struct faraday_ir *) dev->priv;

	iobase = self->io.fir_base;

	/* Save current bank */
	//set = INB1(iobase+SSR);
	//switch_bank(iobase, SET0);
	
	//icr = INB1(iobase+ICR); 
	ier=INW(iobase + SERIAL_IER);
	iir=fLib_SerialIntIdentification(iobase);
	//iir = INB1(iobase+ISR) & icr; / Mask out the interesting ones 	
	//OUTB1(0, iobase+ICR); 
	// Disable interrupts 
	fLib_ResetSerialInt(iobase); 
	
	//if (iir) {
		/* Dispatch interrupt handler for the current speed */
//		if (self->io.speed > PIO_MAX_SPEED )
	//		icr = faraday_ir_fir_interrupt(self, iir);
		//else
			ier = faraday_ir_sir_interrupt(self, iir);
	//}

	//OUTB1(icr, iobase+ICR);    // Restore (new) interrupts 
	fLib_EnableSerialInt(iobase,ier); 
	//OUTB1(set, iobase+SSR);    /* Restore bank register */

}

/*
 * Function faraday_ir_is_receiving (self)
 *
 *    Return TRUE is we are currently receiving a frame
 *
 */
static int faraday_ir_is_receiving(struct faraday_ir *self)
{
	int status = FALSE;
	int iobase;
	//__u8 set;

	ASSERT(self != NULL, return FALSE;);

	if (self->io.speed > 115200) {
		iobase = self->io.fir_base;

		/* Check if rx FIFO is not empty */
		//set = INB1(iobase+SSR);
		//switch_bank(iobase, SET2);
		//if ((INB1(iobase+RXFDTH) & 0x3f) != 0) {
		if((fLib_ReadSerialLineStatus(iobase) & 0x3f)!=0)
		 {
			/* We are receiving something */
			status =  TRUE;
		}
		//OUTB1(set, iobase+SSR);
	} else 
		status = (self->rx_buff.state != OUTSIDE_FRAME);
	
	return status;
}
//------------------------------------------------
/*
 * Function faraday_ir_net_init (dev)
 *
 *    
 *
 */
static int faraday_ir_net_init(struct net_device *dev)
{
	P_DEBUG( __FUNCTION__ "()\n");

	/* Set up to be a normal IrDA network device driver */
	irda_device_setup(dev);

	/* Insert overrides below this line! */

	return 0;
}

//----------------------------------------------
/*
 * Function faraday_ir_net_open (dev)
 *
 *    Start the device
 *
 */
static int faraday_ir_net_open(struct net_device *dev)
{
	struct faraday_ir *self;
	int iobase;
	char hwname[32];
	//__u8 set;
	
	P_DEBUG( __FUNCTION__ "()\n");
	
	ASSERT(dev != NULL, return -1;);
	self = (struct faraday_ir *) dev->priv;
	
	ASSERT(self != NULL, return 0;);
	
	iobase = self->io.fir_base;
	
	//enable interrupt
	fLib_SetIntTrig(self->io.irq,LEVEL,H_ACTIVE);
	if (request_irq(self->io.irq, faraday_ir_interrupt, 0, dev->name, 
			(void *) dev)) {
		return -EAGAIN;
	}
	//paulong add 20030825 test test
	fLib_SetIntTrig(self->io.irq2,LEVEL,H_ACTIVE);
	//if(self->io.irq2!=0)
		if (request_irq(self->io.irq2, faraday_ir_interrupt, 0, dev->name, (void *) dev)) 
			{
			free_irq(self->io.irq, dev);	
			return -EAGAIN;
			}
	/*
	 * Always allocate the DMA channel after the IRQ,
	 * and clean up on failure.
	 */
	if (request_dma(self->io.dma, dev->name)) {
		free_irq(self->io.irq, self);
		free_irq(self->io.irq2, self);
		return -EAGAIN;
	}
		
	/* Save current set */
	//set = INB1(iobase+SSR);

 	/* Enable some interrupts so we can receive frames again */
 	//switch_bank(iobase, SET0);
/*#ifdef USE_IR_DMA 	
 	if (self->io.speed > 115200) {	
 		//OUTB1(ICR_EFSFI, iobase+ICR);//FIR test test
 		faraday_ir_dma_receive(self);
 	} else
#endif 	*/
 		//OUTB1(ICR_ERBRI, iobase+ICR);
 		fLib_EnableSerialInt(iobase,SERIAL_IER_DR);

	/* Restore bank register */
	//OUTB1(set, iobase+SSR);

	/* Ready to play! */
	netif_start_queue(dev);
	
	/* Give self a hardware name */
	sprintf(hwname, "faraday @ 0x%03x", self->io.fir_base);

	/* 
	 * Open new IrLAP layer instance, now that everything should be
	 * initialized properly 
	 */
	self->irlap = irlap_open(dev, &self->qos, hwname);

	MOD_INC_USE_COUNT;

	return 0;
}
//----------------------------------------
/*
 * Function faraday_ir_net_close (dev)
 *
 *    Stop the device
 *
 */
static int faraday_ir_net_close(struct net_device *dev)
{
	struct faraday_ir *self;
	int iobase;
	//__u8 set;

	P_DEBUG( __FUNCTION__ "()\n");

	ASSERT(dev != NULL, return -1;);
	
	self = (struct faraday_ir *) dev->priv;
	
	ASSERT(self != NULL, return 0;);
	
	iobase = self->io.fir_base;

	/* Stop device */
	netif_stop_queue(dev);
	
	/* Stop and remove instance of IrLAP */
	if (self->irlap)
		irlap_close(self->irlap);
	self->irlap = NULL;

	disable_dma(self->io.dma);

	/* Save current set */
	//set = INB1(iobase+SSR);
	
	/* Disable interrupts */
	//switch_bank(iobase, SET0);
	//OUTB1(0, iobase+ICR); 
	fLib_ResetSerialInt(iobase);

	free_irq(self->io.irq, dev);
	free_irq(self->io.irq2, dev);	//paulong add
	free_dma(self->io.dma);

	/* Restore bank register */
	//OUTB1(set, iobase+SSR);

	MOD_DEC_USE_COUNT;

	return 0;
}
//------------------------------------------------
/*
 * Function faraday_ir_net_ioctl (dev, rq, cmd)
 *
 *    Process IOCTL commands for this device
 *
 */
static int faraday_ir_net_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct if_irda_req *irq = (struct if_irda_req *) rq;
	struct faraday_ir *self;
	unsigned long flags;
	int ret = 0;

	ASSERT(dev != NULL, return -1;);

	self = dev->priv;

	ASSERT(self != NULL, return -1;);

	//P_DEBUG( __FUNCTION__ "(), %s, (cmd=0x%X)\n", dev->name, cmd);
	
	/* Disable interrupts & save flags */
	save_flags(flags);
	cli();
	
	switch (cmd) {
	case SIOCSBANDWIDTH: /* Set bandwidth */
		if (!capable(CAP_NET_ADMIN)) {
			ret = -EPERM;
			break;
		}
		faraday_ir_change_speed(self, irq->ifr_baudrate);
		break;
	case SIOCSMEDIABUSY: /* Set media busy */
		if (!capable(CAP_NET_ADMIN)) {
			ret = -EPERM;
			goto out;
		}
		irda_device_set_media_busy(self->netdev, TRUE);
		break;
	case SIOCGRECEIVING: /* Check if we are receiving right now */
		irq->ifr_receiving = faraday_ir_is_receiving(self);
		break;
	default:
		ret = -EOPNOTSUPP;
	}
out:
	restore_flags(flags);
	return ret;
}
//-------------------------------------------------------
static struct net_device_stats *faraday_ir_net_get_stats(struct net_device *dev)
{
	struct faraday_ir *self = (struct faraday_ir *) dev->priv;
	
	return &self->stats;
}
//---------------------------------------
#ifdef MODULE

MODULE_AUTHOR("Paul Chiang <paulong@faraday.com.tw>");
MODULE_DESCRIPTION("Faraday CPE Platfrom IrDA Device Driver");
MODULE_LICENSE("GPL");


//MODULE_PARM(qos_mtt_bits, "i");
//MODULE_PARM_DESC(qos_mtt_bits, "Mimimum Turn Time");
//MODULE_PARM(io, "1-4i");
//MODULE_PARM_DESC(io, "Base I/O addresses");
//MODULE_PARM(irq, "1-4i");
//MODULE_PARM_DESC(irq, "IRQ lines");

#endif /* MODULE */

//---------------------------------------
/*
 * Function faraday_ir_init ()
 *
 *    Initialize chip. Just try to find out how many chips we are dealing with
 *    and where they are
 */
int __init faraday_ir_init(void)
{
        int i,ioaddr;

	P_DEBUG( __FUNCTION__ "()\n");

	//for (i=0; (io[i] < 2000) && (i < 4); i++) { 
	for (i=0; (io[i] != 0) && (i < 4); i++) { 
		P_DEBUG("faraday_ir_init() probe %d io=0x%X irq=%d/%d dma=%d\n",i,io[i],irq1[i],irq2[i],dma[i]);
		ioaddr = io[i];
		//if (CHECK_IO_REGION(ioaddr, CHIP_IO_EXTENT) < 0)
		//	continue;
		if (faraday_ir_open(i, io[i], irq1[i],irq2[i], dma[i]) == 0)
			return 0;
	}
	return -ENODEV;
}
//---------------------------------------
/*
 * Function faraday_ir_cleanup ()
 *
 *    Close all configured chips
 *
 */

void faraday_ir_cleanup(void)
{
	int i;

        P_DEBUG( __FUNCTION__ "()\n");

	for (i=0; i < 4; i++) {
		if (dev_self[i])
			faraday_ir_close(dev_self[i]);
	}
}
//-----------------------
module_init (faraday_ir_init);
module_exit (faraday_ir_cleanup);