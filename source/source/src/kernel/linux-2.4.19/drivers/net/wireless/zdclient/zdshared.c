#ifndef __ZDSHARED_C__
#define __ZDSHARED_C__

#include "zd80211.h"

//ivan added
extern unsigned char gtk_mic_tx[8];
extern unsigned char gtk_mic_rx[8];
extern unsigned char ptk_mic_tx[8];
extern unsigned char ptk_mic_rx[8];
extern MICvar  gtk_mic_tx_var;
extern MICvar  gtk_mic_rx_var;
extern MICvar  ptk_mic_tx_var;
extern MICvar  ptk_mic_rx_var;

extern void zd1205_dump_data(char *info, u8 *data, u32 data_len);
extern BOOLEAN zd_SendPkt(mlme_data_t *mlme_p,U8 *pHdr, U8 *pBody, U32 bodyLen, void *buf, U8 bEapol, void *pHash);	// tx request

void mkFragment(Signal_t *signal, FrmDesc_t *pfrmDesc)
{
	Frame_t *mpdu, *curMpdu;
	FrmInfo_t *pfrmInfo;
	BOOLEAN bWep;
	U16 pdusize;
	U8 *pBody;
	U16 len;
	U8 fn;
	U8 *pByte;
	int i;
//	Hash_t * pHash;
//	MICvar *pTxMicKey = NULL;
//	U8 KeyInstalled = 0;
	U8 vapId = 0;
	U8 Num;	
	U8 bDataFrm = signal->bDataFrm;
	//U8 bDataFrm = pfrmDesc->bDataFrm;
	U16 HdrLen;

#if 0
if(mlme_dbg_level&MLME_DEBUG_TX_DUMP_PKT)
if(signal->buf)
    printk("mkFragment2,skb_shinfo(skb)->nr_frags=0x%x,0x%x,0x%x\n",signal->buf,skb_shinfo((struct sk_buff *)signal->buf),skb_shinfo((struct sk_buff *)signal->buf)->nr_frags);
#endif
	
	ZDEBUG("mkFragment");
	pfrmInfo = &signal->frmInfo;
	pfrmInfo->frmDesc = pfrmDesc; //make connection for signal and frmDesc
	//PSDEBUG_V("mkFrag pfrmDesc", (U32)pfrmInfo->frmDesc);
	mpdu = pfrmDesc->mpdu;
	vapId = signal->vapId;
	//pHash = pfrmDesc->pHash; //yarco debug

#if 1 //ivan
    //KeyInstalled = mGkInstalled;
    if(mlme_dbg_level&MLME_DEBUG_WPA_DATA)
        printk("\nmDynKeyMode=%d bDataFrm=%d\n",mDynKeyMode,bDataFrm);

    if(mlme_dbg_level&MLME_DEBUG_TX_DUMP_PKT)
    { 
        //printk("\nsend header----------------------\n");
        printk("\nsend header length=0x%x\n",mpdu->HdrLen);
        zd1205_dump_data("header",(unsigned char *)mpdu->header,(int)mpdu->HdrLen);
        //printk("body----------------------\n");   
        printk("body length=0x%x\n",mpdu->bodyLen);
        zd1205_dump_data("body", mpdu->body, mpdu->bodyLen);        
    }

    if((mDynKeyMode==DYN_KEY_TKIP)&&(bDataFrm))
    {
        MICvar *mic_var=0;

#if 0
if(mlme_dbg_level&MLME_DEBUG_TX_DUMP_PKT)
if(signal->buf)
    printk("mkFragment6,skb_shinfo(skb)->nr_frags=0x%x,0x%x,0x%x\n",signal->buf,skb_shinfo((struct sk_buff *)signal->buf),skb_shinfo((struct sk_buff *)signal->buf)->nr_frags);
#endif   
#if 0        
        if(isGroup(addr3(mpdu))) //GTK
        {
//            printk("\nUse GTK\n");
            if(mGkInstalled)
                mic_var=&gtk_mic_tx_var;
        }
        else //PTK
#endif        
        {
//            printk("\nUse PTK\n");
            if(mPkInstalled)
                mic_var=&ptk_mic_tx_var;
        }
               
        if(mic_var)
        {
#if 0
        //printk("\nsend header----------------------\n");
        printk("\nsend header length=0x%x\n",mpdu->HdrLen);
        zd1205_dump_data("header", mpdu->header, mpdu->HdrLen);
        //printk("body----------------------\n");   
        printk("body length=0x%x\n",mpdu->bodyLen);
        zd1205_dump_data("body", mpdu->body, mpdu->bodyLen);        
#endif

            MICclear(mic_var);
    		pByte = &mpdu->header[16];
//    		printk("DA=");
    		for(i=0; i<6; i++){ //for DA
//    		    printk("%02x ",*pByte);
    			MICappendByte(*pByte++, mic_var);    			
    		}
    		
    		pByte = &mpdu->header[10];
//    		printk("SA=");
    		for(i=0; i<6; i++){ //for SA
//    		    printk("%02x ",*pByte);
    			MICappendByte(*pByte++, mic_var);
    		}
    
    		MICappendByte(0, mic_var);
    		MICappendByte(0, mic_var);
    		MICappendByte(0, mic_var);
    		MICappendByte(0, mic_var);
    		
    		pByte = mpdu->body;
    		for (i=0; i<mpdu->bodyLen; i++)
    		{
    			MICappendByte(*pByte++, mic_var);
            }
#if 0    		
if(mlme_dbg_level&MLME_DEBUG_TX_DUMP_PKT)
if(signal->buf)
    printk("mkFragment8,skb_shinfo(skb)->nr_frags=0x%x,0x%x,0x%x\n",signal->buf,skb_shinfo((struct sk_buff *)signal->buf),skb_shinfo((struct sk_buff *)signal->buf)->nr_frags);
#endif		
    		MICgetMIC(pByte, mic_var); // Append MIC (8 byte)
    		//printk("*****************************\n");   		
#if 0
    		printk("\nMIC code(len:%d)=",mpdu->bodyLen);
    		for(i=0;i<8;i++)
    		    printk("%02x ",pByte[i]);
#endif    		    
    		mpdu->bodyLen += MIC_LNG;
#if 0
if(mlme_dbg_level&MLME_DEBUG_TX_DUMP_PKT)
if(signal->buf)
    printk("mkFragment9,skb_shinfo(skb)->nr_frags=0x%x,0x%x,0x%x\n",signal->buf,skb_shinfo((struct sk_buff *)signal->buf),skb_shinfo((struct sk_buff *)signal->buf)->nr_frags);
#endif
//    		printk("\n");
        }
    }    
#else
	if (mDynKeyMode == DYN_KEY_TKIP)
	{
		if (bDataFrm)
		{
			if (isGroup(addr1(mpdu)))
			{
				KeyInstalled = mGkInstalled;
				
				if (mDynKeyMode == DYN_KEY_TKIP){
					pTxMicKey = &mBcMicKey;
				if (mWpaBcKeyLen != 32) // Not TKIP, don't make MIC
					KeyInstalled = 0;
				}		
			}
			else{ //unicast
				pHash = HashSearch(addr1(mpdu));
				if (!pHash){
					FPRINT("HashSearch2 failed !!!");
					zd1205_dump_data("addr1 = ", (U8 *)addr1(mpdu), 6);
					KeyInstalled = 0;
				}
				else {
					if (mDynKeyMode == DYN_KEY_TKIP)
					pTxMicKey = &pHash->TxMicKey;	
					KeyInstalled = pHash->pkInstalled;
				}	
			}	
			
			if ((KeyInstalled) && (mDynKeyMode == DYN_KEY_TKIP))
			{
				U16 len = mpdu->bodyLen;
				
				// calculate and append MIC to payload before fragmentation
				MICclear(pTxMicKey);
				
				//pByte = (U8 *)addr1(mpdu);
				pByte = &mpdu->header[4];
				for(i=0; i<6; i++){ //for DA
					MICappendByte(*pByte++, pTxMicKey);
				}
				
				//pByte = (U8 *)addr3(mpdu);
				pByte = &mpdu->header[16];
				for(i=0; i<6; i++){ //for SA
					MICappendByte(*pByte++, pTxMicKey);
				}

				MICappendByte(0, pTxMicKey);
				MICappendByte(0, pTxMicKey);
				MICappendByte(0, pTxMicKey);
				MICappendByte(0, pTxMicKey);
				
				pByte = mpdu->body;
				for (i=0; i<len; i++){
					MICappendByte(*pByte++, pTxMicKey);
				}
				MICgetMIC(pByte, pTxMicKey); // Append MIC (8 byte)
				mpdu->bodyLen += MIC_LNG;
				
				//if (pfrmDesc->bEapol)
				//	FPRINT("Tx Eapol Frame !!!");
				//zd1205_dump_data("Tx MIC = ", pByte, MIC_LNG);
			}	
		} // end of BT_DATA	
	}	   
#endif //#if 1 ,else
	
	bWep = mPrivacyInvoked;
	if ((!bDataFrm) || (pfrmDesc->bEapol))
		bWep = FALSE;
#if 0		
if(mlme_dbg_level&MLME_DEBUG_TX_DUMP_PKT)
if(signal->buf)
    printk("mkFragment2,skb_shinfo(skb)->nr_frags=0x%x,0x%x,0x%x\n",signal->buf,skb_shinfo((struct sk_buff *)signal->buf),skb_shinfo((struct sk_buff *)signal->buf)->nr_frags);
#endif
		
//if(0)	//if tx packet needn't encryption
	if(mGkInstalled&&mPkInstalled&&bDataFrm)
	{
	    //printk("*************bWep = TRUE\n");
		bWep = TRUE;
    }
//printk("mpdu->bodyLen=%d\n",mpdu->bodyLen);    
//printk("<bWep=%d mPrivacyInvoked=%d>",bWep,mPrivacyInvoked);
	//pfrmInfo->fTot	= 1;
	pfrmInfo->eol = 0;

	pdusize = mFragThreshold;
	if ((!isGroup(addr1(mpdu))) && (mpdu->HdrLen + mpdu->bodyLen + CRC_LNG > pdusize))
	{ //Need fragment
		pdusize -= mpdu->HdrLen + CRC_LNG;
		pfrmInfo->fTot	= (mpdu->bodyLen + (pdusize-1)) / pdusize;
		if (pfrmInfo->fTot == 0) 
			pfrmInfo->fTot = 1;
	}
	else{
		pdusize = mpdu->bodyLen;
		pfrmInfo->fTot = 1;
	}
	
	curMpdu = mpdu;
	pBody = mpdu->body;
	len = mpdu->bodyLen;
	Num = pfrmInfo->fTot;
//printk("fragment num=%d\n",Num);	
	HdrLen = mpdu->HdrLen;

if(mlme_dbg_level&MLME_DEBUG_TX_DUMP_PKT)
if(signal->buf)
    printk("mkFragment3,skb_shinfo(skb)->nr_frags=0x%x\n",skb_shinfo((struct sk_buff *)signal->buf)->nr_frags);

	for (fn=0; fn<Num; fn++)
	{
		if (fn){
			curMpdu = &pfrmDesc->mpdu[fn];
			memcpy(&curMpdu->header[0], &mpdu->header[0], HdrLen); //make header
			curMpdu->HdrLen = HdrLen;
			curMpdu->body = pBody;
		}
		//setFrag(curMpdu, fn);			
		curMpdu->header[22] = ((curMpdu->header[22] & 0xF0) | fn);	

if(mlme_dbg_level&MLME_DEBUG_TX_DUMP_PKT)
if(signal->buf)
    printk("mkFragment3,skb_shinfo(skb)->nr_frags=0x%x\n",skb_shinfo((struct sk_buff *)signal->buf)->nr_frags);


		if (fn == (Num - 1))
		{
			curMpdu->bodyLen = len;
			//setMoreFrag(curMpdu, 0);		
			curMpdu->header[1] &= ~MORE_FRAG_BIT;
//printk("curMpdu->bodyLen=%d\n",curMpdu->bodyLen);
		}
		else
		{
			curMpdu->bodyLen = pdusize;
			pBody += pdusize;
			len -= pdusize;
			//setMoreFrag(curMpdu, 1);		
			curMpdu->header[1] |= MORE_FRAG_BIT;
		}

		if (bWep)
			//setWepBit(curMpdu, 1);
			curMpdu->header[1] |= WEP_BIT;
	}
}


