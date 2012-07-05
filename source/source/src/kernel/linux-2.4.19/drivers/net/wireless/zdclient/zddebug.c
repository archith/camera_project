#ifndef _ZD_DEBUG_C_
#define _ZD_DEBUG_C_

#include "zddebug.h"


//for debug message show
extern u32 freeSignalCount;
extern u32 freeFdescCount;
extern void ShowQInfo(void);
extern unsigned short mlme_dbg_level;
extern void zd1205_config_wep_keys(struct zd1205_private *macp);

int send_debug=0;
int send_enc_debug=0;

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

    printk("**** macp->regp 0x%x *******************************************\n",(int)regp);
	printk("InterruptCtrl(0x0510)      = %08x, PCI_TxAddr_p1(0x600)= %08x\n", 
	    readl(regp+InterruptCtrl),readl(regp+PCI_TxAddr_p1));

	zd1205_disable_int();
    printk("ReadTcbAddress(0x6e8)     = %08x, ReadRfdAddress(0x6ec)= %04x\n",
        readl(regp+ReadTcbAddress), readl(regp+ReadRfdAddress));
//	printk("MACAddr_P1 = %04x    MACAddr_P2 = %04x\n",
//        readl(regp+MACAddr_P1), readl(regp+MACAddr_P2));
	printk("BCNInterval(0x520)        = %08x. BCNPLCPCfg(0x620)    = %04x\n",
        readl(regp+BCNInterval), readl(regp+BCNPLCPCfg));
	printk("TSF_LowPart(0x514)        = %08x, TSF_HighPart  = %04x\n",
        readl(regp+TSF_LowPart), readl(regp+TSF_HighPart));
	printk("BCN_FIFO_Semaphore(0x694) = %08x, CtlReg1(0x680)       = %04x\n",
        readl(regp+BCN_FIFO_Semaphore),  readl(regp+CtlReg1));
	printk("DeviceState(0x684)        = %08x, NAV_CCA(0x6c8)       = %04x\n",
        readl(regp+DeviceState), readl(regp+NAV_CCA));
	printk("Rx_Filter(0x68c)          = %04x\n", readl(regp+Rx_Filter));
	printk("CRC32Cnt(0x6a4)           = %08x, CRC16Cnt(0x6a8)      = %04x\n",
        readl(regp+CRC32Cnt), readl(regp+CRC16Cnt));
	printk("TotalRxFrm(0x6a0)         = %08x, TotalTxFrm(0x6f4)    = %04x\n",
        readl(regp+TotalRxFrm), readl(regp+TotalTxFrm));
	printk("RxFIFOOverrun(0x6b0)      = %08x, UnderrunCnt(0x688)   = %04x\n",
        readl(regp+RxFIFOOverrun), readl(regp+UnderrunCnt));
	printk("DecrypErr_UNI(0x6ac)      = %08x, DecrypErr_Mul(0x6bc) = %04x\n",
        readl(regp+DecrypErr_UNI), readl(regp+DecrypErr_Mul));
#if 0        
    printk("RX_OFFSET_BYTE     = %08x, RX_TIME_OUT   = %04x\n",
        readl(regp+RX_OFFSET_BYTE), readl(regp+RX_TIME_OUT));
	printk("CAM_DEBUG          = %08x, CAM_STATUS    = %04x\n",
        readl(regp+CAM_DEBUG), readl(regp+CAM_STATUS));
	printk("CAM_ROLL_TB_LOW    = %08x, CAM_ROLL_TB_HIGH = %04x\n",
        readl(regp+CAM_ROLL_TB_LOW), readl(regp+CAM_ROLL_TB_HIGH));
    printk("CAM_MODE           = %08x\n", readl(regp+CAM_MODE));
#endif    
	zd1205_enable_int();
}


