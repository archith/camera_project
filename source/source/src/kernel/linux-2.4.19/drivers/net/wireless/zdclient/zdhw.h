#ifndef __ZDHW_H__
#define __ZDHW_H__
#include "zdapi.h"

#define		ZD_CR0			0x0000
#define		ZD_CR1			0x0004
#define		ZD_CR2			0x0008
#define		ZD_CR3			0x000C
#define		ZD_CR5			0x0010
#define		ZD_CR6			0x0014
#define		ZD_CR7			0x0018
#define		ZD_CR8			0x001C
#define		ZD_CR4			0x0020
#define		ZD_CR9			0x0024
#define		ZD_CR10			0x0028
#define		ZD_CR11			0x002C
#define		ZD_CR12			0x0030
#define		ZD_CR13			0x0034
#define		ZD_CR14			0x0038
#define		ZD_CR15			0x003C
#define		ZD_CR16			0x0040
#define		ZD_CR17			0x0044
#define		ZD_CR18			0x0048
#define		ZD_CR19			0x004C
#define		ZD_CR20			0x0050
#define		ZD_CR21			0x0054
#define		ZD_CR22			0x0058
#define		ZD_CR23			0x005C
#define		ZD_CR24			0x0060
#define		ZD_CR25			0x0064
#define		ZD_CR26			0x0068
#define		ZD_CR27			0x006C
#define		ZD_CR28			0x0070
#define		ZD_CR29			0x0074
#define		ZD_CR30			0x0078
#define		ZD_CR31			0x007C
#define		ZD_CR32			0x0080
#define		ZD_CR33			0x0084
#define		ZD_CR34			0x0088
#define		ZD_CR35			0x008C
#define		ZD_CR36			0x0090
#define		ZD_CR37			0x0094
#define		ZD_CR38			0x0098
#define		ZD_CR39			0x009C
#define		ZD_CR40			0x00A0
#define		ZD_CR41			0x00A4
#define		ZD_CR42			0x00A8
#define		ZD_CR43			0x00AC
#define		ZD_CR44			0x00B0
#define		ZD_CR45			0x00B4
#define		ZD_CR46			0x00B8
#define		ZD_CR47			0x00BC
#define		ZD_CR48			0x00C0
#define		ZD_CR49			0x00C4
#define		ZD_CR50			0x00C8
#define		ZD_CR51			0x00CC
#define		ZD_CR52			0x00D0
#define		ZD_CR53			0x00D4
#define		ZD_CR54			0x00D8
#define		ZD_CR55			0x00DC
#define		ZD_CR56			0x00E0
#define		ZD_CR57			0x00E4
#define		ZD_CR58			0x00E8
#define		ZD_CR59			0x00EC
#define		ZD_CR60			0x00F0
#define		ZD_CR61			0x00F4
#define		ZD_CR62			0x00F8
#define		ZD_CR63			0x00FC
#define		ZD_CR64			0x0100
#define		ZD_CR65			0x0104
#define		ZD_CR66			0x0108
#define		ZD_CR67			0x010C
#define		ZD_CR68			0x0110
#define		ZD_CR69			0x0114
#define		ZD_CR70			0x0118
#define		ZD_CR71			0x011C
#define		ZD_CR72			0x0120
#define		ZD_CR73			0x0124
#define		ZD_CR74			0x0128
#define		ZD_CR75			0x012C
#define		ZD_CR76			0x0130
#define		ZD_CR77			0x0134
#define		ZD_CR78			0x0138
#define		ZD_CR79			0x013C
#define		ZD_CR80			0x0140
#define		ZD_CR81			0x0144
#define		ZD_CR82			0x0148
#define		ZD_CR83			0x014C
#define		ZD_CR84			0x0150
#define		ZD_CR85			0x0154
#define		ZD_CR86			0x0158
#define		ZD_CR87			0x015C
#define		ZD_CR88			0x0160
#define		ZD_CR89			0x0164
#define		ZD_CR90			0x0168
#define		ZD_CR91			0x016C
#define		ZD_CR92			0x0170
#define		ZD_CR93			0x0174
#define		ZD_CR94			0x0178
#define		ZD_CR95			0x017C
#define		ZD_CR96			0x0180
#define		ZD_CR97			0x0184
#define		ZD_CR98			0x0188
#define		ZD_CR99			0x018C
#define		ZD_CR100		0x0190
#define		ZD_CR101		0x0194
#define		ZD_CR102		0x0198
#define		ZD_CR103		0x019C
#define		ZD_CR104		0x01A0
#define		ZD_CR105		0x01A4
#define		ZD_CR106		0x01A8
#define		ZD_CR107		0x01AC
#define		ZD_CR108		0x01B0
#define		ZD_CR109		0x01B4
#define		ZD_CR110		0x01B8
#define		ZD_CR111		0x01BC
#define		ZD_CR112		0x01C0
#define		ZD_CR113		0x01C4
#define		ZD_CR114		0x01C8
#define		ZD_CR115		0x01CC
#define		ZD_CR116		0x01D0
#define		ZD_CR117		0x01D4
#define		ZD_CR118		0x01D8
#define		ZD_CR119		0x01DC
#define		ZD_CR120		0x01E0
#define		ZD_CR121		0x01E4
#define		ZD_CR122		0x01E8
#define		ZD_CR123		0x01EC
#define		ZD_CR124		0x01F0
#define		ZD_CR125		0x01F4
#define		ZD_CR126		0x01F8
#define		ZD_CR127		0x01FC
#define		ZD_RF_IF_CLK			0x0400
#define		ZD_RF_IF_DATA			0x0404
#define		ZD_PE1_PE2				0x0408
#define		ZD_PE2_DLY				0x040C
#define		ZD_LE1					0x0410
#define		ZD_LE2					0x0414
#define		ZD_GPI_EN				0x0418
#define		ZD_RADIO_PD				0x042C
#define		ZD_RF2948_PD			0x042C
#define		ZD_LED1					0x0430
#define		ZD_LED2					0x0434
#define		ZD_EnablePSManualAGC	0x043C	// 1: enable
#define		ZD_CONFIGPhilips		0x0440
#define		ZD_SA2400_SER_AP		0x0444
#define		ZD_I2C_WRITE			0x0444	// Same as SA2400_SER_AP (for compatible with ZD1201)
#define		ZD_SA2400_SER_RP		0x0448
#define		ZD_AfterPNP				0x0454
#define		ZD_RADIO_PE				0x0458
#define		ZD_RstBusMaster			0x045C

