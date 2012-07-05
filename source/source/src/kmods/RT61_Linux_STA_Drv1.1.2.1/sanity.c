/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2006, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	sanity.c

	Abstract:

	Revision History:
	Who			When			What
	--------	----------		----------------------------------------------
	John Chang  2004-09-01      add WMM support
*/

#include "rt_config.h"
#include <net/iw_handler.h>

UCHAR	WPA_OUI[] = {0x00, 0x50, 0xf2, 0x01};
UCHAR	RSN_OUI[] = {0x00, 0x0f, 0xac};
UCHAR   WME_INFO_ELEM[]  = {0x00, 0x50, 0xf2, 0x02, 0x00, 0x01};
UCHAR   WME_PARM_ELEM[] = {0x00, 0x50, 0xf2, 0x02, 0x01, 0x01};
UCHAR   RALINK_OUI[]  = {0x00, 0x0c, 0x43};


/* 
    ==========================================================================
    Description:
        MLME message sanity check
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
 */
BOOLEAN MlmeScanReqSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT UCHAR *pBssType, 
    OUT CHAR Ssid[], 
    OUT UCHAR *pSsidLen, 
    OUT UCHAR *pScanType) 
{
    MLME_SCAN_REQ_STRUCT *Info;

    Info = (MLME_SCAN_REQ_STRUCT *)(Msg);
    *pBssType = Info->BssType;
    *pSsidLen = Info->SsidLen;
    NdisMoveMemory(Ssid, Info->Ssid, *pSsidLen);
    *pScanType = Info->ScanType;
    
    if ((*pBssType == BSS_INFRA || *pBssType == BSS_ADHOC || *pBssType == BSS_ANY) &&
    	//2007/12/06:KH add to enable FAST_SCAN
       (*pScanType == SCAN_ACTIVE || *pScanType == SCAN_PASSIVE || *pScanType == FAST_SCAN_ACTIVE
 	))
       return TRUE;
    else 
    {
        DBGPRINT(RT_DEBUG_TRACE, "MlmeScanReqSanity fail - wrong BssType or ScanType\n");
        return FALSE;
    }
}

/* 
    ==========================================================================
    Description:
        MLME message sanity check
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
 */
BOOLEAN MlmeStartReqSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT CHAR Ssid[], 
    OUT UCHAR *pSsidLen) 
{
    MLME_START_REQ_STRUCT *Info;

    Info = (MLME_START_REQ_STRUCT *)(Msg);
    
    if (Info->SsidLen > MAX_LEN_OF_SSID)
    {
        DBGPRINT(RT_DEBUG_TRACE, "MlmeStartReqSanity fail - wrong SSID length\n");
        return FALSE;
    }

    *pSsidLen = Info->SsidLen;
    NdisMoveMemory(Ssid, Info->Ssid, *pSsidLen);

    return TRUE;
}

/* 
    ==========================================================================
    Description:
        MLME message sanity check
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
 */
BOOLEAN MlmeAssocReqSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pApAddr, 
    OUT USHORT *pCapabilityInfo, 
    OUT ULONG *pTimeout, 
    OUT USHORT *pListenIntv) 
{
    MLME_ASSOC_REQ_STRUCT *pInfo;

    pInfo = (MLME_ASSOC_REQ_STRUCT *)Msg;
    *pTimeout = pInfo->Timeout;                             // timeout
    COPY_MAC_ADDR(pApAddr, pInfo->Addr);                   // AP address
    *pCapabilityInfo = pInfo->CapabilityInfo;               // capability info
    *pListenIntv = pInfo->ListenIntv;
    
    return TRUE;
}

/* 
    ==========================================================================
    Description:
        MLME message sanity check
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
 */
BOOLEAN MlmeAuthReqSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr, 
    OUT ULONG *pTimeout, 
    OUT USHORT *pAlg) 
{
    MLME_AUTH_REQ_STRUCT *pInfo;

    pInfo  = (MLME_AUTH_REQ_STRUCT *)Msg;
    COPY_MAC_ADDR(pAddr, pInfo->Addr);
    *pTimeout = pInfo->Timeout;
    *pAlg = pInfo->Alg;
    
    if (((*pAlg == Ndis802_11AuthModeShared) ||(*pAlg == Ndis802_11AuthModeOpen)) && 
        ((*pAddr & 0x01) == 0)) 
    {
        return TRUE;
    } 
    else 
    {
        DBGPRINT(RT_DEBUG_TRACE, "MlmeAuthReqSanity fail - wrong algorithm\n");
        return FALSE;
    }
}

/* 
    ==========================================================================
    Description:
        MLME message sanity check
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
 */
