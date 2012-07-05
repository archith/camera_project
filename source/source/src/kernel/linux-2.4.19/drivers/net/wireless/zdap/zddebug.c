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

	zd1205_disable_int();
    printk(KERN_DEBUG "*******************************************\n");
	printk(KERN_DEBUG "InterruptCtrl      = %08x \n", readl(regp+InterruptCtrl));
    printk(KERN_DEBUG "ReadTcbAddress     = %08x, ReadRfdAddress= %04x\n",
        readl(regp+ReadTcbAddress), readl(regp+ReadRfdAddress));
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
#if 0        
    printk(KERN_DEBUG "RX_OFFSET_BYTE     = %08x, RX_TIME_OUT   = %04x\n",
        readl(regp+RX_OFFSET_BYTE), readl(regp+RX_TIME_OUT));
	printk(KERN_DEBUG "CAM_DEBUG          = %08x, CAM_STATUS    = %04x\n",
        readl(regp+CAM_DEBUG), readl(regp+CAM_STATUS));
	printk(KERN_DEBUG "CAM_ROLL_TB_LOW    = %08x, CAM_ROLL_TB_HIGH = %04x\n",
        readl(regp+CAM_ROLL_TB_LOW), readl(regp+CAM_ROLL_TB_HIGH));
    printk(KERN_DEBUG "CAM_MODE           = %08x\n", readl(regp+CAM_MODE));
#endif    
	zd1205_enable_int();
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


void zd1205_update_brate(struct zd1205_private *macp, u32 value)
{
	u8 ii;
	u8 nRate;
	u8 *pRate;
	card_Setting_t *pSetting = &macp->card_setting;
	u8 rate_list[4] = { 0x02, 0x04, 0x0B, 0x16 };
                                        
	/* Get the number of rates we support */
	nRate = pSetting->Info_SupportedRates[1];
	pRate = &(pSetting->Info_SupportedRates[2]);
                                                                
	for(ii = 0; ii < nRate; ii++)
	{
		/* If the rate is less than the basic rate, mask 0x80 with the value. */
		if((*pRate & 0x7f) <= rate_list[value])
			*pRate |= 0x80;
		else
			*pRate &= 0x7f;
		
		pRate++;
	}
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

	for (i=0; i<204; i+=2) {
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
	u32 tmpValue;

	macp->card_setting.SwCipher = value;
	tmpValue = readl(regp+EncryType);
	if (value)
		tmpValue |= BIT_3; 
	else
		tmpValue &= ~BIT_3; 

	writel(tmpValue, regp+EncryType);
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
}  

int zd1205_set_ssid(struct zd1205_private *macp, char *ssid, int len)
{
	int ii;
	char *p;
                
	/* Check for the validation of length */
	if(len > 34 || len < 0)
	{
		printk(KERN_INFO "Illegal SSID length: %d\n", len);
		return -1;
	}
                                                                        
	macp->card_setting.Info_SSID[0] = 0;
	macp->card_setting.Info_SSID[1] = len;
	
	p = &macp->card_setting.Info_SSID[2];
	
	for(ii = 0; ii < len; ii++)
		*p++ = ssid[ii];
                                                                                                                        
	return 0;
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
	        zdreq->value = tmp_value;

            printk(KERN_DEBUG "zd1205 read register:  reg = %4x, value = %4x\n",
                zdreq->addr, zdreq->value);
	        //if (copy_to_user(ifr->ifr_data, &zdreq, sizeof (zdreq)))
	            //return -EFAULT;
	        break;

	    case ZD_IOCTL_REG_WRITE:
	        acquire_ctrl_of_phy_req(regp);
	        writel(zdreq->value, regp + zdreq->addr);
	        release_ctrl_of_phy_req(regp);
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

	case ZD_IOCTL_RATE:
            /* Check for the validation of vale */
            if(zdreq->value > 3 || zdreq->value < 0)
            {
                printk(KERN_DEBUG "zd1205: Basic Rate %x doesn't support\n", zdreq->value);
                break;
            }
                                                                                       
            printk(KERN_DEBUG "zd1205: Basic Rate = %x\n", zdreq->value);
            zd1205_update_brate(macp, zdreq->value);
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
            u32 *p;
            p = (u32 *) bus_to_virt(zdreq->addr);
            printk(KERN_DEBUG "zd1205: read memory addr: 0x%08x value: 0x%08x\n", zdreq->addr, *p);
            break;
        }

        case ZD_IOCTL_MEM_WRITE: {
            u32 *p;
            p = (u32 *) bus_to_virt(zdreq->addr);
            *p = zdreq->value;
            printk(KERN_DEBUG "zd1205: write value: 0x%08x to memory addr: 0x%08x\n", zdreq->value, zdreq->addr);
            break;
        }

        case ZD_IOCTL_KEY_ID:
            printk(KERN_DEBUG "zd1205: keyid = %d\n", zdreq->value);
            macp->card_setting.EncryKeyId = zdreq->value;
            break;

        case ZD_IOCTL_KEY: {
            if(zdreq->value < 0 || zdreq->value > 3)
            {
                printk(KERN_DEBUG "zd1205: Only KeyIds between 0 and 3 are support\n");
                break;
            }

            /* We always copy the maximum key length to the key descriptor */
            memcpy(macp->card_setting.keyVector[zdreq->value], zdreq->data, 16);
            zd1205_config_wep_keys(macp);
        }
        break;            

        case ZD_IOCTL_ENC_MODE:
            if(zdreq->value < WEP64 || zdreq->value > WEP128)
            {
                printk(KERN_DEBUG "zd1205: Encrypt Mode: %d not support\n", zdreq->value);
                break;
            }
                                                                            
            printk(KERN_DEBUG "zd1205: encrypt mode = %d\n", zdreq->value);
            macp->card_setting.EncryMode = zdreq->value;
            break;

        case ZD_IOCTL_SSID:
            if(zd1205_set_ssid(macp, zdreq->data, zdreq->value) == 0)
            {
                printk(KERN_DEBUG "zd1205: SSID = %s\n", zdreq->data);
                zd_UpdateCardSetting(&macp->card_setting);
            }
            break;


        case ZD_IOCTL_AUTH:
            if(zdreq->value < 0 || zdreq->value > 2)
                printk(KERN_DEBUG "zd1205: Auth Type: %d doesn't support\n", zdreq->value);
            else
                macp->card_setting.AuthMode = zdreq->value;
            break;

        default :
            printk(KERN_ERR "zd1205: error command = %x\n", zd_cmd);
	        break;
	}
    
    return 0;
    
}    

void zd1205_lb_mode(struct zd1205_private *macp)
{
	void *regp = macp->regp;
	u32 tmp_value;
	u8 keyLength;
	int i;
	u8 encryMode;

	if (macp->lb_mode == 0) {
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
}

#endif 
