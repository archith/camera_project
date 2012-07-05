#ifndef __ZDHW_C__
#define __ZDHW_C__

#include "zdtypes.h"
#include "zdequates.h"
#include "zdapi.h"
#include "zdhw.h"
#include "zddebug.h"


#define NUM_REG_MASK ( sizeof (MacRegMaskTab) / sizeof (MAC_REGISTER_MASK_TBL) )

typedef struct _MAC_REGISTER_MASK_TBL {

    U32		Address;
	U32		ReadWriteMask;

} MAC_REGISTER_MASK_TBL;

MAC_REGISTER_MASK_TBL MacRegMaskTab[ ] = {
	//Address				Mask
	// GPIO
	ZD_RF_IF_CLK,				0x1,		//0x0400
	ZD_RF_IF_DATA,				0x1,		//0x0404
	ZD_PE1_PE2,					0x83,		//0x0408
	ZD_PE2_DLY,					0xff,		//0x040C
	ZD_LE1,						0x1,		//0x0410
	ZD_LE2,						0x1,		//0x0414
	ZD_GPI_EN,					0x0,		//0x0418
	ZD_RADIO_PD,				0x1,		//0x042C
	ZD_RF2948_PD,				0x1,		//0x042C
	ZD_LED1,					0x1,		//0x0430
	ZD_LED2,					0x1,		//0x0434

	ZD_EnablePSManualAGC,		0x0,		//0x043C	// 1: enable
	ZD_CONFIGPhilips,			0x3,		//0x0440

	ZD_SA2400_SER_AP,			0x0,		//0x0444
	ZD_I2C_WRITE,				0x0,		//0x0444	// Same as SA2400_SER_AP (for compatible with ZD1201)

	ZD_SA2400_SER_RP,			0x0,		//0x0448


	ZD_AfterPNP,				0x1,		//0x0454
	ZD_RADIO_PE,				0x0,		//0x0458
	ZD_RstBusMaster,			0x0,		//0x045C

	ZD_RFCFG,					0x7,		//0x0464
	ZD_HSTSCHG,					0x0,		//0x046C

	ZD_PHY_ON,					0x0,		//0x0474
	ZD_RX_DELAY,				0x0,		//0x0478
	ZD_RX_PE_DELAY,				0xff,		//0x047C


	ZD_GPIO_1,					0x0,		//0x0490
	ZD_GPIO_2,					0x0,		//0x0494


	ZD_EncryBufMux,				0x0,		//0x04A8

	// Power Management Registers
	ZD_PS_Ctrl,					0x0,		//0x0500

	ZD_ADDA_MBIAS_WarmTime,		0x0,		//0x0508

	ZD_InterruptCtrl,			0x0,		//0x0510
	ZD_TSF_LowPart,				0x0,		//0x0514
	ZD_TSF_HighPart,			0x0,		//0x0518
	ZD_ATIMWndPeriod,			0xffff,		//0x051C
	ZD_BCNInterval,				0xffffff,	//0x0520
	ZD_Pre_TBTT,				0xffff,		//0x0524	//In unit of TU(1024us)

	// General Registers
	ZD_PCI_TxAddr_p1,			0x00000000,
	ZD_PCI_TxAddr_p2,			0xffffffff,
	ZD_PCI_RxAddr_p1,			0x00000000,
	ZD_PCI_RxAddr_p2,			0xffffffff,
	//ZD_MACAddr_P1,				0x00000000,
	//ZD_MACAddr_P2,				0x00000000,
	ZD_BSSID_P1,				0xffffffff,
	ZD_BSSID_P2,				0x0000ffff,
	ZD_BCNPLCPCfg,				0xffffffff,
	ZD_GroupHash_P1,			0xffffffff,
	ZD_GroupHash_P2,			0xffffffff,
	ZD_PHYDelay,				0x000000ff,
	ZD_BCNFIFO,					0x000000ff,
	ZD_SnifferOn,				0x00000001,
	ZD_EncryType,				0x0000000f,
	ZD_RetryMAX,				0x0000001f,
	ZD_CtlReg1,					0x000003fc,
	ZD_DeviceState,				0x00000000,
	ZD_UnderrunCnt,				0x00000000,
	ZD_Rx_Filter,				0xffffffff,
	ZD_Ack_Timeout_Ext,			0x0000001f,
	ZD_BCN_FIFO_Semaphore,		0x00000001,
//	ZD_IFS_Value,				0x0fffffff,
	ZD_TotalRxFrm,				0x00000000,
	ZD_CRC32Cnt,				0x00000000,
	ZD_CRC16Cnt,				0x00000000,
	ZD_DecrypErr_UNI,			0x00000000,
	ZD_RxFIFOOverrun,			0x00000000,
	ZD_DecrypErr_Mul,			0x00000000,
	ZD_NAV_CNT,					0x00000000,
	ZD_NAV_CCA,					0x00000000,
	ZD_RetryCnt,				0x00000000,
	ZD_ReadTcbAddress,			0x00000000,
	ZD_ReadRfdAddress,			0x00000000,
	ZD_CWmin_CWmax,				0x03ff7fff,
	ZD_TotalTxFrm,				0x00000000,
};


U32 GRF5101T[] = {
    0x1A0000,   //Null 
    0x1A0000,   //Ch 1
    0x1A8000,   //Ch 2
    0x1A4000,   //Ch 3
    0x1AC000,   //Ch 4
    0x1A2000,   //Ch 5
    0x1AA000,   //Ch 6
    0x1A6000,   //Ch 7
    0x1AE000,   //Ch 8
    0x1A1000,   //Ch 9
    0x1A9000,   //Ch 10
    0x1A5000,   //Ch 11
    0x1AD000,   //Ch 12
    0x1A3000,   //Ch 13
    0x1AB000    //Ch 14
};


U32 AL2210TB[] = {
    0x2396c0,   //;Null 
    0x0196c0,   //;Ch 1
    0x019710,   //;Ch 2
    0x019760,   //;Ch 3
    0x0197b0,   //;Ch 4
    0x019800,   //;Ch 5
    0x019850,   //;Ch 6
    0x0198a0,   //;Ch 7
    0x0198f0,   //;Ch 8
    0x019940,   //;Ch 9
    0x019990,   //;Ch 10
    0x0199e0,   //;Ch 11
    0x019a30,   //;Ch 12
    0x019a80,   //;Ch 13
    0x019b40    //;Ch 14
};

