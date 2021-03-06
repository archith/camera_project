#ifndef _ZDHCI_C_
#define _ZDHCI_C_

#include "zd80211.h"
#include "zdhci.h"

//ivan
#include "mlme_zydas/mlme.h"

extern void zd1205_dump_data(char *info, u8 *data, u32 data_len);

U8	zd_Snap_Header[6] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00};
U8	zd_SnapBridgeTunnel[6] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0xF8};

#define MAX_CHANNEL_ALLOW		13
extern int send_debug; 
extern int send_enc_debug; 

zd_80211Obj_t *pdot11Obj = 0;
U16 CurrScanCH = 1;
extern int zd1205_mode;//ivan
BOOLEAN BeginSleep;

BOOLEAN zd_SendPkt(mlme_data_t *mlme_p,U8 *pHdr, U8 *pBody, U32 bodyLen, void *buf, U8 bEapol, void *pHash)
{
	Signal_t *signal;
	FrmDesc_t *pfrmDesc;
	Frame_t *frame;
	U8	vapId = 0;    

	if (mPsStaCnt){
    	if (zd_CheckTotalQueCnt() > TXQ_THRESHOLD){
        	//FPRINT("Drop Tx packet");
        	return FALSE; 
    	} 
    }	
    
	signal = allocSignal();
	if (!signal){
    	FPRINT("zd_SendPkt out of signal");
//    	FPRINT_V("freeSignalCount", freeSignalCount);
		return FALSE;
	}	
	
	pfrmDesc = allocFdesc();
	if (!pfrmDesc){
		freeSignal(signal);
    	FPRINT("zd_SendPkt out of description");
//    	FPRINT_V("freeFdescCount", freeFdescCount);
		return FALSE;
	}
	
	frame = pfrmDesc->mpdu;
	/*	FrameControl(2) Duration/ID(2) A1(6) A2(6) A3(6) Seq(2) A4/LLCHdr(6) LLCHdr(6) */
//ivan
#if 0
	memcpy((char *)&(frame->header[4]), (char *)&(pHdr[0]), 6);	/* Set DA to A1 */
	memcpy((char *)&(frame->header[16]), (char *)&(pHdr[6]), 6);	/* Set SA to A3 */
#else
    if(zd1205_mode == INDEPENDENT_BSS) 
    {
   	    memcpy((char *)&(frame->header[4]), (char *)&(pHdr[0]), 6);	/* Set DA to A1 */
    	memcpy((char *)&(frame->header[10]), (char *)&(pHdr[6]), 6);
        memcpy((char *)&(frame->header[16]), (char *)&mBssId, 6);   /* Set Bssid to A3 */  
        
        frame->header[1] = 0;       
    }
    else //INFRASTRUCTURE_BSS
    {
    	memcpy((char *)&(frame->header[4]), (char *)&mBssId, 6);	/* Set BSSID to A1 */
    	memcpy((char *)&(frame->header[10]), (char *)&(pHdr[6]), 6);
        memcpy((char *)&(frame->header[16]), (char *)&(pHdr[0]), 6);/* Set DA to A3 */
    }
#endif	

//ivan
if(mlme_dbg_level&MLME_DEBUG_TX_DUMP_PKT)
    zd1205_dump_data("<zd_SendPkt>", (unsigned char *)pBody, 24);

	frame->bodyLen = bodyLen;
	frame->body = pBody;
	signal->buf = buf;
	signal->vapId = vapId;
	pfrmDesc->bIntraBss = 0;
	frame->HdrLen = MAC_HDR_LNG;
	frame->header[0] = ST_DATA;
#if 0//ivan
	frame->header[1] = FROM_DS_BIT;
#else
    {        
        if(zd1205_mode == INDEPENDENT_BSS)
            frame->header[1] = 0;
        else
        {
            frame->header[1] = TO_DS_BIT;
            
	     	if (mPwrState)
	     	{
				frame->header[1] |=	PW_SAVE_BIT;
				PSDEBUG("Set Sleep bit");			
			}
			else
			{
				frame->header[1] &=	~PW_SAVE_BIT; 			
				PSDEBUG("Set Active bit");
			}
		}            
    }
#endif
	setAddr2(frame, &dot11MacAddress);
	pfrmDesc->bEapol = bEapol;
	signal->bDataFrm = 1;
	//pfrmDesc->bDataFrm = 1;
	pfrmDesc->pHash = (Hash_t *)pHash;
	mkFragment(signal, pfrmDesc); //10 us

//ivan to dump packet	
#if 0
if(mDynKeyMode==DYN_KEY_TKIP)
{
    //printk("\nsend header----------------------\n");
    printk("\nheader length=0x%x\n",frame->HdrLen);
    zd1205_dump_data("header", frame->header, frame->HdrLen);
    //printk("body----------------------\n");
    printk("body length=0x%x\n",frame->bodyLen);
    zd1205_dump_data("body", frame->body, frame->bodyLen);
}
#endif

//ivan
if(mlme_dbg_level&MLME_DEBUG_TX_DUMP_PKT)
zd1205_dump_data("<zd_SendPkt2>", (U8 *)pBody, 24);

	SendPkt(signal, pfrmDesc, TRUE);
	
	PSDEBUG("zd_SendPkt finish");

	if (mPwrState != 0)	
		BeginSleep = 1;
			
	return TRUE; 
}


