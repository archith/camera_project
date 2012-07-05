#ifndef __ZDPMFILTER_C__
#define __ZDPMFILTER_C__

#include "zd80211.h"


void AgePsQ(U16 aid);
BOOLEAN TxSend(Signal_t *signal, FrmDesc_t *pfrmDesc);


U8 usedID = 0;
U8	atimBitMap[(MAX_AID/8)+2];
Signal_t *txRequest[BURST_NUM];
static BOOLEAN	mcBuffered = FALSE;


//make tim for beacon frame
void mkTim(Element *tim, TrafficMap_t *trf, U8 dtc, U8 dtp, U16 aidLo, U16 aidHi, BOOLEAN bc)
{
	int	i;
	U8 *map = (U8*)trf->t;
	U16	N1 = 0;
	U16	N2 = 0;
	U8	index = 0;
	
	tim->buf[0] = EID_TIM;
	tim->buf[2] = dtc;	
	tim->buf[3] = dtp;	
	
	// Calculate N1
	for (i=0; i<=(aidHi/8); i++){
		if (map[i] != 0){
			break;
		}
	}
	
	if (i>0){
		// N1 is the largest even number
		N1 = (U16)(i & ~0x01);
	}
	
	// Calculate N2
	for (i=(aidHi/8); i>=0; i--){
		if (map[i] != 0){
			break;
		}
	}
	
	if (i>0){
		N2 = (U16)i;
	}
	
	// Fill the content
	if (N2==0){
		tim->buf[4] = 0;
		tim->buf[5] = map[0];
		tim->buf[1] = 4;
	}
	else{
		tim->buf[4] = (U8)N1;
		for (i=N1; i<=N2; i++){
			tim->buf[5+index++] = map[i];
		}
		tim->buf[1] = N2-N1+4;
	}

	if (bc){
		tim->buf[4] |= 0x01;
	}
}


void TxCompleted(U32 result, U8 retID, U16 aid) //in isr routine
{
	Signal_t *signal;
	FrmInfo_t *pfrmInfo;
	FrmDesc_t *pfrmDesc;
	void *buf;
	void *fragBuf;
	Frame_t *pf;
	int i; 
	Hash_t *pHash = NULL;
	U8	Num;
	U8	bIntraBss;
	
	signal = txRequest[retID];
	pfrmInfo = &signal->frmInfo;
	pfrmDesc = pfrmInfo->frmDesc;

	buf = signal->buf;
	bIntraBss = pfrmDesc->bIntraBss;

	if (aid){
		pHash = sstByAid[aid];
		if (!pHash)
			goto no_rate_info;
			
		if (result == ZD_TX_CONFIRM){
		//for rate adaption
			if (((pHash->CurrTxRate < RATE_11M) && (pHash->ContSuccFrames > RISE_RATE_THRESHOLD))
					|| ((pHash->CurrTxRate > RATE_5M) && (pHash->ContSuccFrames > HIGH_RISE_RATE_THRESHOLD))){
				pHash->ContSuccFrames = 0;
				if (pHash->MaxRate > pHash->CurrTxRate){
					pHash->CurrTxRate++;
				}	
			}
			else{
				pHash->ContSuccFrames++;
			}
		}	
	}	
	
no_rate_info:	
	pf = pfrmDesc->mpdu;
	Num = pfrmInfo->fTot;
	if ((mSwCipher) && (wepBit(pf))){
		//FPRINT_V("Tx comp fTot", pMsdu->fTot);
		if (Num > 1){
			for (i=0; i<Num; i++){
				pf = &pfrmDesc->mpdu[i];
				fragBuf = pf->fragBuf;
				//FPRINT_V(" TX comp fragBuf", (U32)fragBuf);
				pdot11Obj->ReleaseBuffer(fragBuf);
				pf->fragBuf = NULL;
			}	
		}	
	}	
		
	freeFdesc(pfrmDesc);
	pdot11Obj->ReleaseBuffer(buf);
	freeSignal(signal);
	txRequest[retID] = NULL;

	if ((buf) && (!bIntraBss)){	//upper layer data
		pdot11Obj->TxCompleted();
	}		
}	


