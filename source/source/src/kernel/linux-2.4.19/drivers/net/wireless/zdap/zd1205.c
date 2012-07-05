#include <linux/config.h>
#include <net/checksum.h>
#include <linux/tcp.h>
#include <linux/udp.h>

#include "zd1205.h"
#include "zdinlinef.h"
#include "zddebug.h"
#include "zdhw.h"

#if WIRELESS_EXT > 12
#include <net/iw_handler.h>
#endif


/******************************************************************************
*						   C O N S T A N T S
*******************************************************************************
*/
#define NUM_TCB				64//32
#define NUM_TBD_PER_TCB		(2+MAX_SKB_FRAGS)	//3
#define NUM_TBD				(NUM_TCB * NUM_TBD_PER_TCB)
#define NUM_RFD				32         
#define TX_RING_BYTES		(NUM_TCB * (sizeof(hw_tcb_t) + sizeof(ctrl_set_t) + sizeof(header_t)))+ (NUM_TBD * sizeof(tbd_t))
#define ZD1205_REGS_SIZE	4096

#define ZD1205_INT_MASK		TX_COMPLETE_EN | RX_COMPLETE_EN | RETRY_FAIL_EN | CFG_NEXT_BCN_EN | DTIM_NOTIFY_EN | WAKE_UP_EN | BUS_ABORT_EN

#define ZD_RX_OFFSET    0x00

u8	ZD_SNAP_HEADER[6] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00};
u8	ZD_SNAP_BRIDGE_TUNNEL[6] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0xF8};

#define YARCO_DEBUG
int zd_dbg = 0;

#ifdef YARCO_DEBUG

#define DEBUG(n, args...) do { if (zd_dbg>(n)) printk(KERN_DEBUG args); } while(0)
#else
#define DEBUG(n, args...) do { } while (0)
#endif

#define NUM_WEPKEYS     4

#define bGroup(pWlanHdr)			(pWlanHdr->Address1[0] & BIT_0)
#define getSeq(pWlanHdr)			(((u16)pWlanHdr->seq_ctrl[1] << 4) + (u16)((pWlanHdr->seq_ctrl[0] & 0xF0) >> 4))
#define getFrag(pWlanHdr)			(pWlanHdr->seq_ctrl[0] & 0x0F)
#define	getTA(pWlanHdr)				(&pWlanHdr->Address2[0])
#define isWDS(pWlanHdr)				(((pWlanHdr->frame_ctrl[1] & TO_DS_FROM_DS) == TO_DS_FROM_DS) ? 1 : 0) 
#define bRetryBit(pWlanHdr)			(pWlanHdr->frame_ctrl[1] & RETRY_BIT)
#define bWepBit(pWlanHdr)			(pWlanHdr->frame_ctrl[1] & WEP_BIT) 
#define bMoreFrag(pWlanHdr)			(pWlanHdr->frame_ctrl[1] & MORE_FRAG)
#define BaseFrameType(pWlanHdr)		(pWlanHdr->frame_ctrl[0] & 0x0C)
#define SubFrameType(pWlanHdr)		(pWlanHdr->frame_ctrl[0])
#define bDataMgtFrame(pWlanHdr)		(((pWlanHdr->frame_ctrl[0] & 0x04) == 0))
#define nowT()						(readl(regp+TSF_LowPart))
/******************************************************************************
*			   F U N C T I O N	 D E C L A R A T I O N S
*******************************************************************************
*/

#ifdef CONFIG_PROC_FS
    extern int zd1205_create_proc_subdir(struct zd1205_private *);
    extern void zd1205_remove_proc_subdir(struct zd1205_private *);

#else
    #define zd1205_create_proc_subdir(X) 0
    #define zd1205_remove_proc_subdir(X) do {} while(0)
#endif


//extern void zd1205_set_zd_cbs(zd_80211Obj_t *pObj);

static u8 zd1205_pci_setup(struct pci_dev *, struct zd1205_private *);
static unsigned char zd1205_alloc_space(struct zd1205_private *);
static unsigned char zd1205_init(struct zd1205_private *);
static void zd1205_setup_tcb_pool(struct zd1205_private *macp);
static void zd1205_config(struct zd1205_private *macp);
static void zd1205_rd_eaddr(struct zd1205_private *);

static int zd1205_open(struct net_device *);
static int zd1205_close(struct net_device *);
static int zd1205_change_mtu(struct net_device *, int);
static int zd1205_set_mac(struct net_device *, void *);
static void zd1205_set_multi(struct net_device *);
struct net_device_stats *zd1205_get_stats(struct net_device *);
static int zd1205_alloc_tcb_pool(struct zd1205_private *);
static void zd1205_free_tcb_pool(struct zd1205_private *);
static int zd1205_alloc_rfd_pool(struct zd1205_private *);
static void zd1205_free_rfd_pool(struct zd1205_private *);
static void zd1205_clear_pools(struct zd1205_private *macp);
sw_tcb_t * zd1205_first_txq(struct zd1205_private *macp, sw_tcbq_t *Q);
void zd1205_qlast_txq(struct zd1205_private *macp, sw_tcbq_t *Q, sw_tcb_t *signal);
static void zd1205_init_txq(struct zd1205_private *macp, sw_tcbq_t *Q);
static void zd1205_start_ru(struct zd1205_private *);
static void zd1205_intr(int, void *, struct pt_regs *);
static u32	zd1205_rx_isr(struct zd1205_private *);
static void zd1205_tx_isr(struct zd1205_private *);
static void zd1205_retry_failed(struct zd1205_private *);
static void zd1205_dtim_notify(struct zd1205_private *);
static void zd1205_transmit_cleanup	(struct zd1205_private *, sw_tcb_t *sw_tcb);
static int zd1205_validate_frame(struct zd1205_private *macp, rfd_t *rfd);
static int zd1205_xmit_frame(struct sk_buff *, struct net_device *);
static void zd1205_dealloc_space(struct zd1205_private *macp);
inline void zd1205_disable_int(void);
inline void zd1205_enable_int(void);
u8 zd1205_RateAdaption(u16 aid, u8 CurrentRate, u8 gear);
void zd1205_config_wep_keys(struct zd1205_private *macp);
void zd1205_ConvertDbToSetPoint(struct zd1205_private *macp);
void HKeepingCB(struct net_device *dev);
void zd1205_process_wakeup(struct zd1205_private *macp);

//wireless extension helper functions
inline void zd1205_lock(struct zd1205_private *macp);
inline void zd1205_unlock(struct zd1205_private *macp);
static int zd1205_ioctl_setiwencode(struct net_device *dev, struct iw_point *erq);
static int zd1205_ioctl_getiwencode(struct net_device *dev, struct iw_point *erq);

static int zd1205_ioctl_setessid(struct net_device *dev, struct iw_point *erq);
static int zd1205_ioctl_getessid(struct net_device *dev, struct iw_point *erq);
static int zd1205_ioctl_setfreq(struct net_device *dev, struct iw_freq *frq);
static int zd1205_ioctl_getsens(struct net_device *dev, struct iw_param *srq);
//static int zd1205_ioctl_setsens(struct net_device *dev, struct iw_param *srq);
static int zd1205_ioctl_setrts(struct net_device *dev, struct iw_param *rrq);
static int zd1205_ioctl_setfrag(struct net_device *dev, struct iw_param *frq);
static int zd1205_ioctl_getfrag(struct net_device *dev, struct iw_param *frq);
static int zd1205_ioctl_setrate(struct net_device *dev, struct iw_param *frq);
static int zd1205_ioctl_getrate(struct net_device *dev, struct iw_param *frq);
static int zd1205_ioctl_setpower(struct net_device *dev, struct iw_param *prq);
static int zd1205_ioctl_getpower(struct net_device *dev, struct iw_param *prq);

/* Wireless Extension Handler functions */
static int zd1205wext_giwname(struct net_device *dev, struct iw_request_info *info, char *name, char *extra);
static int zd1205wext_giwfreq(struct net_device *dev, struct iw_request_info *info, struct iw_freq *freq, char *extra);
static int zd1205wext_siwfreq(struct net_device *dev, struct iw_request_info *info, struct iw_freq *freq, char *extra);
static int zd1205wext_giwmode(struct net_device *dev, struct iw_request_info *info, __u32 *mode, char *extra);
static int zd1205wext_giwrate(struct net_device *dev, struct iw_request_info *info, struct iw_param *rrq, char *extra);
static int zd1205wext_giwrts(struct net_device *dev, struct iw_request_info *info, struct iw_param *rts, char *extra);
static int zd1205wext_siwrts(struct net_device *dev, struct iw_request_info *info, struct iw_param *rts, char *extra);
static int zd1205wext_giwfrag(struct net_device *dev, struct iw_request_info *info, struct iw_param *frag, char *extra);
static int zd1205wext_siwfrag(struct net_device *dev, struct iw_request_info *info, struct iw_param *frag, char *extra);
static int zd1205wext_giwtxpow(struct net_device *dev, struct iw_request_info *info, struct iw_param *rrq, char *extra);
static int zd1205wext_siwtxpow(struct net_device *dev, struct iw_request_info *info, struct iw_param *rrq, char *extra);
static int zd1205wext_giwap(struct net_device *dev, struct iw_request_info *info, struct sockaddr *ap_addr, char *extra);
static int zd1205wext_giwrange(struct net_device *dev, struct iw_request_info *info, struct iw_point *data, char *extra);

#if 0
static int zd1205wext_siwrate(struct net_device *dev, struct iw_request_info *info, struct iw_param *rrq, char *extra);
static int zd1205wext_siwencode(struct net_device *dev, struct iw_request_info *info, struct iw_point *erq, char *key);
static int zd1205wext_giwencode(struct net_device *dev, struct iw_request_info *info, struct iw_point *erq, char *key);
#endif

/* functions to support 802.11 protocol stack */
void zdcb_rx_ind(U8 *pData, U32 length, void *buf);
void zdcb_release_buffer(void *buf);
void zdcb_tx_completed(void);
void zdcb_start_timer(U32 timeout, U32 event);
void zdcb_stop_timer(void);
void zd1205_set_zd_cbs(zd_80211Obj_t *pObj);
inline void zdcb_set_reg(void *reg, U32 offset, U32 value);
void send_tchal_msg(unsigned long ptr);
inline U32 zdcb_dis_intr(void);
inline void zdcb_set_intr_mask(U32 flags);
inline BOOLEAN zdcb_check_tcb_avail(U8	num_of_frag);
BOOLEAN zdcb_setup_next_send(fragInfo_t *frag_info);
U16 zdcb_status_notify(U16 status, U8 *StaAddr);
U32 zdcb_vir_to_phy_addr(U32 virtAddr);
inline U32 zdcb_get_reg(void *reg, U32 offset);
void zdcb_delay_us(U16 ustime);
/******************************************************************************
*						 P U B L I C   D A T A
*******************************************************************************
*/
/* Global Data structures and variables */
char zd1205_copyright[] __devinitdata = "Copyright (c) 2002 Zydas Corporation";
char zd1205_driver_version[]="0.0.1";
const char *zd1205_full_driver_name = "Zydas ZD1205 Network Driver";
char zd1205_short_driver_name[] = "zd1205";     
char *dev_info = "zd1205"; 

static int zd1205nics = 0;
sw_tcbq_t free_txq_buf, active_txq_buf;
struct net_device *g_dev;
zd_80211Obj_t dot11Obj = {0};
#define RX_COPY_BREAK   0//1518 //we do bridge, don't care IP header alignment

/* Definition of Wireless Extension */

#if WIRELESS_EXT > 12
static iw_handler zd1205wext_handler[] = {
	(iw_handler) NULL,                              /* SIOCSIWCOMMIT */
	(iw_handler) zd1205wext_giwname,                /* SIOCGIWNAME */
	(iw_handler) NULL,                              /* SIOCSIWNWID */
	(iw_handler) NULL,                              /* SIOCGIWNWID */
	(iw_handler) zd1205wext_siwfreq,                /* SIOCSIWFREQ */
	(iw_handler) zd1205wext_giwfreq,                /* SIOCGIWFREQ */
	(iw_handler) NULL,                              /* SIOCSIWMODE */
	(iw_handler) zd1205wext_giwmode,                /* SIOCGIWMODE */
	(iw_handler) NULL,                              /* SIOCSIWSENS */
	(iw_handler) NULL,                              /* SIOCGIWSENS */
	(iw_handler) NULL, /* not used */               /* SIOCSIWRANGE */
	(iw_handler) zd1205wext_giwrange,               /* SIOCGIWRANGE */
	(iw_handler) NULL, /* not used */               /* SIOCSIWPRIV */
	(iw_handler) NULL, /* kernel code */            /* SIOCGIWPRIV */
	(iw_handler) NULL, /* not used */               /* SIOCSIWSTATS */
	(iw_handler) NULL, /* kernel code */            /* SIOCGIWSTATS */
	(iw_handler) NULL,                              /* SIOCSIWSPY */
	(iw_handler) NULL,                              /* SIOCGIWSPY */
	(iw_handler) NULL,                              /* -- hole -- */
	(iw_handler) NULL,                              /* -- hole -- */
	(iw_handler) NULL,                              /* SIOCSIWAP */
	(iw_handler) zd1205wext_giwap,                  /* SIOCGIWAP */
	(iw_handler) NULL,				/* -- hole -- */
	(iw_handler) NULL,                              /* SIOCGIWAPLIST */
#if WIRELESS_EXT > 13
	(iw_handler) NULL,   /* something */            /* SIOCSIWSCAN */
	(iw_handler) NULL,   /* something */            /* SIOCGIWSCAN */
#else /* WIRELESS_EXT > 13 */
	(iw_handler) NULL,      /* null */              /* SIOCSIWSCAN */
	(iw_handler) NULL,      /* null */              /* SIOCGIWSCAN */
#endif /* WIRELESS_EXT > 13 */
	(iw_handler) NULL,                              /* SIOCSIWESSID */
	(iw_handler) NULL,                              /* SIOCGIWESSID */
	(iw_handler) NULL,                              /* SIOCSIWNICKN */
	(iw_handler) NULL,                              /* SIOCGIWNICKN */
	(iw_handler) NULL,                              /* -- hole -- */
	(iw_handler) NULL,                              /* -- hole -- */
	(iw_handler) NULL,                              /* SIOCSIWRATE */
	(iw_handler) zd1205wext_giwrate,                /* SIOCGIWRATE */
	(iw_handler) zd1205wext_siwrts,                 /* SIOCSIWRTS */
	(iw_handler) zd1205wext_giwrts,                 /* SIOCGIWRTS */
	(iw_handler) zd1205wext_siwfrag,                /* SIOCSIWFRAG */
	(iw_handler) zd1205wext_giwfrag,                /* SIOCGIWFRAG */
	(iw_handler) zd1205wext_siwtxpow,               /* SIOCSIWTXPOW */
	(iw_handler) zd1205wext_giwtxpow,               /* SIOCGIWTXPOW */
	(iw_handler) NULL,                              /* SIOCSIWRETRY */
	(iw_handler) NULL,                              /* SIOCGIWRETRY */
	(iw_handler) NULL,                              /* SIOCSIWENCODE */
	(iw_handler) NULL,                              /* SIOCGIWENCODE */
	(iw_handler) NULL,                              /* SIOCSIWPOWER */
	(iw_handler) NULL,                              /* SIOCGIWPOWER */
};

struct iw_handler_def p80211wext_handler_def = {
	num_standard: sizeof(zd1205wext_handler) / sizeof(iw_handler),
	num_private: 0,
	num_private_args: 0,
	standard: zd1205wext_handler,
	private: NULL,
	private_args: NULL
};
                                                
#endif
																																									        						
inline void zd1205_disable_int(void)
{
    struct zd1205_private *macp = g_dev->priv;

	void *regp = macp->regp;

	/* Disable interrupts on our PCI board by setting the mask bit */
	writel(0, regp+InterruptCtrl);
}


inline void zd1205_enable_int(void)
{
    struct zd1205_private *macp = g_dev->priv;
 	void *regp = macp->regp;

	writel(macp->intr_mask, regp+InterruptCtrl);
}             


static void zd1205_action(unsigned long parm)
{
	zd_SigProcess();  //process management frame queue in mgtQ
}


static void zd1205_ps_action(unsigned long parm)
{
    zd_CleanupAwakeQ();
}    


static void zd1205_tx_action(unsigned long parm)
{
   zd_CleanupTxQ();
}    


u8 zd1205_RateAdaption(u16 aid, u8 CurrentRate, u8 gear)
{
	u8	NewRate;

	RATEDEBUG("***** zd1205_RateAdaption");
	RATEDEBUG_V("aid", aid);
	RATEDEBUG_V("CurrentRate", CurrentRate);
	
	if (gear == FALL_RATE){
		if (CurrentRate >= RATE_2M){
			NewRate = CurrentRate - 1;
			zd_EventNotify(EVENT_UPDATE_TX_RATE, (U32)NewRate, (U32)aid, 0);
		}
		else{
			NewRate = CurrentRate;
		}


		return (NewRate);
	}

    return 0;
}


void zd1205_ClearTupleCache(struct zd1205_private *macp)
{
	int i;
	tuple_Cache_t *pCache = &macp->cache;
	
	pCache->freeTpi = 0;
	for (i=0; i<TUPLE_CACHE_SIZE; i++){
		pCache->cache[i].full = 0;
	}
}	


u8 zd1205_SearchTupleCache(struct zd1205_private *macp, u8 *pAddr, u16 seq, u8 frag)
{
	int k;
	tuple_Cache_t *pCache = &macp->cache;
	
	for (k=0; k<TUPLE_CACHE_SIZE; k++){
		if ((memcmp((char *)&pCache->cache[k].ta[0], (char *)pAddr, 6) == 0) 
			&& (pCache->cache[k].sn == seq) && (pCache->cache[k].fn == frag)
			&& (pCache->cache[k].full))
			return 1;
	}
	
	return 0;			
}


void zd1205_UpdateTupleCache(struct zd1205_private *macp, u8 *pAddr, u16 seq ,u8 frag)
{
	int k;
	tuple_Cache_t *pCache = &macp->cache;
	
	for (k=0; k<TUPLE_CACHE_SIZE; k++){
		if (pCache->cache[k].full){
			if ((memcmp((char *)&pCache->cache[k].ta[0], (char *)pAddr, 6) == 0) 
				&& (pCache->cache[k].sn == seq) ){
				pCache->cache[k].fn = frag;
				return;
			}	
		}
	}

	pCache->freeTpi &= (TUPLE_CACHE_SIZE-1);
	memcpy(&pCache->cache[pCache->freeTpi].ta[0], (char *)pAddr, 6);
	pCache->cache[pCache->freeTpi].sn = seq;
	pCache->cache[pCache->freeTpi].fn = frag;
	pCache->cache[pCache->freeTpi].full = 1; 
	pCache->freeTpi++;
}


void zd1205_ArReset(struct zd1205_private *macp)
{
	u8 i;
	defrag_Array_t *pArray = &macp->defragArray;
	
	for (i=0; i<MAX_DEFRAG_NUM; i++)
		pArray->mpdu[i].inUse = 0;
}


void zd1205_ArAge(struct zd1205_private *macp, u32 age)
{
	u8 i;
	defrag_Array_t *pArray = &macp->defragArray;
	
	for (i=0; i<MAX_DEFRAG_NUM; i++){
		if (pArray->mpdu[i].inUse){
			if ((age - pArray->mpdu[i].eol) > MAX_RX_TIMEOUT){
				DFDEBUG("***** zd1205_ArAged");
				dot11Obj.ReleaseBuffer(pArray->mpdu[i].buf);
				pArray->mpdu[i].inUse = 0;
			}
		}
	}	
}


int	zd1205_ArFree(struct zd1205_private *macp)
{
	u8 i;
	defrag_Array_t *pArray = &macp->defragArray;

	for (i=0; i<MAX_DEFRAG_NUM; i++){
		if (!pArray->mpdu[i].inUse)
			return i;
	}
	
	return -1;
}


int	zd1205_ArSearch(struct zd1205_private *macp, u8 *pAddr, u16 seq, u8 frag)
{
	u8 i;
	defrag_Array_t *pArray = &macp->defragArray;

	
	for (i=0; i<MAX_DEFRAG_NUM; i++){
		if (pArray->mpdu[i].inUse){
			if ((memcmp((char *)&pArray->mpdu[i].ta[0], pAddr, 6) == 0) 
				&& (pArray->mpdu[i].sn == seq) && (pArray->mpdu[i].fn == (frag-1)) )
				return i; 
		}
	}

	return -1;	
}