#if 0
int HW_HTP(zd_80211Obj_t *pObj)
{
	void *reg = pObj->reg;
	int i, ret = 0;
	U32 tmpkey, tmpvalue, regvalue, seed;
	
	dbg_pline_1("\r\nHW_HTP Starting....");
	
	// PHY CR Registers Read/Write Test
	dbg_pline_1("\r\nPHY CR Registers Read/Write Test Starting....");
	
	seed = pObj->GetReg(reg, ZD_TSF_LowPart);
	srand(seed);
	LockPhyReg(pObj);
	
	for (i=0; i<0x0200; i+=4){
		if ( (i==0x00) || ((i>=0xc8) && (i<=0xfc)) ||
			((i>=0x1cc) && (i<=0x1d8)) || ((i>=0x1e0) && (i<=0x1ec))){
			// Skip Read Only Register
			continue;
		}
		tmpkey = (U8)rand();
		pObj->SetReg(reg, i, tmpkey);
		tmpvalue = pObj->GetReg(reg, i);
		if (tmpvalue != tmpkey){
			//printf("CR %x Failed (Wr: %x, Rd: %x)\n", i, tmpkey, tmpvalue);
			dbg_plinew_1("\r\nCR ", i);
			dbg_pline_1(" Failed ");
			dbg_plineb_1("(Wr: ", (U8)tmpkey);
			dbg_plineb_1(", Rd: ", (U8)tmpvalue);
			dbg_pline_1(")");
			UnLockPhyReg(pObj);
			ret = 1;
		}
		else{
			//printf("CR %x Success (Wr: %x, Rd: %x)\n", i, tmpkey, tmpvalue);
			dbg_plinew_1("\r\nCR ", i);
			dbg_pline_1(" Success ");
			dbg_plineb_1("(Wr: ", (U8)tmpkey);
			dbg_plineb_1(", Rd: ", (U8)tmpvalue);
			dbg_pline_1(")");
		}
	}
					
	UnLockPhyReg(pObj);
	dbg_pline_1("\r\nPHY CR Registers Read/Write Test End");
	dbg_pline_1("\r\n");

#if 1	
	// MAC Registers Read/Write Test
	dbg_pline_1("\r\nMAC Registers Read/Write Test Starting....");
	//to test 0x408, 0x410, 0x42c must set 0x418 to 0
	pObj->SetReg(reg, ZD_GPI_EN, 0);
	seed = pObj->GetReg(reg, ZD_TSF_LowPart);
	srand(seed);
	for (i=0; i<NUM_REG_MASK; i++){
		tmpkey = (U32)rand();
		tmpkey |= (tmpkey << 16);
		tmpkey &= MacRegMaskTab[i].ReadWriteMask;
		
		if (MacRegMaskTab[i].Address == 0x42c){
			pObj->SetReg(reg, ZD_GPI_EN, 0);
		}	
		
		pObj->SetReg(reg, MacRegMaskTab[i].Address, tmpkey);
		tmpvalue = pObj->GetReg(reg, MacRegMaskTab[i].Address);				
		tmpvalue &= MacRegMaskTab[i].ReadWriteMask;
		if (tmpvalue != tmpkey){
			//printf("MAC %x Failed (Wr: %x, Rd: %x)\n", MacRegMaskTab[i].Address, tmpkey, tmpvalue);
			dbg_plinew_1("\r\nMAC ", MacRegMaskTab[i].Address);
			dbg_pline_1(" Failed ");
			dbg_plinel_1("(Wr: ", tmpkey);
			dbg_plinel_1(", Rd: ", tmpvalue);
			dbg_pline_1(")");
			ret = 2;
		}
		else{
			//printf("MAC %x Success (Wr: %x, Rd: %x)\n", MacRegMaskTab[i].Address, tmpkey, tmpvalue);
			dbg_plinew_1("\r\nMAC ", MacRegMaskTab[i].Address);
			dbg_pline_1(" Success ");
			dbg_plinel_1("(Wr: ", tmpkey);
			dbg_plinel_1(", Rd: ", tmpvalue);
			dbg_pline_1(")");
		}

	}
	dbg_pline_1("\r\nMAC Registers Read/Write Test End");
	dbg_pline_1("\r\n");
#endif
	
#if 0	
	// EEPROM Read/Write Test
	dbg_pline_1("\r\nEEPROM Read/Write Testing...........");
	seed = pObj->GetReg(reg, ZD_TSF_LowPart);
	srand(seed);
	//for (tmpvalue=0; tmpvalue<1; tmpvalue++){
	{
		tmpkey = (U32)rand();
		tmpkey |= (tmpkey << 16);
		for (i=0; i<256; i++){
			//if (i == 1)
				//tmpkey = 0x89;
			pObj->SetReg(reg, ZD_E2P_SUBID+(i*4), tmpkey);
		}
		// Write to EEPROM
		pObj->SetReg(reg, ZD_EEPROM_PROTECT0, 0x55aa44bb);
		pObj->SetReg(reg, ZD_EEPROM_PROTECT1, 0x33cc22dd);				
		pObj->SetReg(reg, ZD_ROMDIR, 0x422);					

		// Sleep 
		//for (i=0; i<1000; i++)
		//	pObj->DelayUs(5000);
		delay1ms(5);	

		// Reset Registers
		for (i=0; i<256; i++){
			pObj->SetReg(reg, ZD_E2P_SUBID+(i*4), 0);
		}

		// Reload EEPROM
		pObj->SetReg(reg, ZD_ROMDIR, 0x424);

		// Sleep
		//for (i=0; i<1000; i++)
		//	pObj->DelayUs(5000);
		delay1ms(5);

		// Check if right
		for (i=0; i<256; i++){
			regvalue = pObj->GetReg(reg, ZD_E2P_SUBID+(i*4));
			if (regvalue != tmpkey){
				//printf("EEPROM Addr (%x) error (Wr: %x, Rd: %x)\n", ZD_E2P_SUBID+(i*4), tmpkey, regvalue);
				dbg_plinew_1("\r\nEEPROM Addr ", ZD_E2P_SUBID+(i*4));
				dbg_pline_1(" error ");
				dbg_plinel_1("(Wr: ", tmpkey);
				dbg_plinel_1(",Rd: ", regvalue);
				dbg_pline_1(")");
				ret = 3;	
			}
		}
	}	
#endif

	//dbg_pline_1("\r\nDigital Loopback Testing...........");
	
	dbg_pline_1("\r\nHW_HTP End");
	dbg_pline_1("\r\n");	
	return 0;			
}	
#endif


void
HW_Set_IF_Synthesizer(zd_80211Obj_t *pObj, U32 InputValue)
{
	U32	S_bit_cnt;
	void *reg = pObj->reg;
	
	S_bit_cnt = pObj->S_bit_cnt;
	InputValue = InputValue << (31 - S_bit_cnt);
	
	pObj->SetReg(reg, ZD_LE2, 0);
	pObj->SetReg(reg, ZD_RF_IF_CLK, 0);
	
	while(S_bit_cnt){
		InputValue = InputValue << 1;
		if (InputValue & 0x80000000){
			pObj->SetReg(reg, ZD_RF_IF_DATA, 1);
		}
		else{
			pObj->SetReg(reg, ZD_RF_IF_DATA, 0);
		}
		pObj->SetReg(reg, ZD_RF_IF_CLK, 1);
		//pObj->DelayUs(50);
		pObj->SetReg(reg, ZD_RF_IF_CLK, 0);
		//pObj->DelayUs(50);
		S_bit_cnt --;
	}
	
	pObj->SetReg(reg, ZD_LE2, 1);
	
	if (pObj->S_bit_cnt == 20){			//Is it Intersil's chipset
		pObj->SetReg(reg, ZD_LE2, 0);
	}
	return;
}