BOOLEAN sendMgtFrame(Signal_t *signal, FrmDesc_t *pfrmDesc)
{
	ZDEBUG("sendMgtFrame");
	pfrmDesc->bIntraBss = 0;
	pfrmDesc->bEapol = 0;
	pfrmDesc->pHash = NULL;
	pdot11Obj->ReleaseBuffer(signal->buf);
    signal->buf = NULL;
    signal->bDataFrm = 0;
    //pfrmDesc->bDataFrm = 0;
	mkFragment(signal, pfrmDesc);
    return SendPkt(signal, pfrmDesc, TRUE);
}


BOOLEAN	getElem(Frame_t	*frame, ElementID  eleID, Element  *elem)
{
	U8 k = 0; 	//offset bytes to first element
	U8 n = 0; 	//num. of element
	U8 pos;		//current position
	U8 len;
	
	switch (frmType(frame)){
		case ST_PROBE_REQ:
			k = 0;
			n = 2;
			if (mDynKeyMode == DYN_KEY_TKIP)	
				n++;			
			break;
		
		case ST_ASOC_REQ:
			k = 4;
			n = 2;
			if (mDynKeyMode == DYN_KEY_TKIP)	
				n++;
			break;
			
		case ST_REASOC_REQ:
			k = 10;
			n = 2;
			if (mDynKeyMode == DYN_KEY_TKIP)		
				n++;
			break;
			
		default:
			return FALSE;	
	}

	while(n--){
		pos = frame->body[k]; 
		len = frame->body[k+1] + 2;
		if (pos == eleID){	//match
			memcpy((U8 *)elem, &frame->body[k], len);
			return TRUE;
		}
		else{
			k += len;
		}
	}
	
	return FALSE;
}