void zd1205_ArUpdate(struct zd1205_private *macp, u8 *pAddr, u16 seq, u8 frag, int i)
{
	void *regp = macp->regp;
	defrag_Array_t *pArray = &macp->defragArray;
	
	pArray->mpdu[i].inUse = 1;
	memcpy(&pArray->mpdu[i].ta[0], (char*)pAddr, 6);
	pArray->mpdu[i].sn = seq;
	pArray->mpdu[i].fn = frag;
	pArray->mpdu[i].eol = nowT();
}


void zd1205_IncreaseTxPower(struct zd1205_private *macp)
{
	switch(macp->RF_Mode){
		case GCT_RF:
			if (macp->TxGainSetting < macp->MaxTxPwrSet)
				macp->TxGainSetting++;
			break;
		
		case AL2210_RF:	
		case AL2210MPVB_RF:
			if (macp->TxGainSetting > macp->MinTxPwrSet)
				macp->TxGainSetting--;
			break;
			
		default:
			break;
	}
}
void zd1205_DecreaseTxPower(struct zd1205_private *macp)
{
	switch(macp->RF_Mode){
		case GCT_RF:
			if (macp->TxGainSetting > macp->MinTxPwrSet)
				macp->TxGainSetting--;
			break;
			
		case AL2210_RF:
		case AL2210MPVB_RF:
			if ( macp->TxGainSetting < macp->MaxTxPwrSet)
				macp->TxGainSetting++;
			break;
			
		default:
			break;
	}
}


void
HKeepingCB(struct net_device *dev)
{
	struct zd1205_private *macp = dev->priv;
	void *regp = macp->regp;
	u32	tmpvalue;
	u16	setpoint = macp->SetPoint;
	u16 channel;
	static	u64 loop = 0;
	static	u16	TrackingLoop = 0;
	static	u32	accumulate = 0;
	u32  average = 0;
    static u8 SuspiciousNoPhyClk = 0;
   	static u8 SuspiciousPciHang = 0;

	loop ++;

    //FPRINT("HKeepingCB is working");
	if (loop == 20){
		HW_RadioOnOff(&dot11Obj, 0);
		HW_RadioOnOff(&dot11Obj, 1);
	}
	
	if (macp->ResetFlag){
		zd1205_sleep_reset(macp);
		return;
	}	
	
	if (macp->bDataTrafficLight){
		writel(0x1, regp+LED2);
		macp->bDataTrafficLight = 0;
	}
	else{
		writel(0x0, regp+LED2);
	}
		
	// HMAC Tx/Rx Dead-Lock protection
	tmpvalue = readl(regp+DeviceState);
	if ((tmpvalue & 0xf0f00) == 0xb0a00){
		macp->HMAC_TxRx_DeadLock++;
		FPRINT("***** HMAC_TxRx_DeadLock");
		zd1205_sleep_reset(macp);
		return;
	}
    else {
    	if (((tmpvalue & 0xf0f00) == 0x10a00) || ((tmpvalue & 0xf0f00) == 0x60a00)){
        	SuspiciousNoPhyClk++;
        	if (SuspiciousNoPhyClk > 3){
           		FPRINT("********No phy Clock");
            	SuspiciousNoPhyClk = 0;
            	macp->HMAC_NoPhyClk++;
            	zd1205_sleep_reset(macp);
            	return;
        	}
        	else
           	SuspiciousNoPhyClk = 0;
       }
       
#if 1		
		if (((tmpvalue & 0xf00f) == 0x1003) || ((tmpvalue & 0xf00f) == 0x1007)){
			SuspiciousPciHang ++;
			if (SuspiciousPciHang > 10){
				FPRINT("***** PCI Hangup");	
				SuspiciousPciHang = 0;
				macp->HMAC_PciHang ++;
				zd1205_sleep_reset(macp);
				return;
			}
		}	
		else{
			SuspiciousPciHang = 0;
		}
#endif		    	
    }

	//++ Recovery mechanism for ZD1202 ASIC Phy-Bus arbitration fault.
	//   We combined tx-power-tracking/Sw Antenna diversity code here to
	//   reduce the frequence of

	//   calling ReleaseCtrOfPhyReg. It's harmful to throughput.
	//if ((loop % 10) == 0){ //every 100 ms
	if ((loop % 1) == 0){ //every 100 ms
		//Collect HW Tally
#if 0

		macp->hwTotalRxFrm += readl(regp+TotalRxFrm);
		macp->hwCRC32Cnt += readl(regp+CRC32Cnt);
		macp->hwCRC16Cnt += readl(regp+CRC16Cnt);
		macp->hwDecrypErr_UNI += readl(regp+DecrypErr_UNI);
		macp->hwDecrypErr_Mul += readl(regp+DecrypErr_Mul);
		macp->hwRxFIFOOverrun += readl(regp+RxFIFOOverrun);
		macp->hwTotalTxFrm += readl(regp+TotalTxFrm);

		macp->hwUnderrunCnt += readl(regp+UnderrunCnt);
		macp->hwRetryCnt += readl(regp+>RetryCnt);
#endif
		LockPhyReg(&dot11Obj);
		if (!macp->bContinueTx){
			tmpvalue = readl(regp+ZD1205_CR32);
			if (tmpvalue != dot11Obj.CR32Value){
				writel(dot11Obj.CR32Value, regp+ZD1205_CR32);
				macp->PhyRegErr++;
			}
		}

		if (macp->bTraceSetPoint){
			if (TrackingLoop == TRACKING_NUM){
				average = (u32)(accumulate / TRACKING_NUM);
				channel = macp->card_setting.Channel;
				
				if (!macp->bContinueTx){
					if (macp->card_setting.TxPowerLevel == 0){ //17dbm
						if (setpoint > macp->EepSetPoint[channel-1])
							setpoint = macp->EepSetPoint[channel-1];
					}
				}			
					
				if (average < setpoint){
					zd1205_IncreaseTxPower(macp);
				}
				else if (average > setpoint){
					zd1205_DecreaseTxPower(macp);
				}
				HW_Write_TxGain(&dot11Obj, macp->TxGainSetting);
				TrackingLoop = 0;
				accumulate = 0;
			}
			else{
				tmpvalue = readl(regp+ZD1205_CR58);
				if ((macp->RF_Mode == AL2210MPVB_RF) || (macp->RF_Mode == AL2210_RF)){
					if (tmpvalue > 0x70){ //filter failed sample
						TrackingLoop++;
						accumulate += tmpvalue;
					}	
				}		
				else {		
					TrackingLoop ++;
					accumulate += tmpvalue;
				}	
			}
		}
		UnLockPhyReg(&dot11Obj);
	}

    mod_timer(&(macp->tm_hking_id), jiffies+ (1*HZ)/10);
}


void zd1205_ConvertDbToSetPoint(struct zd1205_private *macp)
{
	u8 leve = macp->card_setting.TxPowerLevel;

	switch(leve){
		case 0: //17dbm
			if (macp->RF_Mode == GCT_RF){
				macp->SetPoint = 0x75;
				macp->TxGainSetting = 0x0d;
			}else if ((macp->RF_Mode == AL2210MPVB_RF) || (macp->RF_Mode == AL2210_RF)){	
				macp->SetPoint = 0xBF; //CR58
				macp->TxGainSetting = 0x8d; //CR31
			}		
			break;

		case 1: //14 dbm
			if (macp->RF_Mode == GCT_RF){
				macp->SetPoint = 0x61;
				macp->TxGainSetting = 0x07;
			} else if ((macp->RF_Mode == AL2210MPVB_RF) || (macp->RF_Mode == AL2210_RF)){	
				macp->SetPoint = 0xA8; //CR58
				macp->TxGainSetting = 0xA7; //CR31
			}		
			break;

		case 2: //11dbm
			if (macp->RF_Mode == GCT_RF){
				macp->SetPoint = 0x59;
				macp->TxGainSetting = 0x04;
			} else if ((macp->RF_Mode == AL2210MPVB_RF) || (macp->RF_Mode == AL2210_RF)){
				macp->SetPoint = 0x8C; //CR58
				macp->TxGainSetting = 0xBB; //CR31
			}		
			break;

		default:
			break;
	}
}


void zd1205_CollectBssInfo(struct zd1205_private *macp, plcp_wla_header_t *pWlanHdr, u8 *pMacBody, u32 bodyLen)
{
	u8 bssidmatched = 0;
	u8 i, j;
	u8 *pBssid;
	u8 *pByte;
	u32 currPos = 0;
	u8 elemId, elemLen;
	
	if ((*(pMacBody+CAP_OFFSET)) & BIT_1) //IBSS
		pBssid = pWlanHdr->Address3; 
	else 
		pBssid = pWlanHdr->Address2; 	
	
	for (i=0; i<macp->bss_index; i++){
		for (j=0; j<6; j++){
			if (macp->BSSInfo[i].bssid[j] != pBssid[j]){
				break;
			}
		}
		if (j==6){
			bssidmatched = 1;
			break;
		}
	}
		
	if (bssidmatched)
		return;
		
	//get bssid
	for (i=0; i<6; i++){
		macp->BSSInfo[macp->bss_index].bssid[i] = pBssid[i];
	}	
	
	//get beacon interval
	for (i=0; i<2; i++)
		macp->BSSInfo[macp->bss_index].beaconInterval[i] = *(pMacBody+BCN_INTERVAL_OFFSET+i);
	
	//get capability	
	for (i=0; i<2; i++)
		macp->BSSInfo[macp->bss_index].cap[i] = *(pMacBody+CAP_OFFSET+i);
	
	//get element
	pByte = pMacBody+SSID_OFFSET;
	currPos = SSID_OFFSET;
	while(currPos < bodyLen){
		elemId = *pByte;
		elemLen = *(pByte+1);
		switch(elemId){
			case ELEID_SSID: //ssid
				for (i=0; i<elemLen+2; i++){
					macp->BSSInfo[macp->bss_index].ssid[i] = *pByte;
					pByte++;
				}	
				break;
				
			case ELEID_SUPRATES: //supported rateS
				for (i=0; i<elemLen+2; i++){
					macp->BSSInfo[macp->bss_index].supRates[i] = *pByte;
					pByte++;
				}	
				break;	

				
			case ELEID_DSPARMS: //ds parameter
				macp->BSSInfo[macp->bss_index].channel = *(pByte+2);
				pByte += (elemLen+2); 
				break;
				
			case ELEID_EXT_RATES:
				pByte += (elemLen+2); 
				break;	
				
			default:
				pByte += (elemLen+2); 	
				break;
		}
		currPos += elemLen+2;
	}	
	
	macp->BSSInfo[macp->bss_index].signalStrength = macp->rx_signal_sth;
	macp->BSSInfo[macp->bss_index].signalQuality = macp->rx_signal_q;
	
	if (macp->bss_index < (BSS_INFO_NUM-1)){
		macp->bss_index ++;
	}
	return;	
}	



void
zd1205_dump_rfds(struct zd1205_private *macp) 
{
	struct rx_list_elem *rx_struct = NULL;

	struct list_head *entry_ptr = NULL;
	rfd_t *rfd = 0;	
	struct sk_buff *skb;
	int i = 0;

	ZENTER();
	list_for_each(entry_ptr, &(macp->active_rx_list)){
		rx_struct = list_entry(entry_ptr, struct rx_list_elem, list_elem);
		if (!rx_struct)
			return;

		pci_dma_sync_single(macp->pdev, rx_struct->dma_addr,
			macp->rfd_size, PCI_DMA_FROMDEVICE);
		skb = rx_struct->skb;
		rfd = RFD_POINTER(skb, macp);	/* locate RFD within skb */	
#if 0
		printk(KERN_DEBUG "zd1205: i = %x\n", i);
		printk(KERN_DEBUG "zd1205: rx_struct = %x\n", (u32)rx_struct);
		printk(KERN_DEBUG "zd1205: rx_struct->dma_addr = %x\n", (u32)rx_struct->dma_addr);
		printk(KERN_DEBUG "zd1205: rx_struct->skb = %x\n", (u32)rx_struct->skb);

		printk(KERN_DEBUG "zd1205: rfd = %x\n", (u32)rfd);
		printk(KERN_DEBUG "zd1205: CbStatus = %x\n", le32_to_cpu(rfd->CbStatus)); 		
		printk(KERN_DEBUG "zd1205: CbCommand = %x\n", le32_to_cpu(rfd->CbCommand));
		printk(KERN_DEBUG "zd1205: NextCbPhyAddrLowPart = %x\n", le32_to_cpu(rfd->NextCbPhyAddrLowPart));
		printk(KERN_DEBUG "zd1205: NextCbPhyAddrHighPart = %x\n", le32_to_cpu(rfd->NextCbPhyAddrHighPart));
#endif
		zd1205_dump_data("rfd", (u8 *)rfd, 24);
		i++;
	}
	ZEXIT();
}


void zd1205_dump_data(char *info, u8 *data, u32 data_len)
{
    int i;
	
	printk(KERN_DEBUG "%s data [%d]: \n", info, data_len);

	for (i=0; i<data_len; i++){
		printk("%02x", data[i]);
		printk(" ");
		if ((i>0) && ((i+1)%16 == 0))

			printk("\n");
	}

	printk("\n");
}


/**
 * zd1205_get_rx_struct - retrieve cell to hold skb buff from the pool
 * @macp: atapter's private data struct
 *
 * Returns the new cell to hold sk_buff or %NULL.
 */
static inline struct rx_list_elem *
zd1205_get_rx_struct(struct zd1205_private *macp)
{
	struct rx_list_elem *rx_struct = NULL;

	if (!list_empty(&(macp->rx_struct_pool))) {

		rx_struct = list_entry(macp->rx_struct_pool.next,
				       struct rx_list_elem, list_elem);
		list_del(&(rx_struct->list_elem));
	}

	return rx_struct;
}



/**
 * zd1205_alloc_skb - allocate an skb for the adapter
 * @macp: atapter's private data struct
 *
 * Allocates skb with enough room for rfd, and data, and reserve non-data space.
 * Returns the new cell with sk_buff or %NULL.
 */
static inline struct rx_list_elem *
zd1205_alloc_skb(struct zd1205_private *macp)
{
	struct sk_buff *new_skb;
	u32 skb_size = sizeof (rfd_t);
	struct rx_list_elem *rx_struct;

    if (macp->dbg_flag > 4)
	    ZENTER();
        
	new_skb = (struct sk_buff *) dev_alloc_skb(skb_size);
	if (new_skb) {
		/* The IP data should be 
		   DWORD aligned. since the ethernet header is 14 bytes long, 
		   we need to reserve 2 extra bytes so that the TCP/IP headers
		   will be DWORD aligned. */
		//skb_reserve(new_skb, 2); //for zd1202, rx dma must be 4-bytes aligmnebt
		if ((rx_struct = zd1205_get_rx_struct(macp)) == NULL)
			goto err;
        if (macp->dbg_flag > 4)   
		    printk(KERN_DEBUG "zd1205: rx_struct = %x\n", (u32)rx_struct);
		rx_struct->skb = new_skb;
        
        //Rx DMA address  must be 4 bytes alignment
		rx_struct->dma_addr = pci_map_single(macp->pdev, new_skb->data, sizeof (rfd_t), PCI_DMA_FROMDEVICE);
        if (macp->dbg_flag > 4)
            printk(KERN_DEBUG "zd1205: rx_struct->dma_addr = %x\n", (u32)rx_struct->dma_addr);

		if (!rx_struct->dma_addr)
			goto err;

    	skb_reserve(new_skb, macp->rfd_size); //now skb->data point to RxBuffer
        if (macp->dbg_flag > 4)
            ZEXIT();
		return rx_struct;
	} else {
        printk(KERN_DEBUG "zd1205: dev_alloc_skb fail\n");
		return NULL;
	}

err:
    printk(KERN_DEBUG "zd1205: ****** err\n");                
    dev_kfree_skb_irq(new_skb);
	return NULL;
}


/**
 * zd1205_add_skb_to_end - add an skb to the end of our rfd list
 * @macp: atapter's private data struct
 * @rx_struct: rx_list_elem with the new skb
 *
 * Adds a newly allocated skb to the end of our rfd list.
 */

inline void
zd1205_add_skb_to_end(struct zd1205_private *macp, struct rx_list_elem *rx_struct)
{
	rfd_t *rfdn;	/* The new rfd */
	rfd_t *rfd;		/* The old rfd */
	struct rx_list_elem *rx_struct_last;

    if (macp->dbg_flag > 4)
	    ZENTER();
	(rx_struct->skb)->dev = macp->device;
	rfdn = RFD_POINTER(rx_struct->skb, macp);

	rfdn->CbCommand = __constant_cpu_to_le32(RFD_EL_BIT);
	wmb();
	rfdn->CbStatus = 0xffffffff;
	rfdn->ActualCount = 0;
	rfdn->MaxSize = __constant_cpu_to_le32(MAX_WLAN_SIZE);
	rfdn->NextCbPhyAddrHighPart = 0;
	rfdn->NextCbPhyAddrLowPart = 0;
	wmb();

	pci_dma_sync_single(macp->pdev, rx_struct->dma_addr, macp->rfd_size,
			    PCI_DMA_TODEVICE);

	if (!list_empty(&(macp->active_rx_list))) {
		rx_struct_last = list_entry(macp->active_rx_list.prev,
					    struct rx_list_elem, list_elem);
		rfd = RFD_POINTER(rx_struct_last->skb, macp);
        if (macp->dbg_flag > 4)
		    printk(KERN_DEBUG "zd1205: rfd = %x\n", (u32)rfd);
		pci_dma_sync_single(macp->pdev, rx_struct_last->dma_addr,
				    4, PCI_DMA_FROMDEVICE);
		put_unaligned(cpu_to_le32(rx_struct->dma_addr),
			      ((u32 *) (&(rfd->NextCbPhyAddrLowPart))));
		wmb();
		pci_dma_sync_single(macp->pdev, rx_struct_last->dma_addr,
				    8, PCI_DMA_TODEVICE);
		rfd->CbCommand = 0; 
		wmb();
		pci_dma_sync_single(macp->pdev, rx_struct_last->dma_addr,
				    4, PCI_DMA_TODEVICE);

	}
	
	list_add_tail(&(rx_struct->list_elem), &(macp->active_rx_list)); //add elem to active_rx_list
    if (macp->dbg_flag > 4)
	    ZEXIT();

}


static inline void
zd1205_alloc_skbs(struct zd1205_private *macp)
{
	for (; macp->skb_req > 0; macp->skb_req--) {
		struct rx_list_elem *rx_struct;

		if ((rx_struct = zd1205_alloc_skb(macp)) == NULL){
            printk(KERN_DEBUG "zd1205: zd1205_alloc_skb fail\n");
			return;
        }    
		zd1205_add_skb_to_end(macp, rx_struct);
	}
}


void zd1205_transmit_cleanup(struct zd1205_private *macp, sw_tcb_t *sw_tcb)
{
	hw_tcb_t *hw_tcb;

    u32	tbd_cnt;

    int i;
    tbd_t *tbd_arr = sw_tcb->first_tbd;

    if (macp->dbg_flag > 2)
	    ZENTER();

    hw_tcb = sw_tcb->tcb;
    tbd_cnt = le32_to_cpu(hw_tcb->TxCbTbdNumber);
 	tbd_arr += 2; //ctrl_setting and mac_header
    if (macp->dbg_flag > 2)
        printk(KERN_DEBUG "zd1205: umap tbd cnt = %x\n", tbd_cnt-2);

	for (i=0; i<tbd_cnt-2; i++, tbd_arr++) {
        if (macp->dbg_flag > 2)
            printk(KERN_DEBUG "zd1205: umap body_dma = %x\n", le32_to_cpu(tbd_arr->TbdBufferAddrLowPart));
 		pci_unmap_single(macp->pdev,
			le32_to_cpu(tbd_arr->TbdBufferAddrLowPart),
			le32_to_cpu(tbd_arr->TbdCount),
            PCI_DMA_TODEVICE);
	}
        
    if (macp->dbg_flag > 2)
        printk(KERN_DEBUG "zd1205: Free tcb_phys = %x\n", (u32)sw_tcb->tcb_phys);
        
	zd1205_qlast_txq(macp, macp->freeTxQ, sw_tcb);
    if (macp->dbg_flag > 0)
        printk(KERN_DEBUG "zd1205: Cnt of freeTxQ = %x\n", macp->freeTxQ->count);
    
	//sw_tcb->HangDur = 0;
	hw_tcb->CbStatus = 0xffffffff;
	hw_tcb->TxCbTbdNumber = cpu_to_le32(0xaaaaaaaa);	/* for debug */
	hw_tcb->CbCommand = cpu_to_le32(CB_S_BIT);

    if (netif_running(macp->device)){
        netif_wake_queue(macp->device);   //resume tx
    } 
    
    if (macp->dbg_flag > 2)
	    ZEXIT();

	return;		
}


