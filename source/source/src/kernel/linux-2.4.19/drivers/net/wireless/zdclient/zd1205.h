#ifndef _ZD1205_H_
#define _ZD1205_H_

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/reboot.h>
#include <asm/io.h>
#include <asm/unaligned.h>
#include <asm/processor.h>
#include <linux/ethtool.h>
#include <linux/inetdevice.h>
#include <linux/bitops.h>
#include <linux/if.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/ip.h>
#include <linux/wireless.h>
#include <linux/if_arp.h>

#include "zdequates.h"
#include "zdapi.h"
#include "mlme_zydas/mlme.h"

enum zd1205_device_type {
	ZD_1202 = 1,
	ZD_1205,
};



#define	ASOC_RSP		0x10 
#define REASOC_RSP 		0x30
#define PROBE_RSP 		0x50 
#define DISASOC 		0xA0 
#define AUTH 			0xB0 
#define DEAUTH 			0xC0 
#define DATA 			0x08 
#define PS_POLL			0xA4
#define MANAGEMENT		0x00
#define PROBE_REQ		0x40
#define BEACON			0x80
#define ACK				0xD4
#define CONTROL			0x04
#define NULL_FUNCTION   0x48
#define CTS             0xc4
#define LB_DATA			0x88

#define VLAN_SIZE   	4
#define CHKSUM_SIZE 	2

#define false		(0)
#define true		(1)
/**************************************************************************
**		Register Offset Definitions
***************************************************************************
*/