#define	LP_FORWARD		0
#define	BC_FORWARD		1
#define BSS_FORWARD		2	

void zd_WirelessForward(U8 *pHdr, U8 *pBody, U32 len, void *buf, U8 mode, void *pHash)
{
	Signal_t *signal;
	FrmDesc_t *pfrmDesc;
	Frame_t *frame;
	U8	vapId = 0;

printk("zd_WirelessForward");
	//FPRINT("zd_WirelessForward");
    
    if (mPsStaCnt){
    	if (zd_CheckTotalQueCnt() > TXQ_THRESHOLD){
       		//FPRINT("Drop Intra-BSS packet");
        	pdot11Obj->ReleaseBuffer(buf); 
        	return;  
    	} 
    }	
    
	signal = allocSignal();
	if (!signal){
    	FPRINT("zd_WirelessForward out of signal");
//    	FPRINT_V("freeSignalCount", freeSignalCount);
    	pdot11Obj->ReleaseBuffer(buf);
		return;
	}	
	
	pfrmDesc = allocFdesc();
	if (!pfrmDesc){
		freeSignal(signal);
    	FPRINT("zd_WirelessForward out of description");
//    	FPRINT_V("freeFdescCount", freeFdescCount);
    	pdot11Obj->ReleaseBuffer(buf);
		return;
	}
	
	frame = pfrmDesc->mpdu;
	/*	FrameControl(2) Duration/ID(2) A1(6) A2(6) A3(6) Seq(2) A4/LLCHdr(6) LLCHdr(6) */
	memcpy((char *)&(frame->header[4]), (char *)&(pHdr[16]), 6);	/* Set DA to A1 */
	memcpy((char *)&(frame->header[16]), (char *)&(pHdr[10]), 6);	/* Set SA to A3 */
	
	frame->bodyLen = len;
	frame->body = pBody;
	signal->buf = buf;
	signal->vapId = vapId;
	
	if (mode == LP_FORWARD){
		memcpy((char *)&(frame->header[4]), (char *)&(pHdr[10]), 6);	/* Set DA to A1 */
		memcpy((char *)&(frame->header[16]), (char *)&dot11MacAddress, 6);	/* Set SA to A3 */
		frame->body[6] = 0x38;
		frame->body[7] = 0x39;
	}	
	
	pfrmDesc->bIntraBss = 1;
	signal->bDataFrm = 1;
	//pfrmDesc->bDataFrm = 1;
	frame->HdrLen = MAC_HDR_LNG;
	frame->header[0] = ST_DATA;
	frame->header[1] = FROM_DS_BIT;
	setAddr2(frame, &dot11MacAddress);
	pfrmDesc->bEapol = 0;
	pfrmDesc->pHash = (Hash_t *)pHash;
	mkFragment(signal, pfrmDesc);
	SendPkt(signal, pfrmDesc, FALSE);
    return; 
}


void zd_SendClass2ErrorFrame(MacAddr_t *sta, U8 vapId)
{
	Signal_t *signal;
	
	//FPRINT("zd_sendClass2ErrorFrame");
	
	if ((signal = allocSignal()) == NULL)  
		return;
		
	signal->id = SIG_DEAUTH_REQ;
	signal->block = BLOCK_AUTH_REQ;
	signal->vapId = vapId;
	memcpy(&signal->frmInfo.Sta, sta, 6);
	signal->frmInfo.rCode = RC_CLASS2_ERROR;
	sigEnque(pMgtQ, (signal));
	
	return;
}	


void zd_SendClass3ErrorFrame(MacAddr_t *sta, U8 vapId)
{
	Signal_t *signal;
	
	//FPRINT("zd_SendClass3ErrorFrame");
	if ((signal = allocSignal()) == NULL)  
		return;
		
	signal->id = SIG_DIASSOC_REQ;
	signal->block = BLOCK_ASOC;
	signal->vapId = vapId;
	memcpy(&signal->frmInfo.Sta, sta, 6);
	signal->frmInfo.rCode = RC_CLASS3_ERROR;
	sigEnque(pMgtQ, (signal));
	
	return;
}	