static void
zd1205_tx_isr(struct zd1205_private *macp)
{
	sw_tcb_t *sw_tcb, *next_sw_tcb;
    u16 aid;

    if (macp->dbg_flag > 0)
        printk(KERN_DEBUG "***** zd1205_tx_isr enter *****\n");
        
	if (!macp->activeTxQ->count){
        printk(KERN_DEBUG "No element in activeQ\n");
    	return;
	}	
		
	/* Look at the TCB at the head of the queue.  If it has been completed
     then pop it off and place it at the tail of the completed list.
     Repeat this process until all the completed TCBs have been moved to the
     completed list */
     while (macp->activeTxQ->count){
		sw_tcb = macp->activeTxQ->first;
      	// check to see if the TCB has been DMA'd
		// Workaround for hardware problem that seems leap over a TCB
		// and then fill completion token in the next TCB.

        if (macp->dbg_flag > 1){
            printk(KERN_DEBUG "zd1205: hw_tcb = %x\n", (u32)sw_tcb->tcb);
            printk(KERN_DEBUG "zd1205: CbStatus = %x\n", (u16)le32_to_cpu(sw_tcb->tcb->CbStatus));
        }    

        rmb();
       	if ((u16)le32_to_cpu(sw_tcb->tcb->CbStatus) != CB_STATUS_COMPLETE){
			next_sw_tcb = sw_tcb;
			while(1){
        		next_sw_tcb = next_sw_tcb->next;
        		if (!next_sw_tcb)
        			break;

				if ((u16)le32_to_cpu(next_sw_tcb->tcb->CbStatus) == CB_STATUS_COMPLETE)
					break;
			}

			if (!next_sw_tcb)
				break;
		}
	
       	/* Remove the TCB from the active queue. */
       	sw_tcb = zd1205_first_txq(macp, macp->activeTxQ);
        if (macp->dbg_flag > 1)
            printk(KERN_DEBUG "zd1205: Cnt of activeQ = %x\n", macp->activeTxQ->count);
		
		aid = sw_tcb->aid;
       	zd1205_transmit_cleanup(macp, sw_tcb);
		macp->txCmpCnt++;
        
		if (!sw_tcb->last_frag)

            continue;
        
		zd_EventNotify(EVENT_TX_COMPLETE, ZD_TX_CONFIRM, (U32)sw_tcb->msg_id, (U32)aid);
		macp->bDataTrafficLight = 1;
	}

    if (macp->dbg_flag > 0)
        printk(KERN_DEBUG "***** zd1205_tx_isr exit *****\n");

	return;
} 


static void
zd1205_retry_failed(struct zd1205_private *macp)
{
	void *regp = macp->regp;
	sw_tcb_t *sw_tcb;
	sw_tcb_t *next_sw_tcb = NULL;
    hw_tcb_t *hw_tcb;
	ctrl_set_t *ctrl_set;
	u8 CurrentRate, NewRate;
	u8 ShortPreambleFg;
	u16 Len;
	u16 NextLen;
	u16 LenInUs;
	u16 NextLenInUs;
	u8 Service;
	u16 aid;

    if (macp->dbg_flag > 0)
        printk(KERN_DEBUG "+++++ zd1205_retry_failed enter +++++\n");
	
	if (!macp->activeTxQ->count){
		FPRINT("**********empty activeTxQ, got retry failed");
		sw_tcb = macp->freeTxQ->first;
		writel((sw_tcb->tcb_phys | BIT_0), regp+PCI_TxAddr_p1);
		return;
	}
	
		
	// Feature: Rate Adaption
	// - During the procedure of processing a transmitting frame, we must keep
	//   the TaRate consistent.
	// - When to fall OppositeContext.CurrentTxRate:
	//   Whenever RetryFail occurs, change OppositeContext.CurrentTxRate by a value
	//   ((Rate of this TCB) minus a degree) and modify this TCB's control-setting 
	//   with the OppositeContext.CurrentTxRate and then Restart this TCB.
	//	 (Set RetryMAX = 2).
	//   Once the TxRate is 1M and still RetryFail, abandon this frame.
	// - When to rise TxRate:

	//   If there are 10 frames transmitted successfully 
	//   (OppositeContext.ConsecutiveSuccessFrames >= 10), change 
	//   OppositeContext.CurrentTxRate by a value
	//   ((Rate of this TCB) plus a degree).
	// - Adjust OppositeContext.CurrentTxRate manually. (by application tool)
	sw_tcb = macp->activeTxQ->first;
	aid = sw_tcb->aid;
	ctrl_set = sw_tcb->hw_ctrl;

	if (ctrl_set->ctrl_setting[11] & BIT_3){ //management frame
		//FPRINT("Mgt frame, no rate adaption!!")
		goto no_rate_adaption;
	}	
	
	//CurrentRate = (ctrl_set->ctrl_setting[0] & 0x1f);
	CurrentRate = sw_tcb->rate;
	ShortPreambleFg = (ctrl_set->ctrl_setting[0] & 0x20);
	
	if (((!ShortPreambleFg) && (CurrentRate > RATE_1M)) ||
		 ((ShortPreambleFg) && (CurrentRate > RATE_2M))){ 
		// Fall TxRate a degree
		NewRate = zd1205_RateAdaption(aid, CurrentRate, FALL_RATE);
		sw_tcb->rate = NewRate;
		
		// Modify Control-setting
		ctrl_set->ctrl_setting[0] = (ShortPreambleFg | NewRate);
		ctrl_set->ctrl_setting[11] |= BIT_0; // Set need backoff
		
		// LenInUs, Service
		Len = (ctrl_set->ctrl_setting[1] + ((u16)ctrl_set->ctrl_setting[2] << 8));

		cal_us_service(NewRate, Len, &LenInUs, &Service);
		ctrl_set->ctrl_setting[20] = (u8)LenInUs;
		ctrl_set->ctrl_setting[21] = (u8)(LenInUs >> 8);
		ctrl_set->ctrl_setting[22] = Service;
		
		// NextLenInUs
		NextLen = (ctrl_set->ctrl_setting[18] + ((u16)ctrl_set->ctrl_setting[19] << 8));
		cal_us_service(NewRate, NextLen, &NextLenInUs, &Service);
		ctrl_set->ctrl_setting[23] = (u8)NextLenInUs;
		ctrl_set->ctrl_setting[24] = (u8)(NextLenInUs >> 8);

		// Re-Start Tx-Bus master with a lower rate
		writel((sw_tcb->tcb_phys | BIT_0), regp+PCI_TxAddr_p1);
		return;
	}	

	/* Look at the TCB at the head of the queue.  If it has been completed
     then pop it off and place it at the tail of the completed list.
     Repeat this process until all the completed TCBs have been moved to the
     completed list */
no_rate_adaption:     
   	while (macp->activeTxQ->count){
        if (macp->dbg_flag > 1){
            //printk(KERN_DEBUG "zd1205: sw_tcb = %x\n", (u32)sw_tcb);
            printk(KERN_DEBUG "zd1205: hw_tcb = %x\n", (u32)sw_tcb->tcb);
        }
	    
        /* Remove the TCB from the active queue. */
        sw_tcb = zd1205_first_txq(macp, macp->activeTxQ);
        if (macp->dbg_flag > 1)
            printk(KERN_DEBUG "zd1205: Cnt of activeQ = %x\n", macp->activeTxQ->count);

        zd1205_transmit_cleanup(macp, sw_tcb);
        macp->retryFailCnt++;
        if (!sw_tcb->last_frag)
            continue;
 
 	    zd_EventNotify(EVENT_TX_COMPLETE, ZD_RETRY_FAILED, (U32)sw_tcb->msg_id, 0);
		
		if (!macp->activeTxQ->count){
			// Re-Start Tx-Bus master with an suspend TCB
			hw_tcb = (hw_tcb_t *)sw_tcb->tcb;
			// Set BIT_0 to escape from Retry-Fail-Wait State.
			writel((cpu_to_le32(hw_tcb->NextCbPhyAddrLowPart) | BIT_0), regp+PCI_TxAddr_p1);
		}else{	
			next_sw_tcb = macp->activeTxQ->first;
			// Re-Start Tx bus master
			// Set BIT_0 to escape from Retry-Fail-Wait state.
			writel((next_sw_tcb->tcb_phys | BIT_0), regp+PCI_TxAddr_p1);
		}	
		break;		
    }

    
    if (macp->dbg_flag > 0)
        printk(KERN_DEBUG "+++++ zd1205_retry_failed exit +++++\n");

	return;
}


static void zd1205_config(struct zd1205_private *macp)
{
	void *regp = macp->regp;
	u32 tmpValue;

    ZENTER();
    writel(macp->card_setting.EncryMode, regp+EncryType);
    macp->dtimCount = 0;
    
	/* Set bssid = MacAddress */
	macp->bssid[0] = macp->mac_addr[0];
	macp->bssid[1] = macp->mac_addr[1];

	macp->bssid[2] = macp->mac_addr[2];
	macp->bssid[3] = macp->mac_addr[3];
 	macp->bssid[4] = macp->mac_addr[4];
	macp->bssid[5] = macp->mac_addr[5];

	/* Setup Physical Address */
	writel(cpu_to_le32(*(u32 *)&macp->mac_addr[0]), regp+MACAddr_P1);
	writel(cpu_to_le32(*(u32 *)&macp->mac_addr[4]), regp+MACAddr_P2);
	writel(cpu_to_le32(*(u32 *)&macp->mac_addr[0]), regp+BSSID_P1);
	writel(cpu_to_le32(*(u32 *)&macp->mac_addr[4]), regp+BSSID_P2);
	
    macp->intr_mask = ZD1205_INT_MASK;

	if (macp->intr_mask & DTIM_NOTIFY_EN)
		macp->dtim_notify_en = 1;
	else 
		macp->dtim_notify_en = 0;	
	
	if (macp->intr_mask & CFG_NEXT_BCN_EN)
		macp->config_next_bcn_en = 1;
	else 
		macp->config_next_bcn_en = 0;

    zd1205_ClearTupleCache(macp);
	zd1205_ArReset(macp);

	macp->bTraceSetPoint = 1;
	macp->bFixedRate = 0;
    macp->bDeviceInSleep = 0; 
    macp->ResetFlag = 0;
   	macp->bGkInstalled = 0;
   	// read Set Point from EEPROM
#if 1    
	tmpValue = readl(regp+ZD_E2P_PWR_CAL_VALUE1);
	macp->EepSetPoint[0] = (u8)tmpValue;
	macp->EepSetPoint[1] = (u8)(tmpValue >> 8);
	macp->EepSetPoint[2] = (u8)(tmpValue >> 16);
	macp->EepSetPoint[3] = (u8)(tmpValue >> 24);
	
	tmpValue = readl(regp+ZD_E2P_PWR_CAL_VALUE2);
	macp->EepSetPoint[4] = (u8)tmpValue;
	macp->EepSetPoint[5] = (u8)(tmpValue >> 8);
	macp->EepSetPoint[6] = (u8)(tmpValue >> 16);
	macp->EepSetPoint[7] = (u8)(tmpValue >> 24);
	
	tmpValue = readl(regp+ZD_E2P_PWR_CAL_VALUE3);
	macp->EepSetPoint[8] = (u8)tmpValue;
	macp->EepSetPoint[9] = (u8)(tmpValue >> 8);
	macp->EepSetPoint[10] = (u8)(tmpValue >> 16);
	macp->EepSetPoint[11] = (u8)(tmpValue >> 24);
	
	tmpValue = readl(regp+ZD_E2P_PWR_CAL_VALUE4);
	macp->EepSetPoint[12] = (u8)tmpValue;
	macp->EepSetPoint[13] = (u8)(tmpValue >> 8);
	
 	macp->RegionCode = readl(regp+E2P_REGION_CODE);
	dot11Obj.RegionCode = macp->RegionCode;
	HW_Set_FilterBand(&dot11Obj, macp->RegionCode);
#endif    
   	switch(macp->RF_Mode){
		case GCT_RF:
			macp->MaxTxPwrSet = GCT_MAX_TX_PWR_SET;
			macp->MinTxPwrSet = GCT_MIN_TX_PWR_SET;
			macp->card_setting.Rate275 = 0;
			break;
		
		case AL2210_RF:		
		case AL2210MPVB_RF:
			macp->MaxTxPwrSet = AL2210_MAX_TX_PWR_SET;
			macp->MinTxPwrSet = AL2210_MIN_TX_PWR_SET;
			macp->card_setting.Rate275 = 1;
			break;	
			
		default:
			macp->MaxTxPwrSet = GCT_MAX_TX_PWR_SET;
			macp->MinTxPwrSet = GCT_MIN_TX_PWR_SET;
			macp->card_setting.Rate275 = 0;
			break;	
	}	
}	


static void zd1205_dtim_notify(
	struct zd1205_private *macp
)
{
	void *regp = macp->regp;
	sw_tcb_t *sw_tcb;
	u32 tmp_value;

	zd_EventNotify(EVENT_DTIM_NOTIFY, 0, 0, 0);

	if (!macp->activeTxQ->count)
		sw_tcb = macp->freeTxQ->first;
	else
		sw_tcb = macp->activeTxQ->first;		

	tmp_value = readl(regp+DeviceState);
	tmp_value &= 0xf;
	writel((sw_tcb->tcb_phys | BIT_0), regp+PCI_TxAddr_p1);
}



void zd1205_config_wep_keys(struct zd1205_private *macp)
{
	void *reg = dot11Obj.reg;
	u8	encryMode = macp->card_setting.EncryMode;
    card_Setting_t *pSetting = &macp->card_setting;
	u32	tmpKey;
	u8	loop = 0;
	u8	i;

	if (encryMode < 2)
		return;

	if (encryMode == WEP64)
		loop = 5;
	else if (encryMode == WEP128)
		loop = 13;
	else if (encryMode == TKIP)
		loop = 16;

	for (i=0; i<loop; i++){
		tmpKey = dot11Obj.GetReg(reg, (ZD_WEPKey0+4*i));

		//key 1
		tmpKey &= ~0x000000ff;
		tmpKey |= (u32)pSetting->keyVector[0][i] << 0;

		//key 2
		tmpKey &= ~0x0000ff00;
		tmpKey |= (u32)pSetting->keyVector[1][i] << 8;

		//key 3
		tmpKey &= ~0x00ff0000;
		tmpKey |= (u32)pSetting->keyVector[2][i] << 16;

		//key 4
		tmpKey &= ~0xff000000;
		tmpKey |= (u32)pSetting->keyVector[3][i] << 24;

		//set to HMAC
		dot11Obj.SetReg(reg, (ZD_WEPKey0+4*i), tmpKey);
	}

	return;
}


static int zd1205_validate_frame(
	struct zd1205_private *macp,
	rfd_t *rfd 
)
{
	plcp_wla_header_t *wla_hdr;
	u32	plcp_us;
	u32	mac_length;
	u32	min_length;
	u32 frame_len;

	// Accept Data/Management frame only.
	wla_hdr = (plcp_wla_header_t *)&rfd->RxBuffer[macp->rx_offset];
 	frame_len = (le32_to_cpu(rfd->ActualCount) & 0x3fff);
    frame_len -= macp->rx_offset;

	if (bWepBit(wla_hdr)){
        //if (macp->card_setting.EncryMode == ENCRY_TKIP)
        //    min_length = 48;
        //else
            min_length = 44;

		if (frame_len < min_length){
			printk(KERN_DEBUG "Error minimum length\n");
			printk(KERN_DEBUG "frame_len = %x\n", frame_len);
			return false;
		}
	}
	else{
		// Minimum Length = PLCP(5)+MACHeader(24)+EXTINFO(5)+CRC(4)
		if (frame_len < 36){
			printk(KERN_DEBUG "Error minimum length\n");
			printk(KERN_DEBUG "frame_len = %x\n", frame_len);
			return false;
		}
	}

	// Check if frame_len > MAX_WLAN_SIZE.
	if (frame_len > MAX_WLAN_SIZE){
		// Do not worry about the corruption of HwRfd.
		// If the frame_len > 2410, the Rx-Bus-Master skip the bytes that exceed
		// 2410 to protect the structure of HwRfd.
		// However, the Rx-Bus-Master still reports this frame to host if the frame
		// is recognized as good by the FA(Frame Analyzer).
		return false;
	}

	// Check if the SwRfd->frame_len matched the length derived from PLCP.
	plcp_us = wla_hdr->plcp_hdr[2]+(((u16)wla_hdr->plcp_hdr[3]) << 8);
	switch(wla_hdr->plcp_hdr[0]){
		case 0x0A:
			mac_length = (plcp_us >> 3);
            macp->rate = RATE_1M;
			break;
			
		case 0x14:
			mac_length = (plcp_us >> 2);
            macp->rate = RATE_2M;
 			break;
			
		case 0x37:
 			mac_length = ((plcp_us*11) >> 4);
            macp->rate = RATE_5M;
 			break;

 		case 0x6E:
			mac_length = ((plcp_us*11) >> 3);
			if (wla_hdr->plcp_hdr[1] & BIT_7)
 				mac_length -=1;
            macp->rate = RATE_11M;
			break;

        case 0xa5: //16.5M
            mac_length = (((plcp_us*33) >> 4) -          
                2*((wla_hdr->plcp_hdr[1] & BIT_6) >> 6) -


                ((wla_hdr->plcp_hdr[1] & BIT_7) >> 7));
            macp->rate = RATE_16M;    
			break;

        case 0xdc: //22.5M
            mac_length = (((plcp_us*11) >> 2) -
                2*((wla_hdr->plcp_hdr[1] & BIT_6) >> 6) -
                ((wla_hdr->plcp_hdr[1] & BIT_7) >> 7));
            macp->rate = RATE_22M;    
			break;      

        case 0x13: //27.5M
            mac_length = (((plcp_us*55) >> 4) -
                2*((wla_hdr->plcp_hdr[1] & BIT_6) >> 6) -
                ((wla_hdr->plcp_hdr[1] & BIT_7) >> 7));
            macp->rate = RATE_27M; 
			break;
				
		default:
			macp->rx_invalid ++;
			return false;
			break;
	}
	
	if (mac_length != (frame_len - (PLCP_HEADER+EXTRA_INFO_LEN))){	// Minus PLCP(5)+EXT_INFO(5)
 		macp->rx_invalid ++;
		return false;
	}

	macp->rx_signal_q = rfd->RxBuffer[frame_len+macp->rx_offset-EXTRA_INFO_LEN];
	macp->rx_signal_sth = rfd->RxBuffer[frame_len+macp->rx_offset-(EXTRA_INFO_LEN -1)];
	macp->rx_signal_q2 = rfd->RxBuffer[frame_len+macp->rx_offset-(EXTRA_INFO_LEN-2)];

	return true;
}


/**
 * zd1205_alloc_tcb_pool - allocate TCB circular list
 * @macp: atapter's private data struct
 *
 * This routine allocates memory for the circular list of transmit descriptors.
 *
 * Returns:
 *       0: if allocation has failed.
 *       1: Otherwise. 
 */
int
zd1205_alloc_tcb_pool(struct zd1205_private *macp)
{
	ZENTER();

	/* deal with Tx uncached memory */
	/* Allocate memory for the shared transmit resources with enough extra mem
       to paragraph align (4-byte alignment) everything  */

	macp->tx_uncached_size = (macp->num_tcb * 
	 	(sizeof(hw_tcb_t)+ sizeof(ctrl_set_t)+sizeof(header_t)))
	 	+ (macp->num_tbd * sizeof(tbd_t)); 	  

	if (!(macp->tx_uncached = pci_alloc_consistent(macp->pdev, 
			macp->tx_uncached_size, &(macp->tx_uncached_phys)))) {
		return 0;
	}

	memset(macp->tx_uncached, 0x00, macp->tx_uncached_size);

	ZEXIT();

	return 1;

}


void
zd1205_free_tcb_pool(struct zd1205_private *macp)
{

	ZENTER();

	pci_free_consistent(macp->pdev, macp->tx_uncached_size,
	    macp->tx_uncached, macp->tx_uncached_phys);
	macp->tx_uncached_phys = 0;
	ZEXIT();
}


static void
zd1205_free_rfd_pool(struct zd1205_private *macp)
{
	struct rx_list_elem *rx_struct;

	ZENTER();


	while (!list_empty(&(macp->active_rx_list))) {
		rx_struct = list_entry(macp->active_rx_list.next,
			struct rx_list_elem, list_elem);
		list_del(&(rx_struct->list_elem));
		pci_unmap_single(macp->pdev, rx_struct->dma_addr,
			sizeof (rfd_t), PCI_DMA_TODEVICE);
		dev_kfree_skb(rx_struct->skb);
		kfree(rx_struct);
	}

	while (!list_empty(&(macp->rx_struct_pool))) {
		rx_struct = list_entry(macp->rx_struct_pool.next,
				       struct rx_list_elem, list_elem);
		list_del(&(rx_struct->list_elem));
		kfree(rx_struct);
	}
	ZEXIT();
}


