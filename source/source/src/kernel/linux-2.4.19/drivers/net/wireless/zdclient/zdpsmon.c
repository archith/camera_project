#ifndef __ZDPSMON_C__
#define __ZDPSMON_C__

#include "zd80211.h"

#define GetEntry(pMac)		(((pMac->mac[3]) ^ (pMac->mac[4]) ^ (pMac->mac[5])) & (MAX_AID-1))

Hash_t *FreeHashList;
Hash_t HashBuf[MAX_RECORD];
Hash_t *HashTbl[MAX_RECORD];
Hash_t *sstByAid[MAX_RECORD];
U32 freeHashCount;


void initHashBuf(void);
void InitHashTbl(void);
Hash_t *HashInsert(MacAddr_t *pMac);


void CleanupHash(Hash_t *hash)
{
//printk("CleanupHash\n");
	memset(hash->mac, 0, 6);
	hash->asoc = STATION_STATE_DIS_ASOC;
	hash->auth = STATION_STATE_NOT_AUTH;
	hash->psm = PSMODE_STA_ACTIVE;
	hash->encryMode = WEP_NOT_USED;
	hash->ZydasMode = 0;
	hash->pkInstalled = 0;
	hash->AlreadyIn = 0;
	hash->ContSuccFrames = 0;
	hash->ttl = 0;
	hash->bValid = FALSE;
	hash->Preamble = 0;
	hash->keyLength = 0;
	hash->KeyId = 0;
	memset(hash->wepIv, 0, 4);
	memset(&hash->TxSeed, 0, sizeof(Seedvar));
	memset(&hash->RxSeed, 0, sizeof(Seedvar));
	memset(&hash->TxMicKey, 0, sizeof(MICvar));
	memset(&hash->RxMicKey, 0, sizeof(MICvar));
	//hash->vapId = 0;
}	


void CleanupKeyInfo(Hash_t *hash)
{
printk("CleanupKeyInfo\n");    
	hash->encryMode = WEP_NOT_USED;
	hash->pkInstalled = 0;
	hash->keyLength = 0;
	hash->KeyId = 0;
	memset(hash->wepIv, 0, 4);
	memset(&hash->TxSeed, 0, sizeof(Seedvar));
	memset(&hash->RxSeed, 0, sizeof(Seedvar));
	memset(&hash->TxMicKey, 0, sizeof(MICvar));
	memset(&hash->RxMicKey, 0, sizeof(MICvar));
}	


void initHashBuf(void)
{
	int i;

	freeHashCount = MAX_RECORD;

	for (i=0; i<MAX_AID; i++){ //from 0 to 31
		HashBuf[i].pNext = &HashBuf[i+1];
		sstByAid[i] = &HashBuf[i];
		HashBuf[i].aid = i;
		CleanupHash(&HashBuf[i]);
	}
	
	//aid 32 is here
	HashBuf[MAX_AID].pNext = NULL;
	sstByAid[MAX_AID] = &HashBuf[MAX_AID];
	HashBuf[MAX_AID].aid = MAX_AID;
	CleanupHash(&HashBuf[MAX_AID]);

	FreeHashList = &HashBuf[1]; //by pass aid = 0
	
	//deal with aid = 0
	HashBuf[0].pNext = NULL;
}


Hash_t *allocHashBuf(void)
{
	Hash_t *hash = NULL;
	U32 flags;	
	
	//HSDEBUG("*****allocHashBuf*****");
	flags = pdot11Obj->EnterCS();
	if (FreeHashList != NULL){
		hash = FreeHashList;
		FreeHashList = FreeHashList->pNext;
		hash->pNext = NULL;
		freeHashCount--;
	}
	pdot11Obj->ExitCS(flags);
	return hash;
}



void freeHashBuf(Hash_t *hash)
{
	U32 flags;
	
	//HSDEBUG("*****freeHashBuf*****");
	flags = pdot11Obj->EnterCS();
	if (hash->AlreadyIn){
		if (mCurrConnUser > 0)
			mCurrConnUser--;
	}
		
	if (hash->psm == PSMODE_POWER_SAVE){
		if (mPsStaCnt > 0)
			mPsStaCnt--;
	}	
	
	CleanupHash(hash);
	hash->pNext = FreeHashList;
	FreeHashList = hash;
	freeHashCount++;  
	pdot11Obj->ExitCS(flags);
}