BOOLEAN TxSend(Signal_t *signal, FrmDesc_t *pfrmDesc)
{	
	FrmInfo_t *pfrmInfo;
	//FrmDesc_t *pfrmDesc;
   	U32 nextBodyLen; 
	fragInfo_t fragInfo;
	int i;
    U8 bIntraBss = 0;
	Frame_t *pf; 
	Frame_t *nextPf;
	U32 flags;
	Hash_t *pHash = NULL;
	U32	icv = 0xFFFFFFFFL;
	U32 *tmpiv = NULL;
	U8	WepKeyLen = 0;
	U8 *pWepKey = NULL;
	U8 *pIv = NULL;
	U8 KeyId = 5;
	U8 RC4Key[16];
	U32 iv32 = 0;
	BOOLEAN	bExtIV = FALSE;
	U8 EncryType;
	BOOLEAN	bGroupAddr = FALSE;
	U8 bWep = 0;
	U8 vapId = 0;
	U8 Num;
	U8 bDataFrm = signal->bDataFrm;
	//U8 bDataFrm; 
		
	txRequest[usedID] = signal;					
	pfrmInfo = &signal->frmInfo;
	//pfrmDesc = pfrmInfo->frmDesc;
    bIntraBss = pfrmDesc->bIntraBss;
	
	//bDataFrm = pfrmDesc->bDataFrm;
	pf = pfrmDesc->mpdu;
	pHash = pfrmDesc->pHash;
	bWep = wepBit(pf);
	EncryType = mKeyFormat;
	
	if (isGroup(addr1(pf))){
		bGroupAddr = TRUE;
		fragInfo.rate = pdot11Obj->BasicRate;
		fragInfo.aid = 0;
		fragInfo.preamble = 0;

		
		if ((mSwCipher) && (bWep)){
			if ((mDynKeyMode == DYN_KEY_WEP64) || (mDynKeyMode == DYN_KEY_WEP128)){
				WepKeyLen = mBcKeyLen;
				pWepKey = &mBcKeyVector[0];
				pIv = &mBcIv[1];
				KeyId = mBcKeyId;
				tmpiv = (U32 *)mBcIv;
			}
		}	
	}
	else{ // unicast frame		
		//pHash = HashSearch(addr1(pf)); 
		if (!pHash){
			// Should be Probe Response frame
			fragInfo.rate = pdot11Obj->BasicRate;
			fragInfo.aid = 0;
			fragInfo.preamble = 0;
		}	
		else{
			fragInfo.rate = pHash->CurrTxRate;
			fragInfo.aid = (U16)pHash->aid;
			fragInfo.preamble = pHash->Preamble;
			EncryType = pHash->encryMode;
			
			//get pairwise key
			if ((mSwCipher) && (bWep)){
				if ((mDynKeyMode == DYN_KEY_WEP64) || (mDynKeyMode == DYN_KEY_WEP128)){
					WepKeyLen = pHash->keyLength;
					pWepKey = pHash->keyContent;
					pIv = &pHash->wepIv[1];
					KeyId = pHash->KeyId;
					tmpiv = (U32 *)pHash->wepIv;
				}
			}		
		}
	}	

	if ((mSwCipher) && (bWep)){
		if (mDynKeyMode == 0){  // static 4 keys
			WepKeyLen = mWepKeyLen;
			pWepKey = &mKeyVector[mKeyId][0];
			pIv = &mWepIv[1];
			KeyId = mKeyId;
			tmpiv = (U32 *)mWepIv;
		}
	}
	
	Num = pfrmInfo->fTot;
	//FPRINT_V("Tx fTot", pMsdu->fTot);
	for (i=0; i<Num; i++){
		pf = &pfrmDesc->mpdu[i]; 
		if (Num == 1){
           	nextBodyLen = 0; 
			if (!bDataFrm){ //Management frame
				bIntraBss = 0;
				if (frmType(pf) == ST_PROBE_RSP){
					U32 loTm, hiTm;
					HW_GetTsf(pdot11Obj, &loTm, &hiTm);
					setTs(pf, loTm, hiTm);
				}
			}		
		}		
		else{	
			if (Num != (i+1)){
				nextPf = &pfrmDesc->mpdu[i+1];
				nextBodyLen = nextPf->bodyLen;
			}	
			else
				nextBodyLen = 0;
		}	
		
		//prepare frag information
		fragInfo.macHdr[i] = &pf->header[0];
		fragInfo.macBody[i] = pf->body;
		fragInfo.bodyLen[i] = pf->bodyLen;
		fragInfo.nextBodyLen[i] = nextBodyLen;

		if ((mSwCipher) && (bWep)){
			void *fragBuf = NULL;
			U8 *pInBuf = NULL;
			U8 *pOutBuf = NULL;
			U16 NumOfBytes = 0;
	
			if (mDynKeyMode == DYN_KEY_TKIP){
				if (bGroupAddr){
					if ((mGkInstalled) && (mWpaBcKeyLen == 32)){
						mIv16++;
						if (mIv16 == 0)
							mIv32++;
						Tkip_phase1_key_mix(mIv32, &mBcSeed);
						Tkip_phase2_key_mix(mIv16, &mBcSeed);
						Tkip_getseeds(mIv16, RC4Key, &mBcSeed);
						mBcSeed.IV16 = mIv16;
						mBcSeed.IV32 = mIv32;
						WepKeyLen = 13;
						pWepKey = &RC4Key[3];
						pIv = RC4Key;
						KeyId = mWpaBcKeyId;
						iv32 = mIv32;
						bExtIV = TRUE;
					}
					else {
						WepKeyLen = mBcKeyLen;
						pWepKey = &mBcKeyVector[0];
						pIv = &mBcIv[1];
						KeyId = mWpaBcKeyId;
						tmpiv = (U32 *)mBcIv;
					}		
				}
				else { //unicast
					if ((pHash) && (pHash->pkInstalled)){
						pHash->iv16++;
						if (pHash->iv16 == 0)
							pHash->iv32++;
						
						Tkip_phase1_key_mix(pHash->iv32, &pHash->TxSeed);
						Tkip_phase2_key_mix(pHash->iv16, &pHash->TxSeed);
						Tkip_getseeds(pHash->iv16, RC4Key, &pHash->TxSeed);
						// update iv16, iv32
						pHash->TxSeed.IV16 = pHash->iv16;
						pHash->TxSeed.IV32 = pHash->iv32;
						WepKeyLen = 13;
						pWepKey = &RC4Key[3];
						pIv = RC4Key;
						KeyId = pHash->KeyId;
						iv32 = pHash->iv32;
						bExtIV = TRUE;
					}
			 	}	
			} // end of (mDynKeyMode == DYN_KEY_TKIP)		
			
			NumOfBytes = fragInfo.bodyLen[i];
			pInBuf = fragInfo.macBody[i];
			
			if (Num == 1){ // no fragment
				pOutBuf = pInBuf;
			}	
			else {
				fragBuf = pdot11Obj->AllocBuffer((NumOfBytes+ICV_LNG), &pOutBuf);
				if 	(!fragBuf){
					FPRINT("Alloc Frag buffer failed !!!!");
					// to do....
					pOutBuf = pInBuf; // The receiver will decrption error, and drop it
				}
				else {
					//FPRINT_V("Tx fragBuf", (U32)fragBuf);
					pf->fragBuf = fragBuf;
				}	
			}	
			
			//zd1205_dump_data("pInBuf = ", pInBuf, NumOfBytes);
			zd_EncryptData(WepKeyLen, pWepKey, pIv, NumOfBytes,	pInBuf, pOutBuf, &icv);
			
			//FPRINT_V("KeyId", KeyId);
			//FPRINT_V("WepKeyLen", WepKeyLen);
			//FPRINT_V("NumOfBytes", NumOfBytes);
			//zd1205_dump_data("pWepKey = ", pWepKey, WepKeyLen);
			//zd1205_dump_data("pIv = ", pIv, 4);
			//zd1205_dump_data("pOutBuf = ", pOutBuf, NumOfBytes+ICV_LNG);
			
			fragInfo.macBody[i] = pOutBuf;    	
    		fragInfo.bodyLen[i] = NumOfBytes + ICV_LNG;
   		
    		if (Num != (i+1))
				fragInfo.nextBodyLen[i] += ICV_LNG;
			else
				fragInfo.nextBodyLen[i] = 0;	
				
			fragInfo.macHdr[i][MAC_HDR_LNG] = pIv[0];
    		fragInfo.macHdr[i][MAC_HDR_LNG+1] = pIv[1];
    		fragInfo.macHdr[i][MAC_HDR_LNG+2] = pIv[2];
    		fragInfo.macHdr[i][MAC_HDR_LNG+3] = KeyId << 6;
    		
    		*tmpiv  = (((*tmpiv) & 0x00FFFFFF) + 1) | ((*tmpiv) & 0xFF000000);
    		
			if (bExtIV){
				fragInfo.macHdr[i][MAC_HDR_LNG+3] |= 0x20;
				fragInfo.macHdr[i][MAC_HDR_LNG+4] = (U8)(iv32);
				fragInfo.macHdr[i][MAC_HDR_LNG+5] = (U8)(iv32 >> 8);
				fragInfo.macHdr[i][MAC_HDR_LNG+6] = (U8)(iv32 >> 16);
				fragInfo.macHdr[i][MAC_HDR_LNG+7] = (U8)(iv32 >> 24);
			}	
    	} //end of ((mSwCipher) && (wepBit(pf)))	
	}

	fragInfo.msgID = usedID;
    fragInfo.bIntraBss = bIntraBss;
    fragInfo.buf = signal->buf;
	fragInfo.totalFrag = Num;
	fragInfo.hdrLen = MAC_HDR_LNG;
	
	if ((mSwCipher) && (bWep)){
		fragInfo.hdrLen += IV_LNG;
		if (bExtIV) {
			fragInfo.hdrLen += EIV_LNG;
		}	 
	}	
	//fragInfo.encryType = EncryType;	
	//fragInfo.vapId = vapId;	
	
	//flags = pdot11Obj->EnterCS();
	pdot11Obj->SetupNextSend(&fragInfo);
    flags = pdot11Obj->EnterCS();
	usedID++;	
	if (usedID > (BURST_NUM -1))
		usedID = 0;
	pdot11Obj->ExitCS(flags);
	
	return FALSE;
}