BOOLEAN zd_CheckMic(U8 *pHdr, U8 *pBody, U32 bodyLen, Hash_t *pHash)
{
	MICvar *pRxMicKey;
	U8 PkInstalled = 0;
	U8 *pByte;
	U8 CalMic[8];
	int i;
	U8 *pDa = &pHdr[16]; //A3
	U8 *pSa = &pHdr[10]; //A2
	U8 *pIV = pHdr + 24;
	//U16 RxIv16 = ((U16)pIV[0] << 8) + pIV[2];
	//U32 RxIv32 = *(U32 *)(&pIV[4]);
				
	if (pIV[3] & EIV_BIT){
		//zd1205_dump_data("IV = ", pIV, 8);
		//zd1205_dump_data("MIC K0= ", (U8 *)&pRxMicKey->K0, 4);
		//zd1205_dump_data("MIC K1= ", (U8 *)&pRxMicKey->K1, 4);
		pRxMicKey = &pHash->RxMicKey;	
		PkInstalled = pHash->pkInstalled;	
					
		if (PkInstalled){
			MICclear(pRxMicKey);
						
			pByte = (U8 *)pDa;
			//zd1205_dump_data("DA = ", pDa, 6);
			for(i=0; i<6; i++){ //for DA
				MICappendByte(*pByte++, pRxMicKey);
			}
			

			pByte = (U8 *)pSa;
			//zd1205_dump_data("SA = ", pSa, 6);
			for(i=0; i<6; i++){ //for SA
				MICappendByte(*pByte++, pRxMicKey);
			}

			MICappendByte(0, pRxMicKey);
			MICappendByte(0, pRxMicKey);
			MICappendByte(0, pRxMicKey);
			MICappendByte(0, pRxMicKey);
				
			pByte = pBody;
			for (i=0; i<(bodyLen-MIC_LNG); i++){
				MICappendByte(*pByte++, pRxMicKey);
			}
						
			MICgetMIC(CalMic, pRxMicKey); // Append MIC (8 byte)
		
			//update iv sequence
			//pHash->RxSeed.IV16 = RxIv16;
			//pHash->RxSeed.IV32 = RxIv32; 				
			//zd1205_dump_data("CalMic = ", CalMic, 8);
			//zd1205_dump_data("ReceMic = ", pByte, 8);
														
			// now pBye point to MIC area
			if (memcmp(CalMic, pByte, MIC_LNG) != 0){
				FPRINT("***** MIC error *****");
//				zd1205_dump_data("pHdr = ", pHdr, 32);
				zd1205_dump_data("pBody = ", pBody, bodyLen);
				zd1205_dump_data("CalMic = ", CalMic, 8);
				zd1205_dump_data("ReceMic = ", pByte, 8);
				pdot11Obj->MicFailure(pSa);
				return FALSE;
			}
			else{
				//FPRINT("MIC success !!!!");
				return TRUE;
			}
		}	
	}

    return FALSE;
}	


