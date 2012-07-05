/**************************************************
	2002/11/29:         lmc83
    2004/8/26 19:29:    ivan wang modified
***************************************************/
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <asm/bitops.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <linux/pci.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <asm/arch/cpe/cpe.h>
#include <asm/arch/cpe_int.h>
#include "ftmac100.h"

#define FTMAC100_DEBUG      1
#define CARDNAME            "FTMAC100"
static const char       version[]="Faraday FTMAC100 Driver,(Linux Kernel 2.4) 10/18/02 - by Faraday\n";
volatile int            trans_busy=0;
//static struct proc_dir_entry *proc_ftmac100;


void put_mac(int base, char *mac_addr)
{
	int val;

	val = ((u32)mac_addr[0])<<8 | (u32)mac_addr[1];
	outl(val, base);
	val = ((((u32)mac_addr[2])<<24)&0xff000000) |
		  ((((u32)mac_addr[3])<<16)&0xff0000) |
		  ((((u32)mac_addr[4])<<8)&0xff00)  |
		  ((((u32)mac_addr[5])<<0)&0xff);
	outl(val, base+4);
}

static char *mac_string="Faraday MAC";
static void auto_get_mac(int id,char *mac_addr)
{
    mac_addr[0]=0;
    mac_addr[1]=0x84;
    mac_addr[2]=0x14;
    mac_addr[3]=mac_string[2];
    mac_addr[4]=mac_string[3];
    mac_addr[5]=id;
}

void get_mac(int base, char *mac_addr)
{
	int val;

	//printk("+get_mac\n");

	val = inl(base);
	mac_addr[0] = (val>>8)&0xff;
	mac_addr[1] = val&0xff;
	val = inl(base+4);   //john add +4
	mac_addr[2] = (val>>24)&0xff;
	mac_addr[3] = (val>>16)&0xff;
	mac_addr[4] = (val>>8)&0xff;
	mac_addr[5] = val&0xff;
}

// --------------------------------------------------------------------
// 	Print the Ethernet address
// --------------------------------------------------------------------
void print_mac(char *mac_addr)
{
	int i;
	 
	printk("ADDR: ");
	for (i = 0; i < 5; i++)
	{
		printk("%2.2x:", mac_addr[i] );
	}
	printk("%2.2x \n", mac_addr[5] );
}



// --------------------------------------------------------------------
// 	Finds the CRC32 of a set of bytes.
//	Again, from Peter Cammaert's code.
// --------------------------------------------------------------------
static int crc32( char * s, int length ) 
{
	/* indices */
	int perByte;
	int perBit;
	/* crc polynomial for Ethernet */
	const unsigned long poly = 0xedb88320;
	/* crc value - preinitialized to all 1's */
	unsigned long crc_value = 0xffffffff;

	//printk("+crc32\n");

	for ( perByte = 0; perByte < length; perByte ++ ) {
		unsigned char	c;

		c = *(s++);
		for ( perBit = 0; perBit < 8; perBit++ ) {
			crc_value = (crc_value>>1)^
				(((crc_value^c)&0x01)?poly:0);
			c >>= 1;
		}
	}
	return	crc_value;
}


static void ftmac100_reset( struct net_device* dev )
{
	unsigned int	ioaddr = dev->base_addr;

	//printk("+ftmac100_reset\n");

	outl( SW_RST_bit, ioaddr + MACCR_REG );

	/* this should pause enough for the chip to be happy */
	for (; (inl( ioaddr + MACCR_REG ) & SW_RST_bit) != 0; )
		mdelay(10);

	outl( 0, ioaddr + IMR_REG );			/* Disable all interrupts */
}