void FlushQ(SignalQ_t *Q)
{
	Signal_t *signal;
    FrmInfo_t *pfrmInfo;
    FrmDesc_t *pfrmDesc;
	
	while((signal = sigDeque(Q)) != NULL){
        pfrmInfo = &signal->frmInfo;
	    pfrmDesc = pfrmInfo->frmDesc;

	    pdot11Obj->ReleaseBuffer(signal->buf);
		freeFdesc(pfrmDesc);
		freeSignal(signal);
	}
}


void tMapSet(U8 *map, U16 aid, BOOLEAN flag)
{
	U8	mask, index;



	if ((aid == 0) || (aid > MAX_AID)) 
		return;
		
	index = aid / 8;
	mask = 0x01 << (aid % 8);
	
	if (flag)
		map[index] |= mask;
	else
		map[index] &= ~mask;	
}


BOOLEAN CleanupTxQ(void)
{
	Signal_t *signal;
	FrmInfo_t *pfrmInfo;
	FrmDesc_t *pfrmDesc;

	//PSDEBUG("CleanupTxQ");
	if (pTxQ->cnt > 0){
		signal = pTxQ->first;
		if (!signal)
			return FALSE;
	 			
		pfrmInfo = &signal->frmInfo;
		pfrmDesc = pfrmInfo->frmDesc;
	    if (!pdot11Obj->CheckTCBAvail(pfrmInfo->fTot))
	       	return FALSE;
            	
 		signal = sigDeque(pTxQ);
		goto send_PduReq;
	}
	
	return FALSE;


send_PduReq:
    TxSend(signal, pfrmDesc);
    return TRUE;
}



