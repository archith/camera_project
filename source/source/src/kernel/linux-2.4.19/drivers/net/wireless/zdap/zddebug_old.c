#ifndef _ZD_DEBUG_C_
#define _ZD_DEBUG_C_

#include "zddebug.h"


//for debug message show
extern u32 freeSignalCount;
extern u32 freeFdescCount;
extern void ShowQInfo(void);


void zd1205_set_sniffer_mode(struct zd1205_private *macp)
{
    void *regp = macp->regp;
    struct net_device *dev = macp->device;

    dev->type = ARPHRD_IEEE80211;
    dev->hard_header_len = ETH_HLEN;
    dev->addr_len = ETH_ALEN;

    if (netif_running(dev))
		netif_stop_queue(dev);
 
    writel(0x01, regp+SnifferOn);
    writel(0xffffff, regp+Rx_Filter);
    writel(0x08, regp+EncryType);
    macp->intr_mask = RX_COMPLETE_EN;
}


void zd1205_dump_regs(struct zd1205_private *macp)
{
	void *regp = macp->regp;

//	zd1205_disable_int();
    printk(KERN_DEBUG "*******************************************\n");
	printk(KERN_DEBUG "InterruptCtrl      = %08x \n", readl(regp+InterruptCtrl));
    printk(KERN_DEBUG "ReadTcbAddress     = %08x, ReadRfdAddress= %04x\n",
        readl(regp+ReadTcbAddress), readl(regp+ReadRfdAddress));
    printk(KERN_DEBUG "RCB address low= 0x%08x\n", readl(regp+PCI_RxAddr_p1));
//	printk(KERN_DEBUG "MACAddr_P1 = %04x    MACAddr_P2 = %04x\n",
//        readl(regp+MACAddr_P1), readl(regp+MACAddr_P2));
	printk(KERN_DEBUG "BCNInterval        = %08x. BCNPLCPCfg    = %04x\n",
        readl(regp+BCNInterval), readl(regp+BCNPLCPCfg));
	printk(KERN_DEBUG "TSF_LowPart        = %08x, TSF_HighPart  = %04x\n",
        readl(regp+TSF_LowPart), readl(regp+TSF_HighPart));
	printk(KERN_DEBUG "BCN_FIFO_Semaphore = %08x, CtlReg1       = %04x\n",
        readl(regp+BCN_FIFO_Semaphore),  readl(regp+CtlReg1));
	printk(KERN_DEBUG "DeviceState        = %08x, NAV_CCA       = %04x\n",
        readl(regp+DeviceState), readl(regp+NAV_CCA));
//	printk(KERN_DEBUG "Rx_Filter = %04x\n", readl(regp+Rx_Filter));
	printk(KERN_DEBUG "CRC32Cnt           = %08x, CRC16Cnt      = %04x\n",
        readl(regp+CRC32Cnt), readl(regp+CRC16Cnt));
	printk(KERN_DEBUG "TotalRxFrm         = %08x, TotalTxFrm    = %04x\n",
        readl(regp+TotalRxFrm), readl(regp+TotalTxFrm));
	printk(KERN_DEBUG "RxFIFOOverrun      = %08x, UnderrunCnt   = %04x\n",
        readl(regp+RxFIFOOverrun), readl(regp+UnderrunCnt));
	printk(KERN_DEBUG "DecrypErr_UNI      = %08x, DecrypErr_Mul = %04x\n",
        readl(regp+DecrypErr_UNI), readl(regp+DecrypErr_Mul));
printk(KERN_INFO "Finish zd1205_dump_regs\n");
#if 0        
    printk(KERN_DEBUG "RX_OFFSET_BYTE     = %08x, RX_TIME_OUT   = %04x\n",
        readl(regp+RX_OFFSET_BYTE), readl(regp+RX_TIME_OUT));
	printk(KERN_DEBUG "CAM_DEBUG          = %08x, CAM_STATUS    = %04x\n",
        readl(regp+CAM_DEBUG), readl(regp+CAM_STATUS));
	printk(KERN_DEBUG "CAM_ROLL_TB_LOW    = %08x, CAM_ROLL_TB_HIGH = %04x\n",
        readl(regp+CAM_ROLL_TB_LOW), readl(regp+CAM_ROLL_TB_HIGH));
    printk(KERN_DEBUG "CAM_MODE           = %08x\n", readl(regp+CAM_MODE));
#endif    
//	zd1205_enable_int();
}