#define		ZD1205_CR0			0x0000
#define		ZD1205_CR1			0x0004
#define		ZD1205_CR2			0x0008
#define		ZD1205_CR3			0x000C
#define		ZD1205_CR5			0x0010
#define		ZD1205_CR6			0x0014
#define		ZD1205_CR7			0x0018
#define		ZD1205_CR8			0x001C
#define		ZD1205_CR4			0x0020
#define		ZD1205_CR9			0x0024
#define		ZD1205_CR10			0x0028
#define		ZD1205_CR11			0x002C
#define		ZD1205_CR12			0x0030
#define		ZD1205_CR13			0x0034
#define		ZD1205_CR14			0x0038
#define		ZD1205_CR15			0x003C
#define		ZD1205_CR16			0x0040
#define		ZD1205_CR17			0x0044
#define		ZD1205_CR18			0x0048
#define		ZD1205_CR19			0x004C
#define		ZD1205_CR20			0x0050
#define		ZD1205_CR21			0x0054
#define		ZD1205_CR22			0x0058
#define		ZD1205_CR23			0x005C
#define		ZD1205_CR24			0x0060
#define		ZD1205_CR25			0x0064
#define		ZD1205_CR26			0x0068
#define		ZD1205_CR27			0x006C
#define		ZD1205_CR28			0x0070
#define		ZD1205_CR29			0x0074
#define		ZD1205_CR30			0x0078
#define		ZD1205_CR31			0x007C
#define		ZD1205_CR32			0x0080
#define		ZD1205_CR33			0x0084
#define		ZD1205_CR34			0x0088
#define		ZD1205_CR35			0x008C
#define		ZD1205_CR36			0x0090
#define		ZD1205_CR37			0x0094
#define		ZD1205_CR38			0x0098
#define		ZD1205_CR39			0x009C
#define		ZD1205_CR40			0x00A0
#define		ZD1205_CR41			0x00A4
#define		ZD1205_CR42			0x00A8
#define		ZD1205_CR43			0x00AC
#define		ZD1205_CR44			0x00B0
#define		ZD1205_CR45			0x00B4
#define		ZD1205_CR46			0x00B8
#define		ZD1205_CR47			0x00BC
#define		ZD1205_CR48			0x00C0
#define		ZD1205_CR49			0x00C4
#define		ZD1205_CR50			0x00C8
#define		ZD1205_CR51			0x00CC
#define		ZD1205_CR52			0x00D0
#define		ZD1205_CR53			0x00D4
#define		ZD1205_CR54			0x00D8
#define		ZD1205_CR55			0x00DC
#define		ZD1205_CR56			0x00E0
#define		ZD1205_CR57			0x00E4
#define		ZD1205_CR58			0x00E8
#define		ZD1205_CR59			0x00EC
#define		ZD1205_CR60			0x00F0
#define		ZD1205_CR61			0x00F4
#define		ZD1205_CR62			0x00F8
#define		ZD1205_CR63			0x00FC
#define		ZD1205_CR64			0x0100
#define		ZD1205_CR65			0x0104
#define		ZD1205_CR66			0x0108
#define		ZD1205_CR67			0x010C
#define		ZD1205_CR68			0x0110
#define		ZD1205_CR69			0x0114
#define		ZD1205_CR70			0x0118
#define		ZD1205_CR71			0x011C
#define		ZD1205_CR72			0x0120
#define		ZD1205_CR73			0x0124
#define		ZD1205_CR74			0x0128
#define		ZD1205_CR75			0x012C
#define		ZD1205_CR76			0x0130
#define		ZD1205_CR77			0x0134
#define		ZD1205_CR78			0x0138
#define		ZD1205_CR79			0x013C
#define		ZD1205_CR80			0x0140
#define		ZD1205_CR81			0x0144
#define		ZD1205_CR82			0x0148
#define		ZD1205_CR83			0x014C
#define		ZD1205_CR84			0x0150
#define		ZD1205_CR85			0x0154
#define		ZD1205_CR86			0x0158
#define		ZD1205_CR87			0x015C
#define		ZD1205_CR88			0x0160
#define		ZD1205_CR89			0x0164
#define		ZD1205_CR90			0x0168
#define		ZD1205_CR91			0x016C
#define		ZD1205_CR92			0x0170
#define		ZD1205_CR93			0x0174
#define		ZD1205_CR94			0x0178
#define		ZD1205_CR95			0x017C
#define		ZD1205_CR96			0x0180
#define		ZD1205_CR97			0x0184
#define		ZD1205_CR98			0x0188
#define		ZD1205_CR99			0x018C
#define		ZD1205_CR100		0x0190
#define		ZD1205_CR101		0x0194
#define		ZD1205_CR102		0x0198
#define		ZD1205_CR103		0x019C
#define		ZD1205_CR104		0x01A0
#define		ZD1205_CR105		0x01A4
#define		ZD1205_CR106		0x01A8
#define		ZD1205_CR107		0x01AC
#define		ZD1205_CR108		0x01B0
#define		ZD1205_CR109		0x01B4
#define		ZD1205_CR110		0x01B8
#define		ZD1205_CR111		0x01BC
#define		ZD1205_CR112		0x01C0
#define		ZD1205_CR113		0x01C4
#define		ZD1205_CR114		0x01C8
#define		ZD1205_CR115		0x01CC
#define		ZD1205_CR116		0x01D0
#define		ZD1205_CR117		0x01D4
#define		ZD1205_CR118		0x01D8
#define		ZD1205_CR119		0x01EC
#define		ZD1205_CR120		0x01E0
#define		ZD1205_CR121		0x01E4
#define		ZD1205_CR122		0x01E8
#define		ZD1205_CR123		0x01EC
#define		ZD1205_CR124		0x01F0
#define		ZD1205_CR125		0x01F4
#define		ZD1205_CR126		0x01F8
#define		ZD1205_CR127		0x01FC
#define		ZD1205_CR128		0x0200
#define		ZD1205_CR129		0x0204
#define		ZD1205_CR130		0x0208
#define		ZD1205_CR131		0x020C
#define		ZD1205_CR132		0x0210
#define		ZD1205_CR133		0x0214
#define		ZD1205_CR134		0x0218
#define		ZD1205_CR135		0x021C
#define		ZD1205_CR136		0x0220
#define		ZD1205_CR137		0x0224
#define		ZD1205_CR138		0x0228
#define		ZD1205_CR139		0x022C
#define		ZD1205_CR140		0x0230
#define		ZD1205_CR141		0x0234
#define		ZD1205_CR142		0x0238
#define		ZD1205_CR143		0x023C
#define		ZD1205_CR144		0x0240
#define		ZD1205_CR145		0x0244
#define		ZD1205_CR146		0x0248
#define		ZD1205_CR147		0x024C
#define		ZD1205_CR148		0x0250
#define		ZD1205_CR149		0x0254
#define		ZD1205_CR150		0x0258
#define		ZD1205_CR151		0x025C
#define		ZD1205_CR152		0x0260
#define		ZD1205_CR153		0x0264
#define		ZD1205_CR154		0x0268
#define		ZD1205_CR155		0x026C
#define		ZD1205_CR156		0x0270
#define		ZD1205_CR157		0x0274
#define		ZD1205_CR158		0x0278
#define		ZD1205_CR159		0x027C
#define		ZD1205_CR160		0x0280
#define		ZD1205_CR161		0x0284
#define		ZD1205_CR162		0x0288
#define		ZD1205_CR163		0x028C
#define		ZD1205_CR164		0x0290
#define		ZD1205_CR165		0x0294
#define		ZD1205_CR166		0x0298
#define		ZD1205_CR167		0x029C
#define		ZD1205_CR168		0x02A0
#define		ZD1205_CR169		0x02A4
#define		ZD1205_CR170		0x02A8
#define		ZD1205_CR171		0x02AC
#define		ZD1205_CR172		0x02B0
#define		ZD1205_CR173		0x02B4
#define		ZD1205_CR174		0x02B8
#define		ZD1205_CR175		0x02BC
#define		ZD1205_CR176		0x02C0
#define		ZD1205_CR177		0x02C4
#define		ZD1205_CR178		0x02C8
#define		ZD1205_CR179		0x02CC
#define		ZD1205_CR180		0x02D0
#define		ZD1205_CR181		0x02D4
#define		ZD1205_CR182		0x02D8
#define		ZD1205_CR183		0x02DC
#define		ZD1205_CR184		0x02E0
#define		ZD1205_CR185		0x02E4
#define		ZD1205_CR186		0x02E8
#define		ZD1205_CR187		0x02EC
#define		ZD1205_CR188		0x02F0
#define		ZD1205_CR189		0x02F4
#define		ZD1205_CR190		0x02F8
#define		ZD1205_CR191		0x02FC
#define		ZD1205_CR192		0x0300
#define		ZD1205_CR193		0x0304
#define		ZD1205_CR194		0x0308
#define		ZD1205_CR195		0x030C
#define		ZD1205_CR196		0x0310
#define		ZD1205_CR197		0x0314
#define		ZD1205_CR198		0x0318
#define		ZD1205_CR199		0x031C
#define		ZD1205_CR200		0x0320
#define		ZD1205_CR201		0x0324
#define		ZD1205_CR202		0x0328
#define		ZD1205_CR203		0x032C
#define		ZD1205_CR204		0x0330
#define		ZD1205_PHY_END		0x03fc
#define		RF_IF_CLK			0x0400
#define		RF_IF_DATA			0x0404
#define		PE1_PE2				0x0408
#define		PE2_DLY				0x040C
#define		LE1					0x0410
#define		LE2					0x0414
#define		GPI_EN				0x0418
#define		RADIO_PD			0x042C
#define		RF2948_PD			0x042C
#define		LED1				0x0430
#define		LED2				0x0434
#define		EnablePSManualAGC	0x043C	// 1: enable
#define		CONFIGPhilips		0x0440
#define		SA2400_SER_AP		0x0444
#define		I2C_WRITE			0x0444	// Same as SA2400_SER_AP (for compatible with ZD1201)
#define		SA2400_SER_RP		0x0448
#define		AfterPNP			0x0454
#define		RADIO_PE			0x0458
#define		RstBusMaster		0x045C