void InitHashTbl(void)
{
	int i;
	
	for (i=0; i<MAX_RECORD; i++){
		HashTbl[i] = NULL;
	}	
}	


Hash_t *HashSearch(MacAddr_t *pMac)
{
	U8 entry;
	Hash_t *hash = NULL;
	U32 flags;
	
	//HSDEBUG("HashSearch");
	entry = GetEntry(pMac); 
	flags = pdot11Obj->EnterCS();
	if (HashTbl[entry] == NULL) {
		goto exit;
	}	
	else{
		hash = HashTbl[entry];
		do {
			if (memcmp(hash->mac, (U8 *)pMac, 6) == 0){
				//HSDEBUG("Search got one");
				goto exit;
			}	
			else
				hash = hash->pNext;
		}while(hash != NULL);
	}	
	
exit:
	pdot11Obj->ExitCS(flags);		 
	if (hash){
#if 0		
		printf("macaddr = %02x:%02x:%02x:%02x:%02x:%02x\n", 
			hash->mac[0],  hash->mac[1], hash->mac[2], 
			hash->mac[3], hash->mac[4], hash->mac[5]);
		printf("asoc = %x\n", hash->asoc);	
		printf("auth = %x\n", hash->auth);
		printf("psm = %x\n", hash->psm);
		printf("aid = %x\n", hash->aid);
		printf("lsInterval = %x\n", hash->lsInterval);
#endif		
	}	
	else
		;//HSDEBUG("Search no one");
		
	return hash; 
}

	
Hash_t *HashInsert(MacAddr_t *pMac)
{
	U8 entry;
	Hash_t *hash;
	U32 flags;
	
	HSDEBUG("HashInsert");
	hash = allocHashBuf();
	if (!hash){
		HSDEBUG("No free one");
		//Age Hash table
		AgeHashTbl();
		return NULL; // no free one
	}	
	else{
		entry = GetEntry(pMac);
		HSDEBUG_V("entry", entry); 
		if (HashTbl[entry] == NULL){ //entry is null
			HashTbl[entry] = hash;
			HSDEBUG("Entry is null");
		}
		else{ //insert list head
			flags = pdot11Obj->EnterCS();
			hash->pNext = HashTbl[entry];
			HashTbl[entry] = hash;
			pdot11Obj->ExitCS(flags);	
			HSDEBUG("Insert to list head");
		}
		memcpy(hash->mac, (U8 *)pMac, 6);
        hash->ttl = HW_GetNow(pdot11Obj);
		hash->bValid = TRUE;    
		return hash;	
	}	
}	


BOOLEAN AgeHashTbl(void)
{
	U32 now, ttl, idleTime;
	U8 entry, firstLayer;
	int i;
	MacAddr_t *pMac;
	Hash_t *hash, *preHash = NULL;
	BOOLEAN ret = FALSE;
	
	HSDEBUG("*****AgeHashTbl*****");
	now = HW_GetNow(pdot11Obj);
	
	for (i=1; i<(MAX_AID+1); i++){
		ttl = sstByAid[i]->ttl;
		if (now > ttl)
			idleTime = now - ttl;
		else
			idleTime = 	(0xffffffff - ttl) + now;
		
		if (sstByAid[i]->bValid){
			if (idleTime > IDLE_TIMEOUT ){
			    HSDEBUG("*****Age one*****");
			    HSDEBUG_V("aid", i);
			    HSDEBUG_V("now", now);
			    HSDEBUG_V("ttl", ttl);
			    HSDEBUG_V("idleTime", idleTime);
			
			    pMac = (MacAddr_t *)&sstByAid[i]->mac[0];
			    entry = GetEntry(pMac);
			    HSDEBUG_V("entry", entry);
			    hash = HashTbl[entry];
			    firstLayer = 1;
			    do {
				    if (hash == sstByAid[i]){
					    if (firstLayer == 1){
						    HSDEBUG("*****firstLayer*****");
						    if (hash->pNext != NULL)
							    HashTbl[entry] = hash->pNext;
						    else
							    HashTbl[entry] = NULL;
					    }			
					    else{
						    HSDEBUG("*****Not firstLayer*****");
						    preHash->pNext = hash->pNext;
					    }
					    zd_CmdProcess(CMD_DISASOC, &hash->mac[0], ZD_INACTIVITY);
					    freeHashBuf(hash);
					    break;
				    }	
				    else{
					    preHash = hash;
					    hash = hash->pNext;
					    firstLayer = 0;
				    }	
			    }while(hash != NULL);
			
				ret = TRUE;
			}
			else {
				if (sstByAid[i]->ZydasMode == 1)
					mZyDasModeClient = TRUE;
			}		
		}		
	
	}
	
	//HSDEBUG_V("ret", ret);
	return ret;
}
	

