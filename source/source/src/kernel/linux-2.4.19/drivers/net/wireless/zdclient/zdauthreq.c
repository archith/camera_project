#ifndef __ZDAUTHREQ_C__
#define __ZDAUTHREQ_C__

#include "zd80211.h"



BOOLEAN DeauthReq(Signal_t *signal)
{
	FrmDesc_t *pfrmDesc;
	MacAddr_t Sta;
	ReasonCode Rsn = RC_UNSPEC_REASON;
	U8	vapId = 0;
	
	ZDEBUG("DeauthReq");	

	memcpy((U8 *)&Sta, (U8 *)&signal->frmInfo.Sta, 6);
	Rsn = signal->frmInfo.rCode;
	
	vapId = signal->vapId;
	UpdateStaStatus(&Sta, STATION_STATE_NOT_AUTH, vapId);
	
	pfrmDesc = allocFdesc();
	if(!pfrmDesc){
		sigEnque(pMgtQ, (signal));
		return FALSE;
	}
	
	mkDisAssoc_DeAuthFrm(pfrmDesc, ST_DEAUTH, &Sta, Rsn, vapId);
	sendMgtFrame(signal, pfrmDesc);
	
	return FALSE;
}


BOOLEAN AuthReqEntry(Signal_t *signal)
{
	switch(signal->id){
		case SIG_DEAUTH_REQ:
			return DeauthReq(signal);
			
		default:
			return TRUE;	
	}	
}

//ivan
//int send_auth_req(unsigned char *pkt,unsigned char *bssid,int seq)
int send_auth_req(unsigned char *pkt,unsigned int bodylen)
{
    Signal_t *signal;
	FrmDesc_t *pfrmDesc;
//	Frame_t *frame;	

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
	
//	frame = pfrmDesc->mpdu;
//    mkAuthFrm(pfrmDesc, bssid, 0, 1, 0, 0, 0);
    pfrmDesc->mpdu->body=pfrmDesc->buffer;
    memcpy(pfrmDesc->mpdu->header,pkt,MAC_HDR_LNG); //copy header
    pfrmDesc->mpdu->HdrLen = MAC_HDR_LNG;
    memcpy(pfrmDesc->buffer,pkt+MAC_HDR_LNG,6);
    pfrmDesc->mpdu->bodyLen = bodylen;
    
    sendMgtFrame(signal, pfrmDesc);    
    return 1;
}

#endif