#define		RFCFG				0x0464

#define		HSTSCHG				0x046C

#define		PHY_ON				0x0474
#define		RX_DELAY			0x0478
#define		RX_PE_DELAY			0x047C


#define		GPIO_1				0x0490
#define		GPIO_2				0x0494


#define		EncryBufMux			0x04A8


#define		PS_Ctrl				0x0500

#define		ADDA_MBIAS_WarmTime	0x0508

#define		InterruptCtrl		0x0510
#define		TSF_LowPart			0x0514
#define		TSF_HighPart		0x0518
#define		ATIMWndPeriod		0x051C
#define		BCNInterval			0x0520
#define		Pre_TBTT			0x0524	//In unit of TU(1024us)

#define		PCI_TxAddr_p1		0x0600
#define		PCI_TxAddr_p2		0x0604
#define		PCI_RxAddr_p1		0x0608
#define		PCI_RxAddr_p2		0x060C
#define		MACAddr_P1			0x0610
#define		MACAddr_P2			0x0614
#define		BSSID_P1			0x0618
#define		BSSID_P2			0x061C
#define		BCNPLCPCfg			0x0620
#define		GroupHash_P1		0x0624
#define		GroupHash_P2		0x0628
#define		WEPTxIV				0x062C

#define		PHYDelay			0x066C
#define		BCNFIFO				0x0670
#define		SnifferOn			0x0674
#define		EncryType			0x0678
#define		RetryMAX			0x067C
#define		CtlReg1				0x0680	//Bit0:		IBSS mode
										//Bit1:		PwrMgt mode
										//Bit2-4 :	Highest basic rate
										//Bit5:		Lock bit
										//Bit6:		PLCP weight select
										//Bit7:		PLCP switch