BOOLEAN PeerAssocRspSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *pMsg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr2, 
    OUT USHORT *pCapabilityInfo, 
    OUT USHORT *pStatus, 
    OUT USHORT *pAid, 
    OUT UCHAR SupRate[], 
    OUT UCHAR *pSupRateLen,
    OUT UCHAR ExtRate[], 
    OUT UCHAR *pExtRateLen,
    OUT PEDCA_PARM pEdcaParm)
{
    CHAR          IeType, *Ptr;
    PFRAME_802_11 pFrame = (PFRAME_802_11)pMsg;
    PEID_STRUCT   pEid;
    
    COPY_MAC_ADDR(pAddr2, pFrame->Hdr.Addr2);
    Ptr = pFrame->Octet;
        
    NdisMoveMemory(pCapabilityInfo, &pFrame->Octet[0], 2);
    NdisMoveMemory(pStatus,         &pFrame->Octet[2], 2);
    *pExtRateLen = 0;
    pEdcaParm->bValid = FALSE;
    
    if (*pStatus != MLME_SUCCESS) 
        return TRUE;
    
    NdisMoveMemory(pAid, &pFrame->Octet[4], 2);

    // 	change Endian in RTMPFrameEndianChange() on big endian platform
    //*pAid = le2cpu16(*pAid);
    
    // TODO: check big endian issue &0x3fff
    *pAid = (*pAid) & 0x3fff; // AID is low 14-bit
        
    // -- get supported rates from payload and advance the pointer
    IeType = pFrame->Octet[6];
    *pSupRateLen = pFrame->Octet[7];
    if ((IeType != IE_SUPP_RATES) || (*pSupRateLen > MAX_LEN_OF_SUPPORTED_RATES))
    {
        DBGPRINT(RT_DEBUG_TRACE, "PeerAssocRspSanity fail - wrong SupportedRates IE\n");
        return FALSE;
    } 
    else 
   		NdisMoveMemory(SupRate, &pFrame->Octet[8], *pSupRateLen);

    // many AP implement proprietary IEs in non-standard order, we'd better
    // tolerate mis-ordered IEs to get best compatibility
    pEid = (PEID_STRUCT) &pFrame->Octet[8 + (*pSupRateLen)];
            
    // get variable fields from payload and advance the pointer
    while (((UCHAR*)pEid + pEid->Len + 1) < ((UCHAR*)pFrame + MsgLen))
    {
        switch (pEid->Eid)
        {
            case IE_EXT_SUPP_RATES:
                if (pEid->Len <= MAX_LEN_OF_SUPPORTED_RATES)
                {
                    NdisMoveMemory(ExtRate, pEid->Octet, pEid->Len);
                    *pExtRateLen = pEid->Len;
                }
                break;
				
			case IE_VENDOR_SPECIFIC:
                // handle WME PARAMTER ELEMENT
                if (NdisEqualMemory(pEid->Octet, WME_PARM_ELEM, 6) && (pEid->Len == 24))
                {
                    PUCHAR ptr;
                    int i;

                    // parsing EDCA parameters
                    pEdcaParm->bValid          = TRUE;
                    pEdcaParm->bQAck           = FALSE; // pEid->Octet[0] & 0x10;
                    pEdcaParm->bQueueRequest   = FALSE; // pEid->Octet[0] & 0x20;
                    pEdcaParm->bTxopRequest    = FALSE; // pEid->Octet[0] & 0x40;
                    //pEdcaParm->bMoreDataAck    = FALSE; // pEid->Octet[0] & 0x80;
                    pEdcaParm->EdcaUpdateCount = pEid->Octet[6] & 0x0f;
                    pEdcaParm->bAPSDCapable    = (pEid->Octet[6] & 0x80) ? 1 : 0;
                    ptr = &pEid->Octet[8];
                    for (i=0; i<4; i++)
                    {
                        UCHAR aci = (*ptr & 0x60) >> 5; // b5~6 is AC INDEX
                        pEdcaParm->bACM[aci]  = (((*ptr) & 0x10) == 0x10);   // b5 is ACM
                        pEdcaParm->Aifsn[aci] = (*ptr) & 0x0f;               // b0~3 is AIFSN
                        pEdcaParm->Cwmin[aci] = *(ptr+1) & 0x0f;             // b0~4 is Cwmin
                        pEdcaParm->Cwmax[aci] = *(ptr+1) >> 4;               // b5~8 is Cwmax
                        pEdcaParm->Txop[aci]  = *(ptr+2) + 256 * (*(ptr+3)); // in unit of 32-us
                        ptr += 4; // point to next AC
                    }
                }
                break;

#if 0				
            case IE_EDCA_PARAMETER:
                if (pEid->Len == 18)
                {
                    PUCHAR ptr;
                    int i;
                    pEdcaParm->bValid          = TRUE;
                    pEdcaParm->bQAck           = pEid->Octet[0] & 0x10;
                    pEdcaParm->bQueueRequest   = pEid->Octet[0] & 0x20;
                    pEdcaParm->bTxopRequest    = pEid->Octet[0] & 0x40;
//                  pEdcaParm->bMoreDataAck    = pEid->Octet[0] & 0x80;
                    pEdcaParm->EdcaUpdateCount = pEid->Octet[0] & 0x0f;
                    ptr = &pEid->Octet[2];
                    for (i=0; i<4; i++)
                    {
                        UCHAR aci = (*ptr & 0x60) >> 5; // b5~6 is AC INDEX
                        pEdcaParm->bACM[aci]  = (((*ptr) & 0x10) == 0x10);   // b5 is ACM
                        pEdcaParm->Aifsn[aci] = (*ptr) & 0x0f;               // b0~3 is AIFSN
                        pEdcaParm->Cwmin[aci] = *(ptr+1) & 0x0f;             // b0~4 is Cwmin
                        pEdcaParm->Cwmax[aci] = *(ptr+1) >> 4;               // b5~8 is Cwmax
                        pEdcaParm->Txop[aci]  = *(ptr+2) + 256 * (*(ptr+3)); // in unit of 32-us
                        ptr += 4; // point to next AC
                    }
                }
                break;
#endif
            default:
                DBGPRINT(RT_DEBUG_TRACE, "PeerAssocRspSanity - ignore unrecognized EID = %d\n", pEid->Eid);
                break;
        }
                   
        pEid = (PEID_STRUCT)((UCHAR*)pEid + 2 + pEid->Len);        
    }

    return TRUE;
}

/* 
    ==========================================================================
    Description:
        MLME message sanity check
    Return:
        TRUE if all parameters are OK, FALSE otherwise       
    ==========================================================================
 */
BOOLEAN PeerDisassocSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr2, 
    OUT USHORT *pReason) 
{
    PFRAME_802_11 pFrame = (PFRAME_802_11)Msg;

    COPY_MAC_ADDR(pAddr2, pFrame->Hdr.Addr2);
    NdisMoveMemory(pReason, &pFrame->Octet[0], 2);

    return TRUE;
}

/* 
    ==========================================================================
    Description:
        MLME message sanity check
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
 */
BOOLEAN PeerDeauthSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr2, 
    OUT USHORT *pReason) 
{
    PFRAME_802_11 pFrame = (PFRAME_802_11)Msg;

    COPY_MAC_ADDR(pAddr2, pFrame->Hdr.Addr2);
    NdisMoveMemory(pReason, &pFrame->Octet[0], 2);

    return TRUE;
}

/* 
    ==========================================================================
    Description:
        MLME message sanity check
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
 */