void zd_ReceivePkt(mlme_data_t *mlme_p,U8 *pHdr, U32 hdrLen, U8 *pBody, U32 bodyLen, U8 rate, void *buf, U8 bDataFrm, U8 *pEthHdr)
{
	MacAddr_t *pDa, *pSa;
	U32 dataLen;
	U8 *pData;
	
	pDa = (MacAddr_t *)&pHdr[16]; //A3
	pSa = (MacAddr_t *)&pHdr[10]; //A2


	if (bDataFrm)
	{
//printk("bDataFrm:%d\n",bodyLen);   //ivan

		if (!bodyLen)
			goto rx_release;

        //if(!send_debug)
        {
            unsigned int data;
            MLME_ioctl(mlme_p,MLME_IOCTL_QUERY_ASSOC,&data);
//printk("assoc[%d]\n",data);   //ivan            
            if(data==0) //assoc?
                goto rx_release;
            goto rx_ind;
        }
        {
rx_ind:
			if ((bodyLen > 5 ) && (memcmp(pBody, zd_Snap_Header, 6) == 0 || memcmp(pBody, zd_SnapBridgeTunnel, 6) == 0))
		    {
				pData = pBody - 6;	
				dataLen = bodyLen + 6;		/* Plus DA, SA*/
			}
			else
			{
				pData = pBody - 14;	
				dataLen = bodyLen + 14;		/* Plus DA, SA and TypeLen */
				pData[12] = (bodyLen>>8) & 0xFF;
				pData[13] = bodyLen & 0xFF;
			}
//printk("bodyLen=%d\n",bodyLen);			
//printk("pBody:0x%x pData:0x%x\n",pBody,pData);
//zd1205_dump_data("pBody = ", pBody, 30);
			//memcpy(pData, pDa, 6);		/* Set A3 to DA */
			//memcpy(pData+6, pSa, 6);	/* Set A2 to SA */
			
#if 0 //ivan
			memcpy(pData, pEthHdr+6, 6);	/* Set DA */
			memcpy(pData+6, pEthHdr, 6);	/* Set SA */
#else
			memcpy(pData, pEthHdr, 6);	/* Set DA */
			memcpy(pData+6, pEthHdr+6, 6);	/* Set SA */
//			zd1205_dump_data("pEthHdr = ", pEthHdr, 12);
#endif

//printk("dataLen=%d\n",dataLen);
//zd1205_dump_data("pData = ", pData, dataLen);
			pdot11Obj->RxInd(pData, dataLen, buf);
			return;
		}

	}
	else 
	{	//Mgt Frame
#if 1 //ivan
    {
        MLME_rx(mlme_p,pHdr,hdrLen+bodyLen,0);
    }
#else
		pRxSignal = allocSignal();
		if (!pRxSignal)
		{
	   		FPRINT("zd_ReceivePkt out of signal");
	   		FPRINT_V("freeSignalCount", freeSignalCount);
			goto rx_release;
		}	
		
		pRxFdesc = allocFdesc();
		if (!pRxFdesc)
		{
    		FPRINT("zd_ReceivePkt out of description");
    		FPRINT_V("freeFdescCount", freeFdescCount);
    		freeSignal(pRxSignal);
			goto rx_release;
		}	
		else{
			//pRxFdesc->bDataFrm = bDataFrm;
			pRxFrame = pRxFdesc->mpdu;
			pRxFrame->HdrLen = hdrLen;
			pRxFrame->bodyLen = bodyLen;
			memcpy(pRxFrame->header, pHdr, hdrLen);
	    	pRxFrame->body = pBody;
			pRxSignal->buf = buf;
			pRxSignal->vapId = vapId;
			pRxSignal->frmInfo.frmDesc = pRxFdesc;
			if (!RxMgtMpdu(pRxSignal)){
				freeSignal(pRxSignal);
				freeFdesc(pRxFdesc);
				pdot11Obj->ReleaseBuffer(buf);
			}	
			return;	
		}
#endif		
	}
	
rx_release:	
	pdot11Obj->ReleaseBuffer(buf);
	return;
}	

void zd_InitWepData(void)
{
	mWepIv[0] = 0;
	mWepIv[1] = 0;
	mWepIv[2] = 0;
	mWepIv[3] = 0;
	mBcIv[0] = 0;
	mBcIv[1] = 0;
	mBcIv[2] = 0;
	mBcIv[3] = 0;
	initWepState();
}	

void release_80211_Buffer(void)
{
	releaseSignalBuf();
	releaseFdescBuf();
}


//Cmd Functions
BOOLEAN zd_Reset80211(zd_80211Obj_t * pObj)
{
	pdot11Obj = pObj;
	
	initSignalBuf();
	initFdescBuf();
	ResetPSMonitor();
	ResetPMFilter();
	zd_InitWepData();
	return TRUE;
}	


BOOLEAN zd_HandlePsPoll(U8 *pHdr)
{
	Frame_t psPollFrame;
	
	//PSDEBUG("zd_HandlePsPoll");
	psPollFrame.HdrLen = 16;
	psPollFrame.bodyLen = 0;
	memcpy(&psPollFrame.header[0], pHdr, 16);
	RxPsPoll(&psPollFrame);
	return TRUE;
}	


BOOLEAN zd_StartAP(void)
{
printk("zd_StartAP\n");
    
	HW_SwitchChannel(pdot11Obj, mRfChannel);
	HW_SetSupportedRate(pdot11Obj, (U8 *)&mBrates);
	ConfigBcnFIFO();
#if 1 //ivan	
	HW_EnableBeacon(pdot11Obj, mBeaconPeriod, mDtimPeriod);
#endif	
	HW_RadioOnOff(pdot11Obj, mRadioOn);
	return TRUE;
}

//added by ivan
BOOLEAN zd_StartSTA(void)
{
//    printk("zd_StartSTA\n");
	HW_SwitchChannel(pdot11Obj, mRfChannel);
	HW_SetSupportedRate(pdot11Obj, (U8 *)&mBrates);
//	ConfigBcnFIFO();
#if 1 //ivan	
	HW_EnableBeacon(pdot11Obj, mBeaconPeriod, mDtimPeriod);
#endif	
	HW_RadioOnOff(pdot11Obj, mRadioOn);
	return TRUE;
}