/**
 * zd1205_alloc_rfd_pool - allocate RFDs
 * @macp: atapter's private data struct
 *
 * Allocates initial pool of skb which holds both rfd and data,
 * and return a pointer to the head of the list
 */
static int
zd1205_alloc_rfd_pool(struct zd1205_private *macp)
{
	struct rx_list_elem *rx_struct;
	int i;

	ZENTER();
	INIT_LIST_HEAD(&(macp->active_rx_list));
	INIT_LIST_HEAD(&(macp->rx_struct_pool));
	macp->skb_req = macp->num_rfd;
	
	for (i = 0; i < macp->skb_req; i++) {
		rx_struct = kmalloc(sizeof (struct rx_list_elem), GFP_ATOMIC);
		list_add(&(rx_struct->list_elem), &(macp->rx_struct_pool));
	}

	zd1205_alloc_skbs(macp);
	return !list_empty(&(macp->active_rx_list));
}


void
zd1205_clear_pools(struct zd1205_private *macp)
{
	ZENTER();
	zd1205_dealloc_space(macp);
 	zd1205_free_rfd_pool(macp);
	zd1205_free_tcb_pool(macp);

	ZEXIT();
}


/**
 * zd1205_start_ru - start the RU if needed
 * @macp: atapter's private data struct
 *
 * This routine checks the status of the 82557's receive unit(RU),
 * and starts the RU if it was not already active.  However,
 * before restarting the RU, the driver gives the RU the buffers
 * it freed up during the servicing of the ISR. If there are
 * no free buffers to give to the RU, (i.e. we have reached a
 * no resource condition) the RU will not be started till the
 * next ISR.
 */
void
zd1205_start_ru(struct zd1205_private *macp) //TBD
{
	void *regp = macp->regp;
	u32 tmp_value;
	struct rx_list_elem *rx_struct = NULL;
	struct list_head *entry_ptr = NULL;

	rfd_t *rfd = 0;	
	int buffer_found = 0;
	struct sk_buff *skb;
    u32 loopCnt = 0;

    if (macp->dbg_flag > 4)
	    ZENTER();

	list_for_each(entry_ptr, &(macp->active_rx_list)){
		rx_struct = list_entry(entry_ptr, struct rx_list_elem, list_elem);
		if (!rx_struct)
			return;

		pci_dma_sync_single(macp->pdev, rx_struct->dma_addr,
			macp->rfd_size, PCI_DMA_FROMDEVICE);
		skb = rx_struct->skb;

		rfd = RFD_POINTER(skb, macp);	/* locate RFD within skb */		
		if (SKB_RFD_STATUS(rx_struct->skb, macp) !=  __constant_cpu_to_le32(RFD_STATUS_COMPLETE)) {
			buffer_found = 1;
			break;
		}
	}
    
	/* No available buffers */
	if (!buffer_found) {
		printk(KERN_ERR "zd1205: No available buffers\n");
		return;
	}

	while(1){	
		tmp_value = readl(regp+DeviceState);
 		tmp_value &= 0xf0;
		if ((tmp_value == RX_READ_RCB) || (tmp_value == RX_CHK_RCB)){
			/* Device is now checking suspend or not.
			 Keep watching until it finished check. */
			udelay(1);
			continue;
		}
		else{
			break;
		}
        loopCnt++;
        if (loopCnt > 10000000)
            break;
	}
    if (loopCnt > 10000000)
        FPRINT("I am in zd1205_start_ru loop"); 
		
	if (tmp_value == RX_IDLE){ 
		/* Rx bus master is in idle state. */
		if ((u16)le32_to_cpu(rfd->CbStatus) != RFD_STATUS_COMPLETE){
			writel(rx_struct->dma_addr, regp+PCI_RxAddr_p1);
		}
	}	

    if (macp->dbg_flag > 4)
	    ZEXIT();
}


#define ETH_P_80211_RAW     (ETH_P_ECONET + 1)
/**
 * zd1205_rx_isr - service RX queue
 * @macp: atapter's private data struct
 * @max_number_of_rfds: max number of RFDs to process
 * @rx_congestion: flag pointer, to inform the calling function of congestion.

 *
 * This routine processes the RX interrupt & services the RX queues.
 * For each successful RFD, it allocates a new msg block, links that
 * into the RFD list, and sends the old msg upstream.
 * The new RFD is then put at the end of the free list of RFD's.
 * It returns the number of serviced RFDs.
 */
u32
zd1205_rx_isr(struct zd1205_private *macp)
{
	rfd_t *rfd;		/* new rfd, received rfd */
	int i;
	u32 rfd_status;
	struct sk_buff *skb;
	struct net_device *dev;
  	u32 data_sz;
	struct rx_list_elem *rx_struct;

	u32 rfd_cnt = 0;
	plcp_wla_header_t *wla_hdr;
	u8 *pHdr;
    u8 *pIv;
	u8 *pBody = NULL;
	u32 bodyLen = 0;
	u32 hdrLen = WLAN_HEADER;
    u16 seq = 0;
	u8 frag = 0;
	u8 *pTa = NULL;
	defrag_Array_t *pDefArray = &macp->defragArray;
	u8 EthHdr[12];
    void *regp = macp->regp;
    card_Setting_t *pSetting = &macp->card_setting;
    u8 bExtIV = 0;
    u8 bDataFrm = 0;	

	dev = macp->device;

    if (macp->dbg_flag > 4)


	    ZENTER();	
	/* current design of rx is as following:
	 * 1. socket buffer (skb) used to pass network packet to upper layer
	 * 2. all HW host memory structures (like RFDs, RBDs and data buffers)
	 *    are placed in a skb's data room
	 * 3. when rx process is complete, we change skb internal pointers to exclude
	 *    from data area all unrelated things (RFD, RDB) and to leave
	 *    just rx'ed packet netto
	 * 4. for each skb passed to upper layer, new one is allocated instead.
	 * 5. if no skb left, in 2 sec another atempt to allocate skbs will be made
	 *    (watchdog trigger SWI intr and isr should allocate new skbs)
	 */
    //macp->rx_offset = readl(macp->regp + RX_OFFSET_BYTE);

	for (i = 0; i < macp->num_rfd; i++) {
 		if (list_empty(&(macp->active_rx_list))) { 
			printk(KERN_ERR "zd1205: list_empty\n");
			break;
		}    

		rmb();
		rx_struct = list_entry(macp->active_rx_list.next,
				struct rx_list_elem, list_elem);
        if (macp->dbg_flag > 4)        
		    printk(KERN_DEBUG "zd1205: rx_struct = %x\n", (u32)rx_struct);		
		skb = rx_struct->skb;
 		rfd = RFD_POINTER(skb, macp);	/* locate RFD within skb */

		// sync only the RFD header
		pci_dma_sync_single(macp->pdev, rx_struct->dma_addr,
				    macp->rfd_size+PLCP_HEADER+WLAN_HEADER, PCI_DMA_FROMDEVICE);
		rfd_status = SKB_RFD_STATUS(rx_struct->skb, macp);	/* get RFD's status */

        if (macp->dbg_flag > 4)
		    printk(KERN_DEBUG "zd1205: rfd_status = %x\n", rfd_status);
		if (rfd_status != __constant_cpu_to_le32(RFD_STATUS_COMPLETE))	/* does not contains data yet - exit */
			break;

		/* to allow manipulation with current skb we need to unlink it */
		list_del(&(rx_struct->list_elem));
		data_sz = (u16)(le32_to_cpu(rfd->ActualCount) & 0x3fff);

        data_sz -= macp->rx_offset;        
        if (macp->dbg_flag > 4)
    	    printk(KERN_DEBUG "zd1205: data_sz = %x\n", data_sz);
         
		wla_hdr = (plcp_wla_header_t *)&rfd->RxBuffer[macp->rx_offset];
		pHdr = (u8 *)wla_hdr + PLCP_HEADER;
		macp->rxCnt++;
            
        if (!macp->sniffer_on){
        	//BaseFrmType = BaseFrameType(wla_hdr);
	        if (bDataMgtFrame(wla_hdr)){ //Data or Management Frames
			    /* do not free & unmap badly recieved packet.
 		 		* move it to the end of skb list for reuse */
      
                //sync for access correctly
                pci_dma_sync_single(macp->pdev, rx_struct->dma_addr,
				    data_sz + macp->rfd_size, PCI_DMA_FROMDEVICE);
                    
 			    if (zd1205_validate_frame(macp, rfd) == false){
                    printk(KERN_ERR "zd1205: invalid frame\n"); 		
				    macp->invalid_frame_good_crc ++;
				    zd1205_add_skb_to_end(macp, rx_struct);
				    continue;
			    }

			    seq = getSeq(wla_hdr);
				frag = getFrag(wla_hdr);
				pTa = getTA(wla_hdr);
 
    			if (!bGroup(wla_hdr)){ //unicast
					if (memcmp(&wla_hdr->Address1[0], &macp->mac_addr[0], 6) != 0){
						zd1205_add_skb_to_end(macp, rx_struct);
						continue;
					}	
					else{ //check dupicated frame
						if ((bRetryBit(wla_hdr)) 
								&& (zd1205_SearchTupleCache(macp, pTa, seq, frag))){ //dupicated
							zd1205_UpdateTupleCache(macp, pTa, seq, frag);
							zd1205_add_skb_to_end(macp, rx_struct);
							macp->rxDup ++;
							continue; 
						}
						zd1205_UpdateTupleCache(macp, pTa, seq, frag);
					}	
				}
				else { //group address
					if (BaseFrameType(wla_hdr) == DATA){
						zd1205_add_skb_to_end(macp, rx_struct);
						continue;
					}	
				}	
				
				hdrLen = WLAN_HEADER;
 			    pBody = (u8 *)pHdr + WLAN_HEADER;
			    bodyLen = data_sz - PLCP_HEADER - WLAN_HEADER - EXTRA_INFO_LEN - CRC32_LEN;

                //frame with WEP
                if (bWepBit(wla_hdr)) {
                	u16 RxIv16 = 0;
					u32 RxIv32 = 0;
					int result = 0;
				
                    if (macp->dbg_flag > 4)
                        printk(KERN_DEBUG "zd1205: wep frame\n");
                    pIv =  pHdr +  hdrLen;
                    pBody += IV_SIZE;
                    bodyLen =  bodyLen - IV_SIZE - ICV_SIZE;
                    hdrLen += IV_SIZE;
                    if (pIv[3] & EXTENDED_IV){
                    	bExtIV = 1;
                        pBody +=  EXTEND_IV_LEN;
                        bodyLen -=  EXTEND_IV_LEN; 
                        hdrLen += EXTEND_IV_LEN;
						RxIv16 = ((u16)pIv[0] << 8) + pIv[2];
						RxIv32 = pIv[4] + ((u32)pIv[5] << 8) + 
							((u32)pIv[6] << 16) + ((u32)pIv[7] << 24);
                    }

					
					if ((pSetting->SwCipher) && (readl(regp+EncryType) & BIT_3)){ //hardware decryption was disabled
						u8 DecryKeyId;
						u32 icv = 0xFFFFFFFFL;
						u8 WepKeyLen = 0;

						u8 *pWepKey = NULL;
						u8 encryMode;
						u8 KeyCOntent[16];
										
						DecryKeyId = ((pIv[3] & 0xc0) >> 6);
					
						if (pSetting->DynKeyMode == 0){ // use static 4 keys
							WepKeyLen = pSetting->WepKeyLen;

							pWepKey = &pSetting->keyVector[DecryKeyId][0];
						}
						else if ((pSetting->DynKeyMode == DYN_KEY_WEP64) || (pSetting->DynKeyMode == DYN_KEY_WEP128)){
							if (!zd_GetKeyInfo(&wla_hdr->Address2[0], &encryMode, &WepKeyLen, KeyCOntent)){
								// failed to get key info
								zd_CmdProcess(CMD_DISASOC, &wla_hdr->Address2[0], ZD_CLASS3_ERROR);
								zd1205_add_skb_to_end(macp, rx_struct);
								continue;
							}
							else{
								if (WepKeyLen == 0){
									zd1205_add_skb_to_end(macp, rx_struct);
									continue;
								}	
								else{
									pWepKey = KeyCOntent;
								}		
							}		
						}
						else if ((pSetting->DynKeyMode == DYN_KEY_TKIP) && (bExtIV == 1)){
							result = zd_GetKeyInfo_ext(&wla_hdr->Address2[0], &encryMode, &WepKeyLen, KeyCOntent, RxIv16, RxIv32);
							if (result == -1){
								// failed to get key info
								//FPRINT("zd_GetKeyInfo_ext failed !!!");
								//zd1205_dump_data("SA = ", &wla_hdr->Address2[0], 6);
								zd_CmdProcess(CMD_DISASOC, &wla_hdr->Address2[0], ZD_CLASS3_ERROR);
								zd1205_add_skb_to_end(macp, rx_struct);
								continue;
							}
							else if (result == -2){
								//FPRINT("Pairwise not installed !!!!");
								//zd1205_dump_data("SA = ", &wla_hdr->Address2[0], 6);
								zd1205_add_skb_to_end(macp, rx_struct);
								continue;
							}
							else {
								WepKeyLen = 13;
								pWepKey = &KeyCOntent[3];
							}
						}				
						
						if (!zd_DecryptData(WepKeyLen, pWepKey, pIv, 
								(bodyLen+ICV_SIZE), pBody, pBody, &icv)){
							zd1205_add_skb_to_end(macp, rx_struct);
							macp->swDecrypErr ++;
							continue;
						}
						else{
							macp->swDecrypOk ++;
						}	
					}//end of software decryption	
               
                }//end of wep bit
				
                memcpy(EthHdr, &pHdr[10], 12);
				        
                if (BaseFrameType(wla_hdr) == DATA){
                	bDataFrm = 1;
				    if ((isWDS(wla_hdr)) || (bGroup(wla_hdr))){
                        //FPRINT("***** WDS or group");
                        zd1205_add_skb_to_end(macp, rx_struct);
					    continue;
				    }

				    if (frag == 0){ //No fragment or first fragment
					    if (!bMoreFrag(wla_hdr)){ //No more fragment
                            //FPRINT("***** No Frag");
						    goto defrag_ind;
					    }
					    else{	//First fragment
						    DFDEBUG("***** First Frag");

						    i = zd1205_ArFree(macp); //Get a free one
						    if (i < 0){
							    zd1205_ArAge(macp, nowT());
							    i = zd1205_ArFree(macp);
							    if (i < 0){
								    DFDEBUG("***** ArFree fail");
								    zd1205_add_skb_to_end(macp, rx_struct);
								    continue;
							    }
						    }

						    zd1205_ArUpdate(macp, pTa, seq, frag, i);
						    pDefArray->mpdu[i].dataStart = pBody;
						    skb->len = bodyLen;
						    pDefArray->mpdu[i].buf = (void *)skb; //save skb

						    //zd1205_ArAge(macp, nowT());
		                    pci_unmap_single(macp->pdev, rx_struct->dma_addr,
				                sizeof (rfd_t), PCI_DMA_FROMDEVICE);

                    		list_add(&(rx_struct->list_elem), &(macp->rx_struct_pool));
		                    macp->skb_req++;	/* incr number of requested skbs */
		                    zd1205_alloc_skbs(macp);	/* and get them */
		                    rfd_cnt++;
						    continue;
					    }
				    }//end of farg == 0
				    else{ //more frag
                        struct sk_buff *defrag_skb;
					    u8 *pStart;

					    i = zd1205_ArSearch(macp, pTa, seq, frag); //Get exist one
					    if (i < 0){
						    DFDEBUG("***** ArSearch fail");
						    zd1205_ArAge(macp, nowT());
						    zd1205_add_skb_to_end(macp, rx_struct); //discard this one

						    continue;
					    }

					    defrag_skb = (struct sk_buff *)pDefArray->mpdu[i].buf;
					    pStart = (u8 *)pDefArray->mpdu[i].dataStart;
					    pDefArray->mpdu[i].fn = frag;
                        memcpy((pStart+defrag_skb->len), pBody, bodyLen); //copy to reassamble buffer
					    defrag_skb->len += bodyLen;
                                                
					    if (!bMoreFrag(wla_hdr)){ //Last fragment
						    DFDEBUG("***** Last Frag");
						    zd1205_add_skb_to_end(macp, rx_struct);
						    pDefArray->mpdu[i].inUse = 0;
						    skb = defrag_skb;
						    skb->data = (u8 *)pDefArray->mpdu[i].dataStart; //point mac body
						    pBody = skb->data;
						    bodyLen = skb->len;
						    goto defrag_ind;
					    }
					    else{
						    DFDEBUG("***** More Frag");
						    zd1205_ArAge(macp, nowT());
						    zd1205_add_skb_to_end(macp, rx_struct);
						    continue;
					    }
				    }
			    }//end of data frame
			    else if (BaseFrameType(wla_hdr) == MANAGEMENT){
                    if (SubFrameType(wla_hdr) == BEACON){
    					if (dot11Obj.ConfigFlag & CHANNEL_SCAN_SET){
						    zd1205_CollectBssInfo(macp, wla_hdr, pBody, bodyLen);
					    }
                        zd1205_add_skb_to_end(macp, rx_struct);
                        continue;
      			    }
                    else
    				    goto defrag_ind;
			    } //end of management frame

		    }
            else if (SubFrameType(wla_hdr) == PS_POLL){
    	    	if (memcmp(&wla_hdr->Address1[0], &macp->mac_addr[0], 6) == 0) //Ps-Poll for me
			    	zd_CmdProcess(CMD_PS_POLL, (void *)pHdr, 0);
			    zd1205_add_skb_to_end(macp, rx_struct);

			    continue;
		    }	
        }//end of sniffer_on    	

defrag_ind:        
		pci_unmap_single(macp->pdev, rx_struct->dma_addr,
				 sizeof (rfd_t), PCI_DMA_FROMDEVICE);
                 
    	list_add(&(rx_struct->list_elem), &(macp->rx_struct_pool));
 		/* end of dma access to rfd */

		macp->skb_req++;	/* incr number of requested skbs */
		zd1205_alloc_skbs(macp);	/* and get them */
		rfd_cnt++;
        
        if (!macp->sniffer_on){
            zd_ReceivePkt(pHdr, hdrLen, pBody, bodyLen, macp->rate, (void *)skb, bDataFrm, EthHdr);
            macp->bDataTrafficLight = 1;
        }
        else{    
            skb->tail = skb->data = pHdr;
            skb_put(skb, data_sz - PLCP_HEADER);
            skb->mac.raw = skb->data;
            skb->pkt_type = PACKET_OTHERHOST;
            skb->protocol = htons(ETH_P_802_2);
            skb->dev = dev;
            skb->ip_summed = CHECKSUM_NONE;
            netif_rx(skb);
        }    
           
	}/* end of rfd loop */


	/* restart the RU if it has stopped */
    zd1205_start_ru(macp);

    if (macp->dbg_flag > 4)
	    ZEXIT();
        
	return rfd_cnt;
}


 
/**
 * zd1205_intr - interrupt handler
 * @irq: the IRQ number
 * @dev_inst: the net_device struct
 * @regs: registers (unused)
 *
 * This routine is the ISR for the zd1205 board. It services
 * the RX & TX queues & starts the RU if it has stopped due
 * to no resources.
 */