#define		DeviceState			0x0684
#define		UnderrunCnt			0x0688
#define		Rx_Filter			0x068c
#define		Ack_Timeout_Ext		0x0690
#define		BCN_FIFO_Semaphore	0x0694
#define		IFS_Value			0x0698
#define		RX_TIME_OUT			0x069C
#define		TotalRxFrm			0x06A0
#define		CRC32Cnt			0x06A4
#define		CRC16Cnt			0x06A8
#define		DecrypErr_UNI		0x06AC
#define		RxFIFOOverrun		0x06B0

#define		DecrypErr_Mul		0x06BC

#define		NAV_CNT				0x06C4
#define		NAV_CCA				0x06C8
#define		RetryCnt			0x06CC

#define		ReadTcbAddress		0x06E8
#define		ReadRfdAddress		0x06EC
#define		CWmin_CWmax			0x06F0
#define		TotalTxFrm			0x06F4
#define     RX_OFFSET_BYTE      0x06F8

#define		CAM_MODE			0x0700
#define		CAM_ROLL_TB_LOW		0x0704
#define		CAM_ROLL_TB_HIGH	0x0708
#define		CAM_ADDRESS			0x070C
#define		CAM_DATA			0x0710
#define     DECRY_ERR_FLG_LOW   0x0714
#define     DECRY_ERR_FLG_HIGH  0x0718
#define		WEPKey0				0x0720
#define		WEPKey1				0x0724
#define		WEPKey2				0x0728
#define		WEPKey3				0x072C
#define     CAM_DEBUG           0x0728
#define     CAM_STATUS          0x072c
#define		WEPKey4				0x0730
#define		WEPKey5				0x0734
#define		WEPKey6				0x0738
#define		WEPKey7				0x073C
#define		WEPKey8				0x0740
#define		WEPKey9				0x0744
#define		WEPKey10			0x0748
#define		WEPKey11			0x074C
#define		WEPKey12			0x0750
// yarco for TKIP support
#define		WEPKey13			0x0754
#define		WEPKey14			0x0758
#define		WEPKey15			0x075c
#define		TKIP_MODE			0x0760

#define		Dbg_FIFO_Rd			0x0800
#define		Dbg_Select			0x0804
#define		FIFO_Length			0x0808


//#define		RF_Mode					0x080C
#define		RSSI_MGC			0x0810

#define		PON					0x0818
#define		Rx_ON				0x081C
#define		Tx_ON				0x0820
#define		CHIP_EN				0x0824
#define		LO_SW				0x0828
#define		TxRx_SW				0x082C
#define		S_MD				0x0830

// EEPROM Memmory Map Region
#define		E2P_SUBID			0x0900
#define		E2P_POD				0x0904
#define		E2P_MACADDR_P1		0x0908
#define		E2P_MACADDR_P2		0x090C

#define		E2P_PWR_CAL_VALUE	0x0910

#define		E2P_PWR_INT_VALUE	0x0920

#define		E2P_ALLOWED_CHANNEL	0x0930
#define		E2P_PHY_REG			0x0934

#define		E2P_REGION_CODE		0x0960
#define		E2P_FEATURE_BITMAP	0x0964


//-------------------------------------------------------------------------
// Command Block (CB) Field Definitions
//-------------------------------------------------------------------------
//- RFD Command Bits
#define RFD_EL_BIT              BIT_0	     // RFD EL Bit