void mkAuthFrm(FrmDesc_t* pfrmDesc, MacAddr_t *addr1, U16 Alg, U16 Seq, 
	U16 Status, U8 *pChalng, U8 vapId)
{
	U8 *body;
	U16 len;
	Frame_t *pf = pfrmDesc->mpdu;
	
	setFrameType(pf, ST_AUTH);
	pf->body = pfrmDesc->buffer;
	body = pf->body;
	setAddr1(pf, addr1);
	setAddr2(pf, &dot11MacAddress);
	setAddr3(pf, &mBssId);

	pf->HdrLen = MAC_HDR_LNG;		
	
	body[0] = Alg & 0xff;			//AuthAlg
	body[1] = (Alg & 0xff00) >> 8;
	body[2] = Seq & 0xff;			//AuthSeq
	body[3] = (Seq & 0xff00) >> 8;
	body[4] = Status & 0xff;		//Status
	body[5] = (Status & 0xff00) >> 8;
	len = 6;
	
	if ((Alg == SHARE_KEY) && (Seq == 2) && (pChalng)) {
		body[len] = EID_CTEXT;
		body[len+1] = CHAL_TEXT_LEN;
		memcpy(&body[len+2], pChalng, CHAL_TEXT_LEN);
		len += (2+CHAL_TEXT_LEN);
	}
	
	pf->bodyLen = len;
}	