void zd1205_dump_cnters(struct zd1205_private *macp)
{
    zd1205_lock(macp);
    printk(KERN_DEBUG "*******************************************\n");
	printk(KERN_DEBUG "freeTxQ         = %04x, activeTxQ      = %04x\n", macp->freeTxQ->count, macp->activeTxQ->count);
    printk(KERN_DEBUG "freeSignalCount = %04x, freeFdescCount = %04x\n", freeSignalCount, freeFdescCount);
    printk(KERN_DEBUG "bcnCnt          = %04x, dtimCnt        = %04x\n", macp->bcnCnt, macp->dtimCnt);
    printk(KERN_DEBUG "txCnt           = %04x, txCmpCnt       = %04x\n", macp->txCnt, macp->txCmpCnt);
    printk(KERN_DEBUG "retryFailCnt    = %04x, rxCnt          = %04x\n", macp->retryFailCnt, macp->rxCnt);
    printk(KERN_DEBUG "TxRx_DeadLock   = %04x, HMAC_NoPhyClk  = %04x\n", macp->HMAC_TxRx_DeadLock, macp->HMAC_NoPhyClk);
    printk(KERN_DEBUG "HMAC PCI Hang-up= %04x, TX Timeout Cnt = %04x\n", macp->HMAC_PciHang, macp->HMAC_TxTimeout);
    ShowQInfo();
    macp->bcnCnt = 0;
	macp->dtimCnt = 0;
	macp->rxCnt = 0;
	macp->txCmpCnt = 0;
	macp->txCnt = 0;
	macp->retryFailCnt = 0;
	macp->txIdleCnt = 0;
	macp->rxIdleCnt = 0;
    zd1205_unlock(macp);
}


void zd1205_wep_on_off(struct zd1205_private *macp, u32 value) //0 disable, 2 wep64, 3 wep128, 4 tkip
{
	if (value){
		macp->card_setting.EncryOnOff = 1;
	}
	else{
		macp->card_setting.EncryOnOff = 0;
	}
    zd_UpdateCardSetting(&macp->card_setting);
}


void zd1205_dbg_fifo(struct zd1205_private *macp)
{
    void *regp = macp->regp;
    u32 length, data;
	int i;

   	FPRINT("Dump Debug FIFO");

	writel(0x200, regp+Dbg_Select); /* stop log */
	length = readl(regp+FIFO_Length);
	FPRINT_V("length", length);

    if (length == 0)
        return;
	//if (length > 0x100)
		//length = 0x100;
	for (i=0; i<length; i++)
	{
		data = readl(regp+Dbg_FIFO_Rd);
		printk("%2x", data);
		printk(" ");
		//if (i>0 && ((i+1)%10 == 0))
		if (i>0 && ((i+1)%4 == 0))
			printk("\n");
	}
	printk("\n");
}


void zd1205_set_frag(struct zd1205_private *macp, u32 value)
{
	FPRINT("Set Fragment Threshold");
	macp->card_setting.FragThreshold = value;
    zd_UpdateCardSetting(&macp->card_setting);
}


void zd1205_set_rts(struct zd1205_private *macp, u32 value)
{
 	FPRINT("Set RTS Threshold");
	macp->card_setting.RTSThreshold = value;
    zd_UpdateCardSetting(&macp->card_setting);
}           


void zd1205_set_preamble(struct zd1205_private *macp, u32 value)
{
	FPRINT("Set Preamble Type");
	macp->card_setting.PreambleType = value;
    zd_UpdateCardSetting(&macp->card_setting);
}


void acquire_ctrl_of_phy_req(void *regp)
{
    u32 tmpValue;

    tmpValue = readl(regp+CtlReg1);
    tmpValue &= ~0x80;
    writel(tmpValue, regp+CtlReg1);
}


void release_ctrl_of_phy_req(void *regp)
{
    u32 tmpValue;
	
    tmpValue = readl(regp+CtlReg1);
    tmpValue |= 0x80;
    writel(tmpValue, regp+CtlReg1);
}


void zd1205_dump_phy(struct zd1205_private *macp)
{
    void *regp = macp->regp;
	u32 regValue, regValue1;
	int i;

    for (i=0; i<204; i+=2){
        acquire_ctrl_of_phy_req(regp);
		regValue = readl(regp+4*i);
        regValue1 = readl(regp+4*(i+1));
		printk(KERN_DEBUG "CR%d = %04x,     CR%d = %04x\n", i, regValue,  i+1, regValue);
        release_ctrl_of_phy_req(regp);
		//regValue = readl(regp+4*(i+1));
		//printk(KERN_DEBUG "    CR%d = %04x\n", i+1, regValue);
	}
}