void
LockPhyReg(zd_80211Obj_t *pObj)
{
	void *reg = pObj->reg;
	U32	tmpvalue;
	
	tmpvalue = pObj->GetReg(reg, ZD_CtlReg1);
	tmpvalue &= ~0x80;
	pObj->SetReg(reg, ZD_CtlReg1, tmpvalue);
}


void
UnLockPhyReg(zd_80211Obj_t *pObj)
{
	void *reg = pObj->reg;
	U32	tmpvalue;

	tmpvalue = pObj->GetReg(reg, ZD_CtlReg1);
	tmpvalue |= 0x80;
	pObj->SetReg(reg, ZD_CtlReg1, tmpvalue);
}


void
HW_Set_GCT_Chips(zd_80211Obj_t *pObj, U32 ChannelNo, U8 InitChOnly)
{
	void *reg = pObj->reg;
	
	LockPhyReg(pObj);
	pObj->SetReg(reg, ZD_CR47, 0x1c);
	pObj->SetReg(reg, ZD_CR15, 0xdc);
	pObj->SetReg(reg, ZD_CR113, 0xc0); //3910
	pObj->SetReg(reg, ZD_CR20, 0x0c);
	pObj->SetReg(reg, ZD_CR17, 0x65);
	pObj->SetReg(reg, ZD_CR34, 0x04);
	pObj->SetReg(reg, ZD_CR35, 0x35);
	pObj->SetReg(reg, ZD_CR24, 0x20);
	pObj->SetReg(reg, ZD_CR9, 0xe0);
	pObj->SetReg(reg, ZD_CR127, 0x02);
	pObj->SetReg(reg, ZD_CR10, 0x91);
	pObj->SetReg(reg, ZD_CR23, 0x7f);
	pObj->SetReg(reg, ZD_CR27, 0x10);
	pObj->SetReg(reg, ZD_CR28, 0x7a);
	pObj->SetReg(reg, ZD_CR79, 0xb5);
	pObj->SetReg(reg, ZD_CR64, 0x80);
	//++ Enable D.C cancellation (CR33 Bit_5) to avoid
	//	 CCA always high.
	pObj->SetReg(reg, ZD_CR33, 0x28);
	//--                  

	pObj->SetReg(reg, ZD_CR38, 0x30);
	UnLockPhyReg(pObj);
	
	HW_Set_IF_Synthesizer(pObj, 0x1F0000);
	HW_Set_IF_Synthesizer(pObj, 0x1F0000);
	HW_Set_IF_Synthesizer(pObj, 0x1F0200);
	HW_Set_IF_Synthesizer(pObj, 0x1F0600);
	HW_Set_IF_Synthesizer(pObj, 0x1F8600);
	HW_Set_IF_Synthesizer(pObj, 0x1F8600);
	HW_Set_IF_Synthesizer(pObj, 0x002050);
	HW_Set_IF_Synthesizer(pObj, 0x1F8000);
	HW_Set_IF_Synthesizer(pObj, 0x1F8200);
	HW_Set_IF_Synthesizer(pObj, 0x1F8600);
	HW_Set_IF_Synthesizer(pObj, 0x1c0000);
	HW_Set_IF_Synthesizer(pObj, 0x10c458);
	HW_Set_IF_Synthesizer(pObj, 0x088e92);
	HW_Set_IF_Synthesizer(pObj, 0x187b82);
	HW_Set_IF_Synthesizer(pObj, 0x0401b4);
	HW_Set_IF_Synthesizer(pObj, 0x140816);
	HW_Set_IF_Synthesizer(pObj, 0x0c7000);
	HW_Set_IF_Synthesizer(pObj, 0x1c0000);
	HW_Set_IF_Synthesizer(pObj, 0x02ccae);
	HW_Set_IF_Synthesizer(pObj, 0x128023);
	HW_Set_IF_Synthesizer(pObj, 0x0a0000);
	HW_Set_IF_Synthesizer(pObj, GRF5101T[ChannelNo]);
	HW_Set_IF_Synthesizer(pObj, 0x06e380);
	HW_Set_IF_Synthesizer(pObj, 0x16cb94);
	HW_Set_IF_Synthesizer(pObj, 0x0e1740);
	HW_Set_IF_Synthesizer(pObj, 0x014980);
	HW_Set_IF_Synthesizer(pObj, 0x116240);
	HW_Set_IF_Synthesizer(pObj, 0x090000);
	HW_Set_IF_Synthesizer(pObj, 0x192304);
	HW_Set_IF_Synthesizer(pObj, 0x05112f);
	HW_Set_IF_Synthesizer(pObj, 0x0d54a8);
	HW_Set_IF_Synthesizer(pObj, 0x0f8000);
	HW_Set_IF_Synthesizer(pObj, 0x1c0008);
	HW_Set_IF_Synthesizer(pObj, 0x1c0000);
	HW_Set_IF_Synthesizer(pObj, GRF5101T[ChannelNo]);
	HW_Set_IF_Synthesizer(pObj, 0x1c0008);
	HW_Set_IF_Synthesizer(pObj, 0x150000);
	HW_Set_IF_Synthesizer(pObj, 0x0c7000);
	HW_Set_IF_Synthesizer(pObj, 0x150800);
	HW_Set_IF_Synthesizer(pObj, 0x150000);
}


