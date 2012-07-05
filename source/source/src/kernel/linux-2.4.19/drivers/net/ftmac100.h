/*******************************************************
	2002-11-29: lmc83 modified from smc91111.h (2002-11-29)
	2004/8/26 19:29:    ivan wang modified
*******************************************************/
#ifndef                     _FTMAC100_H_
#define                     _FTMAC100_H_

#define ISR_REG             0x00            // interrups status register
#define IMR_REG             0x04            // interrupt maks register
#define MAC_MADR_REG        0x08            // MAC address (Most significant)
#define MAC_LADR_REG        0x0c            // MAC address (Least significant)
                                            
#define MAHT0_REG	        0x10            // Multicast Address Hash Table 0 register
#define MAHT1_REG	        0x14            // Multicast Address Hash Table 1 register
#define TXPD_REG            0x18            // Transmit Poll Demand register
#define RXPD_REG            0x1c            // Receive Poll Demand register
#define TXR_BADR_REG        0x20            // Transmit Ring Base Address register
#define RXR_BADR_REG        0x24            // Receive Ring Base Address register
#define ITC_REG	            0x28            // interrupt timer control register
#define APTC_REG            0x2c            // Automatic Polling Timer control register
#define DBLAC_REG           0x30            // DMA Burst Length and Arbitration control register



#define MACCR_REG           0x88             // MAC control register
#define MACSR_REG           0x8c             // MAC status register
#define PHYCR_REG           0x90             // PHY control register
#define PHYWDATA_REG        0x94             // PHY Write Data register
#define FCR_REG             0x98             // Flow Control register
#define BPR_REG             0x9c             // back pressure register
#define WOLCR_REG           0xa0             // Wake-On-Lan control register
#define WOLSR_REG           0xa4             // Wake-On-Lan status register
#define WFCRC_REG           0xa8             // Wake-up Frame CRC register
#define WFBM1_REG           0xb0             // wake-up frame byte mask 1st double word register
#define WFBM2_REG           0xb4             // wake-up frame byte mask 2nd double word register
#define WFBM3_REG           0xb8             // wake-up frame byte mask 3rd double word register
#define WFBM4_REG           0xbc             // wake-up frame byte mask 4th double word register
#define TM_REG		        0xcc             // test mode register

#define PHYSTS_CHG_bit		(1UL<<9)
#define AHB_ERR_bit			(1UL<<8)
#define RPKT_LOST_bit		(1UL<<7)
#define RPKT_SAV_bit		(1UL<<6)
#define XPKT_LOST_bit		(1UL<<5)
#define XPKT_OK_bit			(1UL<<4)
#define NOTXBUF_bit			(1UL<<3)
#define XPKT_FINISH_bit		(1UL<<2)
#define NORXBUF_bit			(1UL<<1)
#define RPKT_FINISH_bit		(1UL<<0)


typedef struct
{
    unsigned int RXPOLL_CNT:4;
    unsigned int RXPOLL_TIME_SEL:1;
    unsigned int Reserved1:3;
    unsigned int TXPOLL_CNT:4;
    unsigned int TXPOLL_TIME_SEL:1;
    unsigned int Reserved2:19;
}FTMAC100_APTCR_Status;


#define RX_BROADPKT_bit	    (1UL<<17)           // Receiving broadcast packet
#define RX_MULTIPKT_bit	    (1UL<<16)           // receiving multicast packet
#define FULLDUP_bit	        (1UL<<15)           // full duplex
#define CRC_APD_bit	        (1UL<<14)           // append crc to transmit packet
#define MDC_SEL_bit	        (1UL<<13)           // set MDC as TX_CK/10
#define RCV_ALL_bit	        (1UL<<12)           // not check incoming packet's destination address
#define RX_FTL_bit	        (1UL<<11)           // Store incoming packet even its length is great than 1518 byte
#define RX_RUNT_bit	        (1UL<<10)           // Store incoming packet even its length is les than 64 byte
#define HT_MULTI_EN_bit		(1UL<<9)            
#define RCV_EN_bit	        (1UL<<8)           // receiver enable
#define XMT_EN_bit	        (1UL<<5)           // transmitter enable
#define CRC_DIS_bit         (1UL<<4)           
#define LOOP_EN_bit         (1UL<<3)           // Internal loop-back
#define SW_RST_bit	        (1UL<<2)           // software reset/
#define RDMA_EN_bit         (1UL<<1)           // enable DMA receiving channel
#define XDMA_EN_bit         (1UL<<0)           // enable DMA transmitting channel