void zd1205_dbg_port(struct zd1205_private *macp, u32 value)
{
	void *regp = macp->regp;
	u32 tmp_value;

	tmp_value = readl(regp+RX_OFFSET_BYTE);
	writel((tmp_value | BIT_5), regp+RX_OFFSET_BYTE);
	writel(value, regp+Dbg_Select);
}


void zd1205_set_sc(struct zd1205_private *macp, u32 value)
{
	void *regp = macp->regp;
	u32 tmp_value;

	macp->card_setting.SwCipher = value;
	tmp_value = readl(regp+EncryType);
	if (value)
		tmp_value |= BIT_3;
	else
		tmp_value &= ~BIT_3;	

	writel(tmp_value, regp+EncryType);
    zd_UpdateCardSetting(&macp->card_setting);
}


extern void ShowHashInfo(u8 aid);
void zd1205_show_hash(struct zd1205_private *macp, u32 value)
{
	if (value < 33)
		ShowHashInfo(value);
}


void zd1205_show_card_setting(struct zd1205_private *macp)
{
  	printk(KERN_DEBUG "RTSThreshold = %04x\n", macp->card_setting.RTSThreshold);
	printk(KERN_DEBUG "FragThreshold = %04x\n", macp->card_setting.FragThreshold);
	printk(KERN_DEBUG "DtimPeriod = %04x\n", macp->card_setting.DtimPeriod);
	printk(KERN_DEBUG "EncryOnOff = %04x\n", macp->card_setting.EncryOnOff);
	printk(KERN_DEBUG "PreambleType = %04x\n", macp->card_setting.PreambleType);
	printk(KERN_DEBUG "EncryKeyId = %04x\n", macp->card_setting.EncryKeyId);
	printk(KERN_DEBUG "Channel = %04x\n", macp->card_setting.Channel);
	printk(KERN_DEBUG "BeaconInterval = %04x\n", macp->card_setting.BeaconInterval);
#ifdef ZD1205    
	printk(KERN_DEBUG "EncryMode = %04x\n", macp->card_setting.EncryMode);
#endif    
}  