void
HW_Set_AL2210MPVB_Chips(zd_80211Obj_t *pObj, U32 ChannelNo, U8 InitChOnly)
{
	void *reg = pObj->reg;
	U32	tmpvalue;

	pObj->SetReg(reg, ZD_PE1_PE2, 2);

	LockPhyReg(pObj);
	pObj->SetReg(reg, ZD_CR9, 0xe0);
	pObj->SetReg(reg, ZD_CR10, 0x91);
	pObj->SetReg(reg, ZD_CR12, 0x90);
	pObj->SetReg(reg, ZD_CR15, 0xd0);
	pObj->SetReg(reg, ZD_CR16, 0x40);
	pObj->SetReg(reg, ZD_CR17, 0x58);
	pObj->SetReg(reg, ZD_CR18, 0x04);
	pObj->SetReg(reg, ZD_CR23, 0x66);
	pObj->SetReg(reg, ZD_CR24, 0x14);
	pObj->SetReg(reg, ZD_CR26, 0x90);
	pObj->SetReg(reg, ZD_CR27, 0x30);
	pObj->SetReg(reg, ZD_CR31, 0x80);
	pObj->SetReg(reg, ZD_CR34, 0x06);
	pObj->SetReg(reg, ZD_CR35, 0x3e);
	pObj->SetReg(reg, ZD_CR38, 0x38);
	pObj->SetReg(reg, ZD_CR46, 0x90);
	pObj->SetReg(reg, ZD_CR47, 0x10);
	pObj->SetReg(reg, ZD_CR64, 0x64);
	pObj->SetReg(reg, ZD_CR79, 0xb5);
	pObj->SetReg(reg, ZD_CR80, 0x38);
	pObj->SetReg(reg, ZD_CR81, 0x30);
	pObj->SetReg(reg, ZD_CR113, 0xc0);
	pObj->SetReg(reg, ZD_CR127, 0x03);
	UnLockPhyReg(pObj);
	
	HW_Set_IF_Synthesizer(pObj, AL2210TB[ChannelNo]);
	HW_Set_IF_Synthesizer(pObj, 0x007cb1);
	HW_Set_IF_Synthesizer(pObj, 0x358372);
	HW_Set_IF_Synthesizer(pObj, 0x0109b3);
	HW_Set_IF_Synthesizer(pObj, 0x472804);
	HW_Set_IF_Synthesizer(pObj, 0x456415);
	HW_Set_IF_Synthesizer(pObj, 0xEA5556);
	HW_Set_IF_Synthesizer(pObj, 0x800007);
	HW_Set_IF_Synthesizer(pObj, 0x7850f8);
	HW_Set_IF_Synthesizer(pObj, 0x9b01c9);
	HW_Set_IF_Synthesizer(pObj, 0x00000A);
	HW_Set_IF_Synthesizer(pObj, 0x00000B);

	LockPhyReg(pObj);
	pObj->SetReg(reg, ZD_CR47, 0x0);
	tmpvalue = pObj->GetReg(reg, ZD_RADIO_PD);
	tmpvalue &= ~BIT_0;
	pObj->SetReg(reg, ZD_RADIO_PD, tmpvalue);
	tmpvalue |= BIT_0;
	pObj->SetReg(reg, ZD_RADIO_PD, tmpvalue);
	pObj->SetReg(reg, ZD_RFCFG, 0x5);
	pObj->DelayUs(100);
	pObj->SetReg(reg, ZD_RFCFG, 0x0);
	pObj->SetReg(reg, ZD_CR47, 0x1c);
	UnLockPhyReg(pObj);

	pObj->SetReg(reg, ZD_PE1_PE2, 3);

}


void
HW_Set_AL2210_Chips(zd_80211Obj_t *pObj, U32 ChannelNo, U8 InitChOnly)
{
	void *reg = pObj->reg;
	U32	tmpvalue;

	pObj->SetReg(reg, ZD_PE1_PE2, 2);

	LockPhyReg(pObj);
	pObj->SetReg(reg, ZD_CR9, 0xe0);
	pObj->SetReg(reg, ZD_CR10, 0x91);
	pObj->SetReg(reg, ZD_CR12, 0x90);
	pObj->SetReg(reg, ZD_CR15, 0xd0);
	pObj->SetReg(reg, ZD_CR16, 0x40);
	pObj->SetReg(reg, ZD_CR17, 0x58);
	pObj->SetReg(reg, ZD_CR18, 0x04);
	pObj->SetReg(reg, ZD_CR23, 0x66);
	pObj->SetReg(reg, ZD_CR24, 0x14);
	pObj->SetReg(reg, ZD_CR26, 0x90);
	pObj->SetReg(reg, ZD_CR31, 0x80);
	pObj->SetReg(reg, ZD_CR34, 0x06);
	pObj->SetReg(reg, ZD_CR35, 0x3e);
	pObj->SetReg(reg, ZD_CR38, 0x38);
	pObj->SetReg(reg, ZD_CR46, 0x90);
	pObj->SetReg(reg, ZD_CR47, 0x10);
	pObj->SetReg(reg, ZD_CR64, 0x64);
	pObj->SetReg(reg, ZD_CR79, 0xb5);
	pObj->SetReg(reg, ZD_CR80, 0x38);
	pObj->SetReg(reg, ZD_CR81, 0x30);
	pObj->SetReg(reg, ZD_CR113, 0xc0);
	pObj->SetReg(reg, ZD_CR127, 0x3);
	UnLockPhyReg(pObj);
	
	HW_Set_IF_Synthesizer(pObj, AL2210TB[ChannelNo]);
	HW_Set_IF_Synthesizer(pObj, 0x007cb1);
    HW_Set_IF_Synthesizer(pObj, 0x358372);
	HW_Set_IF_Synthesizer(pObj, 0x010a93);
	HW_Set_IF_Synthesizer(pObj, 0x472804);
	HW_Set_IF_Synthesizer(pObj, 0x456415);
	HW_Set_IF_Synthesizer(pObj, 0xEA5556);
	HW_Set_IF_Synthesizer(pObj, 0x800007);
	HW_Set_IF_Synthesizer(pObj, 0x7850f8);
	HW_Set_IF_Synthesizer(pObj, 0xf900c9);
	HW_Set_IF_Synthesizer(pObj, 0x00000A);
	HW_Set_IF_Synthesizer(pObj, 0x00000B);

	LockPhyReg(pObj);
	pObj->SetReg(reg, ZD_CR47, 0x0);
	tmpvalue = pObj->GetReg(reg, ZD_RADIO_PD);
	tmpvalue &= ~BIT_0;
	pObj->SetReg(reg, ZD_RADIO_PD, tmpvalue);
	tmpvalue |= BIT_0;
	pObj->SetReg(reg, ZD_RADIO_PD, tmpvalue);
	pObj->SetReg(reg, ZD_RFCFG, 0x5);
	pObj->DelayUs(100);
	pObj->SetReg(reg, ZD_RFCFG, 0x0);
 	pObj->SetReg(reg, ZD_CR47, 0x1c);
	UnLockPhyReg(pObj);

	pObj->SetReg(reg, ZD_PE1_PE2, 3);
}


void HW_EnableBeacon(zd_80211Obj_t *pObj, U16 BecaonInterval, U16 DtimPeriod) 
{
	U32 tmpValue;
	void *reg = pObj->reg;

	tmpValue = BecaonInterval | AP_MODE | (DtimPeriod<<16) ;
	pObj->SetReg(reg, ZD_BCNInterval, tmpValue);
}	


void HW_SwitchChannel(zd_80211Obj_t *pObj, U16 channel)
{
	void *reg = pObj->reg;

	pObj->SetReg(reg, ZD_CONFIGPhilips, 0x0);
	//FPRINT_V("rfMode", pObj->rfMode);

	switch(pObj->rfMode)
	{
		default:
			FPRINT_V("Invalid RF module parameter", pObj->rfMode);
			break;

		case GCT_RF:
		//	FPRINT_V("GCT Channel", channel);
			pObj->S_bit_cnt = 21;
			HW_Set_GCT_Chips(pObj, channel, 0);
			//HW_UpdateIntegrationValue(pObj, channel);
			break;
		case AL2210MPVB_RF:
			//FPRINT_V("AL2210MPVB_RF Channel", channel);
			pObj->S_bit_cnt = 24;
			HW_Set_AL2210MPVB_Chips(pObj, channel, 0);
			break;	
			
		case AL2210_RF:	
			//FPRINT_V("AL2210_RF Channel", channel);
			pObj->S_bit_cnt = 24;
			HW_Set_AL2210_Chips(pObj, channel, 0);
			break;		
		
		case RALINK_RF:
			FPRINT_V("Ralink Channel", channel);
			break;
			
		case INTERSIL_RF:
			FPRINT_V("Intersil Channel", channel);
			break;
		
		case RFMD_RF:
			FPRINT_V("RFMD Channel", channel);
			break;
			
		case MAXIM_RF:
			FPRINT_V("Maxim Channel", channel);
			break;
			
		case PHILIPS_RF:
			FPRINT_V("Philips SA2400 Channel", channel);
			break;
	}
	
	//HW_OverWritePhyRegFromE2P(pObj);
	    // When channnel == 14 , enable Japan spectrum mask
	if (pObj->RegionCode == 0x40) { //Japan
		if (channel == 14){
			HW_Set_FilterBand(pObj, pObj->RegionCode);  // for Japan, RegionCode = 0x40
		}
		else{
			// For other channels, use default filter.
			HW_Set_FilterBand(pObj, 0); 
		}
	} 

	
	pObj->DelayUs(100);
	
	return;
}