void
zd1205_intr(int irq, void *dev_inst, struct pt_regs *regs)
{
	struct net_device *dev;
	struct zd1205_private *macp;
	void *regp;
	u32 intr_status;
    //u32 rfd_addr;                         

	dev = dev_inst;
	macp = dev->priv;
	regp = macp->regp;

    intr_status = readl(regp+InterruptCtrl);

	if (!intr_status)
		return;
        
	/* disable intr before we ack & after identifying the intr as ours */
	zd1205_disable_int();
	
	/* the device is closed, don't continue or else bad things may happen. */
	if (!netif_running(dev)) {
		zd1205_enable_int();
		return;
	}
	
	if (macp->driver_isolated) {
		goto exit;
	}

	
    {  
		/* Then, do Rx as soon as possible */
		if (intr_status & RX_COMPLETE){ 
			writel((intr_status | RX_COMPLETE), regp+InterruptCtrl);
			macp->drv_stats.rx_intr_pkts += zd1205_rx_isr(macp);
		}	

		/* Then, recycle Tx chain/descriptors */
		if (intr_status & TX_COMPLETE){
			writel((intr_status | TX_COMPLETE), regp+InterruptCtrl);	
			zd1205_tx_isr(macp);
			macp->TxStartTime = 0;
		}

		if (intr_status & RETRY_FAIL) {
			writel((intr_status | RETRY_FAIL), regp+InterruptCtrl);
       		zd1205_retry_failed(macp);
       		macp->TxStartTime = 0;
		}
		
		if (intr_status & CFG_NEXT_BCN) {
			writel((intr_status | CFG_NEXT_BCN), regp+InterruptCtrl);

			if (macp->config_next_bcn_en){
				macp->bcnCnt++;

                if (macp->dtimCount == 0)
                    macp->dtimCount = macp->card_setting.DtimPeriod;
	            macp->dtimCount--;
                zd_EventNotify(EVENT_TBCN, 0, 0, 0);
			}	
		}
		
		if (intr_status & DTIM_NOTIFY){ 
			writel((intr_status | DTIM_NOTIFY), regp+InterruptCtrl);
			if (macp->dtim_notify_en){
				macp->dtimCnt++;
				zd1205_dtim_notify(macp);
			}	
		}

        if (intr_status & BUS_ABORT){
            if (!macp->bDeviceInSleep)
                FPRINT("******Bus Abort!!!");
            writel(0xff, regp+InterruptCtrl);
            //zd1205_sleep_reset(macp);
        }

        if (intr_status & WAKE_UP){
            FPRINT("******WAKE_UP!!!");
            writel(0xff, regp+InterruptCtrl);
            if (macp->bDeviceInSleep)
                zd1205_process_wakeup(macp);

        }
    }    
    
    if (macp->dtimCount == macp->card_setting.DtimPeriod - 1){
		if (dot11Obj.QueueFlag & AWAKE_QUEUE_SET)
           	tasklet_schedule(&macp->zd1205_ps_tasklet);
    }
    
	if (dot11Obj.QueueFlag & MGT_QUEUE_SET)
	    tasklet_schedule(&macp->zd1205_tasklet);

    if (dot11Obj.QueueFlag & TX_QUEUE_SET)
       	tasklet_schedule(&macp->zd1205_tx_tasklet);
         
exit:
	zd1205_enable_int();
}



static int
zd1205_open(struct net_device *dev)
{
	struct zd1205_private *macp;
	int rc = 0;

	ZENTER();	

	macp = dev->priv;

	read_lock(&(macp->isolate_lock));

	if (macp->driver_isolated) {
		rc = -EBUSY;
		goto exit;
	}

	if ((rc = zd1205_alloc_space(macp)) != 0) {
		rc = -ENOMEM;
		goto exit;
	}

	/* setup the tcb pool */
	if (!zd1205_alloc_tcb_pool(macp)) {
        printk(KERN_ERR "zd1205: failed to zd1205_alloc_tcb_pool\n");
		rc = -ENOMEM;
		goto err_exit;
	}

	zd1205_setup_tcb_pool(macp);
	if (!zd1205_alloc_rfd_pool(macp)) {
		printk(KERN_ERR "zd1205: failed to zd1205_alloc_rfd_pool\n");
		rc = -ENOMEM;
		goto err_exit; 
	}
	
	mod_timer(&(macp->watchdog_timer), jiffies + (1*HZ));
	mod_timer(&(macp->tm_hking_id), jiffies + (1*HZ)/10);
	netif_start_queue(dev);
	zd1205_start_ru(macp);

	if ((rc = request_irq(dev->irq, &zd1205_intr, SA_SHIRQ, dev->name, dev)) != 0) {
		printk(KERN_ERR "zd1205: failed to request_irq\n");	
		del_timer_sync(&macp->watchdog_timer);
		del_timer_sync(&macp->tm_hking_id);
		goto err_exit;
	}

	zd_UpdateCardSetting(&macp->card_setting);
	zd_CmdProcess(CMD_ENABLE, &dot11Obj, 0);  //AP start send beacon
	writel(0x1, macp->regp+LED1);
//	zd1205_enable_int();
//	zd1205_dump_regs(macp);
 	goto exit;


err_exit:
	zd1205_clear_pools(macp);
	
exit:
	zd1205_enable_int();
	read_unlock(&(macp->isolate_lock));
	ZEXIT();
	return rc;

}


void zd1205_init_txq(struct zd1205_private *macp, sw_tcbq_t *Q)
{
    unsigned long flags;
    spin_lock_irqsave(&macp->q_lock, flags);
 	Q->first = NULL;
	Q->last = NULL;
	Q->count = 0;
	spin_unlock_irqrestore(&macp->q_lock, flags);
}


void zd1205_qlast_txq(struct zd1205_private *macp, sw_tcbq_t *Q, sw_tcb_t *signal)				
{		
	unsigned long flags;
    spin_lock_irqsave(&macp->q_lock, flags);
    
 	signal->next = NULL;	
	if (Q->last == NULL){			
		Q->first = signal;				
		Q->last = signal;					
	}									
	else{								
		Q->last->next = signal;	
 		Q->last = signal;				
	}									
	Q->count++;							
	spin_unlock_irqrestore(&macp->q_lock, flags);			
}


sw_tcb_t * zd1205_first_txq(struct zd1205_private *macp, sw_tcbq_t *Q)
{
    sw_tcb_t *p = NULL;
	unsigned long flags;
    
    spin_lock_irqsave(&macp->q_lock, flags);
	if (Q->first != NULL){
		Q->count--;
		p = Q->first;
		Q->first = (Q->first)->next;
		if (Q->first == NULL)
			Q->last = NULL;
	}
	spin_unlock_irqrestore(&macp->q_lock, flags);
	return p;
}


static void
zd1205_setup_tcb_pool(struct zd1205_private *macp)
{
	/* TCB local variables */
    sw_tcb_t  	*sw_tcb;         /* cached TCB list logical pointers */
    hw_tcb_t	*hw_tcb;         /* uncached TCB list logical pointers */
    u32		 	HwTcbPhys;       /* uncached TCB list physical pointer */
    u32       	tcb_count;
    /* TBD local variables */

    tbd_t  		*pHwTbd;         /* uncached TBD list pointers */
    u32		 	HwTbdPhys;          /* uncached TBD list physical pointer */
	ctrl_set_t	*hw_ctrl;
	u32			hw_ctrl_phys;
	header_t	*hw_header;
	u32			hw_header_phys;

	ZENTER();
	macp->freeTxQ = &free_txq_buf;
	macp->activeTxQ = &active_txq_buf;
	zd1205_init_txq(macp, macp->freeTxQ);
	zd1205_init_txq(macp, macp->activeTxQ);
 
#if 1
    /* print some basic sizing debug info */
	printk(KERN_DEBUG "sizeof(SwTcb) = %04x\n", sizeof(sw_tcb_t));
    printk(KERN_DEBUG "sizeof(HwTcb) = %04x\n", sizeof(hw_tcb_t));
    printk(KERN_DEBUG "sizeof(HwTbd)= %04x\n", sizeof(tbd_t));
    printk(KERN_DEBUG "sizeof(CTRL_STRUC) = %04x\n", sizeof(ctrl_set_t));
    printk(KERN_DEBUG "sizeof(HEADER_STRUC) = %04x\n", sizeof(header_t));
    printk(KERN_DEBUG "macp->num_tcb = %04x\n", macp->num_tcb);
    printk(KERN_DEBUG "macp->num_tbd_per_tcb = %04x\n", macp->num_tbd_per_tcb);
    printk(KERN_DEBUG "macp->num_tbd = %04x\n", macp->num_tbd);
#endif   
    
    /* Setup the initial pointers to the HW and SW TCB data space */
    sw_tcb = (sw_tcb_t *) macp->tx_cached;
    hw_tcb = (hw_tcb_t *) macp->tx_uncached;
    HwTcbPhys = macp->tx_uncached_phys;	

    /* Setup the initial pointers to the TBD data space.
      TBDs are located immediately following the TCBs */
    pHwTbd = (tbd_t *) (macp->tx_uncached + (sizeof(hw_tcb_t) * macp->num_tcb));
    HwTbdPhys = HwTcbPhys + (sizeof(hw_tcb_t) * macp->num_tcb);

    /* Setup yhe initial pointers to the Control Setting space
	 CTRLs are located immediately following the TBDs */
	hw_ctrl = (ctrl_set_t *) ((u32)pHwTbd + (sizeof(tbd_t) * macp->num_tbd));
	hw_ctrl_phys = HwTbdPhys + (sizeof(tbd_t) * macp->num_tbd);

	/* Setup the initial pointers to the Mac Header space
	 MACHEADERs are located immediately following the CTRLs */
 	hw_header = (header_t *) ((u32)hw_ctrl + (sizeof(ctrl_set_t) * macp->num_tcb));
	hw_header_phys = hw_ctrl_phys + (sizeof(ctrl_set_t) * macp->num_tcb);
		
	/* Go through and set up each TCB */
    for (tcb_count = 0; tcb_count < macp->num_tcb;
    	tcb_count++, sw_tcb++, hw_tcb++, HwTcbPhys += sizeof(hw_tcb_t),
        pHwTbd = (tbd_t *) (((u8 *) pHwTbd) + ((sizeof(tbd_t) * macp->num_tbd_per_tcb))),
        HwTbdPhys += (sizeof(tbd_t) * macp->num_tbd_per_tcb),
		hw_ctrl++, hw_ctrl_phys += sizeof(ctrl_set_t),
		hw_header++, hw_header_phys += sizeof(header_t)){
            /* point the cached TCB to the logical address of the uncached one */
		    sw_tcb->tcb_count = tcb_count;
  		    sw_tcb->skb = 0;
 		    sw_tcb->tcb = hw_tcb;
       	    sw_tcb->tcb_phys = HwTcbPhys;
            sw_tcb->first_tbd = pHwTbd;
            sw_tcb->first_tbd_phys = HwTbdPhys;
 		    sw_tcb->hw_ctrl = hw_ctrl;
		    sw_tcb->hw_ctrl_phys = hw_ctrl_phys;
		#if 0		    
		    // Pre-init control setting
			{
				ctrl_set_t	*ctrl_set = sw_tcb->hw_ctrl;
			
				ctrl_set->ctrl_setting[3] = (u8)(sw_tcb->tcb_phys);
				ctrl_set->ctrl_setting[4] = (u8)(sw_tcb->tcb_phys >> 8);
				ctrl_set->ctrl_setting[5] = (u8)(sw_tcb->tcb_phys >> 16);
				ctrl_set->ctrl_setting[6] = (u8)(sw_tcb->tcb_phys >> 24);
			
				ctrl_set->ctrl_setting[18] = 0; //default for fragment
				ctrl_set->ctrl_setting[19] = 0;
				ctrl_set->ctrl_setting[23] = 0; //default for fragment
				ctrl_set->ctrl_setting[24] = 0;
			}
		#endif		

            sw_tcb->hw_header = hw_header;
		    sw_tcb->hw_header_phys = hw_header_phys;
         
            /* initialize the uncached TCB contents -- status is zeroed */
            hw_tcb->CbStatus = 0xffffffff;
            hw_tcb->CbCommand = cpu_to_le32(CB_S_BIT); 
            hw_tcb->TxCbFirstTbdAddrLowPart = cpu_to_le32(HwTbdPhys);
		    hw_tcb->TxCbFirstTbdAddrHighPart = 0;
		    hw_tcb->TxCbTbdNumber = 0;
            if (tcb_count == (macp->num_tcb -1)){
			    /* Turn around TBD */
			    hw_tcb->NextCbPhyAddrLowPart =  cpu_to_le32(macp->tx_uncached_phys);
			    hw_tcb->NextCbPhyAddrHighPart = 0;
		    }   
            else{
                hw_tcb->NextCbPhyAddrLowPart = cpu_to_le32(HwTcbPhys + sizeof(hw_tcb_t));
			    hw_tcb->NextCbPhyAddrHighPart = 0;
		    }
		
            /* add this TCB to the free list */	
    	    zd1205_qlast_txq(macp, macp->freeTxQ, sw_tcb);
    }	
    ZEXIT();

	return;
} 


/**
 * zd1205_get_stats - get driver statistics
 * @dev: adapter's net_device struct
 *
 * This routine is called when the OS wants the adapter's stats returned.
 * It returns the address of the net_device_stats stucture for the device.
 * If the statistics are currently being updated, then they might be incorrect
 * for a short while. However, since this cannot actually cause damage, no
 * locking is used.
 */

struct net_device_stats *

zd1205_get_stats(struct net_device *dev)
{
	struct zd1205_private *macp = dev->priv;

	macp->drv_stats.net_stats.tx_errors =
		macp->drv_stats.net_stats.tx_carrier_errors +
 		macp->drv_stats.net_stats.tx_aborted_errors;

	macp->drv_stats.net_stats.rx_errors =
		macp->drv_stats.net_stats.rx_crc_errors +
		macp->drv_stats.net_stats.rx_frame_errors +
		macp->drv_stats.net_stats.rx_length_errors +
		macp->drv_stats.rcv_cdt_frames;

	return &(macp->drv_stats.net_stats);
}


/**
 * zd1205_set_mac - set the MAC address
 * @dev: adapter's net_device struct
 * @addr: the new address
 *
 * This routine sets the ethernet address of the board

 * Returns:
 * 0  - if successful
 * -1 - otherwise
 */


static int
zd1205_set_mac(struct net_device *dev, void *addr)
{
	struct zd1205_private *macp;
	int rc = -1;
	struct sockaddr *p_sockaddr = (struct sockaddr *) addr;

	ZENTER();	
	macp = dev->priv;
	read_lock(&(macp->isolate_lock));

	if (macp->driver_isolated) {
		goto exit;
	}

	{
		memcpy(&(dev->dev_addr[0]), p_sockaddr->sa_data, ETH_ALEN);
		rc = 0;
	}

exit:
	read_unlock(&(macp->isolate_lock));
    ZEXIT();
	return rc;
}



void
zd1205_isolate_driver(struct zd1205_private *macp)
{
	write_lock_irq(&(macp->isolate_lock));
	macp->driver_isolated = true;
	write_unlock_irq(&(macp->isolate_lock));
	del_timer_sync(&macp->watchdog_timer);
    del_timer_sync(&macp->tm_hking_id);

    if (netif_running(macp->device))
		netif_stop_queue(macp->device);
}


static int
zd1205_change_mtu(struct net_device *dev, int new_mtu)
{
	ZENTER();
	if ((new_mtu < 68) || (new_mtu > (ETH_DATA_LEN + VLAN_SIZE)))
		return -EINVAL;
 
	dev->mtu = new_mtu;
	return 0;
}


static int
zd1205_close(struct net_device *dev)
{
	struct zd1205_private *macp = dev->priv;
	void *regp = macp->regp;

	ZENTER();
	writel(0x0, regp+LED1);
	writel(0x0, regp+LED2);
	writel(0x0, regp+BCNInterval);
	macp->intr_mask = 0;
	//writel(0x01, regp+Pre_TBTT);
//	zd1205_sleep_reset(macp);
	zd1205_device_reset(macp);
	zd1205_isolate_driver(macp);

	netif_carrier_off(macp->device);
	free_irq(dev->irq, dev);

 	zd1205_clear_pools(macp);

	/* set the isolate flag to false, so e100_open can be called */

	macp->driver_isolated = false;

	ZEXIT();
	return 0;
}


u8 CalNumOfFrag(struct zd1205_private *macp, u32 length)

{
	u8 FragNum = 1;
	u32 pdusize;

	pdusize = macp->card_setting.FragThreshold;
	
	if ((length + CRC32_LEN) > pdusize){ //Need fragment
		pdusize -= WLAN_HEADER + CRC32_LEN;
		FragNum = ((length - WLAN_HEADER)+ (pdusize-1)) / pdusize;
		if (FragNum == 0) 
			FragNum = 1;
	}

	return FragNum;
}


static int
zd1205_xmit_frame(struct sk_buff *skb, struct net_device *dev)
{

	int rc = 0;
	int notify_stop = false;
	struct zd1205_private *macp = dev->priv;
    u16 TypeLen;
	u8 *pHdr = skb->data;
	u32 bodyLen;
	u32 TotalLen;
	u8 *pBody;
	u8 NumOfFrag = 1;
	u8 EtherHdr[12];
	u8 bEapol = 0;
	void *pHash = NULL;

    if (macp->dbg_flag > 0)
	    ZENTER();
 		
	read_lock(&(macp->isolate_lock));

	if (macp->driver_isolated) {
		rc = -EBUSY;
        printk(KERN_DEBUG "zd1205: macp->driver_isolated\n");
		goto exit2;
	}

    if (!spin_trylock(&macp->bd_non_tx_lock)){
        notify_stop = true;
        rc = 1;
        goto exit2;  
    }

  
    if (!(pHdr[0] & BIT_0)) { //da is unicast
    	if (!zd_QueryStaTable(pHdr, &pHash)){
            dev_kfree_skb_irq(skb);
            rc = 0;
            goto exit1;
        }
    }
    
	memcpy(&EtherHdr[0], pHdr, 12); //save ethernet header
    TypeLen = (((u16) pHdr[12]) << 8) + (pHdr[13]);
	if (TypeLen > 1500){	/* Ethernet 2 frame */
		/* DA(6) SA(6) Type(2) Data....(reserved array) */
		if ((TypeLen == 0x8137) || (TypeLen == 0x80F3)) 
			memcpy(pHdr+6, ZD_SNAP_BRIDGE_TUNNEL, sizeof(ZD_SNAP_BRIDGE_TUNNEL));
		else
			memcpy(pHdr+6, (void *)ZD_SNAP_HEADER, sizeof(ZD_SNAP_HEADER));
		
		if (TypeLen == 0x888e)
			bEapol = 1;

        skb->len -= 6;  /* Minus DA, SA; Plus 802.2LLC Header */      		
		bodyLen = skb->len;	
        skb->data += 6;
	}
	else{	/* 802.3 frame */
		/* DA(6) SA(6) Len(2) 802.2_LLC(3) 802.2_SNAP(3+2) Data.... */
        skb->len -= 14;
		bodyLen = TypeLen;
        skb->data += 14;
	}
    pBody = skb->data;	
	
	TotalLen = bodyLen + WLAN_HEADER; //Mac Header(24)
	NumOfFrag = CalNumOfFrag(macp, TotalLen);
	
    if (macp->freeTxQ->count < (NumOfFrag+1)){
        printk(KERN_DEBUG "********Queue to upper layer************\n");
        notify_stop = true;
        rc = 1;
        goto exit1;
    }

     if (!zd_SendPkt(EtherHdr, pBody, bodyLen, (void *)skb, bEapol, pHash)){
        notify_stop = true;
        rc = 1;
        goto exit1;
    }
 
	macp->drv_stats.net_stats.tx_bytes += skb->len;
    macp->drv_stats.net_stats.tx_packets++;

exit1:
    spin_unlock(&macp->bd_non_tx_lock);
    
exit2:
    read_unlock(&(macp->isolate_lock));
	if (notify_stop) {
		netif_stop_queue(dev);
	}
    
    if (macp->dbg_flag > 0)
        ZEXIT();
        
	return rc;
}
      

void zd1205_sw_release(void)
{
    zd_EventNotify(EVENT_BUF_RELEASE, 0, 0, 0);
}    


void zd1205_sleep_reset(struct zd1205_private *macp)
{
	void *regp = (void *)macp->regp;
	u32 tmp_value;
	u32 flags = 0;

	spin_lock_irqsave(&macp->q_lock, flags);
	
	macp->bDeviceInSleep = 1;
	writel(0x00, regp+PCI_RxAddr_p1);
	writel(0x00, regp+PCI_TxAddr_p1);
	HW_RadioOnOff(&dot11Obj, 0);
	writel(0x01, regp+Pre_TBTT);
	tmp_value = readl(regp+PS_Ctrl);
	writel(tmp_value|BIT_0, regp+PS_Ctrl);
	macp->TxStartTime = 0;
	mdelay(10);
	macp->TxStartTime = 0;
	spin_unlock_irqrestore(&macp->q_lock, flags);
	//HW_ResetPhy(&dot11Obj);
	//HW_Set_GCT_Chips(&dot11Obj, macp->card_setting.Channel, 0);
}  

void update_beacon_interval(struct zd1205_private *macp, int val)
{
	int BcnInterval;
	int ul_PreTBTT;
	int tmpvalue;
	void *regp = (void *)macp->regp;

	BcnInterval = val;
	/* One thing must be sure that BcnInterval > Pre_TBTT > ATIMWnd >= 0 */
	if(BcnInterval < 5) {
		BcnInterval = 5;
	}
                                    
	ul_PreTBTT = readl(regp + Pre_TBTT);
                                        
	if(ul_PreTBTT < 4) {
		ul_PreTBTT = 4;
	}
                                                        
	if(ul_PreTBTT >= BcnInterval) {
		ul_PreTBTT = BcnInterval - 1;
	}

	writel(ul_PreTBTT, regp + Pre_TBTT);
    
	tmpvalue = readl(regp + BCNInterval);
	tmpvalue &= ~0xffffffff;
	tmpvalue |= BcnInterval;
                
	writel(tmpvalue, regp + BCNInterval);
}