#define		ZD_RFCFG				0x0464

#define		ZD_HSTSCHG				0x046C

#define		ZD_PHY_ON				0x0474
#define		ZD_RX_DELAY				0x0478
#define		ZD_RX_PE_DELAY			0x047C


#define		ZD_GPIO_1				0x0490
#define		ZD_GPIO_2				0x0494


#define		ZD_EncryBufMux			0x04A8


#define		ZD_PS_Ctrl				0x0500
#define		ZD_ADDA_PwrDwn_Ctrl		0x0504
#define		ZD_ADDA_MBIAS_WarmTime	0x0508
#define		ZD_MAC_PS_STATE			0x050C
#define		ZD_InterruptCtrl		0x0510
#define		ZD_TSF_LowPart			0x0514
#define		ZD_TSF_HighPart			0x0518
#define		ZD_ATIMWndPeriod		0x051C
#define		ZD_BCNInterval			0x0520
#define		ZD_Pre_TBTT				0x0524	//In unit of TU(1024us)

#define		ZD_PCI_TxAddr_p1		0x0600
#define		ZD_PCI_TxAddr_p2		0x0604
#define		ZD_PCI_RxAddr_p1		0x0608
#define		ZD_PCI_RxAddr_p2		0x060C
#define		ZD_MACAddr_P1			0x0610
#define		ZD_MACAddr_P2			0x0614
#define		ZD_BSSID_P1				0x0618
#define		ZD_BSSID_P2				0x061C
#define		ZD_BCNPLCPCfg			0x0620
#define		ZD_GroupHash_P1			0x0624
#define		ZD_GroupHash_P2			0x0628
#define		ZD_WEPTxIV				0x062C

#define		ZD_PHYDelay				0x066C
#define		ZD_BCNFIFO				0x0670
#define		ZD_SnifferOn			0x0674
#define		ZD_EncryType			0x0678
#define		ZD_RetryMAX				0x067C
#define		ZD_CtlReg1				0x0680	//Bit0:		IBSS mode
										//Bit1:		PwrMgt mode
										//Bit2-4 :	Highest basic rate
										//Bit5:		Lock bit
										//Bit6:		PLCP weight select
										//Bit7:		PLCP switch
#define		ZD_DeviceState			0x0684
#define		ZD_UnderrunCnt			0x0688
#define		ZD_Rx_Filter			0x068c
#define		ZD_Ack_Timeout_Ext		0x0690
#define		ZD_BCN_FIFO_Semaphore	0x0694
#define		ZD_IFS_Value			0x0698
#define		ZD_RX_TIME_OUT			0x069C
#define		ZD_TotalRxFrm			0x06A0
#define		ZD_CRC32Cnt				0x06A4
#define		ZD_CRC16Cnt				0x06A8
#define		ZD_DecrypErr_UNI		0x06AC
#define		ZD_RxFIFOOverrun		0x06B0

#define		ZD_DecrypErr_Mul		0x06BC

#define		ZD_NAV_CNT				0x06C4
#define		ZD_NAV_CCA				0x06C8
#define		ZD_RetryCnt				0x06CC

#define		ZD_ReadTcbAddress		0x06E8
#define		ZD_ReadRfdAddress		0x06EC
#define		ZD_CWmin_CWmax			0x06F0
#define		ZD_TotalTxFrm			0x06F4