BOOLEAN CleanupAwakeQ(void)
{
	Signal_t *signal;
	FrmInfo_t *pfrmInfo;
	FrmDesc_t *pfrmDesc;

	//PSDEBUG("CleanupAwakeQ");
 	if (pAwakeQ->cnt > 0){
        signal = pAwakeQ->first;
        if (!signal)
			return FALSE;
	 			
        pfrmInfo = &signal->frmInfo;
        pfrmDesc = pfrmInfo->frmDesc;
        if (!pdot11Obj->CheckTCBAvail(pfrmInfo->fTot))
            	return FALSE;
            	
 		signal = sigDeque(pAwakeQ);
		//PSDEBUG("===== Queue out awakeQ");	
		//PSDEBUG_V("pAwakeQ->cnt", pAwakeQ->cnt);	
 		goto send_PduReq;
	}
	
	return FALSE;


send_PduReq:
    TxSend(signal, pfrmDesc);
    return TRUE;
}


void AgePsQ(U16 aid)
{
	Signal_t *psSignal;
	U16 interval;
    FrmDesc_t *pfrmDesc;
    U32 eol;
    FrmInfo_t *pfrmInfo;

	if ((aid == 0) || (aid > MAX_AID))	//Invalid AID
		return;
	
	while (pPsQ[aid]->cnt){ 
		interval = sstByAid[aid]->lsInterval; 
		if (interval == 0)
			interval = 1;

			
		psSignal = pPsQ[aid]->first;
		
		if (!psSignal)
			break;
			
		pfrmInfo = &psSignal->frmInfo;
		eol = pfrmInfo->eol;
		
		if ((HW_GetNow(pdot11Obj) - eol) < (2*interval*mBeaconPeriod*1024)) //TBD
			break;
		
		if ((HW_GetNow(pdot11Obj) - eol) < (1024*1024)) //TBD
			break;	
		


		//Data life time-out
		psSignal = sigDeque(pPsQ[aid]); 

		if (!psSignal)	
			break;
			
		PSDEBUG_V("*****Data life time-out, AID", aid);	
		//FPRINT_V("*****Data life time-out, AID", aid);
        pfrmDesc = pfrmInfo->frmDesc;
        freeFdesc(pfrmDesc);
        pdot11Obj->ReleaseBuffer(psSignal->buf);
        freeSignal(psSignal);
	}
	
	if (!pPsQ[aid]->cnt)
		tMapSet(atimBitMap, aid, FALSE);
		
	return;
}	


