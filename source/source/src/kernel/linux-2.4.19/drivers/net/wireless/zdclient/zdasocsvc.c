#ifndef __ZDASOCSVC_C__
#define __ZDASOCSVC_C__

#include "zd80211.h"


U8 RateConvert(U8 rate, U16 capability)
{
	switch (rate)
	{ 
	    case 2  :  return 0;  // 1M
	    case 4  :  return 1;  // 2M
	    case 11 :  return 2;  // 5.5M
	    case 22 :  return 3;  // 11M
	    case 33 :  return 4;  // 16.5M
	    case 44 :  
			if (capability & CAP_PBCC_ENABLE){ // Support PBCC
				return 3;
			}
			else{
				return 5;  // 22M
			}
	    case 55 :  return 6;  // 27.5M
	}

	return 3;	// 11M
}


BOOLEAN Disasoc(Signal_t *signal)
{
	FrmDesc_t *pfrmDesc;
	Frame_t *rdu;
	MacAddr_t Sta;
	ReasonCode Rsn;
	U8 vapId = 0;
	
	ZDEBUG("Disasoc");
	pfrmDesc	= signal->frmInfo.frmDesc;
	rdu = pfrmDesc->mpdu;
	memcpy((U8 *)&Sta, (U8 *)(addr2(rdu)), 6);
	Rsn = (ReasonCode)(reason(rdu));

	if(memcmp(addr1(rdu), (U8*)&mBssId, 6)){ //Not for this BSSID
		freeFdesc(pfrmDesc);
		return TRUE;
	}

	UpdateStaStatus(&Sta, STATION_STATE_DIS_ASOC, vapId);
	freeFdesc(pfrmDesc);
	
	//here to handle disassoc ind.
	pdot11Obj->StatusNotify(STA_DISASSOCIATED, (U8 *)&Sta);
	return TRUE;
}


BOOLEAN DisasocReq(Signal_t *signal)
{
	FrmDesc_t *pfrmDesc;
	MacAddr_t Sta;
	ReasonCode Rsn = RC_UNSPEC_REASON;
	U8 vapId = 0;

	ZDEBUG("DisasocReq");
	
	memcpy((U8 *)&Sta, (U8 *)&signal->frmInfo.Sta, 6);
	Rsn = signal->frmInfo.rCode;
	pdot11Obj->StatusNotify(STA_DISASSOCIATED, (U8 *)&Sta);

	vapId = signal->vapId;
	UpdateStaStatus(&Sta, STATION_STATE_DIS_ASOC, vapId);
	
	pfrmDesc = allocFdesc();
	if(!pfrmDesc){
		sigEnque(pMgtQ, (signal));
		return FALSE;
	}

	mkDisAssoc_DeAuthFrm(pfrmDesc, ST_DISASOC, &Sta, Rsn, vapId);
	sendMgtFrame(signal, pfrmDesc);
	
	return FALSE;
}