void HW_SetBeaconFIFO(zd_80211Obj_t *pObj, U8 *pBeacon, U16 index)
{
	U32 tmpValue, BCNPlcp;
	U16 j;
	void *reg = pObj->reg;

	pObj->SetReg(reg, ZD_BCN_FIFO_Semaphore, 0x0);
	tmpValue = pObj->GetReg(reg, ZD_BCN_FIFO_Semaphore);

	while (tmpValue & BIT_1){
		pObj->DelayUs(1);
		tmpValue = pObj->GetReg(reg, ZD_BCN_FIFO_Semaphore);
	}
	
	/* Write (Beacon_Len -1) to Beacon-FIFO */
	pObj->SetReg(reg, ZD_BCNFIFO, (index - 1));

	for (j=0; j<index; j++){
		pObj->SetReg(reg, ZD_BCNFIFO, pBeacon[j]);	
	}
	pObj->SetReg(reg, ZD_BCN_FIFO_Semaphore, 1);	
	
	/* Configure BCNPLCP */
	BCNPlcp = 0x00000400;
	index = (index << 3);
	BCNPlcp |= (((U32)index) << 16);
	pObj->SetReg(reg, ZD_BCNPLCPCfg, BCNPlcp);
}


#define	SR_1M		0x2
#define	SR_2M		0x4
#define	SR_5_5M		0xb
#define	SR_11M		0x16
#define	SR_16_5M	0x21
#define	SR_27_5M	0x37
void HW_SetSupportedRate(zd_80211Obj_t *pObj, U8 *prates)
{
	U8 HighestBasicRate = SR_1M;
	U8 HighestRate = SR_1M;
	U8 SRate;
	U32 tmpValue;
	U16 j;
	
	void *reg = pObj->reg;	
	
	for (j=0; j<(*(prates+1)); j++){
		switch((*(prates+2+j)) & 0x7f){
			case SR_1M:
				SRate = SR_1M;
				break;
				
			case SR_2M:
				SRate = SR_2M;
				break;
				
			case SR_5_5M:
				SRate = SR_5_5M;
				break;
				
			case SR_11M:
				SRate = SR_11M;
				break;
				
			default:
				SRate = SR_1M;
				break;
		}

		if (HighestRate < SRate)
			HighestRate = SRate;

		if ((*(prates+2+j)) & 0x80){
		/* It's a basic rate */
			if (HighestBasicRate < SRate)
				HighestBasicRate = SRate;
		}
	}

	tmpValue = pObj->GetReg(reg, ZD_CtlReg1);
	
	switch(HighestBasicRate){
		case SR_1M:
			tmpValue &= ~0x1c;
			tmpValue |= 0x00;
			pObj->SetReg(reg, ZD_CtlReg1, tmpValue);
			pObj->BasicRate = 0x0;
			break;
			
		case SR_2M:
			tmpValue &= ~0x1c;
			tmpValue |= 0x04; 
			pObj->SetReg(reg, ZD_CtlReg1, tmpValue);
			pObj->BasicRate = 0x1;
			break;
			
		case SR_5_5M:
			tmpValue &= ~0x1c;
			tmpValue |= 0x08;
			pObj->SetReg(reg, ZD_CtlReg1, tmpValue);
			pObj->BasicRate = 0x2;
			break;
			
		case SR_11M:
			tmpValue &= ~0x1c;
			tmpValue |= 0x0c;
			pObj->SetReg(reg, ZD_CtlReg1, tmpValue);
			pObj->BasicRate = 0x3;
			break;
			
		default:
			break;
	}
}		


void HW_SetSTA_PS(zd_80211Obj_t *pObj, U8 op)
{
	void *reg = pObj->reg;
	U32 tmpValue;
	
	tmpValue = pObj->GetReg(reg, ZD_BCNInterval);

	if (op)
		tmpValue |= STA_PS;
	else
		tmpValue &= ~STA_PS;	
	
	pObj->SetReg(reg, ZD_BCNInterval, tmpValue);
}


void HW_GetTsf(zd_80211Obj_t *pObj, U32 *loTsf, U32 *hiTsf)
{
	void *reg = pObj->reg;
	
	*loTsf = pObj->GetReg(reg, ZD_TSF_LowPart);
	*hiTsf = pObj->GetReg(reg, ZD_TSF_HighPart);
}		


U32 HW_GetNow(zd_80211Obj_t *pObj)
{
	void *reg = pObj->reg;
	
	return pObj->GetReg(reg, ZD_TSF_LowPart);
}	


void HW_RadioOnOff(zd_80211Obj_t *pObj, U8 on)
{
	void *reg = pObj->reg;
	U32	tmpvalue;
	
	if (on){
		//++ Turn on RF
		switch(pObj->rfMode){
			case GCT_RF:
				HW_Set_IF_Synthesizer(pObj, 0x1c0008);
				break;
			
			case AL2210_RF:	
			case AL2210MPVB_RF:
				HW_Set_IF_Synthesizer(pObj, 0x456415);
				HW_Set_IF_Synthesizer(pObj, 0x4c72804);
			
				LockPhyReg(pObj);
				pObj->SetReg(reg, ZD_CR10, 0x91);
	  	 		pObj->SetReg(reg, ZD_CR47, 0x0);
				tmpvalue = pObj->GetReg(reg, ZD_RADIO_PD);
				tmpvalue &= ~BIT_0;
				pObj->SetReg(reg, ZD_RADIO_PD, tmpvalue);
				tmpvalue |= BIT_0;
				pObj->SetReg(reg, ZD_RADIO_PD, tmpvalue);
				pObj->SetReg(reg, ZD_RFCFG, 0x5);
				pObj->DelayUs(100);
				pObj->SetReg(reg, ZD_RFCFG, 0x0);
				pObj->SetReg(reg, ZD_CR47, 0x1c);
				UnLockPhyReg(pObj);
				break;
			
			default:
				break;	
		}		
	}
	else{
		//++ Turn off RF
		switch(pObj->rfMode){
			case GCT_RF:
				HW_Set_IF_Synthesizer(pObj, 0x1c0000);
				break;
			
			case AL2210_RF:		
			case AL2210MPVB_RF:
				HW_Set_IF_Synthesizer(pObj, 0xc56415);
				HW_Set_IF_Synthesizer(pObj, 0xcc72804);
				LockPhyReg(pObj);
				pObj->SetReg(reg, ZD_CR10, 0xd1);
				UnLockPhyReg(pObj);
				pObj->SetReg(reg, ZD_RADIO_PD, 0);
				break;
					
			default:
				break;		
		}		
			
	}
}	