BOOLEAN zd_CmdDisasoc(MacAddr_t *sta, U8 rCode)
{
	Signal_t *signal;
	
	if (isGroup(sta))
		return FALSE;
		
	if ((signal = allocSignal()) == NULL)  
		return FALSE;
	
	signal->id = SIG_DIASSOC_REQ;
	signal->block = BLOCK_ASOC;
	memcpy(&signal->frmInfo.Sta, sta, 6);
	signal->frmInfo.rCode = (ReasonCode)rCode;
	sigEnque(pMgtQ, (signal));
	
	return TRUE;
}	


BOOLEAN zd_CmdDeauth(MacAddr_t *sta, U8 rCode)
{
	Signal_t *signal;
	
	if ((signal = allocSignal()) == NULL)  
		return FALSE;
		
	signal->id = SIG_DEAUTH_REQ;
	signal->block = BLOCK_AUTH_REQ;
	memcpy(&signal->frmInfo.Sta, sta, 6);
	signal->frmInfo.rCode = (ReasonCode)rCode;
	sigEnque(pMgtQ, (signal));
	
	return TRUE;
}	


BOOLEAN zd_PassiveScan(void)
{
	void *reg = pdot11Obj->reg;
	
	if (pdot11Obj->ConfigFlag & CHANNEL_SCAN_SET)
		return FALSE;
		
	pdot11Obj->ConfigFlag |= CHANNEL_SCAN_SET;
	pdot11Obj->SetReg(reg, ZD_Rx_Filter, 0x100); //only accept beacon frame 
	HW_SwitchChannel(pdot11Obj, CurrScanCH);
	pdot11Obj->StartTimer(SCAN_TIMEOUT, DO_SCAN);
	
	return TRUE;
}	


BOOLEAN zd_DisasocAll(U8 rCode)
{
	int i;
	MacAddr_t *sta;
	
	for (i=1; i<(MAX_AID+1); i++){ 
		if (sstByAid[i]->asoc == STATION_STATE_ASOC){
			sta = (MacAddr_t *)&sstByAid[i]->mac[0];
			zd_CmdDisasoc(sta, rCode);
		}
	}	
	
	FlushQ(pTxQ);
	return TRUE;
	
}	


BOOLEAN zd_CmdProcess(U16 CmdId, void *parm1, U32 parm2)
{
	BOOLEAN status;
	
	switch(CmdId){
		case CMD_RESET_80211:
			status = zd_Reset80211((zd_80211Obj_t *)parm1);
			break;
		
		case CMD_ENABLE:
#if 0 //ivan
			status = zd_StartAP();
#else
            status = zd_StartSTA();			
#endif			
			break;
			
		case CMD_DISASOC: //IAPP cth
			status = zd_CmdDisasoc((MacAddr_t*)parm1, (U8)parm2);
			break;
			
		case CMD_DEAUTH://MAC filter cth
			status = zd_CmdDeauth((MacAddr_t*)parm1, (U8)parm2);
			break;
			
		case CMD_PS_POLL:
			//PSDEBUG("CMD_PS_POLL");
			status = zd_HandlePsPoll((U8 *)parm1);
			break;	
		
		case CMD_SCAN:
			status = zd_PassiveScan();
			break;
			
		case CMD_DiSASOC_ALL:	
			status = zd_DisasocAll((U8)parm2);
			break;
		
		default:
			status = FALSE;	
			break;		
	}	
	
	return status;
}	


//Event Nofify Functions
void zd_NextBcn(void)
{
    printk("zd_NextBcn\n");
	if (mDtimCount == 0)
		mDtimCount = mDtimPeriod;
	mDtimCount--;
	ConfigBcnFIFO();
	
	if (pTxQ->cnt)
   		pdot11Obj->QueueFlag |= TX_QUEUE_SET;
   			
	return;
}	


void zd_DtimNotify(void)
{

	SendMcPkt();

	return; 
}


extern BOOLEAN Tchal_WaitChalRsp(Signal_t *signal);
void zd_SendTChalMsg(void)
{
	Tchal_WaitChalRsp(NULL);
	return;
}	


void zd_SwitchNextCH(void)
{
	void *reg = pdot11Obj->reg;
	
	if (CurrScanCH >MAX_CHANNEL_ALLOW){
#if 1 //ivan
		pdot11Obj->SetReg(reg, ZD_Rx_Filter, ((0x400 << 16) | (0xffff & ~0x100))); 
#else
        pdot11Obj->SetReg(reg, ZD_Rx_Filter, ((0x400 << 16) | (0xffff))); 
#endif		
		pdot11Obj->ConfigFlag &= ~CHANNEL_SCAN_SET;
		HW_SwitchChannel(pdot11Obj, mRfChannel);
		CurrScanCH = 1;
		return;
	}	
		
	CurrScanCH++;
	HW_SwitchChannel(pdot11Obj, CurrScanCH);
	pdot11Obj->StartTimer(SCAN_TIMEOUT, DO_SCAN);
	return;
}	