BOOLEAN PeerAuthSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr, 
    OUT USHORT *pAlg, 
    OUT USHORT *pSeq, 
    OUT USHORT *pStatus, 
    CHAR *pChlgText) 
{
    PFRAME_802_11 pFrame = (PFRAME_802_11)Msg;

    COPY_MAC_ADDR(pAddr,   pFrame->Hdr.Addr2);
    NdisMoveMemory(pAlg,    &pFrame->Octet[0], 2);
    NdisMoveMemory(pSeq,    &pFrame->Octet[2], 2);
    NdisMoveMemory(pStatus, &pFrame->Octet[4], 2);

    if (*pAlg == Ndis802_11AuthModeOpen)
    {
        if (*pSeq == 1 || *pSeq == 2) 
        {
            return TRUE;
        } 
        else 
        {
            DBGPRINT(RT_DEBUG_TRACE, "PeerAuthSanity fail - wrong Seg#\n");
            return FALSE;
        }
    } 
    else if (*pAlg == Ndis802_11AuthModeShared) 
    {
        if (*pSeq == 1 || *pSeq == 4) 
        {
            return TRUE;
        } 
        else if (*pSeq == 2 || *pSeq == 3) 
        {
            NdisMoveMemory(pChlgText, &pFrame->Octet[8], CIPHER_TEXT_LEN);
            return TRUE;
        } 
        else 
        {
            DBGPRINT(RT_DEBUG_TRACE, "PeerAuthSanity fail - wrong Seg#\n");
            return FALSE;
        }
    } 
    else 
    {
        DBGPRINT(RT_DEBUG_TRACE, "PeerAuthSanity fail - wrong algorithm\n");
        return FALSE;
    }
}

/* 
    ==========================================================================
    Description:
        MLME message sanity check
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
 */
BOOLEAN PeerProbeReqSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr2,
    OUT CHAR Ssid[], 
    OUT UCHAR *pSsidLen) 
{
    UCHAR         Idx;
	UCHAR	      RateLen;
	CHAR          IeType;
    PFRAME_802_11 pFrame = (PFRAME_802_11)Msg;

    COPY_MAC_ADDR(pAddr2, pFrame->Hdr.Addr2);

    if ((pFrame->Octet[0] != IE_SSID) || (pFrame->Octet[1] > MAX_LEN_OF_SSID)) 
    {
        DBGPRINT(RT_DEBUG_TRACE, "PeerProbeReqSanity fail - wrong SSID IE(Type=%d,Len=%d)\n",pFrame->Octet[0],pFrame->Octet[1]);
        return FALSE;
    } 
    
    *pSsidLen = pFrame->Octet[1];
    NdisMoveMemory(Ssid, &pFrame->Octet[2], *pSsidLen);

    Idx = *pSsidLen + 2;

	// -- get supported rates from payload and advance the pointer
	IeType = pFrame->Octet[Idx];
	RateLen = pFrame->Octet[Idx + 1];
	if (IeType != IE_SUPP_RATES) 
    {
        DBGPRINT(RT_DEBUG_TRACE, "PeerProbeReqSanity fail - wrong SupportRates IE(Type=%d,Len=%d)\n",pFrame->Octet[Idx],pFrame->Octet[Idx+1]);
        return FALSE;
    }
	else
	{
		if ((pAd->PortCfg.AdhocMode == 2) && (RateLen < 8))
			return (FALSE);
	}

    return TRUE;
}

/* 
    ==========================================================================
    Description:
        MLME message sanity check
    Return:
        TRUE if all parameters are OK, FALSE otherwise   
    ==========================================================================
 */
BOOLEAN PeerBeaconAndProbeRspSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr2, 
    OUT PUCHAR pBssid, 
    OUT CHAR Ssid[], 
    OUT UCHAR *pSsidLen, 
    OUT UCHAR *pBssType, 
    OUT USHORT *pBeaconPeriod, 
    OUT UCHAR *pChannel,
    OUT UCHAR *pNewChannel,
    OUT LARGE_INTEGER *pTimestamp, 
    OUT CF_PARM *pCfParm, 
    OUT USHORT *pAtimWin, 
    OUT USHORT *pCapabilityInfo, 
    OUT UCHAR *pErp,
    OUT UCHAR *pDtimCount, 
    OUT UCHAR *pDtimPeriod, 
    OUT UCHAR *pBcastFlag, 
    OUT UCHAR *pMessageToMe, 
    OUT UCHAR SupRate[],
	OUT UCHAR *pSupRateLen,
	OUT UCHAR ExtRate[],
	OUT UCHAR *pExtRateLen,
	OUT	UCHAR *pCkipFlag,
	OUT	UCHAR *pAironetCellPowerLimit,
	OUT PEDCA_PARM       pEdcaParm,
	OUT PQBSS_LOAD_PARM  pQbssLoad,
	OUT PQOS_CAPABILITY_PARM pQosCapability,
	OUT ULONG *pRalinkIe,