#define		ZD_CAM_MODE				0x0700
#define		ZD_CAM_ROLL_TB_LOW		0x0704
#define		ZD_CAM_ROLL_TB_HIGH		0x0708
#define		ZD_CAM_ADDRESS			0x070C
#define		ZD_CAM_DATA				0x0710

#define		ZD_ROMDIR				0x0714

#define		ZD_WEPKey0				0x0720
#define		ZD_WEPKey1				0x0724
#define		ZD_WEPKey2				0x0728
#define		ZD_WEPKey3				0x072C
#define		ZD_WEPKey4				0x0730
#define		ZD_WEPKey5				0x0734
#define		ZD_WEPKey6				0x0738
#define		ZD_WEPKey7				0x073C
#define		ZD_WEPKey8				0x0740
#define		ZD_WEPKey9				0x0744
#define		ZD_WEPKey10				0x0748
#define		ZD_WEPKey11				0x074C
#define		ZD_WEPKey12				0x0750
// yarco for TKIP support
#define		ZD_WEPKey13				0x0754
#define		ZD_WEPKey14				0x0758
#define		ZD_WEPKey15				0x075c
#define		ZD_TKIP_MODE			0x0760

#define		ZD_EEPROM_PROTECT0		0x0758
#define		ZD_EEPROM_PROTECT1		0x075C

#define		ZD_Dbg_FIFO_Rd			0x0800
#define		ZD_Dbg_Select			0x0804
#define		ZD_FIFO_Length			0x0808


//#define		RF_Mode					0x080C
#define		ZD_RSSI_MGC				0x0810

#define		ZD_PON					0x0818
#define		ZD_Rx_ON				0x081C
#define		ZD_Tx_ON				0x0820
#define		ZD_CHIP_EN				0x0824
#define		ZD_LO_SW				0x0828
#define		ZD_TxRx_SW				0x082C
#define		ZD_S_MD					0x0830

// EEPROM Memmory Map Region
#define		ZD_E2P_SUBID			0x0900
#define		ZD_E2P_POD				0x0904
#define		ZD_E2P_MACADDR_P1		0x0908
#define		ZD_E2P_MACADDR_P2		0x090C
#define     ZD_E2P_PWR_CAL_VALUE1   0x0910
#define     ZD_E2P_PWR_CAL_VALUE2   0x0914
#define     ZD_E2P_PWR_CAL_VALUE3   0x0918
#define     ZD_E2P_PWR_CAL_VALUE4   0x091c
#define		ZD_E2P_PWR_INT_VALUE	0x0920
#define		ZD_E2P_ALLOWED_CHANNEL	0x0930
#define		ZD_E2P_PHY_REG			0x0934
#define		ZD_E2P_REGION_CODE		0x0960
#define		ZD_E2P_FEATURE_BITMAP	0x0964	


void HW_Set_Intersil_Chips(zd_80211Obj_t *pObj, U32 ChannelNo, U8 InitChOnly);
void HW_Set_GCT_Chips(zd_80211Obj_t *pObj, U32 ChannelNo, U8 InitChOnly);
void LockPhyReg(zd_80211Obj_t *pObj);
void UnLockPhyReg(zd_80211Obj_t *pObj);
void HW_SetSupportedRate(zd_80211Obj_t *pObj, U8 *prates);
void HW_EnableBeacon(zd_80211Obj_t *pObj, U16 BecaonInterval, U16 DtimPeriod); 
U32 HW_GetNow(zd_80211Obj_t *pObj);
void HW_GetTsf(zd_80211Obj_t *pObj, U32 *loTsf, U32 *hiTsf);
void HW_SetBeaconFIFO(zd_80211Obj_t *pObj, U8 *pBeacon, U16 index);
void HW_SwitchChannel(zd_80211Obj_t *pObj, U16 channel);
int HW_HTP(zd_80211Obj_t *pObj);
void HW_RadioOnOff(zd_80211Obj_t *pObj, U8 on);
void HW_Set_IF_Synthesizer(zd_80211Obj_t *pObj, U32 InputValue);
void HW_ResetPhy(zd_80211Obj_t *pObj);
void HW_InitHMAC(zd_80211Obj_t *pObj);
void HW_UpdateIntegrationValue(zd_80211Obj_t *pObj, U32 ChannelNo);
void HW_WritePhyReg(zd_80211Obj_t *pObj, U8 PhyIdx, U8 PhyValue);
void HW_OverWritePhyRegFromE2P(zd_80211Obj_t *pObj);
void HW_E2P_AutoPatch(zd_80211Obj_t *pObj);
void HW_SetSTA_PS(zd_80211Obj_t *pObj, U8 op);
void HW_Write_TxGain(zd_80211Obj_t *pObj, U32 txgain);
void HW_Set_FilterBand(zd_80211Obj_t *pObj, U32	region_code);
void HW_UpdateATIMWindow(zd_80211Obj_t *pObj, U16 AtimWnd);
void HW_UpdatePreTBTT(zd_80211Obj_t *pObj, U32 pretbtt);
#endif