void zd_UpdateCurrTxRate(U8 rate, U16 aid)
{
	Hash_t * pHash;
//ivan	void *reg = pdot11Obj->reg;
		
	if (aid){
		pHash = sstByAid[aid];
		pHash->CurrTxRate = rate;
	}	
}	

void zd_PsChange(U8 PwrState)
{
	//FPRINT("zd_PsChange");
	
	mPwrState = PwrState;
	mRequestFlag |= PS_CHANGE_SET;
	return;
}

void zd_EventNotify(U16 EventId, U32 parm1, U32 parm2, U32 parm3)
{
	switch(EventId){
		case EVENT_TBCN:
//ivan
//			zd_NextBcn();
			break;
			
		case EVENT_DTIM_NOTIFY:
			zd_DtimNotify();
			break;		
			
		case EVENT_TX_COMPLETE:
			TxCompleted(parm1, (U8)parm2, (U16)parm3);
			break;
			
		case EVENT_TCHAL_TIMEOUT:	
			zd_SendTChalMsg();	
			break;
			
		case EVENT_SCAN_TIMEOUT:
			zd_SwitchNextCH();
			break;	
			
		case EVENT_UPDATE_TX_RATE:
			zd_UpdateCurrTxRate((U8)parm1, (U16)parm2);
			break;		
		
		case EVENT_SW_RESET:
            //zd_SwReset();
            break;

        case EVENT_BUF_RELEASE:
            release_80211_Buffer();
            break;        
            
		case EVENT_PS_CHANGE:
			zd_PsChange((U8)parm1);
			break;
			
		case EVENT_MORE_DATA:
			mPwrState = 0;
			mRequestFlag |= PS_POLL_SET;
			break;                           
            	
		default:
			break;		
	}	
	
	return;
}	


BOOLEAN zd_CleanupTxQ(void)
{	
	//FPRINT("*****zd_CleanupTxQ*****");
    while(CleanupTxQ());
    
    if (!pTxQ->cnt){
   		pdot11Obj->QueueFlag &= ~TX_QUEUE_SET;
        return TRUE;
    }    
    else
        return FALSE;    
      
}  


BOOLEAN zd_CleanupAwakeQ(void)
{	
#if 0 //ivan
	Signal_t *signal;
	FrmInfo_t *pfrmInfo;
	FrmDesc_t *pfrmDesc;
#endif
	
	//PSDEBUG("*****zd_CleanupAwakeQ*****");
    while(CleanupAwakeQ());
    
    if (!pAwakeQ->cnt){
   		pdot11Obj->QueueFlag &= ~AWAKE_QUEUE_SET;
        return TRUE;
    }    
    else{
#if 0    
    	while(pAwakeQ->cnt){
    		signal = sigDeque(pAwakeQ);
    		pfrmInfo = &signal->frmInfo;
			pfrmDesc = pfrmInfo->frmDesc;
			freeFdesc(pfrmDesc);
       		pdot11Obj->ReleaseBuffer(signal->buf);
        	freeSignal(signal);
		}	
#endif		
        return FALSE;
    }        
      
}  


void ShowQInfo(void)
{	
	 printk(KERN_DEBUG "AwakeQ = %x, MgtQ = %x, TxQ  = %x, mcQ  = %x\n",
        pAwakeQ->cnt, pMgtQ->cnt, pTxQ->cnt, pPsQ[0]->cnt);
     printk(KERN_DEBUG "PsQ1   = %x, PsQ2 = %x, PsQ3 = %x, PsQ4 = %x\n",
        pPsQ[1]->cnt, pPsQ[2]->cnt, pPsQ[3]->cnt, pPsQ[4]->cnt);     
}