int zd1205_zd_dbg_ioctl(struct zd1205_private *macp, struct zdap_ioctl *zdreq)
{
    void *regp = macp->regp;
    u16 zd_cmd;
    u32 tmp_value;
    
        zd_cmd = zdreq->cmd;
        
	switch(zd_cmd) {
        case ZD_IOCTL_REG_READ:
	        acquire_ctrl_of_phy_req(regp);
	        tmp_value = readl(regp + zdreq->addr);
	        release_ctrl_of_phy_req(regp);
	printk(KERN_INFO "Reg: 0x%08x = 0x%08x\n", zdreq->addr, tmp_value);
//	        zdreq->value = tmp_value;

            printk(KERN_DEBUG "zd1205 read register:  reg = %4x, value = %4x\n",
                zdreq->addr, zdreq->value);
	        //if (copy_to_user(ifr->ifr_data, &zdreq, sizeof (zdreq)))
	            //return -EFAULT;
	        break;

	    case ZD_IOCTL_REG_WRITE:
	        acquire_ctrl_of_phy_req(regp);
	        writel(zdreq->value, regp + zdreq->addr);
	        release_ctrl_of_phy_req(regp);
	printk(KERN_INFO "Write value: 0x%08x to Reg: 0x%08x\n", zdreq->value, zdreq->addr);
            if (zdreq->addr == RX_OFFSET_BYTE)
                macp->rx_offset = zdreq->value;
	        break;

	    case ZD_IOCTL_MEM_DUMP:
	        zd1205_dump_data("mem", (u8 *)zdreq->addr, zdreq->value);
	        //memcpy(&zdreq->data[0], (u8 *)zdreq->addr, zdreq->value);
	        //if (copy_to_user(ifr->ifr_data, &zdreq, sizeof (zdreq)))
	            //return -EFAULT;
	        break;

	    case ZD_IOCTL_REGS_DUMP:
	        zd1205_dump_regs(macp);
	        break;

        case ZD_IOCTL_DBG_MESSAGE:
            macp->dbg_flag =  zdreq->value;
            printk(KERN_DEBUG "zd1205: dbg_flag = %x\n", macp->dbg_flag);
            break;

        case ZD_IOCTL_DBG_COUNTER:
            zd1205_dump_cnters(macp);
            break;

        case ZD_IOCTL_LB_MODE:
            macp->lb_mode = zdreq->value;
            printk(KERN_DEBUG "zd1205: lb_mode = %x\n", macp->lb_mode);
            zd1205_lb_mode(macp);
            break;

        case ZD_IOCTL_TX_ON:
            macp->txOn = zdreq->value;
            printk(KERN_DEBUG "zd1205: txOn = %x\n", macp->txOn);
            break;

        case ZD_IOCTL_SNIFFER:
            macp->sniffer_on = zdreq->value;
            printk(KERN_DEBUG "zd1205: sniffer_on = %x\n", macp->sniffer_on);
            zd1205_set_sniffer_mode(macp);
            break;

        case ZD_IOCTL_WEP_ON_OFF:
            printk(KERN_DEBUG "zd1205: wep_on_off = %x\n", zdreq->value);
            zd1205_wep_on_off(macp, zdreq->value);
            break;

#if 0                        
        case ZD_IOCTL_CAM:
            printk(KERN_DEBUG "zd1205: cam = %x\n", zdreq->value);
            zd1205_do_cam(macp, zdreq->value);
            break;

        case ZD_IOCTL_VAP:
            printk(KERN_DEBUG "zd1205: vap\n");
            zd1205_do_vap(macp, zdreq->value);
            break;

        case ZD_IOCTL_APC:
            printk(KERN_DEBUG "zd1205: apc\n");
            zd1205_do_apc(macp, zdreq->value);
            break;    

        case ZD_IOCTL_CAM_DUMP:
            printk(KERN_DEBUG "zd1205: cam_dump\n");
            zd1205_cam_dump(macp);
            break;

        case ZD_IOCTL_CAM_CLEAR:
            printk(KERN_DEBUG "zd1205: cam_clear\n");
            zd1205_cam_clear(macp);
            break;
#endif            
#if 0
        case ZD_IOCTL_WDS:
            printk(KERN_DEBUG "zd1205: wds\n");
            zd1205_do_wds(macp);
            break;
#endif
        case ZD_IOCTL_FIFO:
            printk(KERN_DEBUG "zd1205: fifo\n");
            zd1205_dbg_fifo(macp);
            break;

        case ZD_IOCTL_FRAG:
            printk(KERN_DEBUG "zd1205: frag = %x\n", zdreq->value);
            zd1205_set_frag(macp, zdreq->value);
            break;

        case ZD_IOCTL_RTS:
            printk(KERN_DEBUG "zd1205: rts = %x\n", zdreq->value);
            zd1205_set_rts(macp, zdreq->value);
            break;

        case ZD_IOCTL_PREAMBLE:
            printk(KERN_DEBUG "zd1205: pre = %x\n", zdreq->value);
            zd1205_set_preamble(macp, zdreq->value);
            break;

        case ZD_IOCTL_DUMP_PHY:
            printk(KERN_DEBUG "zd1205: dump phy\n");
            zd1205_dump_phy(macp);
            break;

        case ZD_IOCTL_DBG_PORT:
            printk(KERN_DEBUG "zd1205: port = %x\n", zdreq->value);
            zd1205_dbg_port(macp, zdreq->value);
            break;

        case ZD_IOCTL_CARD_SETTING:
            printk(KERN_DEBUG "zd1205: card setting\n");
            zd1205_show_card_setting(macp);
            break;

        case ZD_IOCTL_RESET:
            printk(KERN_DEBUG "zd1205: card reset\n");
            zd1205_sleep_reset(macp);
            break;
            
        case ZD_IOCTL_SW_CIPHER:
            printk(KERN_DEBUG "zd1205: sc = %x\n", zdreq->value);
            zd1205_set_sc(macp, zdreq->value);
            break;    
            
        case ZD_IOCTL_HASH_DUMP:
            printk(KERN_DEBUG "zd1205: aid = %x\n", zdreq->value);
            zd1205_show_hash(macp, zdreq->value);
            break;    

	case ZD_IOCTL_RFD_DUMP:
            printk(KERN_DEBUG "===== zd1205 rfd dump =====\n");
            zd1205_dump_rfds(macp);
            break;

	case ZD_IOCTL_CHANNEL:
	    printk(KERN_DEBUG "zd1205: channel = %d\n", zdreq->value);
	    macp->card_setting.Channel = zdreq->value;
	    zd_UpdateCardSetting(&macp->card_setting);
	    break;

	case ZD_IOCTL_MEM_READ: {
	    unsigned int *p;
	    p = (unsigned int *) (0xc0000000 | zdreq->addr);
//	    p = (unsigned int *) bus_to_virt(zdreq->addr);
//	    printk(KERN_DEBUG "zd1205: bus: 0x%08x, virt: 0x%08x\n", zdreq->addr, p);
	    printk(KERN_DEBUG "zd1205: read memory addr: 0x%08x value: 0x%08x\n", zdreq->addr, *p);
	    break;
	}

	case ZD_IOCTL_MEM_WRITE: {
	    u32 *p;
	    p = (unsigned int *) (0xc0000000 | zdreq->addr);
//	    p = (u32 *) bus_to_virt(zdreq->addr);
	    *p = zdreq->value;
	    printk(KERN_DEBUG "zd1205: write value: 0x%08x to memory addr: 0x%08x\n", zdreq->value, zdreq->addr);
	    break;
	}	

#if 0	
	case ZD_IOCLT_SET_KEYID:
            printk(KERN_DEBUG "zd1205: Set WEP KeyID = %d\n", zdreq->value);
            macp->card_setting.EncryKeyId = zdreq->value;
            zd_UpdateCardSetting(&macp->card_setting);            
	    break;

        case ZD_IOCTL_GET_KEYID:
            printk(KERN_DEBUG "zd1205: Get WEP KeyID = %d\n", macp->card_setting.EncryKeyId);
	    break;
#endif

	    default :
            printk(KERN_ERR "zd1205: error command = %x\n", zd_cmd);
	        break;

	}
    
    return 0;
    
}    