//	OUT UCHAR *LengthVIE,	
	OUT USHORT *LengthVIE,	// edit by johnli, variable ie length could be > 256
    OUT	PNDIS_802_11_VARIABLE_IEs pVIE)
{
    CHAR				*Ptr, TimLen;
    PFRAME_802_11		pFrame;
    PEID_STRUCT         pEid;
    UCHAR				SubType;
	UCHAR				Sanity;

	// Add for 3 necessary EID field check
	Sanity = 0;

    *pAtimWin = 0;
    *pErp = 0;	
    *pDtimCount = 0;
    *pDtimPeriod = 0;
    *pBcastFlag = 0;
    *pMessageToMe = 0;
    *pExtRateLen = 0;
    *pCkipFlag = 0;			        // Default of CkipFlag is 0
	*pAironetCellPowerLimit = 0xFF;  // Default of AironetCellPowerLimit is 0xFF
	*LengthVIE = 0;					// Set the length of VIE to init value 0
	*pRalinkIe = 0;
	*pNewChannel = 0;
	pCfParm->bValid = FALSE;        // default: no IE_CF found
    pQbssLoad->bValid = FALSE;      // default: no IE_QBSS_LOAD found
    pEdcaParm->bValid = FALSE;      // default: no IE_EDCA_PARAMETER found
    pQosCapability->bValid = FALSE; // default: no IE_QOS_CAPABILITY found
    
    pFrame = (PFRAME_802_11)Msg;
    
    // get subtype from header
    SubType = (UCHAR)pFrame->Hdr.FC.SubType;

    // get Addr2 and BSSID from header
    COPY_MAC_ADDR(pAddr2, pFrame->Hdr.Addr2);
    COPY_MAC_ADDR(pBssid, pFrame->Hdr.Addr3);
    
    Ptr = pFrame->Octet;
    
    // get timestamp from payload and advance the pointer
    NdisMoveMemory(pTimestamp, Ptr, TIMESTAMP_LEN);
    Ptr += TIMESTAMP_LEN;

    // get beacon interval from payload and advance the pointer
    NdisMoveMemory(pBeaconPeriod, Ptr, 2);
    Ptr += 2;

    // get capability info from payload and advance the pointer
    NdisMoveMemory(pCapabilityInfo, Ptr, 2);
    Ptr += 2;
    if (CAP_IS_ESS_ON(*pCapabilityInfo)) 
        *pBssType = BSS_INFRA;
    else 
        *pBssType = BSS_ADHOC;

    pEid = (PEID_STRUCT) Ptr;

    // get variable fields from payload and advance the pointer
    while(((UCHAR*)pEid + pEid->Len + 1) < ((UCHAR*)pFrame + MsgLen))
    {
        switch(pEid->Eid)
        {
            case IE_SSID:
				// Already has one SSID EID in this beacon, ignore the second one
				if (Sanity & 0x1)
					break;				
                if(pEid->Len <= MAX_LEN_OF_SSID)
                {
                    NdisMoveMemory(Ssid, pEid->Octet, pEid->Len);
                    *pSsidLen = pEid->Len;
					Sanity |= 0x1;
                }
                else
                {
                    DBGPRINT(RT_DEBUG_TRACE, "PeerBeaconAndProbeRspSanity - wrong IE_SSID (len=%d)\n",pEid->Len);
                    return FALSE;
                }
                break;

            case IE_SUPP_RATES:
                if(pEid->Len <= MAX_LEN_OF_SUPPORTED_RATES)
            	{
					Sanity |= 0x2;
					NdisMoveMemory(SupRate, pEid->Octet, pEid->Len);
					*pSupRateLen = pEid->Len;

					// TODO: 2004-09-14 not a good design here, cause it exclude extra rates 
					// from ScanTab. We should report as is. And filter out unsupported
					// rates in MlmeAux.
					// Check against the supported rates
					// RTMPCheckRates(pAd, SupRate, pSupRateLen);
                }
                else
                {
                    DBGPRINT(RT_DEBUG_TRACE, "PeerBeaconAndProbeRspSanity - wrong IE_SUPP_RATES (len=%d)\n",pEid->Len);
                    return FALSE;
                }
                break;

            case IE_FH_PARM:
                DBGPRINT(RT_DEBUG_TRACE, "PeerBeaconAndProbeRspSanity(IE_FH_PARM) \n");
                break;

            case IE_DS_PARM:
                if(pEid->Len == 1)
                {
                    *pChannel = *pEid->Octet;                    
                    if (ChannelSanity(pAd, *pChannel) == 0)
                    {
                        DBGPRINT(RT_DEBUG_INFO, "PeerBeaconAndProbeRspSanity - wrong IE_DS_PARM (ch=%d)\n",*pChannel);
                        return FALSE;
                    }
					Sanity |= 0x4;
                }
                else
                {
                    DBGPRINT(RT_DEBUG_TRACE, "PeerBeaconAndProbeRspSanity - wrong IE_DS_PARM (len=%d)\n",pEid->Len);
                    return FALSE;
                }
                break;

            case IE_CF_PARM:
                if(pEid->Len == 6)
                {
                    pCfParm->bValid = TRUE;
                    pCfParm->CfpCount = pEid->Octet[0];
                    pCfParm->CfpPeriod = pEid->Octet[1];
                    pCfParm->CfpMaxDuration = pEid->Octet[2] + 256 * pEid->Octet[3];
                    pCfParm->CfpDurRemaining = pEid->Octet[4] + 256 * pEid->Octet[5];
                }
                else
                {
                    DBGPRINT(RT_DEBUG_TRACE, "PeerBeaconAndProbeRspSanity - wrong IE_CF_PARM\n");
                    return FALSE;
                }
                break;

            case IE_IBSS_PARM:
                if(pEid->Len == 2)
                {
                    NdisMoveMemory(pAtimWin, pEid->Octet, pEid->Len);
                }
                else
                {
                    DBGPRINT(RT_DEBUG_TRACE, "PeerBeaconAndProbeRspSanity - wrong IE_IBSS_PARM\n");
                    return FALSE;
                }
                break;

            case IE_TIM:
                if(INFRA_ON(pAd) && SubType == SUBTYPE_BEACON)
                {
                    GetTimBit((PUCHAR)pEid, pAd->ActiveCfg.Aid, &TimLen, pBcastFlag, pDtimCount, pDtimPeriod, pMessageToMe);
                }
                break;
            case IE_CHANNEL_SWITCH_ANNOUNCEMENT:
                if(pEid->Len == 3)
                {
                	*pNewChannel = pEid->Octet[1];	//extract new channel number
                }
                break;
			// New for WPA
			// Wifi WMM use the same IE vale, need to parse that too
			// case IE_WPA:
            case IE_VENDOR_SPECIFIC:
				// Check the OUI version, filter out non-standard usage
				if (NdisEqualMemory(pEid->Octet, RALINK_OUI, 3) && (pEid->Len == 7))
				{
		            *pRalinkIe = pEid->Octet[3];
				}
				else if (NdisEqualMemory(pEid->Octet, WPA_OUI, 4))
				{
				    if ((*LengthVIE + pEid->Len + 2) >= MAX_VIE_LEN)
        			{
            			DBGPRINT(RT_DEBUG_WARN, "PeerBeaconAndProbeRspSanity - Variable IEs out of resource [len(=%d) > MAX_VIE_LEN(=%d)]\n",
                    		    (*LengthVIE + pEid->Len + 2), MAX_VIE_LEN);
            			break;
        			}
					// Copy to pVIE which will report to microsoft bssid list.
					Ptr = (PUCHAR) pVIE;
					NdisMoveMemory(Ptr + *LengthVIE, &pEid->Eid, pEid->Len + 2);
					*LengthVIE += (pEid->Len + 2);
				}
				else if (NdisEqualMemory(pEid->Octet, WME_PARM_ELEM, 6) && (pEid->Len == 24))
				{
		            PUCHAR ptr;
		            int i;

		            // parsing EDCA parameters
		            pEdcaParm->bValid          = TRUE;
		            pEdcaParm->bQAck           = FALSE; // pEid->Octet[0] & 0x10;
		            pEdcaParm->bQueueRequest   = FALSE; // pEid->Octet[0] & 0x20;
		            pEdcaParm->bTxopRequest    = FALSE; // pEid->Octet[0] & 0x40;
		            pEdcaParm->EdcaUpdateCount = pEid->Octet[6] & 0x0f;
                    pEdcaParm->bAPSDCapable    = (pEid->Octet[6] & 0x80) ? 1 : 0;
		            ptr = &pEid->Octet[8];
		            for (i=0; i<4; i++)
		            {
		                UCHAR aci = (*ptr & 0x60) >> 5; // b5~6 is AC INDEX
		                pEdcaParm->bACM[aci]  = (((*ptr) & 0x10) == 0x10);   // b5 is ACM
		                pEdcaParm->Aifsn[aci] = (*ptr) & 0x0f;               // b0~3 is AIFSN
		                pEdcaParm->Cwmin[aci] = *(ptr+1) & 0x0f;             // b0~4 is Cwmin
		                pEdcaParm->Cwmax[aci] = *(ptr+1) >> 4;               // b5~8 is Cwmax
		                pEdcaParm->Txop[aci]  = *(ptr+2) + 256 * (*(ptr+3)); // in unit of 32-us
		                ptr += 4; // point to next AC
		            }
				}
				else if (NdisEqualMemory(pEid->Octet, WME_INFO_ELEM, 6) && (pEid->Len == 7))
				{
		            // parsing EDCA parameters
		            pEdcaParm->bValid          = TRUE;
		            pEdcaParm->bQAck           = FALSE; // pEid->Octet[0] & 0x10;
		            pEdcaParm->bQueueRequest   = FALSE; // pEid->Octet[0] & 0x20;
		            pEdcaParm->bTxopRequest    = FALSE; // pEid->Octet[0] & 0x40;
		            pEdcaParm->EdcaUpdateCount = pEid->Octet[6] & 0x0f;
                    pEdcaParm->bAPSDCapable    = (pEid->Octet[6] & 0x80) ? 1 : 0;

		            // use default EDCA parameter
				    // set AC_BE value
			        pEdcaParm->bACM[QID_AC_BE]  = 0;
		            pEdcaParm->Aifsn[QID_AC_BE] = 3;
				    pEdcaParm->Cwmin[QID_AC_BE] = CW_MIN_IN_BITS;
				    pEdcaParm->Cwmax[QID_AC_BE] = CW_MAX_IN_BITS;
				    pEdcaParm->Txop[QID_AC_BE]  = 0;

    			    // set AC_BK value
    			    pEdcaParm->bACM[QID_AC_BK]  = 0;
		            pEdcaParm->Aifsn[QID_AC_BK] = 7;
					pEdcaParm->Cwmin[QID_AC_BK] = CW_MIN_IN_BITS;
					pEdcaParm->Cwmax[QID_AC_BK] = CW_MAX_IN_BITS;
					pEdcaParm->Txop[QID_AC_BK]  = 0;

				    // set AC_VI value
				    pEdcaParm->bACM[QID_AC_VI]  = 0;
				    pEdcaParm->Aifsn[QID_AC_VI] = 2;
					pEdcaParm->Cwmin[QID_AC_VI] = CW_MIN_IN_BITS-1;
					pEdcaParm->Cwmax[QID_AC_VI] = CW_MAX_IN_BITS;
					pEdcaParm->Txop[QID_AC_VI]  = 96;   // AC_VI: 96*32us ~= 3ms

				    // set AC_VO value
				    pEdcaParm->bACM[QID_AC_VO]  = 0;
				    pEdcaParm->Aifsn[QID_AC_VO] = 2;
					pEdcaParm->Cwmin[QID_AC_VO] = CW_MIN_IN_BITS-2;
					pEdcaParm->Cwmax[QID_AC_VO] = CW_MAX_IN_BITS-1;
		            pEdcaParm->Txop[QID_AC_VO]  = 48;   // AC_VO: 48*32us ~= 1.5ms
				}
#if 1	// Let pVIE dedicated to wpa1 and wpa2 ie + wsc_ie
				else	
				{
				    //
				    // Gemtek ask us to pass other vendor's IE for their applications
				    //
				    if ((*LengthVIE + pEid->Len + 2) >= MAX_VIE_LEN)
        			{
	            		DBGPRINT(RT_DEBUG_WARN, "PeerBeaconAndProbeRspSanity - Variable IEs out of resource [len(=%d) > MAX_VIE_LEN(=%d)]\n",
                    		    (*LengthVIE + pEid->Len + 2), MAX_VIE_LEN);
            			break;
        			}				
					
					Ptr = (PUCHAR) pVIE;
					NdisMoveMemory(Ptr + *LengthVIE, &pEid->Eid, pEid->Len + 2);
					*LengthVIE += (pEid->Len + 2);
				}
#endif				
			
				DBGPRINT(RT_DEBUG_INFO, "PeerBeaconAndProbeRspSanity - Receive IE_WPA\n");
				break;

			case IE_EXT_SUPP_RATES:
                if (pEid->Len <= MAX_LEN_OF_SUPPORTED_RATES)
                {
					NdisMoveMemory(ExtRate, pEid->Octet, pEid->Len);
					*pExtRateLen = pEid->Len;

					// TODO: 2004-09-14 not a good design here, cause it exclude extra rates 
					// from ScanTab. We should report as is. And filter out unsupported
					// rates in MlmeAux.
					// Check against the supported rates
					// RTMPCheckRates(pAd, ExtRate, pExtRateLen);
                }
			    break;

            case IE_ERP:
                if (pEid->Len == 1)
                {
                    *pErp = (UCHAR)pEid->Octet[0];
                }
                break;
						
			// WPA2 & 802.11i RSN
			case IE_RSN:
				// There is no OUI for version anymore, check the group cipher OUI before copying
				if (RTMPEqualMemory(pEid->Octet + 2, RSN_OUI, 3))
				{
					if ((*LengthVIE + pEid->Len + 2) >= MAX_VIE_LEN)
        			{
            			DBGPRINT(RT_DEBUG_WARN, "PeerBeaconAndProbeRspSanity - Variable IEs out of resource [len(=%d) > MAX_VIE_LEN(=%d)]\n",
                    		    (*LengthVIE + pEid->Len + 2), MAX_VIE_LEN);
            			break;
        			}
			
					// Copy to pVIE which will report to microsoft bssid list.
					Ptr = (PUCHAR) pVIE;
					NdisMoveMemory(Ptr + *LengthVIE, &pEid->Eid, pEid->Len + 2);
					*LengthVIE += (pEid->Len + 2);
				}
				DBGPRINT(RT_DEBUG_INFO, "IE_RSN length = %d\n", pEid->Len);
				break;				
#if 0
            case IE_QBSS_LOAD:
                if (pEid->Len == 5)
                {
                    pQbssLoad->bValid = TRUE;
                    pQbssLoad->StaNum = pEid->Octet[0] + pEid->Octet[1] * 256;
                    pQbssLoad->ChannelUtilization = pEid->Octet[2];
                    pQbssLoad->RemainingAdmissionControl = pEid->Octet[3] + pEid->Octet[4] * 256;
                }
                break;
                
            case IE_EDCA_PARAMETER:
                if (pEid->Len == 18)
                {
                    PUCHAR ptr;
                    int i;
                    pEdcaParm->bValid          = TRUE;
                    pEdcaParm->bQAck           = pEid->Octet[0] & 0x10;
                    pEdcaParm->bQueueRequest   = pEid->Octet[0] & 0x20;
                    pEdcaParm->bTxopRequest    = pEid->Octet[0] & 0x40;
//                  pEdcaParm->bMoreDataAck    = pEid->Octet[0] & 0x80;
                    pEdcaParm->EdcaUpdateCount = pEid->Octet[0] & 0x0f;
                    ptr = &pEid->Octet[2];
                    for (i=0; i<4; i++)
                    {
                        UCHAR aci = (*ptr & 0x60) >> 5; // b5~6 is AC INDEX
                        pEdcaParm->bACM[aci]  = (((*ptr) & 0x10) == 0x10);   // b5 is ACM
                        pEdcaParm->Aifsn[aci] = (*ptr) & 0x0f;               // b0~3 is AIFSN
                        pEdcaParm->Cwmin[aci] = *(ptr+1) & 0x0f;             // b0~4 is Cwmin
                        pEdcaParm->Cwmax[aci] = *(ptr+1) >> 4;               // b5~8 is Cwmax
                        pEdcaParm->Txop[aci]  = *(ptr+2) + 256 * (*(ptr+3)); // in unit of 32-us
                        ptr += 4; // point to next AC
                    }
                }
                break;

            case IE_QOS_CAPABILITY:
                // this IE contains redundant information as stated in EDCA_IE
                if (pEid->Len == 1)
                {
                    pQosCapability->bValid          = TRUE;
                    pQosCapability->bQAck           = pEid->Octet[0] & 0x01;
                    pQosCapability->bQueueRequest   = pEid->Octet[0] & 0x02;
                    pQosCapability->bTxopRequest    = pEid->Octet[0] & 0x04;
//                  pQosCapability->bMoreDataAck    = pEid->Octet[0] & 0x08;
                    pQosCapability->EdcaUpdateCount = pEid->Octet[0] >> 4;
                }
                break;
#endif                
            default:
                break;
        }
        
        pEid = (PEID_STRUCT)((UCHAR*)pEid + 2 + pEid->Len);        
    }

	// For some 11a AP. it did not have the channel EID, patch here
	if ((pAd->LatchRfRegs.Channel > 14) && ((Sanity & 0x04) == 0))
	{
		*pChannel = pAd->LatchRfRegs.Channel;				 
		Sanity |= 0x4;		
	}
	
    if (Sanity != 0x7)
    {
        DBGPRINT(RT_DEBUG_WARN, "PeerBeaconAndProbeRspSanity - missing field, Sanity=0x%02x\n", Sanity);
        return FALSE;
    }
    else
    {
        return TRUE;
    }

}