/*
 . Function: ftmac100_enable
 . Purpose: let the chip talk to the outside work
 . Method:
 .	1.  Enable the transmitter
 .	2.  Enable the receiver
 .	3.  Enable interrupts
*/
static void ftmac100_enable( struct net_device *dev )
{
	unsigned int ioaddr 	= dev->base_addr;
	int i;
	struct ftmac100_local *lp 	= (struct ftmac100_local *)dev->priv;
    char mac_addr[6];

	//printk("+ftmac100_enable\n");
	//printk("%s:ftmac100_enable\n", dev->name);

	for (i=0; i<RXDES_NUM; ++i)
	{
		lp->rx_descs[i].RXDMA_OWN = OWNBY_FTMAC100;				// owned by FTMAC100
	}
	lp->rx_idx = 0;
	
	for (i=0; i<TXDES_NUM; ++i)
	{
		lp->tx_descs[i].TXDMA_OWN = OWNBY_SOFTWARE;			// owned by software
	}
	lp->tx_idx = 0;


	/* set the MAC address */
	put_mac(ioaddr + MAC_MADR_REG, dev->dev_addr);

	//john add
    get_mac(ioaddr + MAC_MADR_REG, mac_addr);
    print_mac(mac_addr);

	outl( lp->rx_descs_dma, ioaddr + RXR_BADR_REG);
	outl( lp->tx_descs_dma, ioaddr + TXR_BADR_REG);
	outl( 0x00001010, ioaddr + ITC_REG);
	outl( (0UL<<TXPOLL_CNT)|(0x1<<RXPOLL_CNT), ioaddr + APTC_REG);
	outl( 0x1df, ioaddr + DBLAC_REG );

	/* now, enable interrupts */
	outl( AHB_ERR_bit|NORXBUF_bit|RPKT_FINISH_bit,ioaddr + IMR_REG);
        
	/// enable trans/recv,...
	outl(lp->maccr_val, ioaddr + MACCR_REG );
}

/*
 . Function: ftmac100_shutdown
 . Purpose:  closes down the SMC91xxx chip.
 . Method:
 .	1. zero the interrupt mask
 .	2. clear the enable receive flag
 .	3. clear the enable xmit flags
 .
 . TODO:
 .   (1) maybe utilize power down mode.
 .	Why not yet?  Because while the chip will go into power down mode,
 .	the manual says that it will wake up in response to any I/O requests
 .	in the register space.   Empirical results do not show this working.
*/
static void ftmac100_shutdown( unsigned int ioaddr )
{
	//printk("+ftmac100_shutdown\n");

	outl( 0, ioaddr + IMR_REG );
	outl( 0, ioaddr + MACCR_REG );
}

/*
 . Function: ftmac100_wait_to_send_packet( struct sk_buff * skb, struct device * )
 . Purpose:
 .    Attempt to allocate memory for a packet, if chip-memory is not
 .    available, then tell the card to generate an interrupt when it
 .    is available.
 .
 . Algorithm:
 .
 . o	if the saved_skb is not currently null, then drop this packet
 .		on the floor.  This should never happen, because of TBUSY.
 . o	if the saved_skb is null, then replace it with the current packet,
 . o	See if I can sending it now.
 . o 	(NO): Enable interrupts and let the interrupt handler deal with it.
 . o	(YES):Send it now.
*/
static int ftmac100_wait_to_send_packet( struct sk_buff * skb, struct net_device * dev )
{
	struct ftmac100_local   *lp=(struct ftmac100_local *)dev->priv;
	unsigned int            ioaddr=dev->base_addr;
	volatile TX_DESC        *cur_desc;
	int		                length;
	unsigned long           flags;

	//printk("+ftmac100_wait_to_send_packet\n");

	spin_lock_irqsave(&lp->lock, flags);
	if (skb==NULL)
	{
		printk("%s(%d): NULL skb???\n", __FILE__,__LINE__);
		spin_unlock_irqrestore(&lp->lock, flags);
		return 0;
	}

	cur_desc = &lp->tx_descs[lp->tx_idx];

	for (; cur_desc->TXDMA_OWN != OWNBY_SOFTWARE;)
		;//printk("no empty TX descriptor:0x%x:0x%x\n",(unsigned int)cur_desc,(unsigned int)cur_desc[0]);
	length = ETH_ZLEN < skb->len ? skb->len : ETH_ZLEN;
	length = min(length, TX_BUF_SIZE);

	
#if FTMAC100_DEBUG > 2
	printk("Transmitting Packet at 0x%x\n",(unsigned int)cur_desc->VIR_TXBUF_BADR);
	print_packet( skb->data, length );
#endif
	memcpy((char *)cur_desc->VIR_TXBUF_BADR, skb->data, length);		/// waiting to do: 將 data 切成許多 segment

	cur_desc->TXBUF_Size = length;
	cur_desc->LTS = 1;
	cur_desc->FTS = 1;
	cur_desc->TX2FIC = 0;
	cur_desc->TXIC = 0;
	cur_desc->TXDMA_OWN = OWNBY_FTMAC100;
	outl( 0xffffffff, ioaddr + TXPD_REG);
	lp->tx_idx = (lp->tx_idx + 1) % TXDES_NUM;
	lp->stats.tx_packets++;
	dev_kfree_skb_any (skb);
	dev->trans_start = jiffies;
    spin_unlock_irqrestore(&lp->lock, flags);
	return 0;
}