void ResetPSMonitor(void)
{
	ZDEBUG("ResetPSMonitor");
	initHashBuf();
	InitHashTbl();
	mPsStaCnt = 0;
}


Hash_t *RxInfoIndicate(MacAddr_t *sta, PsMode psm, U8 rate)
{
	Hash_t *pHash;
	
	ZDEBUG("RxInfoIndicate");
	if (isGroup(sta))
		return NULL;

	pHash = HashSearch(sta);
	if (!pHash)
		return NULL;	
	else{
		PsMode oldPsm = pHash->psm;
		StationState asoc = pHash->asoc;
		
		if (rate > pHash->MaxRate)
			pHash->MaxRate = rate;
		pHash->RxRate = rate;
		pHash->ttl = HW_GetNow(pdot11Obj);
		
		if (psm == PSMODE_STA_ACTIVE){
			if (oldPsm == PSMODE_POWER_SAVE){
				StaWakeup(sta);
				if (asoc == STATION_STATE_ASOC){
					if (mPsStaCnt >0){ 
						mPsStaCnt--;
					}	
				}
			}		
		}
		else {
			if (oldPsm == PSMODE_STA_ACTIVE){
				if (asoc == STATION_STATE_ASOC){
					if (mPsStaCnt < MAX_AID){ 
						mPsStaCnt++;
					}	
				}	
			}
			else if (oldPsm == PSMODE_POWER_SAVE){
				if (asoc == STATION_STATE_ASOC){
					if (mPsStaCnt == 0) 
						mPsStaCnt++;
				}	
			}		
		}	
		
		pHash->psm = psm;
	}
	
	return pHash;
}


BOOLEAN UpdateStaStatus(MacAddr_t *sta, StationState staSte, U8 vapId)
{
	Hash_t *pHash;
	
	ZDEBUG("UpdateStaStatus");
	
	pHash = HashSearch(sta);
	if (pHash)
		goto UpdateStatus;
	else{	
		if ((STATION_STATE_AUTH_OPEN == staSte) || (STATION_STATE_AUTH_KEY == staSte)){
			if ((mCurrConnUser + 1) > mLimitedUser){
				//AgeHashTbl();
				return FALSE;
			}	
			else{	
				pHash = HashInsert(sta);
				if (!pHash)
					return FALSE; 
			}		
		}	
		else
			return FALSE; 
	}

UpdateStatus:	
	switch(staSte){
		case STATION_STATE_AUTH_OPEN:
		case STATION_STATE_AUTH_KEY:
			pHash->auth = staSte;
			break;

		case STATION_STATE_ASOC:
			if (((mCurrConnUser + 1) > mLimitedUser) && (!pHash->AlreadyIn)){
				return FALSE; 
			}
				
			if (pHash->psm == PSMODE_POWER_SAVE){
				if (mPsStaCnt > 0){ 
					mPsStaCnt--;
				}	
			}	
						
			pHash->asoc = STATION_STATE_ASOC;
			if (!pHash->AlreadyIn){
				pHash->AlreadyIn = 1;
				mCurrConnUser++;
			}	
			
			CleanupKeyInfo(pHash);
			break;

		case STATION_STATE_NOT_AUTH:
		case STATION_STATE_DIS_ASOC:
			if (pHash->asoc == STATION_STATE_ASOC){
				if (pHash->psm == PSMODE_POWER_SAVE){
					FlushQ(pPsQ[pHash->aid]);
					if (mPsStaCnt > 0){ 
						mPsStaCnt--;
						if (mPsStaCnt == 0){
							FlushQ(pAwakeQ);
							FlushQ(pPsQ[0]);
						}	
					}	
				}
				if (pHash->AlreadyIn){
					pHash->AlreadyIn = 0;
					mCurrConnUser--;	
				}	
			}		
			
			pHash->auth = STATION_STATE_NOT_AUTH;
			pHash->asoc = STATION_STATE_DIS_ASOC;
			CleanupKeyInfo(pHash);
			break;
	}
	
	return TRUE;
}