/* 
    ==========================================================================
    Description:
        
    ==========================================================================
 */
BOOLEAN GetTimBit(
    IN CHAR *Ptr, 
    IN USHORT Aid, 
    OUT UCHAR *TimLen, 
    OUT UCHAR *BcastFlag, 
    OUT UCHAR *DtimCount, 
    OUT UCHAR *DtimPeriod,
    OUT UCHAR *MessageToMe) 
{
    UCHAR          BitCntl, N1, N2, MyByte, MyBit;
    CHAR          *IdxPtr;

    IdxPtr = Ptr;
    
    IdxPtr ++;
    *TimLen = *IdxPtr;
    
    // get DTIM Count from TIM element
    IdxPtr ++;
    *DtimCount = *IdxPtr;
    
    // get DTIM Period from TIM element
    IdxPtr++;
    *DtimPeriod = *IdxPtr;
        
    // get Bitmap Control from TIM element
    IdxPtr++;
    BitCntl = *IdxPtr;

    if ((*DtimCount == 0) && (BitCntl & 0x01)) 
        *BcastFlag = TRUE;
    else 
        *BcastFlag = FALSE;
    
    // Parse Partial Virtual Bitmap from TIM element
    N1 = BitCntl & 0xfe;    // N1 is the first bitmap byte#
    N2 = *TimLen - 4 + N1;  // N2 is the last bitmap byte#
    
    if ((Aid < (N1 << 3)) || (Aid >= ((N2 + 1) << 3)))
        *MessageToMe = FALSE;
    else
    {
        MyByte = (Aid >> 3) - N1;                       // my byte position in the bitmap byte-stream
        MyBit = Aid % 16 - ((MyByte & 0x01)? 8:0);

        IdxPtr += (MyByte + 1);

        //if (*IdxPtr)
        //    DBGPRINT(RT_DEBUG_WARN, "TIM bitmap = 0x%02x\n", *IdxPtr);
            
        if (*IdxPtr & (0x01 << MyBit))
            *MessageToMe = TRUE;
        else 
            *MessageToMe = FALSE;
    }

    return TRUE;
}

