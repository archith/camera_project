#ifndef _ZDHCI_H_
#define _ZDHCI_H_
#include "zdapi.h" 

extern zd_80211Obj_t *pdot11Obj;
void zd_SendClass3ErrorFrame(MacAddr_t *sta, U8 vapId);
U8 zd_CheckTotalQueCnt(void);
#endif