/*-------------------------------------------------------------------------
 |
 | ftmac100_init( struct device * dev )
 |   Input parameters:
 |	dev->base_addr == 0, try to find all possible locations
 |	dev->base_addr == 1, return failure code
 |	dev->base_addr == 2, always allocate space,  and return success
 |	dev->base_addr == <anything else>   this is the address to check
 |
 |   Output:
 |	0 --> there is a device
 |	anything else, error
 |
 ---------------------------------------------------------------------------
*/
int __init ftmac100_init(struct net_device *dev,u32 irq, u32 base_addr)
{
	//static int inited=0;

	//printk("+ftmac100_init\n");

	SET_MODULE_OWNER (dev);

	/* john 
	if (inited != 0)
	{
		return -ENODEV;				// no more device
	}
	++inited;
    */

	dev->irq = irq;
	dev->base_addr = base_addr;
	return ftmac100_probe(dev, dev->base_addr);
}

int FTCmac_probe(struct net_device *dev)
{
	static int initialized;
	u32 irq, base_addr;

	//printk("+FTCmac_probe : dev = 0x%x\n",dev);

#if (CONFIG_FTMAC100_2)
	if (initialized >= 2)	/* Already initialized? */
		return 1;
#else
	if (initialized >= 1)	/* Already initialized? */
		return 1;
#endif

	initialized++;

	if (initialized == 1) {
       irq = IRQ_MAC;
       base_addr = IO_ADDRESS(CPE_FTMAC_BASE);
	}
	else {
       irq = IRQ_A321_MAC2;
       base_addr = IO_ADDRESS(CPE_FTMAC2_BASE);
	}

    /* setup mac address */
    auto_get_mac(initialized,dev->dev_addr);

	return ftmac100_init(dev,irq,base_addr);
}

/*-------------------------------------------------------------------------
 |
 | ftmac100_destructor( struct device * dev )
 |   Input parameters:
 |	dev, pointer to the device structure
 |
 |   Output:
 |	None.
 |
 ---------------------------------------------------------------------------
*/
void ftmac100_destructor(struct net_device *dev)
{
	//printk("+ftmac100_destructor\n");

	return;
}