void mkRe_AsocRspFrm(FrmDesc_t* pfrmDesc, TypeSubtype subType, MacAddr_t *addr1, 
	U16 Cap, U16 Status, U16 Aid, Element *pSupRates, Element *pExtRates, U8 vapId)
{
	U8 *body;
	U8 elemLen;
	U16 len;
	Frame_t *pf = pfrmDesc->mpdu;
	
	setFrameType(pf, subType);
	pf->body = pfrmDesc->buffer;
	body = pf->body;
	setAddr1(pf, addr1);
	setAddr2(pf, &dot11MacAddress);
	setAddr3(pf, &mBssId);
	pf->HdrLen = MAC_HDR_LNG;		
	
	body[0] = Cap & 0xff;			//Cap
	body[1] = (Cap & 0xff00) >> 8;
	body[2] = Status & 0xff;		//Status
	body[3] = (Status & 0xff00) >> 8;
	body[4] = Aid & 0xff;			//AID
	body[5] = (Aid & 0xff00) >> 8;
	len = 6;
	
	elemLen = pSupRates->buf[1]+2;
	memcpy(&body[len], (U8 *)pSupRates, elemLen); //Support Rates
	len += elemLen;
	
#if defined(OFDM)
	if ((mMacMode != PURE_B_MODE) && (pExtRates)){
		elemLen = pExtRates->buf[1]+2;
		memcpy(&body[len], (U8 *)pExtRates, elemLen); //Extended rates
		len += elemLen;
	}	
#endif		

	pf->bodyLen = len;		
}	