void zd1205_device_reset(struct zd1205_private *macp)
{
	u32  tmp_value;
	void *regp = macp->regp;
        
	/* Update the value of Beacon Interval and Pre TBTT */
	update_beacon_interval(macp, 0x2);
	writel(0x01, regp + Pre_TBTT);	
                    
	tmp_value = readl(regp + PS_Ctrl);
	writel(tmp_value | BIT_0, regp + PS_Ctrl);

	/* Sleep for 5 msec */
	mdelay(5);
}

void zd1205_recycle_tx(struct zd1205_private *macp)
{
	sw_tcb_t *sw_tcb;
	
	while (macp->activeTxQ->count){
		sw_tcb = macp->activeTxQ->first;
		zd1205_transmit_cleanup(macp, sw_tcb);
		macp->txCmpCnt++;
		
		if (!sw_tcb->last_frag)
            continue;
        
		zd_EventNotify(EVENT_TX_COMPLETE, ZD_RETRY_FAILED, (U32)sw_tcb->msg_id, 0);
	}
}	


void zd1205_process_wakeup(struct zd1205_private *macp)
{
	static u8 SleepResetCnt = 0;
	
    HW_EnableBeacon(&dot11Obj, macp->card_setting.BeaconInterval, macp->card_setting.DtimPeriod);
	HW_SwitchChannel(&dot11Obj, macp->card_setting.Channel);
	HW_RadioOnOff(&dot11Obj, 1);
	
	macp->bDeviceInSleep = 0;
    zd1205_recycle_tx(macp);
    zd1205_start_ru(macp);
    
    SleepResetCnt++;
	if (SleepResetCnt < 2)
		macp->ResetFlag = 1;
	else{
		SleepResetCnt = 0;
		macp->ResetFlag = 0;
	}		
    FPRINT("*********Reset device completed!!!");
}  

void zd1205_sw_reset(struct zd1205_private *macp)
{
    zd1205_disable_int();
    zd1205_tx_isr(macp);
    memset(macp->tx_uncached, 0x00, macp->tx_uncached_size);

    zd1205_setup_tcb_pool(macp);
    zd1205_sleep_reset(macp);
    zd1205_start_ru(macp);
    zd_EventNotify(EVENT_SW_RESET, 0, 0, 0);
    zd1205_enable_int();
    
    if(netif_running(macp->device))
        netif_wake_queue(macp->device);
}  

   
/**
 * zd1205_sw_init - initialize software structs
 * @macp: atapter's private data struct
 * 
 * This routine initializes all software structures. Sets up the
 * circular structures for the RFD's & TCB's. Allocates the per board
 * structure for storing adapter information. The CSR is also memory 
 * mapped in this routine.
 *
 * Returns :
 *      true: if S/W was successfully initialized
 *      false: otherwise
 */
static unsigned char __devinit
zd1205_sw_init(struct zd1205_private *macp)

{
	macp->dbg_flag = 0;
	zd1205_init_card_setting(macp);
	zd1205_ConvertDbToSetPoint(macp);
	zd1205_set_zd_cbs(&dot11Obj);
	zd_CmdProcess(CMD_RESET_80211, &dot11Obj, 0);
    
	/* Initialize our spinlocks */
	spin_lock_init(&(macp->bd_lock));
	spin_lock_init(&(macp->bd_non_tx_lock));
	spin_lock_init(&(macp->q_lock));
	spin_lock_init(&(macp->conf_lock));

	tasklet_init(&macp->zd1205_tasklet, zd1205_action, 0);
	tasklet_init(&macp->zd1205_ps_tasklet, zd1205_ps_action, 0);
	tasklet_init(&macp->zd1205_tx_tasklet, zd1205_tx_action, 0);

	macp->isolate_lock = RW_LOCK_UNLOCKED;
	macp->driver_isolated = false;

	return 1;
}


/**
 * zd1205_hw_init - initialized tthe hardware
 * @macp: atapter's private data struct
 * @reset_cmd: s/w reset or selective reset
 *
 * This routine performs a reset on the adapter, and configures the adapter.
 * This includes configuring the 82557 LAN controller, validating and setting
 * the node address, detecting and configuring the Phy chip on the adapter,
 * and initializing all of the on chip counters.
 *
 * Returns:
 *      true - If the adapter was initialized
 *      false - If the adapter failed initialization
 */
unsigned char __devinit
zd1205_hw_init(struct zd1205_private *macp)

{
	HW_ResetPhy(&dot11Obj);
    HW_InitHMAC(&dot11Obj);
	zd1205_config(macp);
	
	return true;

}


static void zd1205_set_multi(struct net_device *dev)
{
}	



#define  TX_TIMEOUT     (4*1000*1000) //4sec
/**
 * zd1205_watchdog
 * @dev: adapter's net_device struct
 *
 * This routine runs every 1 seconds and updates our statitics and link state,
 * and refreshs txthld value.
 */
void
zd1205_watchdog(struct net_device *dev)
{
	struct zd1205_private *macp = dev->priv;
    void *regp = macp->regp;
    int i;
	u32 diffTime;

    read_lock(&(macp->isolate_lock));

	if (macp->driver_isolated) {
		goto exit;
	}

	if (!netif_running(dev)) {
		goto exit;
	}
    
    // toggle the tx queue according to link status
	// this also resolves a race condition between tx & non-cu cmd flows
	if (netif_carrier_ok(dev)) {
		if (netif_running(dev))
			netif_wake_queue(dev);
	} else {
		netif_stop_queue(dev);
	}

	rmb();

	zd_PerSecTimer();

	if (macp->TxStartTime){
		if (nowT() > macp->TxStartTime)
			diffTime = nowT() - macp->TxStartTime;
		else
			diffTime = (0xffffffff - macp->TxStartTime) + nowT();
			
		if (diffTime > TX_TIMEOUT){ //device maybe hang
			//FPRINT("Tx Timeout !!!!!!!!!!!!");
			//tmpvalue = readl(regp+DeviceState);
			//FPRINT_V("Device State ", tmpvalue);
			macp->HMAC_TxTimeout++;
			zd1205_sleep_reset(macp);
		}	
	}
		


    for (i=0; i<macp->txOn; i++)
        zd1205_tx_test(macp, 100);
    
    mod_timer(&(macp->watchdog_timer), jiffies+(1*HZ));


exit:
	read_unlock(&(macp->isolate_lock));    
}


/**
 * zd1205_pci_setup - setup the adapter's PCI information
 * @pcid: adapter's pci_dev struct
 * @macp: atapter's private data struct
 *
 * This routine sets up all PCI information for the adapter. It enables the bus
 * master bit (some BIOS don't do this), requests memory ans I/O regions, and
 * calls ioremap() on the adapter's memory region.
 *
 * Returns:
 *      true: if successfull
 *      false: otherwise
 */
static unsigned char __devinit
zd1205_pci_setup(struct pci_dev *pcid, struct zd1205_private *macp)
{
	struct net_device *dev = macp->device;
	int rc = 0;

	ZENTER();
	if ((rc = pci_enable_device(pcid)) != 0) {
       	goto err;
	}

    if (!pci_set_dma_mask(pcid, 0xffffffffffffffff)){
		macp->using_dac = 1;
		printk(KERN_DEBUG "zd1205: support 64-bit DMA.\n");
	}

	else if (!pci_set_dma_mask(pcid, 0xffffffff)){
		macp->using_dac = 0;
  		printk(KERN_DEBUG "zd1205: support 32-bit DMA.\n");
	}
	else{
		printk(KERN_ERR "zd1205: No suitable DMA available.\n");
		goto err;
	} 
	
	/* dev and ven ID have already been checked so it is our device */
	pci_read_config_byte(pcid, PCI_REVISION_ID, (u8 *) &(macp->rev_id));

	/* address #0 is a memory region */
 	dev->mem_start = pci_resource_start(pcid, 0);
	dev->mem_end = dev->mem_start + ZD1205_REGS_SIZE;

	/* address #1 is a IO region */
	dev->base_addr = pci_resource_start(pcid, 1);
	if ((rc = pci_request_regions(pcid, zd1205_short_driver_name)) != 0) {
		goto err_disable;
	}

	pci_enable_wake(pcid, 0, 0);

	/* if Bus Mastering is off, turn it on! */
	pci_set_master(pcid);

	/* address #0 is a memory mapping */
	macp->regp = (void *)ioremap_nocache(dev->mem_start, ZD1205_REGS_SIZE);
    dot11Obj.reg = macp->regp;
    //printk(KERN_DEBUG "zd1205: dot11Obj.reg = %x\n", (u32)dot11Obj.reg);


	if (!macp->regp) {
		printk(KERN_ERR "zd1205: %s: Failed to map PCI address 0x%lX\n",
		       dev->name, pci_resource_start(pcid, 0));
		rc = -ENOMEM;
		goto err_region;
	}


	else
		printk(KERN_DEBUG "zd1205: mapping base addr = %x\n", (u32)macp->regp);

    ZEXIT();
	return 0;

err_region:
	pci_release_regions(pcid);
	
err_disable:
	pci_disable_device(pcid);
	
err:
	return rc;
}


/**
 * zd1205_alloc_space - allocate private driver data
 * @macp: atapter's private data struct
 *
 * This routine allocates memory for the driver. Memory allocated is for the
 * selftest and statistics structures.
 *
 * Returns:
 *      0: if the operation was successful
 *      %-ENOMEM: if memory allocation failed
 */


unsigned char
zd1205_alloc_space(struct zd1205_private *macp)
{
	ZENTER();	
	/* deal with Tx cached memory */
	macp->tx_cached_size = (macp->num_tcb * sizeof(sw_tcb_t)); 
	macp->tx_cached = kmalloc(macp->tx_cached_size, GFP_ATOMIC);
	if (!macp->tx_cached){
		printk(KERN_ERR "zd1205: kmalloc tx_cached failed\n");
		return 1;
	}
	else{
		memset(macp->tx_cached, 0, macp->tx_cached_size);
		ZEXIT();
		return 0;
	}   
}


static void
zd1205_dealloc_space(struct zd1205_private *macp)
{
	ZENTER();
	if (macp->tx_cached)
		kfree(macp->tx_cached);
	ZEXIT();   
}


/* Read the permanent ethernet address from the eprom. */
void __devinit
zd1205_rd_eaddr(struct zd1205_private *macp)
{

	ZENTER();

	macp->device->dev_addr[0] =	macp->mac_addr[0] = 0x00;
	macp->device->dev_addr[1] =	macp->mac_addr[1] = 0xa0;
	macp->device->dev_addr[2] =	macp->mac_addr[2] = 0xc5;
	macp->device->dev_addr[3] =	macp->mac_addr[3] = 0x11;
	macp->device->dev_addr[4] =	macp->mac_addr[4] = 0x22;
	macp->device->dev_addr[5] =	macp->mac_addr[5] = 0x33;
//printk(KERN_DEBUG "zd1205: MAC address = %2x:%2x:%2x:%2x:%2x:%2x\n", 
	//macp->device->dev_addr[0], macp->device->dev_addr[1], macp->device->dev_addr[2]
	//macp->device->dev_addr[3], macp->device->dev_addr[4], macp->device->dev_addr[5]);

    macp->card_setting.MacAddr[0] = macp->mac_addr[0];
	macp->card_setting.MacAddr[1] = macp->mac_addr[1];
	macp->card_setting.MacAddr[2] = macp->mac_addr[2];
	macp->card_setting.MacAddr[3] = macp->mac_addr[3];
	macp->card_setting.MacAddr[4] = macp->mac_addr[4];
	macp->card_setting.MacAddr[5] = macp->mac_addr[5];
    
    ZEXIT();
}


inline void
zd1205_lock(struct zd1205_private *macp)
{
    spin_lock_bh(&macp->conf_lock);
}


inline void
zd1205_unlock(struct zd1205_private *macp)
{
    spin_unlock_bh(&macp->conf_lock);
}


//wireless extension helper functions    
/* taken from orinoco.c ;-) */
const long channel_frequency[] = {
	2412, 2417, 2422, 2427, 2432, 2437, 2442,
	2447, 2452, 2457, 2462, 2467, 2472, 2484
};

#define NUM_CHANNELS ( sizeof(channel_frequency) / sizeof(channel_frequency[0]) )
                  
                  
static int
zd1205_ioctl_setiwencode(struct net_device *dev, struct iw_point *erq)
{
	return 0;
}

    
static int

zd1205_ioctl_getiwencode(struct net_device *dev, struct iw_point *erq)

{
    return 0;
}


static int
zd1205_ioctl_setessid(struct net_device *dev, struct iw_point *erq)
{
	struct zd1205_private *macp = dev->priv;
	char essidbuf[IW_ESSID_MAX_SIZE+1];

	memset(&essidbuf, 0, sizeof(essidbuf));

	if (erq->flags) {
		if (erq->length > IW_ESSID_MAX_SIZE)
			return -E2BIG;
		
		if (copy_from_user(&essidbuf, erq->pointer, erq->length))
			return -EFAULT;
	}

	zd1205_lock(macp);
	memcpy(&macp->card_setting.Info_SSID[2], essidbuf, erq->length-1);
	macp->card_setting.Info_SSID[1] = erq->length-1;
	zd1205_unlock(macp);

	zd_UpdateCardSetting(&macp->card_setting);
    return 0;

}
  
static int
zd1205_ioctl_getessid(struct net_device *dev, struct iw_point *erq)
{
    struct zd1205_private *macp = dev->priv;
	char essidbuf[IW_ESSID_MAX_SIZE+1];

	ZENTER();
	zd1205_lock(macp);
	memcpy(essidbuf, &macp->card_setting.Info_SSID[2], sizeof(essidbuf));
	zd1205_unlock(macp);

	erq->flags = 1;
	erq->length = strlen(essidbuf) + 1;
	if (erq->pointer)
		if ( copy_to_user(erq->pointer, essidbuf, erq->length) )
			return -EFAULT;

	ZEXIT();
	return 0;
}

        
static int
zd1205_ioctl_setfreq(struct net_device *dev, struct iw_freq *frq)
{
	struct zd1205_private *macp = dev->priv;
	int chan = -1;
	
	if ( (frq->e == 0) && (frq->m <= 1000) ) {
		/* Setting by channel number */
		chan = frq->m;
	} else {
		/* Setting by frequency - search the table */
		int mult = 1;
		int i;

		for (i = 0; i < (6 - frq->e); i++)
			mult *= 10;

		for (i = 0; i < NUM_CHANNELS; i++)
			if (frq->m == (channel_frequency[i] * mult))
				chan = i+1;
	}

	if ( (chan < 1) || (chan > NUM_CHANNELS) )
		return -EINVAL;

	zd1205_lock(macp);
	macp->card_setting.Channel = chan;
	zd1205_unlock(macp);

	zd_UpdateCardSetting(&macp->card_setting);
    return 0;  
}


static int
zd1205_ioctl_getsens(struct net_device *dev, struct iw_param *srq)
{
    return 0;
}


//static int
//zd1205_ioctl_setsens(struct net_device *dev, struct iw_param *srq)
//{
//    return 0;
//}

    
static int
zd1205_ioctl_setrts(struct net_device *dev, struct iw_param *rrq)
{
	struct zd1205_private *macp = dev->priv;
	int val = rrq->value;

	if (rrq->disabled)
		val = 2347;

	if ( (val < 0) || (val > 2347) )

		return -EINVAL;

	zd1205_lock(macp);
	macp->card_setting.RTSThreshold = val;
	zd1205_unlock(macp);
	zd_UpdateCardSetting(&macp->card_setting);
	
	return 0;
}

    
static int
zd1205_ioctl_setfrag(struct net_device *dev, struct iw_param *frq)
{
	struct zd1205_private *macp = dev->priv;
	int err = 0;

	zd1205_lock(macp);

	if (frq->disabled){
		macp->card_setting.FragThreshold = 2346;
	}	
	else {
		if ( (frq->value < 256) || (frq->value > 2346) ){
			err = -EINVAL;
		}	
		else {
			macp->card_setting.FragThreshold= frq->value & ~0x1; /* must be even */
		}
	}

	zd1205_unlock(macp);
	zd_UpdateCardSetting(&macp->card_setting);

	return err;
}

    
static int
zd1205_ioctl_getfrag(struct net_device *dev, struct iw_param *frq)
{
	struct zd1205_private *macp = dev->priv;
	u16 val;

	zd1205_lock(macp);
	val = macp->card_setting.FragThreshold;
	frq->value = val;

	frq->disabled = (val > 2432);
	frq->fixed = 1;
	zd1205_unlock(macp);

	return 0;
}

  
static int
zd1205_ioctl_setrate(struct net_device *dev, struct iw_param *frq)
{
     return 0;
}

    
static int
zd1205_ioctl_getrate(struct net_device *dev, struct iw_param *frq)
{
	struct zd1205_private *macp = dev->priv;
        
	frq->fixed = 0;
	frq->disabled = 0;
	frq->value = 0;
                                
#define BIT_1M          0x00
#define BIT_2M          0x01
#define BIT_5dot5M      0x02
#define BIT_11M         0x03
                                
	switch(macp->card_setting.CurrTxRate)
	{
		case BIT_1M:
				frq->value = 1000000;
				break;
		case BIT_2M:
				frq->value = 2000000;
				break;
                case BIT_5dot5M:
				frq->value = 5500000;
				break;
		case BIT_11M:
				frq->value = 11000000;
				break;
		
		default:
				return -EINVAL;
	}
                                                                                                                                                                                                                        
	return 0;
}


static int
zd1205_ioctl_setpower(struct net_device *dev, struct iw_param *prq)
{
	struct zd1205_private *macp = dev->priv;
	int ret = 0;
                
#define TX_17dbm        0x00
#define TX_14dbm        0x01
#define TX_11dbm        0x02
                
	if(prq->value >= TX_17dbm && prq->value <= TX_11dbm)
		macp->card_setting.TxPowerLevel = prq->value;
	else
		ret = -EINVAL;
                                                                
	return ret;
}
  
static int
zd1205_ioctl_getpower(struct net_device *dev, struct iw_param *prq)
{
	struct zd1205_private *macp = dev->priv;
        
#define TX_17dbm        0x00
#define TX_14dbm        0x01
#define TX_11dbm        0x02
        
	prq->flags = 0;
	prq->disabled = 0;
	prq->fixed = 0;
                                
	switch(macp->card_setting.TxPowerLevel)
	{
		case TX_17dbm:
				prq->value = 17;
				break;

		case TX_14dbm:
				prq->value = 14;
				break;

                case TX_11dbm:
				prq->value = 11;
				break;

		default:
				return -EINVAL;
	}
	
	return 0;
}

static long
zd1205_hw_get_freq(struct zd1205_private *macp)
{
	long freq = 0;


	zd1205_lock(macp);
	freq = channel_frequency[macp->card_setting.Channel-1] * 100000;
	zd1205_unlock(macp);

	return freq;
}  


static int
zd1205_ioctl_getretry(struct net_device *dev, struct iw_param *prq)
{
	return 0;  
}             

/* For WIRELESS_EXT > 12 */
static int zd1205wext_giwname(struct net_device *dev, struct iw_request_info *info, char *name, char *extra)
{
	strcpy(name, "IEEE 802.11-DS");
	return 0;
}

static int zd1205wext_giwfreq(struct net_device *dev, struct iw_request_info *info, struct iw_freq *freq, char *extra)
{
	struct zd1205_private *macp;
	macp = dev->priv;
                
	freq->m = zd1205_hw_get_freq(macp);
	freq->e = 1;
	return 0;
}

static int zd1205wext_siwfreq(struct net_device *dev, struct iw_request_info *info, struct iw_freq *freq, char *extra)
{
	int err;
	err = zd1205_ioctl_setfreq(dev, freq);
	return err;
}

static int zd1205wext_giwmode(struct net_device *dev, struct iw_request_info *info, __u32 *mode, char *extra)
{
	*mode = IW_MODE_MASTER;
	return 0;
}

static int zd1205wext_siwrate(struct net_device *dev, struct iw_request_info *info, struct iw_param *rrq, char *extra)
{
	return zd1205_ioctl_setrate(dev, rrq);
}

static int zd1205wext_giwrate(struct net_device *dev, struct iw_request_info *info, struct iw_param *rrq, char *extra)
{
	return zd1205_ioctl_getrate(dev, rrq);
}