void HW_ResetPhy(zd_80211Obj_t *pObj)
{
	void *reg = pObj->reg;
	
	LockPhyReg(pObj);
	pObj->SetReg(reg, ZD_CR0, 0x0a);
	pObj->SetReg(reg, ZD_CR1, 0x06);
	pObj->SetReg(reg, ZD_CR2, 0x26);
	pObj->SetReg(reg, ZD_CR3, 0x38);
	pObj->SetReg(reg, ZD_CR4, 0x80);
	
	pObj->SetReg(reg, ZD_CR9, 0xa0);
	pObj->SetReg(reg, ZD_CR10, 0x81);
	pObj->SetReg(reg, ZD_CR11, 0x00);
	pObj->SetReg(reg, ZD_CR12, 0x7f);
	pObj->SetReg(reg, ZD_CR13, 0x8c);
	pObj->SetReg(reg, ZD_CR14, 0x80);
	pObj->SetReg(reg, ZD_CR15, 0x3d);
	pObj->SetReg(reg, ZD_CR16, 0x20);
	pObj->SetReg(reg, ZD_CR17, 0x1e);
	pObj->SetReg(reg, ZD_CR18, 0x0a);
	pObj->SetReg(reg, ZD_CR19, 0x48);
	pObj->SetReg(reg, ZD_CR20, 0x0c);
	pObj->SetReg(reg, ZD_CR21, 0x0c);
	pObj->SetReg(reg, ZD_CR22, 0x23);
	pObj->SetReg(reg, ZD_CR23, 0x90);
	pObj->SetReg(reg, ZD_CR24, 0x14);
	pObj->SetReg(reg, ZD_CR25, 0x40);
	pObj->SetReg(reg, ZD_CR26, 0x10);
	pObj->SetReg(reg, ZD_CR27, 0x19);
	pObj->SetReg(reg, ZD_CR28, 0x7f);
	pObj->SetReg(reg, ZD_CR29, 0x80);
	pObj->SetReg(reg, ZD_CR30, 0x49);
	pObj->SetReg(reg, ZD_CR31, 0x60);
	pObj->SetReg(reg, ZD_CR32, 0x43);
	pObj->SetReg(reg, ZD_CR33, 0x08);
	pObj->CR32Value = 0x43;
	pObj->SetReg(reg, ZD_CR34, 0x06);
	pObj->SetReg(reg, ZD_CR35, 0x0a);
	pObj->SetReg(reg, ZD_CR36, 0x00);
	pObj->SetReg(reg, ZD_CR37, 0x00);
	pObj->SetReg(reg, ZD_CR38, 0x38);
	pObj->SetReg(reg, ZD_CR39, 0x0c);
	pObj->SetReg(reg, ZD_CR40, 0x84);
	pObj->SetReg(reg, ZD_CR41, 0x2a);
	pObj->SetReg(reg, ZD_CR42, 0x80);
	pObj->SetReg(reg, ZD_CR43, 0x10);
	pObj->SetReg(reg, ZD_CR44, 0x12);
	
	pObj->SetReg(reg, ZD_CR46, 0xff);
	pObj->SetReg(reg, ZD_CR47, 0x08);
	pObj->SetReg(reg, ZD_CR48, 0x26);
	pObj->SetReg(reg, ZD_CR49, 0x5b);
	
	pObj->SetReg(reg, ZD_CR64, 0xd0);
	pObj->SetReg(reg, ZD_CR65, 0x04);
	pObj->SetReg(reg, ZD_CR66, 0x58);
	pObj->SetReg(reg, ZD_CR67, 0xc9);
	pObj->SetReg(reg, ZD_CR68, 0x88);
	pObj->SetReg(reg, ZD_CR69, 0x41);
	pObj->SetReg(reg, ZD_CR70, 0x23);
	pObj->SetReg(reg, ZD_CR71, 0x10);
	pObj->SetReg(reg, ZD_CR72, 0xff);
	pObj->SetReg(reg, ZD_CR73, 0x32);
	pObj->SetReg(reg, ZD_CR74, 0x30);
	pObj->SetReg(reg, ZD_CR75, 0x65);
	pObj->SetReg(reg, ZD_CR76, 0x41);
	pObj->SetReg(reg, ZD_CR77, 0x1b);
	pObj->SetReg(reg, ZD_CR78, 0x30);
	pObj->SetReg(reg, ZD_CR79, 0x68);
	pObj->SetReg(reg, ZD_CR80, 0x64);
	pObj->SetReg(reg, ZD_CR81, 0x64);
	pObj->SetReg(reg, ZD_CR82, 0x00);
	pObj->SetReg(reg, ZD_CR83, 0x00);
	pObj->SetReg(reg, ZD_CR84, 0x00);
	pObj->SetReg(reg, ZD_CR85, 0x02);
	pObj->SetReg(reg, ZD_CR86, 0x00);
	pObj->SetReg(reg, ZD_CR87, 0x00);
	pObj->SetReg(reg, ZD_CR88, 0xff);
	pObj->SetReg(reg, ZD_CR89, 0xfc);
	pObj->SetReg(reg, ZD_CR90, 0x00);
	pObj->SetReg(reg, ZD_CR91, 0x00);
	pObj->SetReg(reg, ZD_CR92, 0x00);
	pObj->SetReg(reg, ZD_CR93, 0x08);
	pObj->SetReg(reg, ZD_CR94, 0x00);
	pObj->SetReg(reg, ZD_CR95, 0x00);
	pObj->SetReg(reg, ZD_CR96, 0xff);
	pObj->SetReg(reg, ZD_CR97, 0xe7);
	pObj->SetReg(reg, ZD_CR98, 0x00);
	pObj->SetReg(reg, ZD_CR99, 0x00);
	pObj->SetReg(reg, ZD_CR100, 0x00);
	pObj->SetReg(reg, ZD_CR101, 0xae);
	pObj->SetReg(reg, ZD_CR102, 0x02);
	pObj->SetReg(reg, ZD_CR103, 0x00);
	pObj->SetReg(reg, ZD_CR104, 0x03);
	pObj->SetReg(reg, ZD_CR105, 0x65);
	pObj->SetReg(reg, ZD_CR106, 0x04);
	pObj->SetReg(reg, ZD_CR107, 0x00);
	pObj->SetReg(reg, ZD_CR108, 0x0a);
	pObj->SetReg(reg, ZD_CR109, 0xaa);
	pObj->SetReg(reg, ZD_CR110, 0xaa);
	pObj->SetReg(reg, ZD_CR111, 0x25);
	pObj->SetReg(reg, ZD_CR112, 0x25);
	pObj->SetReg(reg, ZD_CR113, 0x00);
	
	pObj->SetReg(reg, ZD_CR119, 0x1e);
	
	pObj->SetReg(reg, ZD_CR125, 0x90);
	pObj->SetReg(reg, ZD_CR126, 0x00);
	pObj->SetReg(reg, ZD_CR127, 0x00);
	UnLockPhyReg(pObj);
	return;
}	