void ftmac100_ringbuf_alloc(struct ftmac100_local *lp)
{
	int i;

	//printk("+ftmac100_ringbuf_alloc\n");

	lp->rx_descs = consistent_alloc( GFP_DMA|GFP_KERNEL, sizeof(RX_DESC)*RXDES_NUM, &(lp->rx_descs_dma) );

	if (lp->rx_descs == NULL || (( (u32)lp->rx_descs & 0xf)!=0))
	{
		printk("Receive Ring Buffer allocation error\n");
		BUG();
	}
	memset((unsigned int *)lp->rx_descs, 0, sizeof(RX_DESC)*RXDES_NUM);

	lp->rx_buf=consistent_alloc(GFP_DMA|GFP_KERNEL,RX_BUF_SIZE*RXDES_NUM,&(lp->rx_buf_dma));
	if (lp->rx_buf == NULL || (( (u32)lp->rx_buf % 4)!=0))
	{
		printk("Receive Ring Buffer allocation error\n");
		BUG();
	}
	
	for (i=0; i<RXDES_NUM; ++i)
	{
        lp->rx_descs[i].RXBUF_Size = RX_BUF_SIZE;
        lp->rx_descs[i].EDOTR = 0;							// not last descriptor
        lp->rx_descs[i].RXBUF_BADR = lp->rx_buf_dma+RX_BUF_SIZE*i;
        lp->rx_descs[i].VIR_RXBUF_BADR=(unsigned int)lp->rx_buf+RX_BUF_SIZE*i;
	}
	lp->rx_descs[RXDES_NUM-1].EDOTR = 1;					// is last descriptor
    
    lp->tx_descs = consistent_alloc( GFP_DMA|GFP_KERNEL, sizeof(TX_DESC)*TXDES_NUM, &(lp->tx_descs_dma) );
	if (lp->tx_descs == NULL || (( (u32)lp->tx_descs & 0xf)!=0))
	{
        printk("Transmit Ring Buffer allocation error\n");
        BUG();
	}
    memset((void *)lp->tx_descs,0,sizeof(TX_DESC)*TXDES_NUM);
    lp->tx_buf = consistent_alloc( GFP_DMA|GFP_KERNEL, TX_BUF_SIZE*TXDES_NUM, &(lp->tx_buf_dma) );
	if (lp->tx_buf == NULL || (( (u32)lp->tx_buf % 4)!=0))
    {
        printk("Transmit Ring Buffer allocation error\n");
        BUG();
    }

	
	for (i=0; i<TXDES_NUM; ++i)
	{
        lp->tx_descs[i].EDOTR = 0;							// not last descriptor
        lp->tx_descs[i].TXBUF_BADR=lp->tx_buf_dma+TX_BUF_SIZE*i;
        lp->tx_descs[i].VIR_TXBUF_BADR=(unsigned int)lp->tx_buf+TX_BUF_SIZE*i;
	}
    lp->tx_descs[TXDES_NUM-1].EDOTR = 1;					// is last descriptor
#if 0
    PRINTK("lp->rx_descs = %x, lp->rx_rx_descs_dma = %x\n",lp->rx_descs, lp->rx_descs_dma);
    PRINTK("lp->rx_buf = %x, lp->rx_buf_dma = %x\n",lp->rx_buf, lp->rx_buf_dma);
    PRINTK("lp->tx_descs = %x, lp->tx_rx_descs_dma = %x\n", lp->tx_descs, lp->tx_descs_dma);
    PRINTK("lp->tx_buf = %x, lp->tx_buf_dma = %x\n", lp->tx_buf, lp->tx_buf_dma);
#endif			
}

#if 0
static int ftmac100_read_proc(char *page, char **start,  off_t off, int count, int *eof, void *data)
{
    struct net_device       *dev = (struct net_device *)data;
    struct ftmac100_local   *lp 	= (struct ftmac100_local *)dev->priv;
    int                     num;
    int                     i;
    
	//printk("+ftmac100_read_proc\n");

    num = sprintf(page, "lp->rx_idx = %d\n", lp->rx_idx);
    for (i=0; i<RXDES_NUM; ++i)
        num += sprintf(page + num, "[%d].RXDMA_OWN = %d\n", i, lp->rx_descs[i].RXDMA_OWN);
    return num;
}
#endif

/*----------------------------------------------------------------------
 . Function: ftmac100_probe( unsigned int ioaddr )
 .
 . Purpose:
 .	Tests to see if a given ioaddr points to an ftmac100 chip.
 .	Returns a 0 on success
 .---------------------------------------------------------------------
 */