//CAM supported functions
#ifdef ZD1205

//for Pure AP
u8 apMAC[3][6] = {
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	{0x00, 0x60, 0xb3, 0x66, 0x37, 0xff},
	{0x00, 0x60, 0xb3, 0x11, 0x22, 0x33}
};


u8 apEncryType[3][3] = {
	{ENCRY_WEP64, ENCRY_WEP64, ENCRY_TKIP},
	{ENCRY_WEP128, ENCRY_WEP128, ENCRY_TKIP},
	{ENCRY_WEP64, ENCRY_WEP128, ENCRY_TKIP}
	//{ENCRY_WEP64, ENCRY_TKIP, ENCRY_TKIP},
	//{ENCRY_WEP64, ENCRY_WEP256, ENCRY_WEP256}
};


u8 apKey[3][32] = {
	{'3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3',
	'3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3'},
	{'1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1',
	'1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1'},
	{'3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3',
	'3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3'}
};


//for Virtual AP
u8 vapMAC[5][6] = {
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	{0x00, 0x60, 0xb3, 0x66, 0x37, 0xff},
	{0x00, 0x60, 0xb3, 0x12, 0x34, 0x56},
	{0x00, 0x60, 0xb3, 0x22, 0x33, 0x44},
	{0x00, 0x60, 0xb3, 0x33, 0x44, 0x55}

};


u8 vapEncryType[5] = {
	ENCRY_NO, ENCRY_WEP64, ENCRY_WEP64, ENCRY_WEP128, ENCRY_WEP64
};


u8 vapKey[5][32] = { //group key
	{'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
	'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'},
	{'1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1',
	'1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1'},
	{'2', '2', '2', '2', '2', '2', '2', '2', '2', '2', '2', '2', '2', '2', '2', '2',
	'2', '2', '2', '2', '2', '2', '2', '2', '2', '2', '2', '2', '2', '2', '2', '2'},
	{'1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1',
	'1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1'},
	{'3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3',
	'3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3'}

};


//for AP Client
u8 apcMAC[4][6] = {
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	{0x00, 0x60, 0xb3, 0x12, 0x34, 0x56},
	{0x00, 0x60, 0xb3, 0x22, 0x33, 0x44},
	{0x00, 0x60, 0xb3, 0x33, 0x44, 0x55}
};


u8 wdsEncryType[2] = {
	ENCRY_WEP64, ENCRY_WEP64
};

//for WDS
u8 wdsMAC[2][6] = {
	{0x00, 0x60, 0xb3, 0xff, 0xff, 0xff},
	{0x00, 0x60, 0xb3, 0x12, 0x34, 0x56}
};                                      


u8 wdsKey[2][32] = { //group key
	{'1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1',
	'1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1'},
	{'1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1',
	'1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1'}
};


u32 MICErrCntTable[MAX_USER];


void zd1205_cam_clear(struct zd1205_private *macp)
{
	int i;

    FPRINT("Clear CAM memory");
	for (i=0; i<424; i++)
		zd1205_cam_write(macp, i, 0);
}


void zd1205_cam_dump(struct zd1205_private *macp)
{
	u32 data;
	int i;

    FPRINT("Dump CAM memory");
	for (i=0; i<424; i++){
		data = zd1205_cam_read(macp, i);
        //FPRINT_V("data", data);
		if (i==0)
			FPRINT("Addr = 0x0000");
		printk(KERN_DEBUG "%08x", data);
		if (i>0 && ((i+1)%4 == 0)){
			printk(KERN_DEBUG "\n");
			printk(KERN_DEBUG "Addr = %04x\n", i+1);
		}
	}
	printk(KERN_DEBUG "\n");
}