void HW_InitHMAC(zd_80211Obj_t *pObj)
{
	void *reg = pObj->reg;
	
	// Set GPI_EN be zero. ie. Disable GPI (Requested by Ahu)
	pObj->SetReg(reg, ZD_GPI_EN, 0x00);

	// Use Ack_Timeout_Ext to tolerance some peers that response slowly.
	// The influence is that the retry frame will be less competitive. It's acceptable.
	pObj->SetReg(reg, ZD_Ack_Timeout_Ext, 0x20); //only bit0-bit5 are valid

	// Set RX_PE_DELAY 0x10 to enlarge the time for decharging tx power.
	pObj->SetReg(reg, ZD_RX_PE_DELAY, 0x50); //3910
	
	// Set EIFS = 0x200 -- Workaround for Adjacent-Channel issue.
	pObj->SetReg(reg, ZD_IFS_Value, 0x5200032); //0x547c032

	// Keep 44MHz oscillator always on.
	pObj->SetReg(reg, ZD_PS_Ctrl, 0x10000008);
	
	pObj->SetReg(reg, ZD_ADDA_MBIAS_WarmTime, 0x30000808);
	
	// Set CWmin, CWmax, Slot time
	// CWmin = 0x7f (Bit0 ~ Bit9)
	// CWmax = 0x7f (Bit16 ~ Bit25)
	// Slot Timer = 0x2 ((Bit10 ~ Bit14) + 1)
	//pObj->SetReg(reg, ZD_CWmin_CWmax, 0x7f047f);
	pObj->SetReg(reg, ZD_CWmin_CWmax, 0x7f107f);
	
	/* Set RetryMax 8 */
	pObj->SetReg(reg, ZD_RetryMAX, 0x2);

	/* Turn off sniffer mode */	
	pObj->SetReg(reg, ZD_SnifferOn, 0);

	/* Set Rx filter*/
	// filter Beacon and unfilter PS-Poll
	pObj->SetReg(reg, ZD_Rx_Filter, ((BIT_10 << 16) | (0xffff & ~BIT_8))); 

	/* Set Hashing table */
	pObj->SetReg(reg, ZD_GroupHash_P1, 0x00);
	pObj->SetReg(reg, ZD_GroupHash_P2, 0x80000000);
	
	pObj->SetReg(reg, ZD_CtlReg1, 0xa4);
	pObj->SetReg(reg, ZD_ADDA_PwrDwn_Ctrl, 0x7f); 
	
	/* Initialize BCNATIM needed registers */	
	pObj->SetReg(reg, ZD_BCNPLCPCfg, 0x00f00401);
	pObj->SetReg(reg, ZD_PHYDelay, 0x00);
	
	/* Write after PNP port */
	pObj->SetReg(reg, ZD_AfterPNP, 0x01);
}	


void HW_OverWritePhyRegFromE2P(zd_80211Obj_t *pObj)
{
	U32	NumOfReg;
	U32	tmpvalue;
	U32	i;
	U8	PhyIdx;
	U8	PhyValue;
	void *reg = pObj->reg;

	tmpvalue = pObj->GetReg(reg, ZD_E2P_POD);
	if (tmpvalue & BIT_7){
		NumOfReg = pObj->GetReg(reg, ZD_E2P_PHY_REG);
		NumOfReg = NumOfReg & 0xff;
		
		for (i=0; i<NumOfReg; i+=2){
			tmpvalue = pObj->GetReg(reg, (ZD_E2P_PHY_REG+4+i*2));
			PhyIdx = (U8)(tmpvalue & 0xff);
			PhyValue = (U8)((tmpvalue >> 8) & 0xff);
			
			HW_WritePhyReg(pObj, PhyIdx, PhyValue);
			
			if ((i+1) < NumOfReg){
				PhyIdx = (U8)((tmpvalue >> 16) & 0xff);
				PhyValue = (U8)((tmpvalue >> 24) & 0xff);
				HW_WritePhyReg(pObj, PhyIdx, PhyValue);
			}
		}
	}
	return;
}


void HW_WritePhyReg(zd_80211Obj_t *pObj, U8 PhyIdx, U8 PhyValue)
{
	U32	IoAddress;
	void *reg = pObj->reg;

	switch(PhyIdx){
		case 4:
			IoAddress = 0x20;
			break;
			
		case 5:
			IoAddress = 0x10;
			break;
			
		case 6:
			IoAddress = 0x14;
			break;
			
		case 7:
			IoAddress = 0x18;
			break;
			
		case 8:
			IoAddress = 0x1C;
			break;
			
		default:
			IoAddress = (((U32)PhyIdx) << 2);
			break;
	}
	
	LockPhyReg(pObj);
	pObj->SetReg(reg, IoAddress, PhyValue);
	UnLockPhyReg(pObj);
}


void HW_UpdateIntegrationValue(zd_80211Obj_t *pObj, U32 ChannelNo)
{
	U32	tmpvalue;
	void *reg = pObj->reg;

	tmpvalue = pObj->GetReg(reg, ZD_E2P_POD);
	if (tmpvalue & BIT_7){
		tmpvalue = pObj->GetReg(reg, ZD_E2P_PWR_INT_VALUE+((ChannelNo-1) & 0xc));
		tmpvalue = (U8)(tmpvalue >> (((ChannelNo-1) % 4) * 8));
		HW_Write_TxGain(pObj, tmpvalue);
	}
}