void PsPolled(MacAddr_t *sta, U16 aid)
{
	Signal_t *signal;
	FrmInfo_t *pfrmInfo;
	FrmDesc_t *pfrmDesc;
	Frame_t	*frame; 
	int i;
	U8 Num;

	//PSDEBUG("PsPolled");

	signal = sigDeque(pPsQ[aid]);
	if (!signal){
		PSDEBUG("No Queue data for PS-POLL!!!");	
		tMapSet(atimBitMap, aid, FALSE);
		return;
	}
	else{
		PSDEBUG_V("Queue out psQ, AID ", aid);	
		PSDEBUG_V("cnt ", pPsQ[aid]->cnt);
		pfrmInfo = &signal->frmInfo;
		pfrmDesc = pfrmInfo->frmDesc;

		Num = pfrmInfo->fTot;
		for (i=0; i<Num; i++){
			frame = &pfrmDesc->mpdu[i];
			//PSDEBUG_V("pfrmInfo ", (U32)pfrmInfo);
			//PSDEBUG_V("eol ", (U32)pfrmInfo->eol);
			PSDEBUG_V("pfrmDesc ", (U32)pfrmDesc);
			//PSDEBUG_V("frame ", (U32)frame);
			if (!pPsQ[aid]->cnt){
				//setMoreData(frame, 0);
				frame->header[1] &= ~MORE_DATA_BIT;
				PSDEBUG("More bit 0");

			}	
			else {	
				//setMoreData(frame, 1);
				frame->header[1] |= MORE_DATA_BIT;
				PSDEBUG("More bit 1");
			}	

			PSDEBUG_V("bodyLen ", frame->bodyLen);	
		}	

	
		if (!pdot11Obj->CheckTCBAvail(Num)){
            PSDEBUG("*****Fail to send out!!!");	
			PSDEBUG_V("Queue in psQ, AID", aid);
			sigEnque(pPsQ[aid], signal);
			return;
    	}
		else
			sigEnque(pTxQ, signal);
			return;
	}	
}	


