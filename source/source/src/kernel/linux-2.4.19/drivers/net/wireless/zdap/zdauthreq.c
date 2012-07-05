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
#endif