void SsInquiry(MacAddr_t *sta, StationState *sst, StationState *asst)
{
	ZDEBUG("SsInquiry");
	if (isGroup(sta)){
		*asst = STATION_STATE_NOT_AUTH;
		*sst = STATION_STATE_DIS_ASOC;
	}
	else{
		Hash_t *pHash;
		pHash = HashSearch(sta);

		if (!pHash){
			*asst = STATION_STATE_NOT_AUTH;
			*sst = STATION_STATE_DIS_ASOC;
		}
		else{
			*asst = pHash->auth;
			if ((*asst == STATION_STATE_AUTH_OPEN) || (*asst == STATION_STATE_AUTH_KEY))
				*sst = pHash->asoc;
			else
				*sst = STATION_STATE_DIS_ASOC;
		}	
	}
}


U16 AIdLookup(MacAddr_t *sta)
{
	Hash_t *pHash;
	
	ZDEBUG("AIdLookup");
	pHash = HashSearch(sta);
	if (!pHash)
		return (U16)0;
	else
		return pHash->aid;
}


void AssocInfoUpdate(MacAddr_t *sta, U8 MaxRate, U8 lsInterval, U8 ZydasMode, U8 Preamble, BOOLEAN bErpSta, U8 vapId)
{
	Hash_t *pHash;
	
	ZDEBUG("AssocInfoUpdate");
	if (isGroup(sta))
		return;

	pHash = HashSearch(sta);
	if (!pHash)
		return;	
	else{
		pHash->MaxRate = MaxRate;
		pHash->CurrTxRate = MaxRate;
		pHash->lsInterval = lsInterval;
		pHash->ZydasMode = ZydasMode;
		pHash->Preamble = Preamble;
		//pHash->bErpSta = bErpSta;
		//pHash->vapId = vapId;
	}
}


int zd_SetKeyInfo(U8 *addr, U8 encryMode, U8 keyLength, U8 KeyId, U8 *pKeyContent)
{
	Hash_t *pHash;
	MacAddr_t *sta = (MacAddr_t *)addr;
	U8 KeyRsc[6] = {0, 0, 0, 0, 0, 0};
	
	printk("SetKeyInfo");
	if (isGroup(sta))
	{
	    mWpaBcKeyLen = keyLength;
		mWpaBcKeyId = KeyId;
		if (encryMode == DYN_KEY_TKIP)
		{
			//only vaild for Tx
			if (mWpaBcKeyLen == 32)
			{
				Tkip_Init(pKeyContent, (U8 *)&dot11MacAddress, &mBcSeed, KeyRsc); //temp key
				MICsetKey(&pKeyContent[16], &mBcMicKey); //Tx Mic key
			}
			else 
			{
				// mixed mode or WEP group key
				memcpy(&mBcKeyVector[0], pKeyContent, keyLength);
			}		
			mGkInstalled = 1;
			return 0;
		}
		else
			return -1;
	}	

	pHash = HashSearch(sta);
	if (!pHash)
		return -1;	
	else
	{
		pHash->encryMode = encryMode;
		if (encryMode == DYN_KEY_TKIP)
		{
			//pHash->keyLength = 16;
			Tkip_Init(pKeyContent, (U8 *)&dot11MacAddress, &pHash->TxSeed, KeyRsc);
			Tkip_Init(pKeyContent, addr, &pHash->RxSeed, KeyRsc);
			MICsetKey(&pKeyContent[16], &pHash->TxMicKey);
			MICsetKey(&pKeyContent[24], &pHash->RxMicKey);
			pHash->KeyId = KeyId;
			pHash->pkInstalled = 1;
		}	
		else{
			pHash->keyLength = keyLength;
			pHash->KeyId = KeyId;
			memcpy(&pHash->keyContent[0], pKeyContent, keyLength);
		}	
		return 0;	
	}
}	


