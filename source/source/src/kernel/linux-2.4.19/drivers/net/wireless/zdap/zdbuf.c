#ifndef __ZDBUF_C__
#define __ZDBUF_C__

#include "zd80211.h"    
#include <linux/vmalloc.h>


#define MAX_SIGNAL_NUM		64


SignalQ_t mgtQ, txQ, awakeQ, psQ[MAX_RECORD];
SignalQ_t *pMgtQ = &mgtQ, *pTxQ, *pAwakeQ, *pPsQ[MAX_RECORD];


Signal_t *FreeSignalList;
Signal_t *SignalBuf[MAX_SIGNAL_NUM];
U32 freeSignalCount;


FrmDesc_t *FreeFdescList;
FrmDesc_t *FdescBuf[MAX_SIGNAL_NUM];
U32 freeFdescCount;


void initSigQue(SignalQ_t *Q)
{
	U32 flags;
	
	flags = pdot11Obj->EnterCS();
	Q->first = NULL;
	Q->last = NULL;
	Q->cnt = 0;
	pdot11Obj->ExitCS(flags);
}


void resetSigQue(SignalQ_t *Q)
{
	U32 flags;
	
	flags = pdot11Obj->EnterCS();
	if ( Q->cnt){
		Q->last->pNext = FreeSignalList;
		FreeSignalList = Q->first;
		Q->first = NULL;
		Q->last = NULL;
		Q->cnt = 0;
	}
	pdot11Obj->ExitCS(flags);
}	


void releaseSignalBuf(void)
{
	int i;
	
	for (i=0; i<MAX_SIGNAL_NUM; i++)
		vfree((void *)SignalBuf[i]);
}


void initSignalBuf(void)
{
	int i;
	
	initSigQue(pMgtQ);
	FreeSignalList = NULL;
	freeSignalCount = MAX_SIGNAL_NUM;

	for (i=0; i<MAX_SIGNAL_NUM; i++){
		SignalBuf[i] = (Signal_t *)vmalloc(sizeof(Signal_t));  //can't use for DMA operation
		if (!SignalBuf[i]){
			FPRINT("80211: initSignalBuf failed");
			return;
		}
		SignalBuf[i]->pNext = FreeSignalList;
		FreeSignalList = SignalBuf[i];
	}
}


Signal_t *allocSignal(void)
{
	U32 flags;
	Signal_t *sig = NULL;	
	
	flags = pdot11Obj->EnterCS();
	if (FreeSignalList != NULL){
		sig = FreeSignalList;
		FreeSignalList = FreeSignalList->pNext;
		sig->pNext = NULL;  
		sig->buf = NULL;
		freeSignalCount-- ;
	}

    pdot11Obj->ExitCS(flags);
	return sig;
}


void freeSignal(Signal_t *signal)
{
	U32 flags;
	
	flags = pdot11Obj->EnterCS();
    signal->buf = NULL;
	signal->pNext = FreeSignalList;
	FreeSignalList = signal;
	freeSignalCount++;
	pdot11Obj->ExitCS(flags);
}


void initFdescBuf(void)
{
	int i;

	FreeFdescList = NULL;
	freeFdescCount = MAX_SIGNAL_NUM;
	
	for (i=0; i<MAX_SIGNAL_NUM; i++){
        FdescBuf[i] = (FrmDesc_t *) kmalloc(sizeof (FrmDesc_t), GFP_ATOMIC);  //may use for DMA operation
		if (!FdescBuf[i]){
			FPRINT("80211: initFdescBuf failed");
			return;
		}
		FdescBuf[i]->pNext = FreeFdescList;
		FreeFdescList = FdescBuf[i];
	}
}


void releaseFdescBuf(void)
{
	int i;
	
	for (i=0; i<MAX_SIGNAL_NUM; i++)
		kfree((void *)FdescBuf[i]);
}


FrmDesc_t *allocFdesc(void)
{
	FrmDesc_t *pfrmDesc = NULL;
	U32 flags;
	
	flags = pdot11Obj->EnterCS();
	pfrmDesc = FreeFdescList;	

	if (pfrmDesc != NULL){
		FreeFdescList = pfrmDesc->pNext;
		pfrmDesc->pNext = NULL;
		pfrmDesc->bIntraBss = 0;	
		//pfrmDesc->bDataFrm = 0;
		pfrmDesc->pHash = NULL;
		freeFdescCount--;
	}
	pdot11Obj->ExitCS(flags);
	
	PSDEBUG_V("alloc pfrmDesc", (U32)pfrmDesc);
	PSDEBUG_V("FreeFdescList", (U32)FreeFdescList);

	return pfrmDesc;
}


void freeFdesc(FrmDesc_t *pfrmDesc)
{
	U32 flags;
	
	flags = pdot11Obj->EnterCS();
	pfrmDesc->bIntraBss = 0;
	//pfrmDesc->bDataFrm = 0;
	pfrmDesc->pHash = NULL;
	pfrmDesc->pNext = FreeFdescList;
	FreeFdescList = pfrmDesc;
	freeFdescCount++;
	PSDEBUG_V("free pfrmDesc", (U32)pfrmDesc);
	PSDEBUG_V("FreeFdescList", (U32)FreeFdescList);
	pdot11Obj->ExitCS(flags);
}


Signal_t *sigDeque(SignalQ_t *Q)
{
	U32 flags;
	Signal_t *sig = NULL;	

	flags = pdot11Obj->EnterCS();
	if (Q->first != NULL){
		Q->cnt--;
		sig = Q->first;
		Q->first = (Q->first)->pNext;
		if (Q->first == NULL)
			Q->last = NULL;
	}
	pdot11Obj->ExitCS(flags);
	return sig;
}


void sigEnque(SignalQ_t *Q, Signal_t *signal)				
{		
	U32 flags;
	
	flags = pdot11Obj->EnterCS();
	signal->pNext = NULL;	
	if (Q->last == NULL){			
		Q->first = signal;				
		Q->last = signal;					
	}									
	else{								
		Q->last->pNext = signal;	
		Q->last = signal;				
	}									
	Q->cnt++;			
	
	if (Q == pMgtQ)
		pdot11Obj->QueueFlag |= MGT_QUEUE_SET;		
	else if (Q == pTxQ)	
		pdot11Obj->QueueFlag |= TX_QUEUE_SET;
	else if (Q == pAwakeQ)	
		pdot11Obj->QueueFlag |= AWAKE_QUEUE_SET;
					
	pdot11Obj->ExitCS(flags);		
}

#endif