void mkProbeRspFrm(FrmDesc_t* pfrmDesc, MacAddr_t *addr1, U16 BcnInterval, 
	U16 Cap, Element *pSsid, Element *pSupRates, Element *pDsParms, 
	Element *pExtRates, Element *pWpa, U8 vapId)
{
	U8 *body;
	U8 elemLen;
	U16 len;
	Frame_t *pf = pfrmDesc->mpdu;
	
	setFrameType(pf, ST_PROBE_RSP);
	pf->body = pfrmDesc->buffer;
	body = pf->body;
	setAddr1(pf, addr1);
	setAddr2(pf, &dot11MacAddress);
	setAddr3(pf, &mBssId);
	pf->HdrLen = MAC_HDR_LNG;	
	
	body[8] = BcnInterval & 0xff;	//BcnPeriod
	body[9] = (BcnInterval & 0xff00) >> 8;
	body[10] = Cap & 0xff;			//Cap
	body[11] = (Cap & 0xff00) >> 8;
	
	len = 12;
	elemLen = pSsid->buf[1]+2;
	memcpy(&body[len], (U8 *)pSsid, elemLen); //Extended rates
	len += elemLen;
	
	elemLen = pSupRates->buf[1]+2;
	memcpy(&body[len], (U8 *)pSupRates, elemLen); //Extended rates
	len += elemLen;
	
	elemLen = pDsParms->buf[1]+2;
	memcpy(&body[len], (U8 *)pDsParms, elemLen); //Extended rates
	len += elemLen;
	
#if defined(OFDM)
	if ((mMacMode != PURE_B_MODE) && (pExtRates)){
		elemLen = pExtRates->buf[1]+2;
		memcpy(&body[len], (U8 *)pExtRates, elemLen); //Extended rates
		len += elemLen;
	}	
#endif			

	if ((mDynKeyMode == DYN_KEY_TKIP)  && (pWpa)){
		elemLen = pWpa->buf[1]+2;
		memcpy(&body[len], (U8 *)pWpa, elemLen); //WPA IE
		len += elemLen;
	}
	
	pf->bodyLen = len;			
}	


void mkDisAssoc_DeAuthFrm(FrmDesc_t* pfrmDesc, TypeSubtype subType, MacAddr_t *addr1, 
	U16 Reason, U8 vapId)
{
	U8 *body;
	Frame_t *pf = pfrmDesc->mpdu;
	
	setFrameType(pf, subType);
	pf->body = pfrmDesc->buffer;
	body = pf->body;
	setAddr1(pf, addr1);
	setAddr2(pf, &dot11MacAddress);
	setAddr3(pf, &mBssId);
	pf->HdrLen = MAC_HDR_LNG;	
	
	body[0] = Reason & 0xff;	//Reason Code
	body[1] = (Reason & 0xff00) >> 8;	
	
	pf->bodyLen = 2;	
}	 	


void sendProbeRspFrm(MacAddr_t *addr1, U16 BcnInterval, U16 Cap, 
	Element *pSsid, Element *pSupRates, Element *pDsParms, 
	Element *pExtRates, Element *pWpa, U8 vapId)
{
	Signal_t *signal;
	FrmDesc_t *pfrmDesc;
	
	if ((signal = allocSignal()) == NULL)  
		return;
		
	if ((pfrmDesc = allocFdesc()) == NULL){
		freeSignal(signal);
		return;
	}	
	
	mkProbeRspFrm(pfrmDesc, addr1, BcnInterval, Cap, pSsid, pSupRates, pDsParms, 
		pExtRates, pWpa, vapId);		
	sendMgtFrame(signal, pfrmDesc);
}	
	
#endif