BOOLEAN zd_GetKeyInfo(U8 *addr, U8 *encryMode, U8 *keyLength, U8 *pKeyContent)
{
	Hash_t *pHash;
	MacAddr_t *sta = (MacAddr_t *)addr;
	
	ZDEBUG("zd_GetKeyInfo");
	if (isGroup(sta)){
		return FALSE; 
	}	
			
	pHash = HashSearch(sta);
	if (!pHash){
		*encryMode = 0;
		*keyLength = 0;
		return FALSE; 
	}	
	else{
		*encryMode = pHash->encryMode;
		*keyLength = pHash->keyLength;
		memcpy(pKeyContent, &pHash->keyContent[0], pHash->keyLength);
		return TRUE;
	}	
}			


int zd_GetKeyInfo_ext(U8 *addr, U8 *encryMode, U8 *keyLength, U8 *pKeyContent, U16 iv16, U32 iv32)
{
	Hash_t *pHash;
	MacAddr_t *sta = (MacAddr_t *)addr;
	
	ZDEBUG("zd_GetKeyInfo_ext");
	if (isGroup(sta)){
		return -1; 
	}	
	
	if (mDynKeyMode != DYN_KEY_TKIP)
		return -1;
				
	pHash = HashSearch(sta);
	if (!pHash){
		*encryMode = 0;
		*keyLength = 0;
		return -1; 
	}	
	else{
		if (pHash->pkInstalled == 0)
			return -2;
			
		if ((iv16 == pHash->RxSeed.IV16) && (iv32 == pHash->RxSeed.IV32)){
			// iv out of sequence
			//FPRINT_V("iv16", iv16);
			//FPRINT_V("iv32", iv32);
			//return -3;
		}
		
		*encryMode = pHash->encryMode;
		*keyLength = pHash->keyLength;
		//do key mixing	
		Tkip_phase1_key_mix(iv32, &pHash->RxSeed);
		Tkip_phase2_key_mix(iv16, &pHash->RxSeed);
		Tkip_getseeds(iv16, pKeyContent, &pHash->RxSeed);	
		pHash->RxSeed.IV16 = iv16;
		pHash->RxSeed.IV32 = iv32;
		return 0;
	}	
}			


int zd_SetTsc(U8 *addr, U8 KeyId, U8 direction, U32 tscHigh, U16 tscLow)
{
	Hash_t *pHash;
	MacAddr_t *sta = (MacAddr_t *)addr;
	
	ZDEBUG("zd_SetTsc");
	if (isGroup(sta)){
		return -1;
	}	

	pHash = HashSearch(sta);
	if (!pHash)
		return -1;	
	else{
		pHash->KeyId = KeyId;
		if (direction == 0){ //Tx
			pHash->TxSeed.IV16 = tscLow;
			pHash->TxSeed.IV32 = tscHigh;
		}	
		else if (direction == 1){ //Rx
			pHash->RxSeed.IV16 = tscLow;
			pHash->RxSeed.IV32 = tscHigh;
		}	
		return 0;	
	}
}	


int zd_GetTsc(U8 *addr, U8 KeyId, U8 direction, U32 *tscHigh, U16 *tscLow)
{
	Hash_t *pHash;
	MacAddr_t *sta = (MacAddr_t *)addr;
	
	ZDEBUG("zd_GetTsc");
	if (isGroup(sta)){
		return -1;
	}	

	pHash = HashSearch(sta);
	if (!pHash)
		return -1;	
	else{
		if (direction == 0){ //Tx
			*tscLow = pHash->TxSeed.IV16;
			*tscHigh = pHash->TxSeed.IV32;
		}	
		else if (direction == 1){ //Rx
			*tscLow = pHash->RxSeed.IV16;
			*tscHigh = pHash->RxSeed.IV32;
		}	
		return 0;	
	}
}	

#endif