void zd1205_dump_cnters(struct zd1205_private *macp)
{
    zd1205_lock(macp);
    printk("*******************************************\n");
	printk("freeTxQ         = %04x, activeTxQ      = %04x\n", macp->freeTxQ->count, macp->activeTxQ->count);
    printk("freeSignalCount = %04x, freeFdescCount = %04x\n", freeSignalCount, freeFdescCount);
    printk("bcnCnt          = %04x, dtimCnt        = %04x\n", macp->bcnCnt, macp->dtimCnt);
    printk("txCnt           = %04x, txCmpCnt       = %04x\n", macp->txCnt, macp->txCmpCnt);
    printk("retryFailCnt    = %04x, rxCnt          = %04x\n", macp->retryFailCnt, macp->rxCnt);
    printk("TxRx_DeadLock   = %04x, HMAC_NoPhyClk  = %04x\n", macp->HMAC_TxRx_DeadLock, macp->HMAC_NoPhyClk);
    printk("HMAC PCI Hang-up= %04x, TX Timeout Cnt = %04x\n", macp->HMAC_PciHang, macp->HMAC_TxTimeout);
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
		printk("CR%d = %04x,     CR%d = %04x\n", i, regValue,  i+1, regValue);
		release_ctrl_of_phy_req(regp);
		//regValue = readl(regp+4*(i+1));
		//printk("    CR%d = %04x\n", i+1, regValue);
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
printk("zd1205_set_sc\n");
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
  	printk("RTSThreshold = %04x\n", macp->card_setting.RTSThreshold);
	printk("FragThreshold = %04x\n", macp->card_setting.FragThreshold);
	printk("DtimPeriod = %04x\n", macp->card_setting.DtimPeriod);
	printk("EncryOnOff = %04x\n", macp->card_setting.EncryOnOff);
	printk("PreambleType = %04x\n", macp->card_setting.PreambleType);
	printk("EncryKeyId = %04x\n", macp->card_setting.EncryKeyId);
	printk("Channel = %04x\n", macp->card_setting.Channel);
	printk("BeaconInterval = %04x\n", macp->card_setting.BeaconInterval);
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

            printk("zd1205 read register:  reg = %4x, value = %4x\n",
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
#if 1 //ivan
{
    mlme_dbg_level=zdreq->value;
    MLME_ioctl(&macp->mlme,MLME_IOCTL_DEBUG_PRINT,&zdreq->value);
    //extern void sim_printmlme(mlme_data_t *mlme_p,int);    
//            sim_printmlme(&macp->mlme,zdreq->value);
}
#else
            macp->dbg_flag =  zdreq->value;
            printk("zd1205: dbg_flag = %x\n", macp->dbg_flag);
#endif            
            break;

        case ZD_IOCTL_DBG_COUNTER:
            zd1205_dump_cnters(macp);
            break;

        case ZD_IOCTL_LB_MODE:
            macp->lb_mode = zdreq->value;
            printk("zd1205: lb_mode = %x\n", macp->lb_mode);
            zd1205_lb_mode(macp);
            break;

        case ZD_IOCTL_TX_ON:
            macp->txOn = zdreq->value;
            printk("zd1205: txOn = %x\n", macp->txOn);
            break;

	case ZD_IOCTL_RATE:
            /* Check for the validation of vale */
            if(zdreq->value > 3 || zdreq->value < 0)
            {
                printk("zd1205: Basic Rate %x doesn't support\n", zdreq->value);
                break;
            }
                                                                                       
            printk("zd1205: Basic Rate = %x\n", zdreq->value);
            zd1205_update_brate(macp, zdreq->value);
            break;

        case ZD_IOCTL_SNIFFER:
            macp->sniffer_on = zdreq->value;
            printk("zd1205: sniffer_on = %x\n", macp->sniffer_on);
            zd1205_set_sniffer_mode(macp);
            break;

        case ZD_IOCTL_WEP_ON_OFF:
            printk("zd1205: wep_on_off = %x\n", zdreq->value);
            zd1205_wep_on_off(macp, zdreq->value);
            break;

#if 0                        
        case ZD_IOCTL_CAM:
            printk("zd1205: cam = %x\n", zdreq->value);
            zd1205_do_cam(macp, zdreq->value);
            break;

        case ZD_IOCTL_VAP:
            printk("zd1205: vap\n");
            zd1205_do_vap(macp, zdreq->value);
            break;

        case ZD_IOCTL_APC:
            printk("zd1205: apc\n");
            zd1205_do_apc(macp, zdreq->value);
            break;    

        case ZD_IOCTL_CAM_DUMP:
            printk("zd1205: cam_dump\n");
            zd1205_cam_dump(macp);
            break;

        case ZD_IOCTL_CAM_CLEAR:
            printk("zd1205: cam_clear\n");
            zd1205_cam_clear(macp);
            break;
#endif            
#if 0
        case ZD_IOCTL_WDS:
            printk("zd1205: wds\n");
            zd1205_do_wds(macp);
            break;
#endif
        case ZD_IOCTL_FIFO:
            printk("zd1205: fifo\n");
            zd1205_dbg_fifo(macp);
            break;

        case ZD_IOCTL_FRAG:
            printk("zd1205: frag = %x\n", zdreq->value);
            zd1205_set_frag(macp, zdreq->value);
            break;

        case ZD_IOCTL_RTS:
            printk("zd1205: rts = %x\n", zdreq->value);
            zd1205_set_rts(macp, zdreq->value);
            break;

        case ZD_IOCTL_PREAMBLE:
            printk("zd1205: pre = %x\n", zdreq->value);
            zd1205_set_preamble(macp, zdreq->value);
            break;

        case ZD_IOCTL_DUMP_PHY:
            printk("zd1205: dump phy\n");
            zd1205_dump_phy(macp);
            break;

        case ZD_IOCTL_DBG_PORT:
            printk("zd1205: port = %x\n", zdreq->value);
            zd1205_dbg_port(macp, zdreq->value);
            break;

        case ZD_IOCTL_CARD_SETTING:
            printk("zd1205: card setting\n");
            zd1205_show_card_setting(macp);
            break;

        case ZD_IOCTL_RESET:
            printk("zd1205: card reset\n");
            zd1205_sleep_reset(macp);
            break;
            
        case ZD_IOCTL_SW_CIPHER:
            printk("zd1205: sc = %x\n", zdreq->value);
            zd1205_set_sc(macp, zdreq->value);
            break;    
            
        case ZD_IOCTL_HASH_DUMP:
            printk("zd1205: aid = %x\n", zdreq->value);
            zd1205_show_hash(macp, zdreq->value);
            break;    

        case ZD_IOCTL_RFD_DUMP:
            printk("===== zd1205 rfd dump =====\n");
            zd1205_dump_rfds(macp);
            break;
                                            
        case ZD_IOCTL_CHANNEL:
            printk("zd1205: channel = %d\n", zdreq->value);
            macp->card_setting.Channel = zdreq->value;
            zd_UpdateCardSetting(&macp->card_setting);
            break;
        
        case ZD_IOCTL_MEM_READ: {
            u32 *p;
            p = (u32 *) bus_to_virt(zdreq->addr);
            printk("zd1205: read memory addr: 0x%08x value: 0x%08x\n", zdreq->addr, *p);
            break;
        }

        case ZD_IOCTL_MEM_WRITE: {
            u32 *p;
            p = (u32 *) bus_to_virt(zdreq->addr);
            *p = zdreq->value;
            printk("zd1205: write value: 0x%08x to memory addr: 0x%08x\n", zdreq->value, zdreq->addr);
            break;
        }

        case ZD_IOCTL_KEY_ID:
            printk("zd1205: keyid = %d\n", zdreq->value);
            macp->card_setting.EncryKeyId = zdreq->value;
            break;

        case ZD_IOCTL_KEY: {
            if(zdreq->value < 0 || zdreq->value > 3)
            {
                printk("zd1205: Only KeyIds between 0 and 3 are support\n");
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
                printk("zd1205: Encrypt Mode: %d not support\n", zdreq->value);
                break;
            }
                                                                            
            printk("zd1205: encrypt mode = %d\n", zdreq->value);
            macp->card_setting.EncryMode = zdreq->value;
            break;

        case ZD_IOCTL_SSID:
            if(zd1205_set_ssid(macp, zdreq->data, zdreq->value) == 0)
            {
                printk("zd1205: SSID = %s\n", zdreq->data);
                zd_UpdateCardSetting(&macp->card_setting);
            }
            break;


        case ZD_IOCTL_AUTH:
            if(zdreq->value < 0 || zdreq->value > 2)
                printk("zd1205: Auth Type: %d doesn't support\n", zdreq->value);
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
	//u8 keyLength;
//	int i;
//	u8 encryMode;

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