static int __init ftmac100_probe(struct net_device *dev, unsigned int ioaddr )
{
    int                     retval;
    static unsigned         version_printed = 0;
    struct ftmac100_local   *lp;
    
	//printk("+ftmac100_probe\n");

    if (version_printed++ == 0)
        printk("%s", version);
    dev->base_addr = ioaddr;
    
    /* now, print out the card info, in a short format.. */
    printk("%s: device at %#3x IRQ:%d NOWAIT:%d\n",dev->name, ioaddr, dev->irq, dev->dma);
    
    dev->priv = kmalloc(sizeof(struct ftmac100_local), GFP_KERNEL);
    if (dev->priv == NULL) 
    {
        retval = -ENOMEM;
        goto err_out;
    }
    
    memset(dev->priv, 0, sizeof(struct ftmac100_local));
    lp = (struct ftmac100_local *)dev->priv;
    spin_lock_init(&lp->lock);
    lp->maccr_val = FULLDUP_bit | CRC_APD_bit | MDC_SEL_bit | RCV_EN_bit | XMT_EN_bit  | RDMA_EN_bit	| XDMA_EN_bit ;
    ftmac100_ringbuf_alloc(lp);
    
    
    /* now, reset the chip, and put it into a known state */
    ftmac100_reset( dev );
    
    /* Fill in the fields of the device structure with ethernet values. */
    ether_setup(dev);
    
    cpe_int_set_irq(dev->irq, LEVEL, H_ACTIVE);
    /* Grab the IRQ */
    retval = request_irq(dev->irq, &ftmac100_interrupt, SA_INTERRUPT, dev->name, dev);
    if (retval)
    {
        printk("%s: unable to get IRQ %d (irqval=%d).\n", dev->name, dev->irq, retval);
        kfree (dev->priv);
        dev->priv = NULL;
        goto err_out;
    }
    
    dev->open	        	= ftmac100_open;
    dev->stop	        	= ftmac100_close;
    dev->hard_start_xmit    = ftmac100_wait_to_send_packet;
    dev->tx_timeout			= ftmac100_timeout;
    dev->get_stats			= ftmac100_query_statistics;
#ifdef	HAVE_MULTICAST
    dev->set_multicast_list = &ftmac100_set_multicast_list;
#endif
#if 0
    if ((proc_ftmac100 = create_proc_entry( "ftmac100", 0, 0 )))
    {
        proc_ftmac100->read_proc = ftmac100_read_proc;
        proc_ftmac100->data = dev;
        proc_ftmac100->owner = THIS_MODULE;
    }
#endif
    return 0;

err_out:
	return retval;
}


#if FTMAC100_DEBUG > 2
static void print_packet( unsigned char * buf, int length )
{
#if FTMAC100_DEBUG > 3
    int i;
    int remainder;
    int lines;
#endif

//	printk("Packet of length %d \n", length );

#if FTMAC100_DEBUG > 3
    lines = length / 16;
    remainder = length % 16;
    
    for ( i = 0; i < lines ; i ++ ) 
    {
        int cur;
    
        for ( cur = 0; cur < 8; cur ++ ) 
        {
            unsigned char a, b;
    
            a = *(buf ++ );
            b = *(buf ++ );
            printk("%02x%02x ", a, b );
        }
        printk("\n");
    }
    for ( i = 0; i < remainder/2 ; i++ ) 
    {
        unsigned char a, b;
        
        a = *(buf ++ );
        b = *(buf ++ );
        printk("%02x%02x ", a, b );
    }
    printk("\n");
#endif
}
#endif


/*
 * Open and Initialize the board
 *
 * Set up everything, reset the card, etc ..
 *
 */
static int ftmac100_open(struct net_device *dev)
{
    //printk("+%s:ftmac100_open\n", dev->name);

    netif_start_queue(dev);

#ifdef MODULE
    MOD_INC_USE_COUNT;
#endif
    
    /* reset the hardware */
    ftmac100_reset( dev );
    ftmac100_enable( dev );
    
    /* Configure the PHY */
    ftmac100_phy_configure(dev);
    
    netif_start_queue(dev);
    return 0;
}