void HW_E2P_AutoPatch(zd_80211Obj_t *pObj)
{
	U32		tmpvalue;
	U16		i;
	U32		POD_Value;
	BOOLEAN	Write_E2P = FALSE;
	void *reg = pObj->reg;

	// Auto Patch -- 3616
	POD_Value = pObj->GetReg(reg, ZD_E2P_POD);
	if (!(POD_Value & BIT_7)){
		// Subsystem = 0
		pObj->SetReg(reg, ZD_E2P_SUBID, 0x0);
		// Mark patch-code
		tmpvalue = pObj->GetReg(reg, ZD_E2P_POD);
		tmpvalue &= 0xff;
		tmpvalue |= BIT_7;
		pObj->SetReg(reg, ZD_E2P_POD, tmpvalue);
		
		// Set default Set point
		for (i=0; i<16; i+=4){
			pObj->SetReg(reg, ZD_E2P_PWR_CAL_VALUE1, 0x28282828);
		}
		
		// Set default IntegrationValue
		for (i=0; i<16; i+=4){
			pObj->SetReg(reg, (ZD_E2P_PWR_INT_VALUE+i), 0x1c1c1c1c);
		}
		
		// Set others zero
		for (i=ZD_E2P_ALLOWED_CHANNEL; i<(ZD_E2P_SUBID+256); i+=4){
			pObj->SetReg(reg, i, 0x0);
		}

		Write_E2P = TRUE;
	}

	// Auto Patch -- 3811
	if (!(POD_Value & BIT_6)){
		// Mark patch-code
		tmpvalue = pObj->GetReg(reg, ZD_E2P_POD);
		tmpvalue &= 0xff;
		tmpvalue |= BIT_6;
		pObj->SetReg(reg, ZD_E2P_POD, tmpvalue);

		// Set Allowed-Channel(Channel 1~11)/Default-Channel(Channel 1)
		pObj->SetReg(reg, ZD_E2P_ALLOWED_CHANNEL, 0x000107ff);

		// Set Region Code (USA)
		tmpvalue = pObj->GetReg(reg, ZD_E2P_REGION_CODE);
		tmpvalue &= ~0xffff;
		tmpvalue |= 0x10;
		pObj->SetReg(reg, ZD_E2P_REGION_CODE, tmpvalue);

		Write_E2P = TRUE;
	}

	if (Write_E2P){
		// Write into EEPROM
		pObj->SetReg(reg, ZD_EEPROM_PROTECT0, 0x55aa44bb);
		pObj->SetReg(reg, ZD_EEPROM_PROTECT1, 0x33cc22dd);
		pObj->SetReg(reg, ZD_ROMDIR, 0x422);

		// Wait 4ms
		pObj->DelayUs(4000);
	}
}


void HW_Write_TxGain(zd_80211Obj_t *pObj, U32 txgain)
{
	U32	tmpvalue;
	void *reg = pObj->reg;
	U8	i;
	
	switch(pObj->rfMode){
		case GCT_RF:
			txgain &= 0x3f;
			//FPRINT_V("Set tx gain", txgain);
			tmpvalue = 0;
			// Perform Bit-Reverse
			for (i=0; i<6; i++){
				if (txgain & BIT_0){
					tmpvalue |= (0x1 << (15-i));
				}
				txgain = (txgain >> 1);
			}
			tmpvalue |= 0x0c0000;
			HW_Set_IF_Synthesizer(pObj, tmpvalue);
			//FPRINT_V("HW_Set_IF_Synthesizer", tmpvalue);
			HW_Set_IF_Synthesizer(pObj, 0x150800);
			HW_Set_IF_Synthesizer(pObj, 0x150000);
			break;
		
		case AL2210_RF:	
		case AL2210MPVB_RF:
			if (txgain > AL2210_MAX_TX_PWR_SET){
				txgain = AL2210_MAX_TX_PWR_SET;
			}
			else if (txgain < AL2210_MIN_TX_PWR_SET){
				txgain = AL2210_MIN_TX_PWR_SET;
			}
			
			LockPhyReg(pObj);
			pObj->SetReg(reg, ZD_CR31, (U8)txgain);
			UnLockPhyReg(pObj);
			break;	
			
		default:
			break;

	}
}

void HW_Set_FilterBand(zd_80211Obj_t *pObj, U32	region_code)
{
	void *reg = pObj->reg;
	
	switch(region_code){
		case 0x40:	// Japan
			LockPhyReg(pObj);
            // For ZD1202+AL2210 and Japan, set CR47 from 0x1c to 0x10 2004/01/15 Roger Huang 
			if (pObj->rfMode == AL2210MPVB_RF || pObj->rfMode == AL2210_RF) {
				pObj->SetReg(reg, ZD_CR47, 0x10); 
			}

			pObj->SetReg(reg, ZD_CR82, 0x00);
			pObj->SetReg(reg, ZD_CR83, 0x00);
			pObj->SetReg(reg, ZD_CR84, 0x00);
			pObj->SetReg(reg, ZD_CR85, 0x01);
			pObj->SetReg(reg, ZD_CR86, 0x00);
			pObj->SetReg(reg, ZD_CR87, 0x00);
			pObj->SetReg(reg, ZD_CR88, 0xff);
			pObj->SetReg(reg, ZD_CR89, 0xf7);
			pObj->SetReg(reg, ZD_CR90, 0xff);
			pObj->SetReg(reg, ZD_CR91, 0xf4);
			pObj->SetReg(reg, ZD_CR92, 0x00);
			pObj->SetReg(reg, ZD_CR93, 0x18);
			pObj->SetReg(reg, ZD_CR94, 0x00);
			pObj->SetReg(reg, ZD_CR95, 0x46);
			pObj->SetReg(reg, ZD_CR96, 0x00);
			pObj->SetReg(reg, ZD_CR97, 0x00);
			pObj->SetReg(reg, ZD_CR98, 0xff);
			pObj->SetReg(reg, ZD_CR99, 0x3d);
			pObj->SetReg(reg, ZD_CR100, 0xff);
			pObj->SetReg(reg, ZD_CR101, 0x3a);
			pObj->SetReg(reg, ZD_CR102, 0x01);
			pObj->SetReg(reg, ZD_CR103, 0x56);
			pObj->SetReg(reg, ZD_CR104, 0x04);
			pObj->SetReg(reg, ZD_CR105, 0xb6);
			pObj->SetReg(reg, ZD_CR106, 0x06);
			pObj->SetReg(reg, ZD_CR107, 0x66);
			UnLockPhyReg(pObj);
			break;
			
		default:
			LockPhyReg(pObj);
			pObj->SetReg(reg, ZD_CR82, 0x00);
			pObj->SetReg(reg, ZD_CR83, 0x00);
			pObj->SetReg(reg, ZD_CR84, 0x00);
			pObj->SetReg(reg, ZD_CR85, 0x02);
			pObj->SetReg(reg, ZD_CR86, 0x00);
			pObj->SetReg(reg, ZD_CR87, 0x00);
			pObj->SetReg(reg, ZD_CR88, 0xff);
			pObj->SetReg(reg, ZD_CR89, 0xfc);
			pObj->SetReg(reg, ZD_CR90, 0x00);
			pObj->SetReg(reg, ZD_CR91, 0x00);
			pObj->SetReg(reg, ZD_CR92, 0x00);
			pObj->SetReg(reg, ZD_CR93, 0x08);
			pObj->SetReg(reg, ZD_CR94, 0x00);
			pObj->SetReg(reg, ZD_CR95, 0x00);
			pObj->SetReg(reg, ZD_CR96, 0xff);
			pObj->SetReg(reg, ZD_CR97, 0xe7);
			pObj->SetReg(reg, ZD_CR98, 0x00);
			pObj->SetReg(reg, ZD_CR99, 0x00);
			pObj->SetReg(reg, ZD_CR100, 0x00);
			pObj->SetReg(reg, ZD_CR101, 0xae);
			pObj->SetReg(reg, ZD_CR102, 0x02);
			pObj->SetReg(reg, ZD_CR103, 0x00);
			pObj->SetReg(reg, ZD_CR104, 0x03);
			pObj->SetReg(reg, ZD_CR105, 0x65);
			pObj->SetReg(reg, ZD_CR106, 0x04);
			pObj->SetReg(reg, ZD_CR107, 0x00);
			UnLockPhyReg(pObj);;
			break;
	}
}

#endif