UCHAR ChannelSanity(
    IN PRTMP_ADAPTER pAd, 
    IN UCHAR channel)
{
    int i;

    for (i = 0; i < pAd->ChannelListNum; i ++)
    {
        if (channel == pAd->ChannelList[i].Channel)
            return 1;
    }
    return 0;
}

/*
	========================================================================
	Routine Description:
		Sanity check NetworkType (11b, 11g or 11a)
		
	Arguments:
		pBss - Pointer to BSS table.

	Return Value:
        Ndis802_11DS .......(11b)
        Ndis802_11OFDM24....(11g)
        Ndis802_11OFDM5.....(11a)
        	
	========================================================================
*/
NDIS_802_11_NETWORK_TYPE NetworkTypeInUseSanity(
    IN PBSS_ENTRY pBss)
{
	NDIS_802_11_NETWORK_TYPE	NetWorkType;
	UCHAR						rate, i;

	NetWorkType = Ndis802_11DS; // default
	
	if (pBss->Channel <= 14)
	{
		//
		// First check support Rate.
		//
		for (i = 0; i < pBss->SupRateLen; i++)
		{
			rate = pBss->SupRate[i] & 0x7f; // Mask out basic rate set bit
			if ((rate == 2) || (rate == 4) || (rate == 11) || (rate == 22))
			{
				continue;
			}
			else
			{
				//
				// Otherwise (even rate > 108) means Ndis802_11OFDM24
				//
				NetWorkType = Ndis802_11OFDM24;
				break;
			}	
		}

		//
		// Second check Extend Rate.
		//
		if (NetWorkType != Ndis802_11OFDM24)
		{
			for (i = 0; i < pBss->ExtRateLen; i++)
			{
				rate = pBss->SupRate[i] & 0x7f; // Mask out basic rate set bit
				if ((rate == 2) || (rate == 4) || (rate == 11) || (rate == 22))
				{
					continue;
				}
				else
				{
					//
					// Otherwise (even rate > 108) means Ndis802_11OFDM24
					//
					NetWorkType = Ndis802_11OFDM24;
					break;
				}
			}
		}
	}
	else
	{
		NetWorkType = Ndis802_11OFDM5;
	}

	return NetWorkType;
}	