void zd1205_get_ap(struct zd1205_private *macp, u32 n)
{
	u8 keyLength;
	int i;

	FPRINT("*****zd1205_get_ap was called");
	//n--;
	for (i=0; i<2; i++){
		cam_get_mac(macp, i, &apMAC[i][0]);
		keyLength = cam_get_encry_type(macp, i);
		if (keyLength)
			cam_set_key(macp, i, keyLength, &apKey[i][0]);
	}
}


void zd1205_get_mic_error_count(struct zd1205_private *macp)
{
	void *regp = macp->regp;
	u32 counterWordAddr;
	u32 counterByteOffset;
	u32 tmpValue;
	u8 counter;
	u8 userCount = 0;
	u8 userTable[32];
	int i, n;

	FPRINT("*****zd1205_get_mic_error_count was called");

	tmpValue = readl(regp+DECRY_ERR_FLG_LOW);
	if (tmpValue == 0)
		return;

	for (i=0; i<32; i++){
		if (tmpValue & BIT_0){
			userTable[userCount] = i+1;
			userCount++;
		}
		else
			tmpValue >>= 1;
	}

	for (i=0; i<userCount; i++){
		n = userTable[i];
		cam_get_mac(macp, n, &apMAC[i][0]);

		//overflow counter
		counterWordAddr = COUNTER_START_ADDR + (n/8);
		counterByteOffset = (n/2 + n%2)/4;
		FPRINT_V("counterWordAddr", counterWordAddr);
		FPRINT_V("counterByteOffset", counterByteOffset);

		tmpValue = zd1205_cam_read(macp, counterWordAddr);
		counter = (u8)(tmpValue >> (counterByteOffset*8));
		if (n%2)
			counter &= 0x0f; //get low part
		else
			counter >>= 4; //get hignt part
		MICErrCntTable[n] += counter;
	}
}


#if 0
void zd1205_set_wds(struct zd1205_private *macp, u32 n)
{
	void *regp = macp->regp;
	int index;
	u8 keyLength;
	static int firstTime = 1;

	FPRINT("*****zd1205_set_wds was called");
	if ((n > MAX_USER) || (n == 0))
		return;

	index = n-1;
	//CAMresetRollTbl(macp);
	if (firstTime){
		writel(CAM_AP_WDS, regp+CAM_MODE); //virtual AP Mode
		macp->card_setting.OperationMode = CAM_AP_WDS;
		firstTime = 0;
	}

	cam_set_mac(macp, n-1, &wdsMAC[index][0]);
	keyLength = cam_set_encry_type(macp, n-1, wdsEncryType[index]);
	FPRINT_V("keyLength", keyLength);
	if (keyLength){
		cam_set_key(macp, n-1, keyLength, &wdsKey[index][0]);
		zd1205_dump_data("Dynamic Keys = ", (u8 *)&wdsKey[index][0], keyLength);
	}
	cam_update_roll_tbl(macp, n-1);
}


void zd1205_get_wds(struct zd1205_private *macp, u32 n)
{
	void *regp = macp->regp;
	int index;
	u8 keyLength;

	FPRINT("****zd1205_get_wds was called");
	index = n-1;
	cam_get_mac(macp, n-1, &wdsMAC[index][0]);
	keyLength = cam_get_encry_type(macp, n-1);
	if (keyLength)
		cam_get_key(macp, n-1, keyLength, &wdsKey[index][0]);
}
#endif


void zd1205_set_ap(struct zd1205_private *macp, u32 n) //n means case 1, 2, 3...
{
	void *regp = macp->regp;
	u8 keyLength;
	int i;
	static int firstTime = 1;

	FPRINT("*****zd1205_set_ap was called");
     if (firstTime){
		writel(CAM_AP, regp+CAM_MODE); //AP Mode
		macp->card_setting.OperationMode = CAM_AP;
        zd1205_wep_on_off(macp, 1);
		firstTime = 0;
	}

	for (i=0; i<2; i++){
		cam_set_mac(macp, i, &apMAC[i][0]);
		keyLength = cam_set_encry_type(macp, i, apEncryType[n-1][i]);
		FPRINT_V("keyLength", keyLength);
		if (keyLength){
			cam_set_key(macp, i, keyLength, &apKey[i][0]);
			zd1205_dump_data("Dynamic Keys = ", (u8 *)&apKey[i][0], keyLength);
		}
		cam_update_roll_tbl(macp, i);
	}
}