static int zd1205wext_giwrts(struct net_device *dev, struct iw_request_info *info, struct iw_param *rts, char *extra)
{
	struct zd1205_private *macp;
	macp = dev->priv;
                
	rts->value = macp->card_setting.RTSThreshold;
	rts->disabled = (rts->value > 2432);
	rts->fixed = 1;
	return 0;
}

static int zd1205wext_siwrts(struct net_device *dev, struct iw_request_info *info, struct iw_param *rts, char *extra)
{
	return zd1205_ioctl_setrts(dev, rts);
}

static int zd1205wext_giwfrag(struct net_device *dev, struct iw_request_info *info, struct iw_param *frag, char *extra)
{
	return zd1205_ioctl_getfrag(dev, frag);
}

static int zd1205wext_siwfrag(struct net_device *dev, struct iw_request_info *info, struct iw_param *frag, char *extra)
{
	return zd1205_ioctl_setfrag(dev, frag);
}

static int zd1205wext_giwtxpow(struct net_device *dev, struct iw_request_info *info, struct iw_param *rrq, char *extra)
{
	return zd1205_ioctl_getpower(dev, rrq);
}

static int zd1205wext_siwtxpow(struct net_device *dev, struct iw_request_info *info, struct iw_param *rrq, char *extra)
{
	return zd1205_ioctl_setpower(dev, rrq);
}

static int zd1205wext_giwap(struct net_device *dev, struct iw_request_info *info, struct sockaddr *ap_addr, char *extra)
{
	struct zd1205_private *macp;
	macp = dev->priv;
                
	ap_addr->sa_family = ARPHRD_ETHER;
	memcpy(ap_addr->sa_data, macp->mac_addr, 6);
	return 0;
}

#if 0
static int zd1205wext_siwencode(struct net_device *dev, struct iw_request_info *info, struct iw_point *erq, char *key)
{
	return zd1205_ioctl_setiwencode(dev, erq);
}

static int zd1205wext_giwencode(struct net_device *dev, struct iw_request_info *info, struct iw_point *erq, char *key)
{
	return zd1205_ioctl_getiwencode(dev, erq);
}
#endif

static int zd1205wext_giwrange(struct net_device *dev, struct iw_request_info *info, struct iw_point *data, char *extra)
{
        struct iw_range *range = (struct iw_range *) extra;
	int i, val;
                
#if WIRELESS_EXT > 9
	range->txpower_capa = IW_TXPOW_DBM;
	// XXX what about min/max_pmp, min/max_pmt, etc.
#endif
                                
#if WIRELESS_EXT > 10
	range->we_version_compiled = WIRELESS_EXT;
	range->we_version_source = 13;
                                                
	range->retry_capa = IW_RETRY_LIMIT;
	range->retry_flags = IW_RETRY_LIMIT;
	range->min_retry = 0;
	range->max_retry = 255;
#endif /* WIRELESS_EXT > 10 */
                                                                                
	range->num_channels = NUM_CHANNELS;

        /* XXX need to filter against the regulatory domain &| active set */
	val = 0;
	for (i = 0; i < NUM_CHANNELS ; i++) {
		range->freq[val].i = i + 1;
		range->freq[val].m = channel_frequency[i] * 100000;
		range->freq[val].e = 1;
		val++;
	}
                                                                                                
	range->num_frequency = val;
	
	/* Max of /proc/net/wireless */
	range->max_qual.qual = 92;
	range->max_qual.level = 154;
	range->max_qual.noise = 154;
	range->sensitivity = 3;

	// XXX these need to be nsd-specific!
	range->min_rts = 256;
	range->max_rts = 2346;
	range->min_frag = 256;
        range->max_frag = 2346;
	range->max_encoding_tokens = NUM_WEPKEYS;
	range->num_encoding_sizes = 2;
	range->encoding_size[0] = 5;
	range->encoding_size[1] = 13;
                                        
	// XXX what about num_bitrates/throughput?
	range->num_bitrates = 0;
                                                        
	/* estimated max throughput */
	// XXX need to cap it if we're running at ~2Mbps..
	range->throughput = 5500000;
	
	return 0;
}

static int
zd1205_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	struct zd1205_private *macp;
	void *regp;
	struct zdap_ioctl zdreq;
	struct iwreq *wrq = (struct iwreq *)ifr;
	int err = 0;
	int changed = 0;

 	//ZENTER();
	macp = dev->priv;
	regp = macp->regp;

	if(!netif_running(dev))
		return -EINVAL;	

	switch (cmd) {
        case SIOCGIWNAME:
		    DEBUG(1, "%s: SIOCGIWNAME\n", dev->name);
		    strcpy(wrq->u.name, "IEEE 802.11-DS");
		    break;

        case SIOCGIWAP:
		    DEBUG(1, "%s: SIOCGIWAP\n", dev->name);
		    wrq->u.ap_addr.sa_family = ARPHRD_ETHER;
                    memcpy(wrq->u.ap_addr.sa_data, macp->mac_addr, 6);
		    break;

	case SIOCGIWRANGE:
                    DEBUG(1, "%s: SIOCGIWRANGE\n", dev->name);
                    if(wrq->u.data.pointer != NULL) {
                        struct iw_range range;
                        err = zd1205wext_giwrange(dev, NULL, &wrq->u.data,
                                                  (char *) &range);                    
                        /* Push that up to the caller */
                        if(copy_to_user(wrq->u.data.pointer, &range, sizeof(range)))
                            err = -EFAULT;
                    }

                    break;

        case SIOCSIWMODE:
                    DEBUG(1, "%s: SIOCSIWMODE\n", dev->name);
                    break;

        case SIOCGIWMODE:
		    DEBUG(1, "%s: SIOCGIWMODE\n", dev->name);
                    err = zd1205wext_giwmode(dev, NULL, &wrq->u.mode, NULL);
		    break;

#if 0
        case SIOCSIWENCODE:
		    DEBUG(1, "%s: SIOCSIWENCODE\n", dev->name);
		    err = zd1205_ioctl_setiwencode(dev, &wrq->u.encoding);

		    if (!err)
			    changed = 1;
		    break;


        case SIOCGIWENCODE:
		    DEBUG(1, "%s: SIOCGIWENCODE\n", dev->name);
			err = zd1205_ioctl_getiwencode(dev, &wrq->u.encoding);
		    break;
#endif

        case SIOCSIWESSID:
		    DEBUG(1, "%s: SIOCSIWESSID\n", dev->name);
		    err = zd1205_ioctl_setessid(dev, &wrq->u.essid);
		    if (! err)
			    changed = 1;
		    break;

	case SIOCGIWESSID:
		    DEBUG(1, "%s: SIOCGIWESSID\n", dev->name);
		    err = zd1205_ioctl_getessid(dev, &wrq->u.essid);
		    break;

        case SIOCGIWFREQ:
		    DEBUG(1, "%s: SIOCGIWFREQ\n", dev->name);
		    wrq->u.freq.m = zd1205_hw_get_freq(macp);
		    wrq->u.freq.e = 1;
		    break;

	case SIOCSIWFREQ:
		    DEBUG(1, "%s: SIOCSIWFREQ\n", dev->name);
		    err = zd1205_ioctl_setfreq(dev, &wrq->u.freq);
		    if (!err)
			    changed = 1;
		    break;

#if 0
        case SIOCGIWSENS:
		    DEBUG(1, "%s: SIOCGIWSENS\n", dev->name);
		    err = zd1205_ioctl_getsens(dev, &wrq->u.sens);
		    break;

	case SIOCSIWSENS:
		    DEBUG(1, "%s: SIOCSIWSENS\n", dev->name);
		    break;          
#endif
                                                                                                                          
        case SIOCGIWRTS:
		    DEBUG(1, "%s: SIOCGIWRTS\n", dev->name);
                    err = zd1205wext_giwrts(dev, NULL, &wrq->u.rts, NULL);
		    break;

	case SIOCSIWRTS:
		    DEBUG(1, "%s: SIOCSIWRTS\n", dev->name);
		    err = zd1205_ioctl_setrts(dev, &wrq->u.rts);
		    if (! err)
			    changed = 1;
		    break;

        case SIOCSIWFRAG:
		    DEBUG(1, "%s: SIOCSIWFRAG\n", dev->name);
		    err = zd1205_ioctl_setfrag(dev, &wrq->u.frag);
		    if (! err)
			    changed = 1;
		    break;

	case SIOCGIWFRAG:
		    DEBUG(1, "%s: SIOCGIWFRAG\n", dev->name);
		    err = zd1205_ioctl_getfrag(dev, &wrq->u.frag);
		    break;

#if 0
        case SIOCSIWRATE:
		    DEBUG(1, "%s: SIOCSIWRATE\n", dev->name);
		    err = zd1205_ioctl_setrate(dev, &wrq->u.bitrate);
		    if (! err)
			    changed = 1;
		    break;
#endif

	case SIOCGIWRATE:
		    DEBUG(1, "%s: SIOCGIWRATE\n", dev->name);
		    err = zd1205_ioctl_getrate(dev, &wrq->u.bitrate);
		    break;

#if 0
        case SIOCSIWPOWER:
		    DEBUG(1, "%s: SIOCSIWPOWER\n", dev->name);
		    err = zd1205_ioctl_setpower(dev, &wrq->u.power);
		    if (! err)
			    changed = 1;
		    break;

	case SIOCGIWPOWER:
		    DEBUG(1, "%s: SIOCGIWPOWER\n", dev->name);
		    err = zd1205_ioctl_getpower(dev, &wrq->u.power);
		    break;
#endif

#if WIRELESS_EXT > 10
	    case SIOCSIWRETRY:
		    DEBUG(1, "%s: SIOCSIWRETRY\n", dev->name);
		    err = -EOPNOTSUPP;
		    break;


	    case SIOCGIWRETRY:
		    DEBUG(1, "%s: SIOCGIWRETRY\n", dev->name);
		    err = zd1205_ioctl_getretry(dev, &wrq->u.retry);
		    break;
#endif /* WIRELESS_EXT > 10 */                                          


        case SIOCIWFIRSTPRIV + 0x5: /* get_preamble */
		    DEBUG(1, "%s: SIOCIWFIRSTPRIV + 0x5 (get_preamble)\n", dev->name);
            {
		        int *val = (int *)wrq->u.name;

			    zd1205_lock(macp);
			    *val = macp->card_setting.PreambleType;
			    zd1205_unlock(macp);

            }    
		    break;

        
	case ZDAPIOCTL:    //ZD1202 debug command
		if (copy_from_user(&zdreq, ifr->ifr_data, sizeof (zdreq))) {
			printk(KERN_ERR "zd1205: copy_from_user error\n");
		        return -EFAULT;
	        }

	        printk(KERN_DEBUG "zd1205: cmd = %2x, reg = %4x, value = %4x\n",
                zdreq.cmd, zdreq.addr, zdreq.value);
		zd1205_zd_dbg_ioctl(macp, &zdreq);
		break;    
			
		case SIOCSIWRATE:
		case SIOCSIWPOWER:
		case SIOCGIWPOWER:
		case SIOCGIWENCODE:
		case SIOCSIWENCODE:
		default:
			err =  -EOPNOTSUPP;	
	}
	//ZEXIT();

	return err;
}


/**
 * zd1205init - initialize the adapter
 * @macp: atapter's private data struct
 *
 * This routine is called when this driver is loaded. This is the initialization
 * routine which allocates memory, configures the adapter and determines the
 * system resources.
 *
 * Returns:
 *      true: if successful
 *      false: otherwise

 */
static unsigned char __devinit
zd1205_init(struct zd1205_private *macp)
{
 	void *regp = macp->regp;

	ZENTER();	
	/* read the MAC address from the eprom */
	zd1205_rd_eaddr(macp);
	macp->RF_Mode = readl(regp+E2P_POD);
	macp->RF_Mode &= 0x0f;
	dot11Obj.rfMode = macp->RF_Mode;
	
	zd1205_sw_init(macp);
	zd1205_hw_init(macp);
	zd1205_disable_int();

	ZEXIT();
	return true;
}


void zd1205_init_card_setting(struct zd1205_private *macp)
{
	card_Setting_t *pSetting = &macp->card_setting;
	
    pSetting->AuthMode = 2; 	//auto auth
	pSetting->HiddenSSID = 0; 	//disable hidden essid

	pSetting->LimitedUser = 32;
	pSetting->RadioOn = 1;
	pSetting->BlockBSS = 0;
	pSetting->EncryOnOff = 0;
	pSetting->PreambleType = 0; //long preamble
	pSetting->Channel = 6;
	pSetting->EncryMode = NO_WEP;

	pSetting->EncryKeyId = 0;
    pSetting->TxPowerLevel = 0;
	pSetting->Info_SSID[0] = 0;
	pSetting->Info_SSID[1] = 0x08;
 	pSetting->Info_SSID[2] = 'Z';
	pSetting->Info_SSID[3] = 'D';
	pSetting->Info_SSID[4] = '1';

	pSetting->Info_SSID[5] = '2';
	pSetting->Info_SSID[6] = '0';

	pSetting->Info_SSID[7] = '2';
	pSetting->Info_SSID[8] = 'A';
	pSetting->Info_SSID[9] = 'P';
 
	pSetting->Info_SupportedRates[0] = 0x01;
	pSetting->Info_SupportedRates[1] = 0x05;
	pSetting->Info_SupportedRates[2] = 0x82;
	pSetting->Info_SupportedRates[3] = 0x84;
  	pSetting->Info_SupportedRates[4] = 0x8B;
	pSetting->Info_SupportedRates[5] = 0x96;
    pSetting->Info_SupportedRates[6] = 0x21;
    
	if ((macp->RF_Mode == AL2210MPVB_RF) || (macp->RF_Mode == AL2210_RF)){
		pSetting->Rate275 = 1;
		pSetting->Info_SupportedRates[7] = 0x2C;//22
		pSetting->Info_SupportedRates[8] = 0x37;//27.5
		pSetting->Info_SupportedRates[1] = 0x07;
	}
	else
		pSetting->Rate275 = 0;    
 
	pSetting->FragThreshold = 0x980;
	pSetting->RTSThreshold = 0x980;
	pSetting->BeaconInterval = 100;
	pSetting->DtimPeriod = 3;

    pSetting->SwCipher = 0;
	pSetting->DynKeyMode = 0;
	pSetting->WpaBcKeyLen = 32; // Tmp key(16) + Tx Mic key(8) + Rx Mic key(8)

	//dot11Obj.MicFailure = NULL;
	//dot11Obj.AssocRequest = NULL;
	//dot11Obj.WpaIe =  NULL;
}	


static int __devinit
zd1205_found1(struct pci_dev *pcid, const struct pci_device_id *ent)
{
	static int first_time = true;
  	struct net_device *dev = NULL;
	struct zd1205_private *macp = NULL;
	int rc = 0;
	
	ZENTER();

	dev = alloc_etherdev(sizeof (struct zd1205_private));
	if (dev == NULL) {
		printk(KERN_ERR "zd1205: Not able to alloc etherdev struct\n");
		rc = -ENODEV;
		goto out;

	}

 	g_dev = dev;  //save this for CBs use
	SET_MODULE_OWNER(dev);

	if (first_time) {
		first_time = false;
        printk(KERN_NOTICE "%s - version %s\n",
	               zd1205_full_driver_name, zd1205_driver_version);
		printk(KERN_NOTICE "%s\n", zd1205_copyright);
		printk(KERN_NOTICE "\n");
	}

	macp = dev->priv;
	macp->pdev = pcid;
	macp->device = dev;
 	pci_set_drvdata(pcid, dev);
	
	macp->num_tcb = NUM_TCB;
	macp->num_tbd = NUM_TBD;
	macp->num_rfd = NUM_RFD;
	macp->num_tbd_per_tcb = NUM_TBD_PER_TCB;
	macp->regp = 0;
    macp->rx_offset  = ZD_RX_OFFSET;
    macp->rfd_size = 24; // form CbStatus to NextCbPhyAddrHighPart

	init_timer(&macp->watchdog_timer);
    macp->watchdog_timer.data = (unsigned long) dev;
	macp->watchdog_timer.function = (void *) &zd1205_watchdog;

    init_timer(&macp->tm_hking_id);
    macp->tm_hking_id.data = (unsigned long) dev;
	macp->tm_hking_id.function = (void *) &HKeepingCB;

	if ((rc = zd1205_pci_setup(pcid, macp)) != 0) {
		goto err_dev;
	}

	if (!zd1205_init(macp)) {
		printk(KERN_ERR "zd1025: Failed to initialize, instance #%d\n",
		       zd1205nics);
		rc = -ENODEV;
		goto err_pci;
	}

	dev->irq = pcid->irq;
	dev->open = &zd1205_open;
	dev->hard_start_xmit = &zd1205_xmit_frame;
	dev->stop = &zd1205_close;
	dev->change_mtu = &zd1205_change_mtu;
	dev->get_stats = &zd1205_get_stats;
 	dev->set_multicast_list = &zd1205_set_multi;
	dev->set_mac_address = &zd1205_set_mac;
	dev->do_ioctl = &zd1205_ioctl;

    dev->features |= NETIF_F_SG | NETIF_F_HW_CSUM;
	zd1205nics++;

	if ((rc = register_netdev(dev)) != 0) {
		goto err_pci;

	}
	
    memcpy(macp->ifname, dev->name, IFNAMSIZ);
    macp->ifname[IFNAMSIZ-1] = 0;	
    if (netif_carrier_ok(macp->device))
		macp->cable_status = "Cable OK";
	else
		macp->cable_status = "Not Available";

    if (zd1205_create_proc_subdir(macp) < 0) {
		printk(KERN_ERR "zd1205: Failed to create proc dir for %s\n",
		       macp->device->name);
	}    

	printk(KERN_NOTICE "\n");
	goto out;

err_pci:
	iounmap(macp->regp);
	pci_release_regions(pcid);
	pci_disable_device(pcid);
	
err_dev:
	pci_set_drvdata(pcid, NULL);
	kfree(dev);
	
out:
    ZEXIT();
	return rc;
}


/**
 * zd1205_clear_structs - free resources

 * @dev: adapter's net_device struct
 *
 * Free all device specific structs, unmap i/o address, etc.
 */
static void __devexit
zd1205_clear_structs(struct net_device *dev)
{
	struct zd1205_private *macp = dev->priv;
	ZENTER();
    
 	zd1205_sw_release();
	iounmap(macp->regp);
	pci_release_regions(macp->pdev);
	pci_disable_device(macp->pdev);
	pci_set_drvdata(macp->pdev, NULL);
	kfree(dev);
	ZEXIT();
}


static void __devexit
zd1205_remove1(struct pci_dev *pcid)
{
	struct net_device *dev;
 	struct zd1205_private *macp;

	ZENTER();	
	if (!(dev = (struct net_device *) pci_get_drvdata(pcid)))
		return;

	macp = dev->priv;

	unregister_netdev(dev);

    zd1205_remove_proc_subdir(macp);
	zd1205_clear_structs(dev);

	--zd1205nics;

    ZEXIT();
}


static struct pci_device_id zd1205_id_table[] __devinitdata =
{
	{0x167b, 0x2102, PCI_ANY_ID, PCI_ANY_ID, 0, 0, ZD_1202},
	{0x167b, 0x2100, PCI_ANY_ID, PCI_ANY_ID, 0, 0, ZD_1202},
    {0x167b, 0x2105, PCI_ANY_ID, PCI_ANY_ID, 0, 0, ZD_1205},
	{0,}			/* terminate list */


};


MODULE_DEVICE_TABLE(pci, zd1205_id_table);
static struct pci_driver zd1205_driver = {
	.name         = "zd1205",
	.id_table     = zd1205_id_table,
	.probe        = zd1205_found1,
	.remove       = __devexit_p(zd1205_remove1),
};


static int __init

zd1205_init_module(void)
{
	int ret;

	ZENTER();
    ret = pci_module_init(&zd1205_driver);
    ZEXIT();

	return ret;
}


static void __exit
zd1205_cleanup_module(void)
{
	ZENTER();
	pci_unregister_driver(&zd1205_driver);
	ZEXIT();
}

module_init(zd1205_init_module);
module_exit(zd1205_cleanup_module);