BOOLEAN Re_AsocReq(Signal_t *signal)
{
	FrmDesc_t *pfrmDesc;
	Frame_t *rdu;
	MacAddr_t Sta;
	U16 aid = 0;
	StatusCode asStatus;
	U8 lsInterval;
	Element WPA;
	Element asSsid;
	Element asRates;
	//Element extRates;
	U16 cap;
	U8 ZydasMode = 0;
	int i;
	U8 tmpMaxRate = 0x02;
	U8 MaxRate;
	U16 notifyStatus = STA_ASOC_REQ;
	U16 notifyStatus1 = STA_ASSOCIATED;
	TypeSubtype type = ST_ASOC_RSP;
	U8	Preamble = 0;
	U8	HigestBasicRate = 0;
	U8	vapId = 0;
	U8	Len;
	BOOLEAN bErpSta = FALSE;

	ZDEBUG("Re_AsocReq");
	pfrmDesc = signal->frmInfo.frmDesc;
	rdu = pfrmDesc->mpdu;
	lsInterval = listenInt(pfrmDesc->mpdu);
	//FPRINT_V("lsInterval", lsInterval);
	cap = cap(pfrmDesc->mpdu);
	memcpy((U8 *)&Sta, (U8 *)addr2(rdu), 6);
	
	if ((isGroup(addr2(rdu))) ||  (!getElem(rdu, EID_SSID, &asSsid))
		|| (!getElem(rdu, EID_SUPRATES, &asRates))){
		freeFdesc(pfrmDesc);
		return TRUE;
	}
	
	if ((eLen(&asSsid) != eLen(&dot11DesiredSsid) || 
		memcmp(&asSsid, &dot11DesiredSsid, eLen(&dot11DesiredSsid)+2) != 0)){
		freeFdesc(pfrmDesc);
		return TRUE;
	}		
	//chaeck capability
	if (cap & CAP_SHORT_PREAMBLE)
		Preamble = 1;
	else
		Preamble = 0;
		
	// Privacy not match
	if (cap & CAP_PRIVACY){
		if (!mPrivacyInvoked){
			freeFdesc(pfrmDesc);
			return TRUE;
		}	
	}
	else {
		if (mPrivacyInvoked){
			freeFdesc(pfrmDesc);
			return TRUE;
		}	
	}		
	Len = eLen(&asRates);	
	for (i=0; i<Len; i++){
		if ( (asRates.buf[2+i] & 0x7f) > tmpMaxRate ){
			tmpMaxRate = (asRates.buf[2+i] & 0x7f);
			if (asRates.buf[2+i] & 0x80)
				HigestBasicRate = asRates.buf[2+i];
		}	
				
		if (((asRates.buf[2+i] & 0x7f) == 0x21) && (!(cap & CAP_PBCC_ENABLE))){ //Zydas 16.5M

			ZydasMode = 1;
			mZyDasModeClient = TRUE;
			//FPRINT("ZydasMode");
		}	
	}	
	
	MaxRate = RateConvert((tmpMaxRate & 0x7f), cap);			
	
	if (signal->id == SIG_REASSOC_REQ)	
		notifyStatus = STA_REASOC_REQ;
		
	if (!pdot11Obj->StatusNotify(notifyStatus, (U8 *)&Sta)){ //Accept it
		if (mDynKeyMode == DYN_KEY_TKIP){	
			if (getElem(rdu, EID_WPA, &WPA)){
				//zd1205_OctetDump("AssocRequest = ", asRdu->body, asRdu->bodyLen);
				//zd1205_OctetDump("AssocRequest WPA_IE = ", &WPA.buf[2], WPA.buf[1]);
				
				if (pdot11Obj->AssocRequest((U8 *)&Sta, rdu->body, rdu->bodyLen)){ //reject
					asStatus = SC_UNSPEC_FAILURE;
					goto check_failed;
					//we need reason code here
				}	
			}
			else{
				asStatus = SC_UNSPEC_FAILURE;
				goto wpa_check_failed;		
			}		
		}		
			
		if (!UpdateStaStatus(&Sta, STATION_STATE_ASOC, vapId)){
			asStatus = SC_AP_FULL;
		}
		else{
			AssocInfoUpdate(&Sta, MaxRate, lsInterval, ZydasMode, Preamble, bErpSta, vapId);
			aid = AIdLookup(&Sta);
			asStatus = SC_SUCCESSFUL;
			if (signal->id == SIG_REASSOC_REQ)	
				notifyStatus1 = STA_REASSOCIATED;
			pdot11Obj->StatusNotify(notifyStatus1, (U8 *)&Sta);
		}
	}
	else{
wpa_check_failed:	
		asStatus = SC_UNSPEC_FAILURE;
	}	
	
	aid |= 0xC000;
	if (aid != 0xC000){
		#ifdef B500_DEBUG
			FPRINT_V("Aid", aid);
			FPRINT_V("MaxRate", MaxRate);
		#endif

	}	
	
check_failed:
	if (signal->id == SIG_REASSOC_REQ)	
		type = ST_REASOC_RSP;	
	mkRe_AsocRspFrm(pfrmDesc, type, &Sta, mCap, asStatus, aid, &mBrates, NULL, vapId);
	sendMgtFrame(signal, pfrmDesc);

	return FALSE;
}


BOOLEAN AsocEntry(Signal_t *signal)
{
	switch(signal->id){
		case SIG_DIASSOC_REQ:
			return DisasocReq(signal);
			
		case SIG_DISASSOC:
			return Disasoc(signal);
			
		case SIG_ASSOC_REQ:
		case SIG_REASSOC_REQ:
			return Re_AsocReq(signal);
			
		default:
			return TRUE; 		
	}	
}


//ivan
//int send_assoc_req(unsigned char *bssid,unsigned short cap,unsigned short BcnInterval,unsigned char *essid)
int send_assoc_req(unsigned char *pkt,unsigned int bodylen)
{
    Signal_t *signal;
	FrmDesc_t *pfrmDesc;
//	Frame_t *frame;	
//	U8 *body;

	signal = allocSignal();
	if (!signal)
	{
    	FPRINT("zd_SendPkt out of signal");
		return FALSE;
	}		
	pfrmDesc = allocFdesc();
	if (!pfrmDesc)
	{
		freeSignal(signal);
    	FPRINT("zd_SendPkt out of description");
		return FALSE;
	}
#if 0
    frame = pfrmDesc->mpdu;    
    setFrameType(frame, ST_ASOC_REQ);    
    
	body = frame->body;	
	//make header
	setAddr1(frame , bssid);
	setAddr2(frame , &dot11MacAddress);
	setAddr3(frame , bssid);	
	frame->HdrLen = MAC_HDR_LNG;
	
	//make frame body
	{
	    unsigned char len,len2;
	    unsigned char *rate=&mBrates;
	    
    	body[0] = cap & 0xff;			//Cap
    	body[1] = (cap & 0xff00) >> 8;	
    	body[2] = BcnInterval & 0xff;	//BcnPeriod
    	body[3] = (BcnInterval & 0xff00) >> 8;
    	
    	body[4] = 0;    //element id=ssid
    	body[5] = len= (unsigned char)strlen(essid);
    	memcpy(&body[6],essid,len);    	
    	len2=rate[1];    	
    	memcpy(&body[6+len],rate,len2+2);
    	frame->bodyLen=6+len+len2+2;

//ivan dump
    	zd1205_dump_data("===>", body, frame->bodyLen);
    }
#endif
    pfrmDesc->mpdu->body=pfrmDesc->buffer;
    memcpy(pfrmDesc->mpdu->header,pkt,MAC_HDR_LNG); //copy header
    pfrmDesc->mpdu->HdrLen = MAC_HDR_LNG;
    memcpy(pfrmDesc->buffer,pkt+MAC_HDR_LNG,bodylen);
    pfrmDesc->mpdu->bodyLen = bodylen;
   
    sendMgtFrame(signal, pfrmDesc);    
    return 1;
}


#endif