//- CB Command Word
#define CB_S_BIT                0x1          // CB Suspend Bit

//- CB Status Word
#define CB_STATUS_COMPLETE      0x1234       // CB Complete Bit

#define RFD_STATUS_COMPLETE     0x1234       // RFD Complete Bit


/**************************************************************************
**		MAC Register Bit Definitions
***************************************************************************
*/
// Interrupt STATUS
#define		TX_COMPLETE				BIT_0
#define		RX_COMPLETE				BIT_1
#define		RETRY_FAIL				BIT_2
#define     WAKE_UP                 BIT_3
#define		DTIM_NOTIFY				BIT_5
#define		CFG_NEXT_BCN			BIT_6
#define     BUS_ABORT               BIT_7

#define		TX_COMPLETE_EN			BIT_16
#define		RX_COMPLETE_EN			BIT_17
#define		RETRY_FAIL_EN			BIT_18
#define     WAKE_UP_EN              BIT_19
#define		DTIM_NOTIFY_EN			BIT_21
#define		CFG_NEXT_BCN_EN			BIT_22
#define     BUS_ABORT_EN            BIT_23

#define		FILTER_BEACON			0xFEFF //mask bit 8
#define		UN_FILTER_PS_POLL		0x0400 

#define		DBG_MSG_SHOW			0x1
#define		DBG_MSG_HIDE			0x0


#define 	RFD_POINTER(skb, macp)      ((rfd_t *) (((unsigned char *)((skb)->data))-((macp)->rfd_size)))
#define 	SKB_RFD_STATUS(skb, macp)   ((RFD_POINTER((skb),(macp)))->CbStatus)

#define KEVENT_MGT_MON_TIMEOUT  1
#define KEVENT_HOUSE_KEEPING    2
#define KEVENT_WATCH_DOG        3
#define KEVENT_PROCESS_SIGNAL   4
#define KEVENT_DIS_CONNECT      5
#define KEVENT_CARD_RESET       6

/**************************************************************************
**		Descriptor Data Structure
***************************************************************************/
struct driver_stats {
	struct net_device_stats net_stats;

	unsigned long tx_late_col;
	unsigned long tx_ok_defrd;
	unsigned long tx_one_retry;
	unsigned long tx_mt_one_retry;
	unsigned long rcv_cdt_frames;
	unsigned long xmt_fc_pkts;
	unsigned long rcv_fc_pkts;
	unsigned long rcv_fc_unsupported;
	unsigned long xmt_tco_pkts;
	unsigned long rcv_tco_pkts;
	unsigned long rx_intr_pkts;
};

//-------------------------------------------------------------------------
// Transmit Command Block (TxCB)
//-------------------------------------------------------------------------
typedef struct hw_tcb_s {
	u32	CbStatus;					// Bolck status
	u32	CbCommand;					// Block command
	u32	NextCbPhyAddrLowPart; 		// Next TCB address(low part)
	u32	NextCbPhyAddrHighPart;		// Next TCB address(high part)
	u32 TxCbFirstTbdAddrLowPart; 	// First TBD address(low part)
	u32 TxCbFirstTbdAddrHighPart;	// First TBD address(high part)
	u32 TxCbTbdNumber;				// Number of TBDs for this TCB
} hw_tcb_t;

//-------------------------------------------------------------------------
// Transmit Buffer Descriptor (TBD)
//-------------------------------------------------------------------------
typedef struct tbd_s {
    u32	TbdBufferAddrLowPart;		// Physical Transmit Buffer Address
    u32	TbdBufferAddrHighPart;		// Physical Transmit Buffer Address
	u32	TbdCount;		// Data length
} tbd_t;

//-------------------------------------------------------------------------
// Receive Frame Descriptor (RFD)
//-------------------------------------------------------------------------
typedef struct rfd_s {
	u32	CbStatus;				// Bolck status
	u32	ActualCount;			// Rx buffer length
	u32	CbCommand;				// Block command
	u32	MaxSize;				// 
	u32	NextCbPhyAddrLowPart;		// Next RFD address(low part)
	u32	NextCbPhyAddrHighPart;		// Next RFD address(high part)
	u8	RxBuffer[MAX_WLAN_SIZE];	// Rx buffer
	u32	Pad[2];			// Pad to 16 bytes alignment - easy view for debug
} rfd_t __attribute__ ((__packed__));