/*************************************************************************/
BOOLEAN zdcb_setup_next_send(fragInfo_t *frag_info)
{
	struct zd1205_private *macp = g_dev->priv;

	void *regp = macp->regp;
	struct sk_buff *skb = (struct sk_buff *)frag_info->buf;

    U8 bIntraBss =  frag_info->bIntraBss;
    U8 msg_id = frag_info->msgID;
    U8			numOfFrag = frag_info->totalFrag;
    U16			aid = frag_info->aid;
    U8			hdrLen = frag_info->hdrLen;
	sw_tcb_t 	*sw_tcb;
	sw_tcb_t 	*next_sw_tcb;
 	hw_tcb_t	*hw_tcb;
 	tbd_t		*pTbd;
	U8			*hdr, *pBody;
 	U32			bodyLen, length;
	U32 		tmp_value, tmp_value3;
	U32 		tcb_tbd_num = 0;
	int 		i;
	U16 		pdu_size = 0;
 	void 		*addr;
	wla_header_t *wla_hdr;
	U32			curr_frag_len;
	U32			next_frag_len;
	skb_frag_t *frag = NULL;
    unsigned long lock_flag;
    ctrl_setting_parm_t ctrl_setting_parms;
    U32         loopCnt = 0;

    if (macp->dbg_flag > 0)
        printk(KERN_DEBUG "===== zdcb_setup_next_send enter =====\n");

    if (macp->dbg_flag > 2){
        printk(KERN_DEBUG "zd1205: bIntraBss = %x\n", bIntraBss);
        printk(KERN_DEBUG "zd1205: numOfFrag = %x\n", numOfFrag);
        printk(KERN_DEBUG "zd1205: skb = %x\n", (U32)skb);
        printk(KERN_DEBUG "zd1205: aid = %x\n", aid);
    }

    spin_lock_irqsave(&macp->bd_non_tx_lock, lock_flag);
	if ((skb) && (!bIntraBss)){   //data frame from upper layer
	    if (skb_shinfo(skb)->nr_frags){   //got frag buffer
		    frag = &skb_shinfo(skb)->frags[0];
            
            if (skb->len > macp->card_setting.FragThreshold){  //need fragment
		        pdu_size = macp->card_setting.FragThreshold - 24 - 4; //mac header and crc32 length
		        numOfFrag = (skb->len + (pdu_size-1) ) / pdu_size;
                if (numOfFrag == 0)
			        numOfFrag = 1;

                if (macp->dbg_flag > 2)
                    printk(KERN_DEBUG "zd1205: numOfFrag = %x\n", numOfFrag);
	        }
	    }
     }

	if (macp->freeTxQ->count -1 < numOfFrag){
        printk(KERN_ERR "zd1205: Not enough freeTxQ\n");
        //printk(KERN_ERR "zd1205: Cnt of freeTxQ = %x\n", macp->freeTxQ->count);
        spin_unlock_irq(&macp->bd_non_tx_lock);
		return FALSE;
	}

   
	ctrl_setting_parms.rate = frag_info->rate;
	ctrl_setting_parms.preamble = frag_info->preamble;
	//ctrl_setting_parms.encry_type = frag_info->encryType;
	//ctrl_setting_parms.vapid = frag_info->vapId;

	for (i=0; i<numOfFrag; i++){
         if (macp->dbg_flag > 2){
             printk(KERN_DEBUG "zd1205: Cnt of freeTxQ = %x\n", macp->freeTxQ->count);
             printk(KERN_DEBUG "zd1205: Frag Num = %x\n", i);
         }    

		sw_tcb = zd1205_first_txq(macp, macp->freeTxQ);
		hdr = frag_info->macHdr[i];

        if (macp->dbg_flag > 4)
            zd1205_dump_data("header part", (U8 *)hdr, 24);
            
        if (skb){
            if ((bIntraBss) || (!frag)){  //wireless forwarding or tx data from upper layer and no linux frag
                if (macp->dbg_flag > 2)
                    printk(KERN_DEBUG "zd1205: Wireless forwarding or no linux frag\n");
                
                pBody = frag_info->macBody[i];
                bodyLen = frag_info->bodyLen[i];
                curr_frag_len = bodyLen;

                next_frag_len = frag_info->nextBodyLen[i];
                if (i == (numOfFrag - 1))
                    sw_tcb->last_frag = 1;

                 else
                    sw_tcb->last_frag = 0;
            }
            else{ //tx data from upper layer with linux frag
                if (macp->dbg_flag > 2)
                    printk(KERN_DEBUG "zd1205: tx data from upper layer with linux frag\n");
                pBody = skb->data;
  		        bodyLen = skb->len;

                if (i == (numOfFrag - 1)){
                    curr_frag_len = bodyLen - i*pdu_size;
			        next_frag_len = 0;
                    sw_tcb->last_frag = 1;
                }
                else{
                    curr_frag_len = pdu_size;
                    sw_tcb->last_frag = 0;

			        if (i == (numOfFrag-2))
				        next_frag_len = bodyLen - (i+1)*pdu_size;
			        else
				        next_frag_len = pdu_size;
                }
            }
        }
        else{  //mgt frame
            if (macp->dbg_flag > 2)
                printk(KERN_DEBUG "zd1205: mgt frame\n");
            pBody = frag_info->macBody[i];
            bodyLen = frag_info->bodyLen[i];
            curr_frag_len = bodyLen;
            next_frag_len = frag_info->nextBodyLen[i];
            sw_tcb->last_frag = 1;
        }

        wla_hdr = (wla_header_t *)hdr;
  		hw_tcb = sw_tcb->tcb;
 		pTbd = sw_tcb->first_tbd;
        tcb_tbd_num = 0;
		hw_tcb->TxCbTbdNumber = 0;
		sw_tcb->frame_type = hdr[0];
		sw_tcb->msg_id = msg_id;
		sw_tcb->aid = aid;
        sw_tcb->skb = skb;
        sw_tcb->bIntraBss = bIntraBss;
        sw_tcb->rate = frag_info->rate;

        if (macp->dbg_flag > 2){
            //printk(KERN_DEBUG "zd1205: sw_tcb = %x\n", sw_tcb);
            printk(KERN_DEBUG "zd1205: hw_tcb = %x\n", (u32)hw_tcb);
            printk(KERN_DEBUG "zd1205: first tbd = %x\n", (u32)pTbd);
        }
        
        ctrl_setting_parms.curr_frag_len = curr_frag_len;
        ctrl_setting_parms.next_frag_len = next_frag_len;

		/* Control Setting */
		length = cfg_ctrl_setting(macp, sw_tcb, wla_hdr, &ctrl_setting_parms);
		pTbd->TbdBufferAddrHighPart = 0;
		pTbd->TbdBufferAddrLowPart = cpu_to_le32(sw_tcb->hw_ctrl_phys);
		pTbd->TbdCount = cpu_to_le32(length);
  		pTbd++;
		tcb_tbd_num++;

		/* MAC Header */
		length = cfg_mac_header(macp, sw_tcb, wla_hdr, hdrLen);
		pTbd->TbdBufferAddrHighPart = 0;
		pTbd->TbdBufferAddrLowPart = cpu_to_le32(sw_tcb->hw_header_phys);
		pTbd->TbdCount = cpu_to_le32(length);
		pTbd++;
 		tcb_tbd_num++;

		/* Frame Body */
		if ((!skb) || ((skb) && (!frag))) {
            U32 body_dma;

            if (macp->dbg_flag > 2)
                printk(KERN_DEBUG "zd1205: Management frame body or No linux frag\n");
            if (macp->dbg_flag > 4)
                zd1205_dump_data("data part", (U8 *)pBody, 14);

			pTbd->TbdBufferAddrHighPart = 0;

            body_dma =  pci_map_single(macp->pdev, pBody, bodyLen, PCI_DMA_TODEVICE);
            if (macp->dbg_flag > 2)
                printk(KERN_DEBUG "zd1205: body_dma = %x\n", (u32)body_dma);

			pTbd->TbdBufferAddrLowPart =  cpu_to_le32(body_dma);

			pTbd->TbdCount = cpu_to_le32(curr_frag_len);
			pBody += curr_frag_len;
			pTbd++;
 			tcb_tbd_num++;
		} else {
			while(curr_frag_len){
                U32 body_dma;

				if (curr_frag_len >= frag->size){
                    printk(KERN_DEBUG "zd1205: linux more frag skb\n");
 				    addr = ((void *) page_address(frag->page) + frag->page_offset);
               	    pTbd->TbdBufferAddrHighPart = 0;
                    body_dma = 	pci_map_single(macp->pdev, addr, frag->size, PCI_DMA_TODEVICE);

              	    pTbd->TbdBufferAddrLowPart =  cpu_to_le32(body_dma);
                    pTbd->TbdCount = cpu_to_le32(frag->size);
    			    tcb_tbd_num++;
               	    curr_frag_len -= frag->size;
				    frag++;
    			}
				else{
                    printk(KERN_DEBUG "zd1205: linux last frag skb\n");
					addr = ((void *) page_address(frag->page) + frag->page_offset);
					pTbd->TbdBufferAddrHighPart = 0;
                    body_dma = cpu_to_le32(pci_map_single(macp->pdev, addr, pdu_size, PCI_DMA_TODEVICE));
					pTbd->TbdBufferAddrLowPart =  cpu_to_le32(body_dma);
					frag->page_offset += curr_frag_len;
					frag->size -= curr_frag_len;
					curr_frag_len = 0;
				}

                printk(KERN_DEBUG "zd1205: page_address = %x\n", (u32)addr);
                printk(KERN_DEBUG "zd1205: body_dma = %x\n", (u32)body_dma);
				pTbd++;
				tcb_tbd_num++;
			}
		}
        hw_tcb->TxCbTbdNumber = cpu_to_le32(tcb_tbd_num);
		hw_tcb->CbCommand = 0; /* set this TCB belong to bus master */
		macp->txCnt++;
        wmb();

 		while(1){
			tmp_value = readl(regp+DeviceState);
 			tmp_value &= 0xf;
			if ((tmp_value == TX_READ_TCB) || (tmp_value == TX_CHK_TCB)){
				/* Device is now checking suspend or not.
				 Keep watching until it finished check. */
				udelay(1);
				continue;
			}
			else
 				break;
     
            loopCnt++;
            if (loopCnt > 1000000)
                break;
		}

        if (loopCnt > 1000000)
            FPRINT("I am in zdcb_setup_next_send loop") ;

        if (macp->dbg_flag > 1)
	        printk(KERN_DEBUG "zd1205: Device State = %x\n", (u32)tmp_value);

		if (tmp_value == TX_IDLE){ /* bus master get suspended TCB */
			macp->txIdleCnt++;
			/* Tx bus master is in idle state. */
			//tmpValue1 = readl(regp+InterruptCtrl);
			/* No retry fail happened */
			tmp_value3 = readl(regp+ReadTcbAddress);
 			next_sw_tcb = macp->freeTxQ->first;

 			if (tmp_value3 != le32_to_cpu(next_sw_tcb->tcb->NextCbPhyAddrLowPart)){
				/* Restart Tx again */
				writel(sw_tcb->tcb_phys, regp+PCI_TxAddr_p1);
                if (macp->dbg_flag > 1)
                    printk(KERN_DEBUG "zd1205: Write  PCI_TxAddr_p1 = %x\n", sw_tcb->tcb_phys);
			}
		}

        else if (tmp_value == 0xA){ //Dtim Notify Int happened
			writel(sw_tcb->tcb_phys | BIT_0, regp+PCI_TxAddr_p1);
		}

        if (macp->dbg_flag > 1){
            printk(KERN_DEBUG "zd1205: NAV_CCA = %x\n", readl(regp+NAV_CCA));
            printk(KERN_DEBUG "zd1205: NAC_CNT = %x\n", readl(regp+NAV_CNT));
        }

    	zd1205_qlast_txq(macp, macp->activeTxQ, sw_tcb);


        if (macp->dbg_flag > 1)
            printk(KERN_DEBUG "zd1205: Cnt of activeQ = %x\n", macp->activeTxQ->count);
	}
    g_dev->trans_start = jiffies;
    macp->TxStartTime = readl(regp+TSF_LowPart);
    spin_unlock_irqrestore(&macp->bd_non_tx_lock, lock_flag);

    if (macp->dbg_flag > 0)
        printk(KERN_DEBUG "===== zdcb_setup_next_send exit =====\n");

	return TRUE;
}


void zdcb_release_buffer(void *buf)
{
	struct sk_buff *skb = (struct sk_buff *)buf;

	if (skb)
		dev_kfree_skb_irq(skb);
}


void zdcb_rx_ind(U8 *pData, U32 length, void *buf)
{
	struct zd1205_private *macp = g_dev->priv;
	struct sk_buff *skb = (struct sk_buff *)buf;

    if (macp->dbg_flag > 3)
	    ZENTER();
 
    //copy packet for IP header is located on 4-bytes alignment
    if (length < RX_COPY_BREAK){
        dev_kfree_skb_irq(skb);
        skb = dev_alloc_skb(length+2);
        if (skb){
            skb->dev = g_dev;
            skb_reserve(skb, 2);
            eth_copy_and_sum(skb, pData, length, 0);
            skb_put(skb, length);
        }
    }
    else{
        skb->tail = skb->data = pData;
        skb_put(skb, length);
    }

    if (macp->dbg_flag > 3){
        //zd1205_dump_data("rx_ind", (U8 *)skb->data, skb->len);
        printk(KERN_DEBUG "zd1205: rx_ind length = %x\n", (u32)length);
    }

	/* set the protocol */
	skb->protocol = eth_type_trans(skb, g_dev);
	skb->ip_summed = CHECKSUM_NONE;	//TBD
 	g_dev->last_rx = jiffies;

	switch(netif_rx(skb)){
      case NET_RX_BAD:
      case NET_RX_DROP:
      case NET_RX_CN_MOD:
      case NET_RX_CN_HIGH:
        break;

      default:
        macp->drv_stats.net_stats.rx_packets++;
	    macp->drv_stats.net_stats.rx_bytes += skb->len;
        break;

    }

    if (macp->dbg_flag > 3)
	    ZEXIT();
}



U16 zdcb_status_notify(U16 status, U8 *StaAddr)
{
	U16 result = 0;
 
	switch(status){
		case STA_AUTH_REQ:
			break;
			
		case STA_ASOC_REQ:
			break;
			
		case STA_REASOC_REQ:
			break;		

		case STA_ASSOCIATED:
            printk(KERN_DEBUG "STA_ASSOCIATED\n");
            printk(KERN_DEBUG "mac addr = %02x:%02x:%02x:%02x:%02x:%02x\n",
		        StaAddr[0], StaAddr[1], StaAddr[2], StaAddr[3], StaAddr[4], StaAddr[5]);
			break;

		case STA_REASSOCIATED:
			printk(KERN_DEBUG "STA_REASSOCIATED\n");
            printk(KERN_DEBUG "mac addr = %02x:%02x:%02x:%02x:%02x:%02x\n",
		        StaAddr[0], StaAddr[1], StaAddr[2], StaAddr[3], StaAddr[4], StaAddr[5]);
			break;

		case STA_DISASSOCIATED:
			break;

		default:
			break;
	}
 
	return result;
}


void zdcb_tx_completed(void)
{

}


void send_tchal_msg(unsigned long ptr)
{
	zd_EventNotify(EVENT_TCHAL_TIMEOUT, 0, 0, 0);
}


void scan_tout_cb(unsigned long ptr)
{
	zd_EventNotify(EVENT_SCAN_TIMEOUT, 0, 0, 0);
}


void zdcb_start_timer(U32 timeout, U32 event)
{
	struct zd1205_private *macp = g_dev->priv;

	//ZENTER();
	if (event == DO_CHAL){
		init_timer(&macp->tchal_timer);
		macp->tchal_timer.data = (unsigned long) g_dev;
		macp->tchal_timer.expires = jiffies + timeout;
		macp->tchal_timer.function = send_tchal_msg;
		add_timer(&macp->tchal_timer);
	}
	
	if (event == DO_SCAN) {
		init_timer(&macp->tm_scan_id);

		macp->tm_scan_id.data = (unsigned long) g_dev;
		macp->tm_scan_id.expires = jiffies + timeout;
		macp->tm_scan_id.function = scan_tout_cb;
		add_timer(&macp->tm_scan_id);
	}
}


void zdcb_stop_timer(void)
{
	struct zd1205_private *macp = g_dev->priv;

	del_timer(&macp->tchal_timer);
}


inline U32
zdcb_dis_intr(void)
{
	struct zd1205_private *macp = g_dev->priv;
	//void *regp = macp->regp;
	U32 flags = 0;

	/* Disable interrupts on our PCI board by setting the mask bit */
	//writel(0, regp+InterruptCtrl);
	
	spin_lock_irqsave(&macp->q_lock, flags);
	return flags;
}


inline void
zdcb_set_intr_mask(U32 flags)
{
	struct zd1205_private *macp = g_dev->priv;
 	//void *regp = macp->regp;

	//writel(macp->intr_mask, regp+InterruptCtrl);
	spin_unlock_irqrestore(&macp->q_lock, flags);
}


U32 zdcb_vir_to_phy_addr(U32 virtAddr) //TBD
{
	return virtAddr;
}


inline void zdcb_set_reg(void *reg, U32 offset, U32 value)
{
	writel(value, reg+offset);
}


inline U32 zdcb_get_reg(void *reg, U32 offset)
{
 	return readl(reg+offset);
}


inline BOOLEAN
zdcb_check_tcb_avail(U8	num_of_frag)
{
    struct zd1205_private *macp = g_dev->priv;
    BOOLEAN ret;
    U32 flags;


	spin_lock_irqsave(&macp->q_lock, flags);
    if (macp->freeTxQ->count < (num_of_frag+1))
        ret = FALSE;
    else
    	ret = TRUE;

	spin_unlock_irqrestore(&macp->q_lock, flags);
    return ret;
}


void zdcb_delay_us(U16 ustime)
{
	udelay(ustime);
}


void * zdcb_AllocBuffer(U16 dataSize, U8 **pData)
{
	struct sk_buff *new_skb = NULL;
	
	new_skb = (struct sk_buff *) dev_alloc_skb(dataSize);
	if (new_skb){
		*pData = new_skb->data;
	}
	
	return (void *)new_skb;	
}	

//setup callback functions for protocol stack
void zd1205_set_zd_cbs(zd_80211Obj_t *pObj)
{

    pObj->QueueFlag = 0;
	pObj->SetReg = zdcb_set_reg;
	pObj->GetReg = zdcb_get_reg;
	pObj->ReleaseBuffer = zdcb_release_buffer;
 	pObj->RxInd = zdcb_rx_ind;
	pObj->TxCompleted = zdcb_tx_completed;
	pObj->StartTimer = zdcb_start_timer;
 	pObj->StopTimer = zdcb_stop_timer;
	pObj->SetupNextSend = zdcb_setup_next_send;
	pObj->StatusNotify = zdcb_status_notify;
	pObj->ExitCS = zdcb_set_intr_mask;
	pObj->EnterCS = zdcb_dis_intr;
	pObj->Vir2PhyAddr = zdcb_vir_to_phy_addr;

    pObj->CheckTCBAvail = zdcb_check_tcb_avail;
    pObj->DelayUs = zdcb_delay_us;
    pObj->AllocBuffer = zdcb_AllocBuffer;
    // wpa support
	pObj->MicFailure = NULL;
	pObj->AssocRequest = NULL;
	pObj->WpaIe = NULL;
}


//debug functions
struct sk_buff* zd1205_prepare_tx_data(struct zd1205_private *macp, u16 bodyLen)
{
    struct sk_buff *skb;
    u8 *pData;

    u16 typeLen;
    int i;

    skb = (struct sk_buff *) dev_alloc_skb(bodyLen+14);
    if (!skb){
        printk(KERN_DEBUG "out of skb buffer\n");
        return NULL;  
    }

    pData = skb->data;
    pData[0] = pData[1] = pData[2] = 0xff;
    pData[3] = pData[4] = pData[5] = 0xff;
    pData[6] = macp->mac_addr[0];
    pData[7] = macp->mac_addr[1];
    pData[8] = macp->mac_addr[2];
    pData[9] = macp->mac_addr[3];

    pData[10] = macp->mac_addr[4];
    pData[11] = macp->mac_addr[5];
    typeLen = bodyLen;
    pData[12] = (u8)(typeLen >> 8);
    pData[13] = (u8)typeLen;
    pData[14] = 0xAA;
    pData[15] = 0xAA;
    pData[16] = 0x03;
    pData[17] = pData[18] = pData[19] = 0x00;
    pData[20] = pData[21] = 0x88;

    for (i=0; i<(typeLen-8); i++)
        pData[22+i] = i;

    skb->len = bodyLen + 14;
    if (macp->lb_mode){

        macp->lbFrameLen = bodyLen;
        memcpy(macp->lbFrame, skb->data, skb->len);
    }
    return skb;        

}


void zd1205_tx_test(struct zd1205_private *macp, u16 size)
{
    struct sk_buff *skb;

    skb =  zd1205_prepare_tx_data(macp, size);

    if (skb)
       ;//zd_SendPkt(skb->data, skb->len, (void *)skb);
    else
       printk(KERN_DEBUG "zd1205_tx_test fail\n");         
}