/*--------------------------------------------------------
 . Called by the kernel to send a packet out into the void
 . of the net.  This routine is largely based on
 . skeleton.c, from Becker.
 .--------------------------------------------------------
*/
static void ftmac100_timeout (struct net_device *dev)
{
    /* If we get here, some higher level has decided we are broken.
    There should really be a "kick me" function call instead. */
    printk(KERN_WARNING "%s: transmit timed out?\n",dev->name);

	//printk("+ftmac100_timeout\n");

    ftmac100_reset( dev );
    ftmac100_enable( dev );
    ftmac100_phy_configure(dev);
    netif_wake_queue(dev);
    dev->trans_start = jiffies;
}


/*--------------------------------------------------------------------
 .
 . This is the main routine of the driver, to handle the net_device when
 . it needs some attention.
 .
 . So:
 .   first, save state of the chipset
 .   branch off into routines to handle each case, and acknowledge
 .	    each to the interrupt register
 .   and finally restore state.
 .
 ---------------------------------------------------------------------*/
static void ftmac100_interrupt(int irq, void * dev_id,  struct pt_regs * regs)
{
    struct net_device   *dev=dev_id;
    unsigned int        ioaddr=dev->base_addr;
    unsigned char       status;			// interrupt status
    unsigned char       mask;			// interrupt mask
    int	                timeout;
    struct ftmac100_local *lp = (struct ftmac100_local *)dev->priv;

	//printk("+ftmac100_interrupt\n");

    if (dev == NULL) 
    {
        printk(KERN_WARNING "%s: irq %d for unknown device.\n",	dev->name, irq);
        return;
    }

    /* read the interrupt status register */
    mask = inl( ioaddr + IMR_REG );
    
    /* set a timeout value, so I don't stay here forever */
    
    //	PRINTK(KERN_WARNING "%s: MASK IS %x \n", dev->name, mask);
    for (timeout=1; timeout>0; --timeout)
    {
        /* read the status flag, and mask it */
        status = inl( ioaddr + ISR_REG ) & mask;
        if (!status )
            break;
       
        if ( status & RPKT_FINISH_bit ) 
            ftmac100_rcv(dev);
        else if (status & NORXBUF_bit)			// CPU 來不及處理, packet 送的太快
        {
			///printk("NORXBUF \n");
            //printk("<0x%x:NORXBUF>",status);
            outl( mask & ~NORXBUF_bit, ioaddr + IMR_REG);			/// 暫時關掉 NORXBUF interrupt
            trans_busy = 1;
            lp->rcv_tq.sync = 0;
            lp->rcv_tq.routine=ftmac100_rcv;
            lp->rcv_tq.data = dev;
            queue_task(&lp->rcv_tq, &tq_timer);
        }		
        else if (status & AHB_ERR_bit)
            printk("<0x%x:AHB_ERR>",status);
    }
    return;
}



/*-------------------------------------------------------------
 .
 . ftmac100_rcv -  receive a packet from the card
 .
 . There is ( at least ) a packet waiting to be read from
 . chip-memory.
 .
 . o Read the status
 . o If an error, record it
 . o otherwise, read in the packet
 --------------------------------------------------------------
*/