void StaWakeup(MacAddr_t *sta)
{
	U16 aid;

	aid = AIdLookup(sta);
	if ((aid == 0) || (aid > MAX_AID))
		return;

	while(1){
		Signal_t *signal;
		signal = sigDeque(pPsQ[aid]);
		if (!signal) 
			break;
		sigEnque(pTxQ, signal); 
	}
}


void InitPMFilterQ(void)
{
	U8	i;
	static	BOOLEAN bFirstTime = TRUE;
	
	if (bFirstTime){
		bFirstTime = FALSE;
 		for (i=0; i < MAX_RECORD; i++){
			pPsQ[i] = &psQ[i];
			initSigQue(pPsQ[i]);
		}	
		pTxQ = &txQ;
		pAwakeQ = &awakeQ;
		initSigQue(pTxQ);
		initSigQue(pAwakeQ);
	}
	else{
		for (i=0; i < MAX_RECORD; i++)
			FlushQ(pPsQ[i]);

		FlushQ(pTxQ);
		FlushQ(pAwakeQ);
	}
	
	memset(atimBitMap, 0, sizeof(atimBitMap));
}


void ConfigBcnFIFO(void)
{
	int i, j;	
	BOOLEAN	bcst = FALSE;
    Signal_t *signal;
	U8 tim[256];
	U8 Beacon[128];
	U16 BcnIndex = 0;
	static U32	BcnCount = 0;
	U8	vapId = 0;
	U8	Len;

	if (mPsStaCnt > 0) {
		HW_SetSTA_PS(pdot11Obj, 1);
		for (i=1; i < (MAX_AID+1); i++){
			AgePsQ(i);
			if (pPsQ[i]->cnt){
				tMapSet(atimBitMap, i, TRUE);
				PSDEBUG_V("tMapSet Aid", i);	
			}	
		}	
	}
	else{

		HW_SetSTA_PS(pdot11Obj, 0);
		//send McQ
		if (pPsQ[0]->cnt){
			while(1){
				signal = sigDeque(pPsQ[0]);
				if (!signal)
					break;
				
				sigEnque(pTxQ, signal); 
			}
		}
	}	
		
	/* make beacon frame */
	/* Frame control */
	Beacon[BcnIndex++] = 0x80;	
	Beacon[BcnIndex++] = 0x00; 
	
	/* Duration HMAC will fill this field */
	//Beacon[BcnIndex++] = 0x00;	
	//Beacon[BcnIndex++] = 0x00;
	BcnIndex += 2;
	
	/* Address1 */
	Beacon[BcnIndex++] = 0xff;	
	Beacon[BcnIndex++] = 0xff;
	Beacon[BcnIndex++] = 0xff;
	Beacon[BcnIndex++] = 0xff;
	Beacon[BcnIndex++] = 0xff;
	Beacon[BcnIndex++] = 0xff;
	
	/* Address2 */
	for (j=0; j<6; j++)
		Beacon[BcnIndex++] = mBssId.mac[j];	
	
	/* Address3 */
	for (j=0; j<6; j++)
		Beacon[BcnIndex++] = mBssId.mac[j];
	
	/* Sequence control	HMAC will fill this field */
	//Beacon[BcnIndex++] = 0x00;	
	//Beacon[BcnIndex++] = 0x00;
	BcnIndex += 2;
	
	/* Timestamp	HMAC will fill this field */
	//for (j=0; j<8; j++)
		//Beacon[BcnIndex++] = 0x00;
	BcnIndex += 8;	
	
	/* BeaconInterval */
	Beacon[BcnIndex++] = mBeaconPeriod;
	Beacon[BcnIndex++] = mBeaconPeriod >> 8;
	
	/* Capability */
	Beacon[BcnIndex++] = mCap;
	Beacon[BcnIndex++] = 0;
	
	/* SSID */
	Len = eLen(&mSsid)+2;
	for (j=0; j<Len; j++)
		Beacon[BcnIndex++] = mSsid.buf[j];
	
	/* Supported rates */
	Len = eLen(&mBrates)+2;
	for (j=0; j<Len; j++)
		Beacon[BcnIndex++] = mBrates.buf[j];
	
	/* DS parameter */
	Beacon[BcnIndex++] = mPhpm.buf[0];	
	Beacon[BcnIndex++] = mPhpm.buf[1];	
	Beacon[BcnIndex++] = mPhpm.buf[2];	
	
	/* Tim */
	//if ((mDtimCount == 0) && (pPsQ[0]->cnt > 0)){ //dtim and buffer for mc
	if ((mDtimCount == 0) && mcBuffered){
		bcst = TRUE;
	}	
		
	mkTim((Element*)tim, (TrafficMap_t *)&atimBitMap, mDtimCount, mDtimPeriod, 1, MAX_AID, bcst);
	Len = tim[1]+2;
	for (j=0; j<Len; j++)
		Beacon[BcnIndex++] = tim[j];
	
	//Zydas high rate IE
	Beacon[BcnIndex++] = 0xfe; //16.5M
	Beacon[BcnIndex++] = 0x00;
	
	if ((pdot11Obj->rfMode == 0x04) || (pdot11Obj->rfMode == 0x07)){ //AL2210MPVB_RF
		Beacon[BcnIndex++] = 0xff; //27.5M
		Beacon[BcnIndex++] = 0x00;
	}	
	
	//WPA IE
	if (mDynKeyMode == DYN_KEY_TKIP){
		Len = mWPAIe.buf[1]+2;
		for (j=0; j<Len; j++)
			Beacon[BcnIndex++] = mWPAIe.buf[j];
	}	
		
	/* CRC32 HMAC will calucate this value */
	//for (j=0; j<4; j++)
	//	Beacon[BcnIndex++] = 0x00;

	BcnIndex += 4;
	
	HW_SetBeaconFIFO(pdot11Obj, &Beacon[0], BcnIndex);	
	memset(atimBitMap, 0, sizeof(atimBitMap));
}