/*
	========================================================================
	Routine Description:
		Sanity check pairwise key on Encryption::Ndis802_11Encryption1Enabled
		
	Arguments:
		pAdapter - Pointer to our adapter
		pBuf 	 - Pointer to NDIS_802_11_KEY structure

	Return Value:
		NDIS_STATUS_SUCCESS
		NDIS_STATUS_FAILURE
        
	Note:
		For OID_802_11_ADD_KEY setting, on old wep stuff also need to verify
		the structure of NIDS_802_11_KEY
	========================================================================
*/
NDIS_STATUS	RTMPWPAWepKeySanity(
	IN	PRTMP_ADAPTER	pAd,
	IN	PVOID			pBuf)
{
	PNDIS_802_11_KEY	pKey;
	ULONG				KeyIdx;
	NDIS_STATUS			Status = NDIS_STATUS_SUCCESS;
	BOOLEAN 			bTxKey; 		// Set the key as transmit key
	BOOLEAN 			bPairwise;		// Indicate the key is pairwise key
    UCHAR               CipherAlg;
    UINT                i;

	pKey = (PNDIS_802_11_KEY) pBuf;
	KeyIdx = pKey->KeyIndex & 0x0fffffff;
	// Bit 31 of Add-key, Tx Key
	bTxKey		   = (pKey->KeyIndex & 0x80000000) ? TRUE : FALSE;
	// Bit 30 of Add-key PairwiseKey
	bPairwise	   = (pKey->KeyIndex & 0x40000000) ? TRUE : FALSE;

	// 1. Check Group / Pairwise Key
	if (bPairwise)	// Pairwise Key
	{
		// 1. Check KeyIdx 
		// it is a shared key
		if (KeyIdx > 4)
			return (NDIS_STATUS_FAILURE);
			
		// 2. Check bTx, it must be true, otherwise, return NDIS_STATUS_FAILURE
		if (bTxKey == FALSE)
			return(NDIS_STATUS_FAILURE);

		// 3. If BSSID is all 0xff, return NDIS_STATUS_FAILURE
		if (MAC_ADDR_EQUAL(pKey->BSSID, BROADCAST_ADDR))
			return(NDIS_STATUS_FAILURE);

		// check key length
		if ((pKey->KeyLength != 5) && (pKey->KeyLength != 13))
		    return(NDIS_STATUS_FAILURE);

	}
	else
	{
		// Group Key
		// 1. Check BSSID, if not current BSSID or Bcast, return NDIS_STATUS_FAILURE
		if ((! MAC_ADDR_EQUAL(pKey->BSSID, BROADCAST_ADDR)) &&
			(! MAC_ADDR_EQUAL(pKey->BSSID, pAd->PortCfg.Bssid)))
			return(NDIS_STATUS_FAILURE);

		// 2. Check Key index for supported Group Key
		if (KeyIdx > 4)
			return(NDIS_STATUS_FAILURE);
	
	}
     
	pAd->SharedKey[KeyIdx].KeyLen = (UCHAR) pKey->KeyLength;
	NdisMoveMemory(pAd->SharedKey[KeyIdx].Key, &pKey->KeyMaterial, pKey->KeyLength);

	if (pKey->KeyLength == 5)
		CipherAlg = CIPHER_WEP64;
	else
		CipherAlg = CIPHER_WEP128;
    
	// always expand the KEY to 16-byte here for efficiency sake. so that in case CKIP is used
	// sometime later we don't have to do key expansion for each TX in RTMPHardTransmit().
	// However, we shouldn't change pAd->SharedKey[KeyIdx].KeyLen
	if (pKey->KeyLength < 16)
	{
		for(i = 1; i < (16 / pKey->KeyLength); i++)
		{
			NdisMoveMemory(&pAd->SharedKey[KeyIdx].Key[i * pKey->KeyLength], 
						&pKey->KeyMaterial[0], 
						pKey->KeyLength);
		}
		NdisMoveMemory(&pAd->SharedKey[KeyIdx].Key[i * pKey->KeyLength], 
						&pKey->KeyMaterial[0], 
						16 - (i * pKey->KeyLength));
	}

	pAd->SharedKey[KeyIdx].CipherAlg = CipherAlg;
	if (pKey->KeyIndex & 0x80000000)
	{
		// Default key for tx (shared key)
		pAd->PortCfg.DefaultKeyId = (UCHAR) KeyIdx;
	}
    //always use BSS0=0
	AsicAddSharedKeyEntry(pAd, 0, (UCHAR)KeyIdx, CipherAlg, pAd->SharedKey[KeyIdx].Key, NULL, NULL);


	return (Status);
}