static void ftmac100_rcv(void *devp)
{
    struct net_device       *dev=(struct net_device *)devp;
    struct ftmac100_local   *lp = (struct ftmac100_local *)dev->priv;
    unsigned int            ioaddr=dev->base_addr;
    int                     packet_length;
    int                     rcv_cnt;
    volatile RX_DESC        *cur_desc;
    int                     cpy_length;
    int	                    cur_idx;
    int	                    seg_length;
    int	                    have_package;
    int	                    have_frs;
    int	                    start_idx;

	//printk("+ftmac100_rcv\n");

	start_idx = lp->rx_idx;
	
	for (rcv_cnt=0; rcv_cnt<8 ; ++rcv_cnt)
	{
        packet_length = 0;
        cur_idx = lp->rx_idx;
        
        have_package = 0;
        have_frs = 0;
        for (; (cur_desc = &lp->rx_descs[lp->rx_idx])->RXDMA_OWN==0; )
        {
            have_package = 1;
            lp->rx_idx = (lp->rx_idx+1)%RXDES_NUM;
            if (cur_desc->FRS)
            { 	
                have_frs = 1;
                if (cur_desc->RX_ERR || cur_desc->CRC_ERR || cur_desc->FTL || cur_desc->RUNT || cur_desc->RX_ODD_NB)
                {
                    lp->stats.rx_errors++;			// error frame....
                    break;			
                }
                if (cur_desc->MULTICAST)
                    lp->stats.multicast++;
                packet_length = cur_desc->ReceiveFrameLength;				// normal frame
			}
			if ( cur_desc->LRS )		// packet's last frame
				break;
		}
        if (have_package==0)
            goto done;
        if (have_frs == 0)
            lp->stats.rx_over_errors++;

        if (packet_length>0)
        {
            struct sk_buff  * skb;
            unsigned char		* data;

            // Allocate enough memory for entire receive frame, to be safe
            skb = dev_alloc_skb( packet_length+2 );
            
            if ( skb == NULL )
            {
                printk(KERN_NOTICE "%s: Low memory, packet dropped.\n",	dev->name);
                lp->stats.rx_dropped++;
                goto done;
            }
    
            skb_reserve( skb, 2 );   /* 16 bit alignment */
            skb->dev = dev;
            
            data = skb_put( skb, packet_length);
            cpy_length = 0;
            for (; cur_idx!=lp->rx_idx; cur_idx=(cur_idx+1)%RXDES_NUM)
            {
                seg_length = min(packet_length - cpy_length, RX_BUF_SIZE);
                memcpy(data+cpy_length, (char *)lp->rx_descs[cur_idx].VIR_RXBUF_BADR, seg_length);
                cpy_length += seg_length;
            }

            skb->protocol = eth_type_trans(skb, dev );
            netif_rx(skb);
            lp->stats.rx_packets++;
        }
    }

done:
    if (start_idx != lp->rx_idx)
    {
        for (cur_idx = (start_idx+1)%RXDES_NUM; cur_idx != lp->rx_idx; cur_idx = (cur_idx+1)%RXDES_NUM)
        {
            lp->rx_descs[cur_idx].RXDMA_OWN = 1;				/// 此 frame 已處理完畢, 還給 hardware
        }
        lp->rx_descs[start_idx].RXDMA_OWN = 1;
    }
    if (trans_busy == 1)
    {
        outl( lp->maccr_val, ioaddr + MACCR_REG );
        outl( inl(ioaddr + IMR_REG) | NORXBUF_bit, ioaddr + IMR_REG);
    }
    return;
}



/*----------------------------------------------------
 . ftmac100_close
 .
 . this makes the board clean up everything that it can
 . and not talk to the outside world.   Caused by
 . an 'ifconfig ethX down'
 .
 -----------------------------------------------------*/
static int ftmac100_close(struct net_device *dev)
{
	//printk("+ftmac100_close\n");

    netif_stop_queue(dev);
    
    ftmac100_shutdown( dev->base_addr );
#ifdef MODULE
    MOD_DEC_USE_COUNT;
#endif    
    return 0;
}

/*------------------------------------------------------------
 . Get the current statistics.
 . This may be called with the card open or closed.
 .-------------------------------------------------------------*/
static struct net_device_stats* ftmac100_query_statistics(struct net_device *dev) 
{
	struct ftmac100_local *lp = (struct ftmac100_local *)dev->priv;	

	//printk("+ftmac100_query_statistics\n");

	return &lp->stats;
}


#ifdef HAVE_MULTICAST

/*
 . Function: ftmac100_setmulticast( unsigned int ioaddr, int count, dev_mc_list * adds )
 . Purpose:
 .    This sets the internal hardware table to filter out unwanted multicast
 .    packets before they take up memory.
*/