typedef struct ctrl_set_s {
	u8	ctrl_setting[40];
} ctrl_set_t;

typedef struct header_s {
	u8	mac_header[32];
} header_t;




//-------------------------------------------------------------------------
// ZD1205SwTcb -- Software Transmit Control Block.  This structure contains
// all of the variables that are related to a specific Transmit Control
// block (TCB)
//-------------------------------------------------------------------------

typedef struct sw_tcb_s {

    // Link to the next SwTcb in the list
    struct sw_tcb_s *next;
    struct sk_buff 	*skb;

    // physical and virtual pointers to the hardware TCB
    hw_tcb_t		*tcb;
    dma_addr_t		tcb_phys;

    // Physical and virtual pointers to the TBD array for this TCB
	tbd_t			*first_tbd;
	dma_addr_t		first_tbd_phys;

	ctrl_set_t   	*hw_ctrl;
	dma_addr_t		hw_ctrl_phys;

	header_t		*hw_header;
	dma_addr_t		hw_header_phys;

	u32				tcb_count;
	u8				frame_type;
	u8				last_frag;
	u8				msg_id;
	u8				bIntraBss;
	u16				aid;
	u8				rate;
} sw_tcb_t;


typedef struct sw_tcbq_s
{
	sw_tcb_t 	*first;		/* first sw_tcb_t in Q */
	sw_tcb_t 	*last;		/* last sw_tcb_t in Q */
	u16			count;		/* number of sw_tcb_t in Q */
} sw_tcbq_t;


//- Wireless 24-byte Header
typedef struct wla_header_s {
	u8		frame_ctrl[2];
	u8		Duration[2];
	u8		DA[6];
	u8		bssid[6];
	u8		SA[6];
	u8		seq_ctrl[2];
} wla_header_t;

//from station
typedef struct plcp_wla_header_s {
	u8		plcp_hdr[PLCP_HEADER];    //Oh! shit!!!
	u8		frame_ctrl[2];
	u8		Duration[2];
	u8		Address1[6];
	u8		Address2[6];
	u8		Address3[6];
	u8		seq_ctrl[2];
} plcp_wla_header_t;


/*Rx skb holding structure*/
struct rx_list_elem {
	struct list_head 	list_elem;
	dma_addr_t 			dma_addr;
	struct sk_buff 		*skb;
}__attribute__ ((__packed__));


typedef struct bss_info_s {
	u8	bssid[6];
	u8	beaconInterval[2];
	u8	cap[2];
	u8	ssid[36];
	u8	supRates[NUM_SUPPORTED_RATE];
	u8	channel;
    u8  signalStrength;
    u8 	signalQuality;
} bss_info_t;


typedef struct tuple_s
{
	u8		ta[6]; //TA (Address 2)
	u16		sn;
	u8		fn;
	u8		full;
} tuple_t;


typedef struct tuple_Cache_s
{
	tuple_t cache[TUPLE_CACHE_SIZE];
	u8 freeTpi;				
} tuple_Cache_t;


typedef struct defrag_Mpdu_s
{
	u8	ta[6];	
	u8	inUse;	
	u8	fn;
	u32	eol;		
	u16	sn;		
	void 	*buf;
	void	*dataStart;
} defrag_Mpdu_t;


typedef struct defrag_Array_s
{
	defrag_Mpdu_t mpdu[MAX_DEFRAG_NUM];
} defrag_Array_t;	


struct zd1205_private
{
	//linux used
	struct net_device 	*device;
	struct pci_dev 		*pdev;
	struct driver_stats drv_stats;
	struct timer_list 	watchdog_timer;	/* watchdog timer id */
	struct timer_list 	tchal_timer;
	struct timer_list	tm_scan_id;
	struct timer_list	tm_hking_id;
    struct timer_list	tm_mgt_id;	
	char ifname[IFNAMSIZ];
	spinlock_t 			bd_lock;		/* board lock */
	spinlock_t 			bd_non_tx_lock;	/* Non transmit command lock  */
	spinlock_t          q_lock;
	spinlock_t          conf_lock;
	int	                using_dac;
	struct tasklet_struct zd1205_tasklet;
	struct tasklet_struct zd1205_ps_tasklet;
	struct tasklet_struct zd1205_tx_tasklet;
	struct proc_dir_entry *proc_parent;
    u32                 kevent_flags;
	struct tq_struct    kevent;
		
