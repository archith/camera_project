#ifndef __ZDSYNCH_C__
#define __ZDSYNCH_C__

#include "zd80211.h"


BOOLEAN ProbeReq(Signal_t *signal)
{
	FrmDesc_t *pfrmDesc;
	Frame_t *rdu;
	MacAddr_t sta;
	Element rSsid;
	Element *pWPA = NULL;
	U8 vapId = 0;

	ZDEBUG("ProbeReq");
	pfrmDesc = signal->frmInfo.frmDesc;
	rdu = pfrmDesc->mpdu;
	
	if (!getElem(rdu, EID_SSID, &rSsid))
		goto release;
	
	if (mHiddenSSID){ //discard broadcast ssid
		if (eLen(&rSsid) == 0){
			goto release;
		}
	}	
	memcpy((U8*)&sta, (U8*)addr2(rdu), 6);
	
	if (eLen(&rSsid) == 0){
		
		//WPA
		if (mDynKeyMode == DYN_KEY_TKIP)	
			pWPA = &mWPAIe;
		mkProbeRspFrm(pfrmDesc, &sta, mBeaconPeriod, mCap, &dot11DesiredSsid, &mBrates, &mPhpm, NULL, (Element *)pWPA, vapId);		
		return sendMgtFrame(signal, pfrmDesc);	
	}
	else{
	 	if (memcmp(&rSsid, &dot11DesiredSsid, eLen(&dot11DesiredSsid)+2) == 0){
			//WPA
			if (mDynKeyMode == DYN_KEY_TKIP)	
				pWPA = &mWPAIe;
			
			mkProbeRspFrm(pfrmDesc, &sta, mBeaconPeriod, mCap, &dot11DesiredSsid, &mBrates, &mPhpm, NULL, (Element *)pWPA, vapId);		
			return sendMgtFrame(signal, pfrmDesc);
		}	
	}
	
release:
   	ZDEBUG("goto release");
	freeFdesc(pfrmDesc);
	return TRUE;
}


BOOLEAN SynchEntry(Signal_t *signal)
{
	return ProbeReq(signal);
}
#endif