static void ftmac100_setmulticast( unsigned int ioaddr, int count, struct dev_mc_list * addrs ) 
{
    struct dev_mc_list	* cur_addr;
    int crc_val;
    
	//printk("+ftmac100_setmulticast\n");

    for (cur_addr = addrs ; cur_addr!=NULL ; cur_addr = cur_addr->next )
    {
        if ( !( *cur_addr->dmi_addr & 1 ) )
            continue;
        crc_val = crc32( cur_addr->dmi_addr, 6 );
        crc_val = (crc_val>>26)&0x3f;			// 取 MSB 6 bit
        if (crc_val >= 32)
            outl(inl(ioaddr+MAHT1_REG) | (1UL<<(crc_val-32)), ioaddr+MAHT1_REG);
        else
            outl(inl(ioaddr+MAHT0_REG) | (1UL<<crc_val), ioaddr+MAHT0_REG);
    }
}


/*-----------------------------------------------------------
 . ftmac100_set_multicast_list
 .
 . This routine will, depending on the values passed to it,
 . either make it accept multicast packets, go into
 . promiscuous mode ( for TCPDUMP and cousins ) or accept
 . a select set of multicast packets
*/
static void ftmac100_set_multicast_list(struct net_device *dev)
{
    unsigned int ioaddr = dev->base_addr;
    struct ftmac100_local *lp = (struct ftmac100_local *)dev->priv;
    
	//printk("+ftmac100_set_multicast_list\n");

    if (dev->flags & IFF_PROMISC)
        lp->maccr_val |= RCV_ALL_bit;
    else
        lp->maccr_val &= ~RCV_ALL_bit;

    if ( !(dev->flags & IFF_ALLMULTI) )
        lp->maccr_val |= RX_MULTIPKT_bit;
    else
        lp->maccr_val &= ~RX_MULTIPKT_bit;
        
    if (dev->mc_count)
    {
        lp->maccr_val |= HT_MULTI_EN_bit;
        ftmac100_setmulticast( ioaddr, dev->mc_count, dev->mc_list );
    }
    else
        lp->maccr_val &= ~HT_MULTI_EN_bit;

    outl( lp->maccr_val, ioaddr + MACCR_REG );
}
#endif


#if 0
//#ifdef MODULE
static struct net_device devFMAC;
static int io = 0;
static int irq = 0;
static int nowait = 0;

MODULE_PARM(io, "i");
MODULE_PARM(irq, "i");
MODULE_PARM(nowait, "i");

/*------------------------------------------------------------
 . Module initialization function
 .-------------------------------------------------------------*/
int init_module(void)
{
    int result;
   
	//printk("+init_module\n");

    /* copy the parameters from insmod into the device structure */
    devFMAC.base_addr=io;
    devFMAC.irq=irq;
    devFMAC.dma=nowait; // Use DMA field for nowait		
    devFMAC.init=FTCmac_probe;  //john ftmac100_init;/* Kernel 2.4 Changes - Pramod */
    if ((result = register_netdev(&devFMAC)) != 0)
        return result;
	
	return 0;
}

/*------------------------------------------------------------
 . Cleanup when module is removed with rmmod
 .-------------------------------------------------------------*/
void cleanup_module(void)
{
	//printk("+cleanup_module\n");

    /* No need to check MOD_IN_USE, as sys_delete_module() checks. */
    unregister_netdev(&devFMAC);
    
    free_irq(devFMAC.irq, &devFMAC);
    release_region(devFMAC.base_addr, SMC_IO_EXTENT);
    
    if (devFMAC.priv)
        kfree(devFMAC.priv); /* Kernel 2.4 Changes - Pramod */
}

#endif /* MODULE */


#if 0
/*------------------------------------------------------------
 . Reads a register from the MII Management serial interface
 .-------------------------------------------------------------*/
static word ftmac100_read_phy_register(unsigned int ioaddr, unsigned char phyaddr, unsigned char phyreg)
{
	return 0;
}


/*------------------------------------------------------------
 . Writes a register to the MII Management serial interface
 .-------------------------------------------------------------*/
static void ftmac100_write_phy_register(unsigned int ioaddr,
	unsigned char phyaddr, unsigned char phyreg, word phydata)
{

}

#endif
/*------------------------------------------------------------
 . Configures the specified PHY using Autonegotiation.
 .-------------------------------------------------------------*/
static void ftmac100_phy_configure(struct net_device* dev)
{
    return;
}