// --------------------------------------------------------------------
//		Receive Ring descriptor structure
// --------------------------------------------------------------------
typedef struct
{
    // RXDES0
    unsigned int ReceiveFrameLength:11;//0~10
    unsigned int Reserved1:5;          //11~15
    unsigned int MULTICAST:1;          //16
    unsigned int BROARDCAST:1;         //17
    unsigned int RX_ERR:1;             //18
    unsigned int CRC_ERR:1;            //19
    unsigned int FTL:1;
    unsigned int RUNT:1;
    unsigned int RX_ODD_NB:1;
    unsigned int Reserved2:5;
    unsigned int LRS:1;
    unsigned int FRS:1;
    unsigned int Reserved3:1;
    unsigned int RXDMA_OWN:1;			// 1 ==> owned by FTMAC100, 0 ==> owned by software
    
    // RXDES1
    unsigned int RXBUF_Size:11;			
    unsigned int Reserved:20;
    unsigned int EDOTR:1;
    
    // RXDES2
    unsigned int RXBUF_BADR;
    			
    unsigned int VIR_RXBUF_BADR;			// not defined, 我們拿來放 receive buffer 的 virtual address
    
}RX_DESC;


typedef struct
{
    // TXDES0
    unsigned int TXPKT_LATECOL:1;
    unsigned int TXPKT_EXSCOL:1;
    unsigned int Reserved1:29;
    unsigned int TXDMA_OWN:1;
    
    // TXDES1
    unsigned int TXBUF_Size:11;
    unsigned int Reserved2:16;
    unsigned int LTS:1;
    unsigned int FTS:1;
    unsigned int TX2FIC:1;
    unsigned int TXIC:1;
    unsigned int EDOTR:1;
    
    // RXDES2
    unsigned int TXBUF_BADR;
    unsigned int VIR_TXBUF_BADR;
}TX_DESC;


// waiting to do:
#define	TXPOLL_CNT          8
#define RXPOLL_CNT          0

#define OWNBY_SOFTWARE	    0
#define OWNBY_FTMAC100	    1

// --------------------------------------------------------------------
//		driver related definition
// --------------------------------------------------------------------
#define RXDES_NUM           128
#define RX_BUF_SIZE         512
#define TXDES_NUM           8
#define TX_BUF_SIZE         2048


struct ftmac100_local 
{
    // these are things that the kernel wants me to keep, so users
    // can find out semi-useless statistics of how well the card is
    // performing
    struct net_device_stats stats;
    
    // Set to true during the auto-negotiation sequence
    int	                autoneg_active;
    
    // Address of our PHY port
    unsigned int		phyaddr;
    
    // Type of PHY
    unsigned int		phytype;
    
    // Last contents of PHY Register 18
    unsigned int		lastPhy18;
    
    spinlock_t          lock;
    volatile RX_DESC    *rx_descs;					// receive ring base address
    unsigned int        rx_descs_dma;				// receive ring physical base address
    char                *rx_buf;					// receive buffer cpu address
    int                 rx_buf_dma;					// receive buffer physical address
    int                 rx_idx;						// receive descriptor	
    volatile TX_DESC    *tx_descs;
    unsigned int        tx_descs_dma;
    char                *tx_buf;
    int	                tx_buf_dma;
    int                 tx_idx;
    int                 maccr_val;	

    struct tq_struct    rcv_tq;   //john, queue rcv task
};

int __init ftmac100_init(struct net_device *dev,u32 irq, u32 base_addr);
void ftmac100_destructor(struct net_device *dev);
static int ftmac100_open(struct net_device *dev);
static void ftmac100_timeout (struct net_device *dev);
static int ftmac100_close(struct net_device *dev);
static struct net_device_stats * ftmac100_query_statistics( struct net_device *dev);
static void ftmac100_set_multicast_list(struct net_device *dev);
static void ftmac100_phy_configure(struct net_device* dev);
static void ftmac100_interrupt(int irq, void *, struct pt_regs *regs);
static void ftmac100_rcv(void *dev);
static int ftmac100_probe(struct net_device *dev, unsigned int ioaddr);
static void ftmac100_reset( struct net_device* dev );
static void ftmac100_enable( struct net_device *dev );
//static word ftmac100_read_phy_register(unsigned int ioaddr, unsigned char phyaddr, unsigned char phyreg);
//static void ftmac100_write_phy_register(unsigned int ioaddr, unsigned char phyaddr, unsigned char phyreg, word phydata);

#endif