void zd1205_set_vap(struct zd1205_private *macp, u32 n)
{
	void *regp = macp->regp;
	int index;
	u8 keyLength;
	static int firstTime = 1;

	FPRINT("\r\n*****zd1205_SetVAP was called");
	if ((n > MAX_USER) || (n == 0))
		return;

	if (n >= 33)
		index = n-31;	//from user 33 to 40
	else
		index = n-1;

	if (firstTime){
		writel(CAM_AP_VAP, regp+CAM_MODE); //virtual AP Mode
		macp->card_setting.OperationMode = CAM_AP_VAP;
        zd1205_wep_on_off(macp, 1);
		firstTime = 0;
	}

	cam_set_mac(macp, n-1, &vapMAC[index][0]);
	keyLength = cam_set_encry_type(macp, n-1, vapEncryType[index]);
	FPRINT_V("keyLength", keyLength);
	if (keyLength){
		cam_set_key(macp, n-1, keyLength, &vapKey[index][0]);
		zd1205_dump_data("Dynamic Keys = ", (u8 *)&vapKey[index][0], keyLength);
	}
	cam_update_roll_tbl(macp, n-1);
}


void zd1205_get_vap(struct zd1205_private *macp, u32 n)
{
	int index;
	u8 keyLength;

	FPRINT("*****zd1205_GetVAP was called");
	if (n >= 33)
		index = n-31;	//from user 33 to 40
	else index = n-1;

	cam_get_mac(macp, n-1, &vapMAC[index][0]);
	keyLength = cam_get_encry_type(macp, n-1);
	if (keyLength)
		cam_get_key(macp, n-1, keyLength, &vapKey[index][0]);
}


void zd1205_set_apc(struct zd1205_private *macp, u32 n)
{
	void *regp = macp->regp;
	int index;
	static int firstTime = 1;

	FPRINT("*****zd1205_SetAPC was called");
	if ((n > MAX_USER) || (n == 0))
		return;

	index = n-1;
	if (firstTime){
		writel(CAM_AP_CLIENT, regp+CAM_MODE); //AP Client
		macp->card_setting.OperationMode = CAM_AP_CLIENT;
        zd1205_wep_on_off(macp, 1);
		firstTime = 0;
	}
	cam_set_mac(macp, index, &apcMAC[index][0]);
	cam_update_roll_tbl(macp, index);
}


void zd1205_get_apc(struct zd1205_private *macp, u32 n)
{
	int index;

	FPRINT("*****zd1205_GetAPC was called");
	index = n-1;
	cam_get_mac(macp, index, &apcMAC[index][0]);
}


void zd1205_set_op_mode(struct zd1205_private *macp, u32 mode)
{
	void *regp = macp->regp;

	switch(mode){
		case CAM_IBSS:
			FPRINT("*****Switch to IBSS Mode\n");
			break;

		case CAM_AP:
			FPRINT("*****Switch to AP Mode\n");
			break;

		case CAM_STA:
			FPRINT("*****Switch to STA Mode\n");
			break;

		case CAM_AP_WDS:
			FPRINT("*****Switch to WDS Mode\n");
			break;

		case CAM_AP_CLIENT:
			FPRINT("\r\n*****Switch to AP Client Mode\n");
			break;

		case CAM_AP_VAP:
			FPRINT("*****Switch to Virtual Mode Mode\n");
			break;

		default:
			FPRINT("*****Unsupported Mode\n");
			return;
	}

	writel(mode, regp+CAM_MODE);
}
#endif