	struct list_head 	active_rx_list;	/* list of rx buffers */
	struct list_head 	rx_struct_pool;	/* pool of rx buffer struct headers */
	u16 				rfd_size;	
	u8                  rev_id;			/* adapter PCI revision ID */
	u8	                sniffer_on;	
	int					skb_req;		/* number of skbs neede by the adapter */
	
	void 				*dma_able;		/* dma allocated structs */
	dma_addr_t 			dma_able_phys;
	rwlock_t 			isolate_lock;
	int 				driver_isolated;
	char				*cable_status;
	
	void				*regp;
	u8					mac_addr[8];			
	u8					mcast_addr[8];		
	u32					intr_mask;			

	sw_tcbq_t 			*freeTxQ;
	sw_tcbq_t 			*activeTxQ;

	u32  		        tx_cached_size;
	u8			        *tx_cached;

	u16                 dtimCount;
	u8			        num_tcb;
	u16			        num_tbd;
	u8			        num_rfd;
	u8			        num_tbd_per_tcb;
	u32                 rx_offset;
	
	card_Setting_t	    card_setting;
	u32			        wep_iv;
	u8			        bssid[8];
	u32			        dbg_flag;
	
	//debug counter
	u32			        bcnCnt;
	u32			        dtimCnt;
	u32			        txCmpCnt;
	u32			        rxCnt;
	u32			        retryFailCnt;
	u32			        txCnt;
	u32					txIdleCnt;
	u32					rxIdleCnt;
	u32 				rxDup;
	
	//HMAC counter
	u32					hwTotalRxFrm;
	u32					hwCRC32Cnt;
	u32					hwCRC16Cnt;
	u32					hwDecrypErr_UNI;
	u32					hwDecrypErr_Mul;
	u32					hwRxFIFOOverrun;
	u32					hwTotalTxFrm;
	u32					hwUnderrunCnt;
	u32					hwRetryCnt;
	u32					PhyRegErr;
	u32	                HMAC_TxRx_DeadLock;
	u32                 HMAC_NoPhyClk;
	u32					HMAC_PciHang;
	u32					TxStartTime;
	u32					HMAC_TxTimeout;
	u32					NoRFD;
	u32					swDecrypErr;
	u32					swDecrypOk;
	u8	                bTraceSetPoint;
	u8	                TxGainSetting;
	u8	                bFixedRate;
	u8	                bContinueTx;
	u8                  bDeviceInSleep;
	u8					bDataTrafficLight;
	u16	                SetPoint;
	u8			        dtim_notify_en;
	u8			        config_next_bcn_en;
	u8			        txOn;
	u8					ResetFlag;
	
	//for loopback
	u32			        lbFrameLen;
	u32			        lbTxCnt;
	u32			        lbRxCnt;
	u32			        lbTxCmpCnt;
	u32			        lbErrCnt;
	
	u16			        lb_mode;
	u16			        iv_16;
	u32			        iv_32;
	
	u32			        rx_invalid;
	u32			        invalid_frame_good_crc;
	u8					bGkInstalled;
	u8                  rate;
	u8			        rx_signal_q;
	u8			        rx_signal_sth;
	u8			        rx_signal_q2;
	u8			        rx_descry_type;
	u16  				EepSetPoint[14];

	u32                 RegionCode;
	u32					RF_Mode;
	u8					MaxTxPwrSet;
	u8					MinTxPwrSet;
	
	u8					bss_index;
	bss_info_t 			BSSInfo[BSS_INFO_NUM];
	tuple_Cache_t 		cache;
	defrag_Array_t 		defragArray;
	
	
	//added for STA
	u8					PwrState;
	u32					TotalTxDataFrmBytes;
	u32					TotalRxDataFrmBytes;	
		
	u32  		        tx_uncached_size;
	dma_addr_t	        tx_uncached_phys;
	void		        *tx_uncached;
	u8			        lbFrame[1600];
//ivan
    mlme_data_t         mlme;
    	
}; // __attribute__ ((__packed__));