void SendMcPkt(void)
{
	Signal_t *signal;

	while(pPsQ[0]->cnt > 0){
		signal = pPsQ[0]->first;
		if (!signal) 
			break;

		signal = sigDeque(pPsQ[0]);
		//PSDEBUG("Queue in awakeQ");	
		sigEnque(pAwakeQ, signal);
		//PSDEBUG_V("pAwakeQ->cnt", pAwakeQ->cnt);
	}
		
	if (pAwakeQ->cnt > 0){
		mcBuffered = TRUE;
	}
	else
		mcBuffered = FALSE;
}	


void ResetPMFilter(void)
{
	int i;
	
	for (i=0; i<BURST_NUM; i++)
		txRequest[i] = NULL;
		
	InitPMFilterQ();
}


BOOLEAN SendPkt(Signal_t* signal, FrmDesc_t *pfrmDesc, BOOLEAN bImmediate)
{
	//FrmDesc_t *pfrmDesc;
	FrmInfo_t *pfrmInfo;
	Frame_t	*frame;
	int i;
	Hash_t *pHash;

	U8	Num;

	//PSDEBUG("SendPkt");
	pfrmInfo = &signal->frmInfo;
	//pfrmDesc = pfrmInfo->frmDesc;
	frame = pfrmDesc->mpdu;
	pHash = pfrmDesc->pHash;
	Num = pfrmInfo->fTot;

    if (!signal->bDataFrm){
    //if (!pfrmDesc->bDataFrm){
		goto direct_send;
	}
    
	if (!isGroup(addr1(frame))){ //unicast
		PsMode dpsm;
		U16 aid;
	
		//PsInquiry(addr1(frame), &dpsm, &aid);
		dpsm = pHash->psm;
		aid = pHash->aid;
		
		if ((dpsm == PSMODE_POWER_SAVE) && (aid > 0) && (aid <(MAX_AID+1))){
			AgePsQ(aid);
			if (zd_CheckTotalQueCnt() > TXQ_THRESHOLD){
                PSDEBUG("*****Drop PS packet*****");
                freeFdesc(pfrmDesc);
                pdot11Obj->ReleaseBuffer(signal->buf);
                freeSignal(signal);
                return FALSE;
            }
            else{   
				//for (i=0; i<Num; i++){
				//	setMoreData((&pfrmDesc->mpdu[i]), 1);
				//  pfrmDesc->mpdu[i].header[i] |= MORE_DATA_BIT;
				//}	
				pfrmInfo->eol = HW_GetNow(pdot11Obj);	//Set timestamp
				sigEnque(pPsQ[aid], signal);			//Queue in PS Queue

				PSDEBUG_V("Queue in PS Queue, AID ", aid);
				PSDEBUG_V("cnt ", pPsQ[aid]->cnt);
				//PSDEBUG_V("pfrmInfo ", (U32)pfrmInfo);
				//PSDEBUG_V("eol ", (U32)pfrmInfo->eol);
				PSDEBUG_V("pfrmDesc ", (U32)pfrmDesc);
				//PSDEBUG_V("frame ", (U32)frame);
				PSDEBUG_V("bodyLen ", frame->bodyLen);
				tMapSet(atimBitMap, aid, TRUE);
				return FALSE;
			}	
		}
		else{			
			goto direct_send;
        }    
	}
	else{   //group address
		if ((orderBit(frame) == 0) && (mPsStaCnt > 0)){
            if ((zd_CheckTotalQueCnt() > TXQ_THRESHOLD) || (pPsQ[0]->cnt > MCQ_THRESHOLD)){
                PSDEBUG("*****Drop MC packet*****");
                freeFdesc(pfrmDesc);
                pdot11Obj->ReleaseBuffer(signal->buf);
                freeSignal(signal);
                return FALSE;
            }
            else{   
			    for (i=0; i<Num; i++){
					//setMoreData((&pfrmDesc->mpdu[i]), 1);
					pfrmDesc->mpdu[i].header[i] |= MORE_DATA_BIT;
				}	
			    sigEnque(pPsQ[0], signal);	// psQ[0] is for mcQ
			    //PSDEBUG("+++++ Queue in mcQ");
			    //PSDEBUG_V("mcQ->cnt", pPsQ[0]->cnt);
			    return FALSE;
            }    
		}
		else{
			goto direct_send;
        }     
	}

direct_send:
	if (!bImmediate){
		sigEnque(pTxQ, signal);
		return FALSE;
	}
	else{	
    	if (!pdot11Obj->CheckTCBAvail(Num)){
        	sigEnque(pTxQ, signal);
        	//PSDEBUG("Queue in TxQ");
        	//PSDEBUG_V("Cnt of TxQ", pTxQ->cnt);
        	return FALSE;
    	}
        return TxSend(signal, pfrmDesc); //14 us
   }      
}


#endif