void ShowHashInfo(U8 aid)
{
	Hash_t *pHash = NULL;
	
	if (aid){ 
		pHash = sstByAid[aid];
		zd1205_dump_data("Mac Addr = ", pHash->mac, 6);
		FPRINT_V("Auth", pHash->auth);
		FPRINT_V("Asoc", pHash->asoc);
		FPRINT_V("psm", pHash->psm);
		FPRINT_V("Aid", pHash->aid);
		FPRINT_V("lsInterval", pHash->lsInterval);
		FPRINT_V("encryMode", pHash->encryMode);
		FPRINT_V("pkInstalled", pHash->pkInstalled);
		FPRINT_V("ZydasMode", pHash->ZydasMode);
		FPRINT_V("AlreadyIn", pHash->AlreadyIn);
		FPRINT_V("CurrTxRate", pHash->CurrTxRate);
		FPRINT_V("MaxRate", pHash->MaxRate);
		FPRINT_V("Preamble", pHash->Preamble);
		FPRINT_V("KeyId", pHash->KeyId);
		FPRINT_V("IV16", pHash->TxSeed.IV16);
//ivan		FPRINT_V("IV32", pHash->TxSeed.IV32);
		zd1205_dump_data("TK = ", pHash->TxSeed.TK, 16);
		zd1205_dump_data("Tx MIC K0 = ", (U8 *)&pHash->TxMicKey.K0, 4);
		zd1205_dump_data("Tx MIC K1 = ", (U8 *)&pHash->TxMicKey.K1, 4);
		zd1205_dump_data("Rx MIC K0 = ", (U8 *)&pHash->RxMicKey.K0, 4);
		zd1205_dump_data("Rx MIC K1 = ", (U8 *)&pHash->RxMicKey.K1, 4);
	}
	else {
		FPRINT_V("KeyId", mWpaBcKeyId);
		FPRINT_V("GkInstalled", mGkInstalled);
		FPRINT_V("IV16", mIv16);
//ivan		FPRINT_V("IV32", mIv32);
		zd1205_dump_data("keyContent = ", pHash->keyContent, 16);
		zd1205_dump_data("TK = ", mBcSeed.TK, 16);
		zd1205_dump_data("Tx MIC K0 = ", (U8 *)&mBcMicKey.K0, 4);
		zd1205_dump_data("Tx MIC K1 = ", (U8 *)&mBcMicKey.K1, 4);
	}		
}	

void zd_UpdateCardSetting(card_Setting_t *pSetting)
{
	void *reg = pdot11Obj->reg;
	static BOOLEAN InitConfig = TRUE;

    if(mlme_dbg_level&MLME_DEBUG_HARDWARE_CTL)
        printk("<zd_UpdateCardSetting>\n");
	
	if (pSetting->AuthMode == 0){ //open system only
		mAuthAlogrithms[0] = OPEN_SYSTEM;
		mAuthAlogrithms[1] = NULL_AUTH;
	}else if (pSetting->AuthMode == 1){	//shared key only
		mAuthAlogrithms[0] = SHARE_KEY;
		mAuthAlogrithms[1] = NULL_AUTH;
	}else if (pSetting->AuthMode == 2){	//auto auth mode
		mAuthAlogrithms[0] = OPEN_SYSTEM;
		mAuthAlogrithms[1] = SHARE_KEY;
	}	
	
	if (mLimitedUser != pSetting->LimitedUser){
		mLimitedUser = pSetting->LimitedUser;
	}
		
	mBlockBSS = pSetting->BlockBSS;
	mSwCipher = pSetting->SwCipher;	
	mKeyFormat = pSetting->EncryMode;
	if(mlme_dbg_level&MLME_DEBUG_HARDWARE_CTL)
	    printk("mSwCipher=%d mKeyFormat=%d\n",mSwCipher,mKeyFormat);
	    
	//if (!InitConfig)
	{	    
		pdot11Obj->SetReg(reg, ZD_EncryType, (mKeyFormat | mSwCipher << 3));
//		printk("mKeyFormat=%x\n",mKeyFormat);
	}
	
	mKeyId = pSetting->EncryKeyId;
	mBcKeyId = pSetting->BcKeyId;
	mDynKeyMode = pSetting->DynKeyMode;
	mFragThreshold = pSetting->FragThreshold;
	mRtsThreshold = pSetting->RTSThreshold;
	
	mBeaconPeriod = pSetting->BeaconInterval;
	mDtimPeriod = pSetting->DtimPeriod;
	if (!InitConfig)
		HW_EnableBeacon(pdot11Obj, mBeaconPeriod, mDtimPeriod);
	
	if (mRadioOn != pSetting->RadioOn){
	 	mRadioOn = pSetting->RadioOn;
	 	if (!InitConfig)
	 		HW_RadioOnOff(pdot11Obj, mRadioOn);
	 }	
	
	if (mRfChannel != pSetting->Channel){
		mRfChannel = pSetting->Channel;
		mPhpm.buf[0] = EID_DSPARMS;
		mPhpm.buf[1] = 1;
		mPhpm.buf[2] = mRfChannel;
		if (!InitConfig)
		{
#if 1 //ivan	
            
#else
			HW_SwitchChannel(pdot11Obj, mRfChannel);
#endif			
	    }
	}	
	
	mPreambleType = pSetting->PreambleType;	
	if (mPreambleType)
		mCap |= CAP_SHORT_PREAMBLE;
	else
		mCap &= ~CAP_SHORT_PREAMBLE;
	
	mPrivacyInvoked = pSetting->EncryOnOff;
	if (pSetting->DynKeyMode > 0)
		mPrivacyInvoked = TRUE;
		
	if (mPrivacyInvoked)
		mCap |= CAP_PRIVACY;
	else
		mCap &= ~CAP_PRIVACY;	
		
	memcpy(&dot11DesiredSsid, pSetting->Info_SSID,  pSetting->Info_SSID[1]+2);
	memcpy(&mSsid, &dot11DesiredSsid,  dot11DesiredSsid.buf[1]+2);
		
	mHiddenSSID = pSetting->HiddenSSID;	
	if (mHiddenSSID){
		mSsid.buf[0] = EID_SSID;
		mSsid.buf[1] = 1;
		mSsid.buf[2] = 0x0;
	}
	
	memcpy(&mBrates, pSetting->Info_SupportedRates, pSetting->Info_SupportedRates[1]+2);
	if (!InitConfig)
		HW_SetSupportedRate(pdot11Obj, (U8 *)&mBrates);
#if 0//ivan
	memcpy((U8 *)&mBssId, pSetting->MacAddr, 6);
#endif	
	memcpy((U8 *)&dot11MacAddress, pSetting->MacAddr, 6);
	
	memcpy(&mKeyVector[0][0], &pSetting->keyVector[0][0], sizeof(mKeyVector));
	mWepKeyLen = pSetting->WepKeyLen;
	
	memcpy(&mBcKeyVector[0], &pSetting->BcKeyVector[0], sizeof(mBcKeyVector));
	mBcKeyLen = pSetting->BcKeyLen;
	
	if (mDynKeyMode == DYN_KEY_TKIP){
		memcpy(&mWPAIe.buf[0], pSetting->WPAIe, pSetting->WPAIeLen);
	}
		
	if (!InitConfig)
		zd_CmdProcess(CMD_DiSASOC_ALL, 0, ZD_UNSPEC_REASON);
	
	zd_InitWepData();	
	//mGkInstalled = 0;
	InitConfig = FALSE;
}	