void zd1205_lb_mode(struct zd1205_private *macp)
{
    void *regp = macp->regp;
    u32 tmp_value;
    u8 keyLength;
	int i;
	u8 encryMode;

    if (macp->lb_mode == 0){
        acquire_ctrl_of_phy_req(regp);
        writel(0x08, regp + ZD1205_CR47);
        writel(0x08, regp + ZD1205_CR33);
        release_ctrl_of_phy_req(regp);
        //enable beacon
        tmp_value = readl(regp+BCNInterval);
        tmp_value |= AP_MODE;
        writel(tmp_value, regp + BCNInterval);

        return;
    }

    tmp_value = readl(regp+BCNInterval);
    tmp_value &= ~AP_MODE;
    writel(tmp_value, regp + BCNInterval);
    acquire_ctrl_of_phy_req(regp);
    writel(0x18, regp + ZD1205_CR47);  //for digital loopback
    writel(0x2c, regp + ZD1205_CR33);

    release_ctrl_of_phy_req(regp);

#ifdef ZD1205
	if (macp->lb_mode > 5){
		FPRINT("Enable Loopback Mode");
		return;
	}

	switch(macp->lb_mode)
	{
		case 1:
			FPRINT("nSet Loopback Mode for TKIP");
			encryMode = ENCRY_TKIP;
			break;

		case 2:
			FPRINT("nSet Loopback Mode for CCMP");
			encryMode = ENCRY_CCMP;
			break;

		case 3:
			FPRINT("Set Loopback Mode for WEP256");
			encryMode = ENCRY_WEP256;
			break;

		case 4:
			FPRINT("Set Loopback Mode for WEP64");
			encryMode = ENCRY_WEP64;
			break;

		case 5:
			FPRINT("Set Loopback Mode for WEP128");
			encryMode = ENCRY_WEP128;
			break;

        default:
            FPRINT("Unknown Loopback Mode");
            return;  
	}

	//mac_p->cardSetting.EncryMode = encryMode;
	zd1205_wep_on_off(macp, 1);

	if (encryMode == ENCRY_WEP256){
		//&& (mac_p->cardSetting.OperationMode != CAM_STA) ){ //use static keys
		rid_t rid_s;
		RidStatus status;

		writel(0, regp+CAM_ROLL_TB_LOW); //let CAM to use default keys
		writel(0, regp+CAM_ROLL_TB_HIGH);
		macp->card_setting.EncryMode = encryMode;
		zd1205_cam_write(macp, DEFAULT_ENCRY_TYPE, encryMode);

		rid_s.rid = ZD_RID_CFG_WEP_DEFAULT_KEY;
		status = zd_GetRID(&rid_s);
		if (status == RID_STATUS_SUCCESS){
			FPRINT_V("Rid->length", rid_s.length);
			memcpy((u8 *)&macp->key_vector, (u8 *)&rid_s.u.data[1], rid_s.u.data[0]);
		}

		zd1205_config_static_key(macp);
		zd1205_get_static_key(macp);
	}
	else
	{	//TKIP, CCMP
		for (i=0; i<3; i+=2){ //item 0 & 2
			macp->card_setting.EncryMode = ENCRY_WEP64;
			cam_set_mac(macp, i, &apMAC[i][0]);
			keyLength = cam_set_encry_type(macp, i, encryMode);
			FPRINT_V("keyLength", keyLength);
			if (keyLength){
				cam_set_key(macp, i, keyLength, &apKey[i][0]);
				zd1205_dump_data("Dynamic Keys", (u8 *)&apKey[i][0], keyLength);
			}

			cam_update_roll_tbl(macp, i);
			cam_get_mac(macp, i, &apMAC[i][0]);
			keyLength = cam_get_encry_type(macp, i);
			if (keyLength)
				cam_get_key(macp, i, keyLength, &apKey[i][0]);
		}
	}
#endif
}


#ifdef ZD1205
void zd1205_do_cam(struct zd1205_private *macp, u32 value)
{
   zd1205_set_ap(macp, value);
   zd1205_get_ap(macp, value);
}


void zd1205_do_vap(struct zd1205_private *macp, u32 value)
{
   zd1205_set_vap(macp, 1);  //BC
   zd1205_get_vap(macp, 1);
   
   zd1205_set_vap(macp, 2);  //STA
   zd1205_get_vap(macp, 2);
   
   zd1205_set_vap(macp, 33); //AP1
   zd1205_get_vap(macp, 33);
   
   zd1205_set_vap(macp, 34); //AP2
   zd1205_get_vap(macp, 34);
   
   zd1205_set_vap(macp, 35); //AP3
   zd1205_get_vap(macp, 35);
}


void zd1205_do_apc(struct zd1205_private *macp, u32 value)
{
   zd1205_set_apc(macp, 0);  //BC
   zd1205_get_apc(macp, 0);

   zd1205_set_apc(macp, 1);  //STA1
   zd1205_get_apc(macp, 1);

   zd1205_set_apc(macp, 2);  //STA2
   zd1205_get_apc(macp, 2);

   zd1205_set_apc(macp, 3); //STA3
   zd1205_get_apc(macp, 3);
}
#endif

#if 0
void zd1205_do_wds(struct zd1205_private *macp, u32 value)
{
   zd1205_set_wds(macp, 1);  //TA
   zd1205_get_wds(macp, 1);

   zd1205_set_wds(macp, 2);  //RA
   zd1205_get_wds(macp, 2);

}
#endif

#endif 





       