typedef struct ctrl_setting_parm_s {
	u8			rate;
	u8			preamble;
	u8			encry_type;
	u8			vapid;
	u32			curr_frag_len;
	u32			next_frag_len;
} ctrl_setting_parm_t;


#define ZD_IOCTL_REG_READ		0x01
#define ZD_IOCTL_REG_WRITE		0x02
#define ZD_IOCTL_MEM_DUMP		0x03
#define ZD_IOCTL_REGS_DUMP		0x04
#define ZD_IOCTL_DBG_MESSAGE		0x05
#define ZD_IOCTL_DBG_COUNTER		0x06
#define ZD_IOCTL_LB_MODE		0x07
#define ZD_IOCTL_TX_ONE    		0x08
#define ZD_IOCTL_TX_ON			0x09
#define ZD_IOCTL_WEP			0x0A
#define ZD_IOCTL_TX			0x0B
#define ZD_IOCTL_TX1			0x0C
#define ZD_IOCTL_RATE			0x0D
#define ZD_IOCTL_PORT			0x0E
#define ZD_IOCTL_SNIFFER		0x0F
#define ZD_IOCTL_WEP_ON_OFF		0x10
#define ZD_IOCTL_CAM       		0x11
#define ZD_IOCTL_VAP			0x12
#define ZD_IOCTL_APC			0x13
#define ZD_IOCTL_CAM_DUMP		0x14
#define ZD_IOCTL_CAM_CLEAR		0x15
#define ZD_IOCTL_WDS			0x16
#define ZD_IOCTL_FIFO			0x17
#define ZD_IOCTL_FRAG			0x18
#define ZD_IOCTL_RTS			0x19
#define ZD_IOCTL_PREAMBLE		0x1A
#define ZD_IOCTL_DUMP_PHY		0x1B
#define ZD_IOCTL_DBG_PORT		0x1C
#define ZD_IOCTL_CARD_SETTING		0x1D
#define ZD_IOCTL_RESET			0x1F
#define ZD_IOCTL_SW_CIPHER		0x20
#define ZD_IOCTL_HASH_DUMP		0x21
#define ZD_IOCTL_RFD_DUMP               0x22
#define ZD_IOCTL_CHANNEL                0x23
#define ZD_IOCTL_MEM_READ               0x24
#define ZD_IOCTL_MEM_WRITE              0x25
#define ZD_IOCTL_KEY_ID                 0x26
#define ZD_IOCTL_KEY                    0x27
#define ZD_IOCTL_SSID                   0x28
#define ZD_IOCTL_AUTH                   0x29
#define ZD_IOCTL_ENC_MODE               0x2A

#define	ZDAPIOCTL	SIOCDEVPRIVATE

#define   Lo8(v16)   ((u8)( (v16)       & 0x00FF))
#define   Hi8(v16)   ((u8)(((v16) >> 8) & 0x00FF))

struct zdap_ioctl {
	u16 cmd;                /* Command to run */
	u32 addr;                /* Length of the data buffer */
	u32 value;              /* Pointer to the data buffer */
	u8	data[0x100];
};
/**************************************************************************
**		Function Declarations
***************************************************************************
*/
void zd1205_sleep_reset(struct zd1205_private *macp);
void zd1205_sw_reset(struct zd1205_private *macp);
void zd1205_watchdog(struct net_device *);
void update_beacon_interval(struct zd1205_private *macp, int val);
void zd1205_device_reset(struct zd1205_private *macp);

void zd1205_dump_data(char *info, u8 *data, u32 data_len);
void zd1205_init_card_setting(struct zd1205_private *macp);
sw_tcb_t * zd1205_first_txq(struct zd1205_private *macp, sw_tcbq_t *Q);
inline void zd1205_disable_int(void);
inline void zd1205_enable_int(void);
inline void zd1205_lock(struct zd1205_private *macp);
inline void zd1205_unlock(struct zd1205_private *macp);


struct sk_buff* zd1205_prepare_tx_data(struct zd1205_private *macp, u16 bodyLen);
void zd1205_tx_test(struct zd1205_private *macp, u16 size);
void zd1205_qlast_txq(struct zd1205_private *macp, sw_tcbq_t *Q, sw_tcb_t *signal);


#endif	/* _ZD1205_H_ */