extern BOOLEAN SynchEntry(Signal_t* signal);
extern BOOLEAN AuthReqEntry(Signal_t* signal);
extern BOOLEAN AuthRspEntry(Signal_t* signal);
extern BOOLEAN AsocEntry(Signal_t* signal);	
//State machine entry point
void zd_SigProcess(void)
{
	Signal_t* 	signal = NULL;
	BOOLEAN		ret;
	
	while((signal = sigDeque(&mgtQ)) != NULL){
		switch(signal->block){
			case BLOCK_SYNCH:
				ret = SynchEntry(signal);
				break;
				
			case BLOCK_AUTH_REQ:
				ret = AuthReqEntry(signal);
				break;	
				
			case BLOCK_AUTH_RSP:
				ret = AuthRspEntry(signal);
				break;	
				
			case BLOCK_ASOC:
				ret = AsocEntry(signal);
				break;	
				
			default:
				ret = TRUE;
				break;		
		}	
		
		if (ret){
			pdot11Obj->ReleaseBuffer(signal->buf);
			freeSignal(signal);
		}	
	}
	pdot11Obj->QueueFlag &= ~MGT_QUEUE_SET;
}


U8 zd_CheckTotalQueCnt(void)
{
	U8 TotalQueCnt = 0;
	U32 flags;
	int i;
		
	flags = pdot11Obj->EnterCS();
	
	for (i=0; i<MAX_AID+1; i++)
		TotalQueCnt += pPsQ[i]->cnt;
		
	TotalQueCnt += pAwakeQ->cnt;
	TotalQueCnt += pTxQ->cnt;
	TotalQueCnt += pMgtQ->cnt;	
	pdot11Obj->ExitCS(flags);
	
	return TotalQueCnt;
}	


void zd_PerSecTimer(void)
{
	static U32 sec = 0;

	sec++;
	if (sec > AGE_HASH_PERIOD){
//		void *reg = pdot11Obj->reg;
		
		mZyDasModeClient = FALSE;
		AgeHashTbl();
		sec = 0;
	}	
}
	
BOOLEAN zd_QueryStaTable(U8 *sta, void **ppHash)
{
	Hash_t *pHash = NULL;
	MacAddr_t *addr = (MacAddr_t*) sta;
	
	pHash = HashSearch(addr); 
	
	*ppHash = pHash;
	
	if (!pHash) 
		return FALSE;
		
	if (pHash->asoc == STATION_STATE_ASOC)
        return TRUE; 
    else
    	return FALSE; 
}		

#endif