/* 
    ==========================================================================
    Description:
        MLME message sanity check to get config data from AP
    Return:
        TRUE if all parameters are OK, FALSE otherwise
        
    ==========================================================================
 */
BOOLEAN BackDoorProbeRspSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen,
    OUT CHAR *pCfgDataBuf)
{
//	PFRAME_802_11       pFrame = (PFRAME_802_11)Msg;  // remove by johnli, fix WPS & WPAPSK/WPA2PSK bugs for receiving EAPoL fragmentation packets
    CHAR                *Ptr, CfgData[255] = {0};
    PEID_STRUCT         eid_ptr;
    USHORT              cfgDataLen = 0;

    // edit by johnli, fix WPS & WPAPSK/WPA2PSK bugs for receiving EAPoL fragmentation packets
//    Ptr = pFrame->Octet;
    Ptr = (CHAR *)Msg;
    // end johnli
    
    // timestamp from payload and advance the pointer
    Ptr += TIMESTAMP_LEN;

    // beacon interval from payload and advance the pointer
    Ptr += 2;

    // capability info from payload and advance the pointer
    Ptr += 2;

    eid_ptr = (PEID_STRUCT) Ptr;

    // get variable fields from payload and advance the pointer
    // edit by johnli, fix WPS & WPAPSK/WPA2PSK bugs for receiving EAPoL fragmentation packets
//    while(((UCHAR*)eid_ptr + eid_ptr->Len + 1) < ((UCHAR*)pFrame + MsgLen))
    while(((UCHAR*)eid_ptr + eid_ptr->Len + 1) < ((UCHAR*)Msg + MsgLen))
    // end johnli
    {
        memset(CfgData, 0, 255);
        switch(eid_ptr->Eid)
        {
	        case IE_VENDOR_SPECIFIC:
                if (NdisEqualMemory(eid_ptr->Octet, RALINK_OUI, 3))
                {
                    if ((eid_ptr->Octet[3] & 0x80) == 0x80) 
                    {
                        if ( (cfgDataLen + eid_ptr->Len - 4) <= MAX_CFG_BUFFER_LEN) 
                        {
                            //NdisMoveMemory((pCfgDataBuf + cfgDataLen), (eid_ptr->Octet + 4), (eid_ptr->Len - 4));
                            NdisMoveMemory(CfgData, (eid_ptr->Octet + 4), (eid_ptr->Len - 4));
                            printk("%s\n", CfgData);
                            return TRUE;
                        }
                        else 
                        {
                            printk("BackDoorProbeRspSanity: cfgDataLen > MAX_CFG_BUFFER_LEN\n");
                            return FALSE;
                        }
                    }
                    else if ((eid_ptr->Octet[3] & 0x40) == 0x40) 
                    {
                        //NdisMoveMemory((pCfgDataBuf + cfgDataLen), (eid_ptr->Octet + 4), (eid_ptr->Len - 4));
                        cfgDataLen += (eid_ptr->Len - 4);
                        NdisMoveMemory(CfgData, (eid_ptr->Octet + 4), (eid_ptr->Len - 4));
                        if (cfgDataLen > MAX_CFG_BUFFER_LEN) 
                        {
                            printk("BackDoorProbeRspSanity: cfgDataLen > MAX_CFG_BUFFER_LEN\n");
                            return FALSE;
                        }
                        else
                            printk("%s", CfgData);
                    }
                    break;
                }            
            default:
                break;
        }
        eid_ptr = (PEID_STRUCT)((UCHAR*)eid_ptr + 2 + eid_ptr->Len);
    }

    return FALSE;
}

/*
	========================================================================
	Routine Description:
		Get Peer TX phy mode(CCK or OFDM)
	Arguments:
		Channel				Current Channel
		SupRate				Peer's Supported Rate Buffer
		SupRateLen			Peer's Supported Rate Length
		ExtRate				Peer's Extended Rate Buffer
		ExtRateLen			Peer's Extended Rate Length
		
	Return Value:
    	1 - CCK
    	2 - OFDM
    	3 - CCK+OFDM
	========================================================================
*/
UCHAR PeerTxTypeInUseSanity(
	IN UCHAR  Channel,
	IN UCHAR  SupRate[],
	IN UCHAR  SupRateLen,
	IN UCHAR  ExtRate[],
	IN UCHAR  ExtRateLen)
{
	UCHAR						rate, i;
	UCHAR						Type = 0;

	if (Channel <= 14)
	{
		//
		// First check support Rate.
		//
		for (i = 0; i < SupRateLen; i++)
		{
			rate = SupRate[i] & 0x7f; // Mask out basic rate set bit
			if ((rate == 2) || (rate == 4) || (rate == 11) || (rate == 22))
			{
				Type |= 0x01;
				continue;
			}
			else
			{
				//
				// Otherwise (even rate > 108) means Ndis802_11OFDM24
				//
				Type |= 0x02;
				break;
			}	
		}

		//
		// Second check Extend Rate.
		// Maybe OFDM rate store on Extend Rate. 
		//
		if ((Type & 0x02) == 0)
		{
			for (i = 0; i < ExtRateLen; i++)
			{
				rate = ExtRate[i] & 0x7f; // Mask out basic rate set bit
				if ((rate == 2) || (rate == 4) || (rate == 11) || (rate == 22))
				{
					continue;
				}
				else
				{
					//
					// Otherwise (even rate > 108) means Ndis802_11OFDM24
					//
					Type |= 0x02;
					break;
				}
			}
		}
	}
	else
	{
		Type |= 0x02;
	}

	return Type;
}	

