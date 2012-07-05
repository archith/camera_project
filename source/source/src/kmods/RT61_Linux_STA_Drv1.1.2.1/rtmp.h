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
    rtmp.h

    Abstract:
    Miniport generic portion header file

    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------
    Paul Lin    2002-08-01    created
	James Tan   2002-09-06    modified (Revise NTCRegTable)
    John Chang  2004-09-06    modified for RT2600     
*/

#ifndef __RTMP_H__
#define __RTMP_H__

#include "mlme.h"
#include "oid.h"
#include "wpa.h"


//
// Extern
//
extern UCHAR BROADCAST_ADDR[MAC_ADDR_LEN];
extern UCHAR ZERO_MAC_ADDR[MAC_ADDR_LEN];
extern ULONG BIT32[32];
extern UCHAR BIT8[8];
extern char* CipherName[];

extern UCHAR SNAP_802_1H[6];
extern UCHAR SNAP_BRIDGE_TUNNEL[6];
extern UCHAR EAPOL_LLC_SNAP[8];
extern UCHAR EAPOL[2];
extern UCHAR IPX[2];
extern UCHAR APPLE_TALK[2];
extern UCHAR OfdmSignalToRateId[16] ;
extern UCHAR default_cwmin[4];
extern UCHAR default_cwmax[4];
extern UCHAR default_sta_aifsn[4];
extern UCHAR MapUserPriorityToAccessCategory[8];


extern UCHAR  Phy11BNextRateDownward[];
extern UCHAR  Phy11BNextRateUpward[];
extern UCHAR  Phy11BGNextRateDownward[];
extern UCHAR  Phy11BGNextRateUpward[];
extern UCHAR  Phy11ANextRateDownward[];
extern UCHAR  Phy11ANextRateUpward[];
extern CHAR   RssiSafeLevelForTxRate[];
extern UCHAR  RateIdToMbps[];
extern USHORT RateIdTo500Kbps[];

extern UCHAR  CipherSuiteWpaNoneTkip[];
extern UCHAR  CipherSuiteWpaNoneTkipLen;

extern UCHAR  CipherSuiteWpaNoneAes[];
extern UCHAR  CipherSuiteWpaNoneAesLen;

extern UCHAR  SsidIe;
extern UCHAR  SupRateIe;
extern UCHAR  ExtRateIe;
extern UCHAR  ErpIe;
extern UCHAR  DsIe;
extern UCHAR  TimIe;
extern UCHAR  WpaIe;
extern UCHAR  Wpa2Ie;
extern UCHAR  IbssIe;

extern UCHAR  WPA_OUI[];
extern UCHAR  RSN_OUI[];
extern UCHAR  WME_INFO_ELEM[];
extern UCHAR  WME_PARM_ELEM[];
extern UCHAR  RALINK_OUI[];

//---> Add by Zero:Jul.04.2007
typedef struct net_device	* PNET_DEV;
typedef void			* PNDIS_PACKET;
typedef char			NDIS_PACKET;
typedef PNDIS_PACKET		* PPNDIS_PACKET;
typedef char 			* PNDIS_BUFFER;
typedef spinlock_t		NDIS_SPIN_LOCK;

/*
 * packet helper 
 * 	- convert internal rt packet to os packet or 
 *             os packet to rt packet
 */      
#define RTPKT_TO_OSPKT(_p)		((struct sk_buff *)(_p))
#define OSPKT_TO_RTPKT(_p)		((PNDIS_PACKET)(_p))

#define GET_OS_PKT_DATAPTR(_pkt) \
		(RTPKT_TO_OSPKT(_pkt)->data)

#define GET_OS_PKT_LEN(_pkt) \
		(RTPKT_TO_OSPKT(_pkt)->len)

#define GET_OS_PKT_TYPE(_pkt) \
		(RTPKT_TO_OSPKT(_pkt))

#define GET_OS_PKT_NEXT(_pkt) \
		(RTPKT_TO_OSPKT(_pkt)->next)

#define OS_NTOHS(_Val) \
		(ntohs(_Val))
#define OS_HTONS(_Val) \
		(htons(_Val))
#define OS_NTOHL(_Val) \
		(ntohl(_Val))
#define OS_HTONL(_Val) \
		(htonl(_Val))
//<---

//
//  Queue structure and macros
//
typedef struct  _QUEUE_ENTRY    {
    struct _QUEUE_ENTRY     *Next;
}   QUEUE_ENTRY, *PQUEUE_ENTRY;

// Queue structure
typedef struct  _QUEUE_HEADER   {
    PQUEUE_ENTRY    Head;
    PQUEUE_ENTRY    Tail;
    ULONG           Number;
}   QUEUE_HEADER, *PQUEUE_HEADER;

#define InitializeQueueHeader(QueueHeader)              \
{                                                       \
    (QueueHeader)->Head = (QueueHeader)->Tail = NULL;   \
    (QueueHeader)->Number = 0;                          \
}

#define RemoveHeadQueue(QueueHeader)                \
(QueueHeader)->Head;                                \
{                                                   \
    PQUEUE_ENTRY pNext;                             \
    if ((QueueHeader)->Head != NULL)				\
    {												\
        pNext = (QueueHeader)->Head->Next;              \
        (QueueHeader)->Head = pNext;                    \
        if (pNext == NULL)                              \
            (QueueHeader)->Tail = NULL;                 \
        (QueueHeader)->Number--;                        \
    }						                        \
}

#define InsertHeadQueue(QueueHeader, QueueEntry)            \
{                                                           \
    ((PQUEUE_ENTRY)QueueEntry)->Next = (QueueHeader)->Head; \
    (QueueHeader)->Head = (PQUEUE_ENTRY)(QueueEntry);       \
    if ((QueueHeader)->Tail == NULL)						\
        (QueueHeader)->Tail = (PQUEUE_ENTRY)(QueueEntry);	\
	(QueueHeader)->Number++;                                \
}

#define InsertTailQueue(QueueHeader, QueueEntry)                \
{                                                               \
    ((PQUEUE_ENTRY)QueueEntry)->Next = NULL;                    \
    if ((QueueHeader)->Tail)                                    \
        (QueueHeader)->Tail->Next = (PQUEUE_ENTRY)(QueueEntry); \
    else                                                        \
        (QueueHeader)->Head = (PQUEUE_ENTRY)(QueueEntry);       \
    (QueueHeader)->Tail = (PQUEUE_ENTRY)(QueueEntry);           \
    (QueueHeader)->Number++;                                    \
}

//
//  MACRO for debugging information
//
#ifdef DBG
extern ULONG    RTDebugLevel;

#define DBGPRINT(Level, fmt, args...)                   \
{                                                       \
    if (Level <= RTDebugLevel)                                     \
    {                                                   \
        printk(NIC_DBG_STRING);                         \
        printk(KERN_DEBUG fmt, ## args);                \
    }                                                   \
}

#define DBGPRINT_ERR(fmt, args...)  \
{                                   \
    printk("ERROR!!! ");			\
    printk(KERN_DEBUG fmt, ## args);\
}

#define DBGPRINT_RAW(Level, fmt, args...)           \
{									                \
	if (Level <= RTDebugLevel)      				\
	{								                \
		printk(" ");					            \
		printk(KERN_DEBUG fmt, ## args);			\
	}								                \
}
#else
#define DBGPRINT(Level, fmt, args...)
#define DBGPRINT_ERR(fmt, args...)
#define DBGPRINT_RAW(Level, fmt, args...)
#endif


//
//  spin_lock enhanced for Nested spin lock
//
#define NdisAllocateSpinLock(lock)      \
{                                       \
    spin_lock_init(lock);               \
}

#define NdisFreeSpinLock(lock)          \
{                                       \
}

#define RTMPFreeSkbBuffer(skb)          \
{                                       \
    dev_kfree_skb_any(skb);             \
}

//  Assert MACRO to make sure program running
//
#undef  ASSERT
#define ASSERT(x)                                                               \
{                                                                               \
    if (!(x))                                                                   \
    {                                                                           \
        printk(KERN_WARNING __FILE__ ":%d assert " #x "failed\n", __LINE__);    \
    }                                                                           \
}

//
//  Macros for flag and ref count operations
//
#define RTMP_SET_FLAG(_M, _F)       ((_M)->Flags |= (_F))
#define RTMP_CLEAR_FLAG(_M, _F)     ((_M)->Flags &= ~(_F))
#define RTMP_CLEAR_FLAGS(_M)        ((_M)->Flags = 0)
#define RTMP_TEST_FLAG(_M, _F)      (((_M)->Flags & (_F)) != 0)
#define RTMP_TEST_FLAGS(_M, _F)     (((_M)->Flags & (_F)) == (_F))

#define OPSTATUS_SET_FLAG(_pAd, _F)     ((_pAd)->PortCfg.OpStatusFlags |= (_F))
#define OPSTATUS_CLEAR_FLAG(_pAd, _F)   ((_pAd)->PortCfg.OpStatusFlags &= ~(_F))
#define OPSTATUS_TEST_FLAG(_pAd, _F)    (((_pAd)->PortCfg.OpStatusFlags & (_F)) != 0)

#define CLIENT_STATUS_SET_FLAG(_pEntry,_F)      ((_pEntry)->ClientStatusFlags |= (_F))      
#define CLIENT_STATUS_CLEAR_FLAG(_pEntry,_F)    ((_pEntry)->ClientStatusFlags &= ~(_F))
#define CLIENT_STATUS_TEST_FLAG(_pEntry,_F)     (((_pEntry)->ClientStatusFlags & (_F)) != 0)

#define INC_RING_INDEX(_idx, _RingSize)    \
{                                          \
    (_idx)++;                              \
    if ((_idx) >= (_RingSize)) _idx=0;     \
}

// Increase TxTsc value for next transmission
// TODO: 
// When i==6, means TSC has done one full cycle, do re-keying stuff follow specs
// Should send a special event microsoft defined to request re-key
#define INC_TX_TSC(_tsc)                                \
{                                                       \
    INT i=0;                                            \
	while (++_tsc[i] == 0x0)                            \
    {                                                   \
        i++;                                            \
		if (i == 6)                                     \
			break;                                      \
	}                                                   \
}

#undef	NdisEqualMemory
#undef	RTMPMoveMemory
#define NdisEqualMemory(Source1, Source2, Length)   RTMPEqualMemory(Source1, Source2, Length)
#define RTMPMoveMemory(Destination, Source, Length) NdisMoveMemory(Destination, Source, Length)

//
// MACRO for 32-bit PCI register read / write
#if 0
#ifdef RTMP_EMBEDDED
#define RTMP_IO_READ32(_A, _R, _pV)     (*_pV = PCIMemRead32(__mem_pci((_A)->CSRBaseAddress + (_R))))
#define RTMP_IO_WRITE32(_A, _R, _V)     (PCIMemWrite32(__mem_pci((_A)->CSRBaseAddress + (_R)), _V))
#define RTMP_IO_WRITE8(_A, _R, _V)		(PCIMemWrite8((PUCHAR)__mem_pci((_A)->CSRBaseAddress + (_R)), (_V)))
#define RTMP_IO_WRITE16(_A, _R, _V)		(PCIMemWrite16((PUSHORT)__mem_pci((_A)->CSRBaseAddress + (_R)), (_V)))

#else
#define RTMP_IO_READ32(_A, _R, _pV)		(*_pV = readl((void *)((_A)->CSRBaseAddress + (_R))))
#define RTMP_IO_WRITE32(_A, _R, _V)		(writel(_V, (void *) ((_A)->CSRBaseAddress + (_R))))
#define RTMP_IO_WRITE8(_A, _R, _V)		(writeb((_V), (PUCHAR)((_A)->CSRBaseAddress + (_R))))
#define RTMP_IO_WRITE16(_A, _R, _V)		(writew((_V), (PUSHORT)((_A)->CSRBaseAddress + (_R))))
#endif
#else
//Patch for ASIC turst read/write bug, needs to remove after metel fix
#define RTMP_IO_READ32(_A, _R, _pV)								\
{																\
	(*_pV = readl((void *)((_A)->CSRBaseAddress + MAC_CSR0)));			\
	(*_pV = readl((void *)((_A)->CSRBaseAddress + (_R))));			    \
}
#define RTMP_IO_READ8(_A, _R, _pV)								\
{																\
	(*_pV = readl((void *)((_A)->CSRBaseAddress + MAC_CSR0)));			\
	(*_pV = readb((void *)((_A)->CSRBaseAddress + (_R))));				\
}
#define RTMP_IO_WRITE32(_A, _R, _V)							    \
{															    \
	ULONG	Val;											    \
	Val = readl((void *)((_A)->CSRBaseAddress + MAC_CSR0));			    \
	writel(_V, (void *)((_A)->CSRBaseAddress + (_R)));				    \
}
#define RTMP_IO_WRITE8(_A, _R, _V)							    \
{															    \
	ULONG	Val;											    \
	Val = readl((void *)((_A)->CSRBaseAddress + MAC_CSR0));			    \
	writeb((_V), (PUCHAR)((_A)->CSRBaseAddress + (_R)));		\
}
#define RTMP_IO_WRITE16(_A, _R, _V)						        \
{															    \
	ULONG	Val;											    \
	Val = readl((void *)((_A)->CSRBaseAddress + MAC_CSR0));			    \
	writew((_V), (PUSHORT)((_A)->CSRBaseAddress + (_R)));	    \
}
#endif

//
// BBP & RF are using indirect access. Before write any value into it.
// We have to make sure there is no outstanding command pending via checking busy bit.
//
#define	MAX_BUSY_COUNT	100			// Number of retry before failing access BBP & RF indirect register
//
#define	RTMP_RF_IO_WRITE32(_A, _V)				    \
{												    \
	PHY_CSR4_STRUC	Value;						    \
	ULONG			BusyCnt = 0;				    \
	do {										    \
		RTMP_IO_READ32(_A, PHY_CSR4, &Value.word);  \
		if (Value.field.Busy == IDLE)		        \
			break;								    \
		BusyCnt++;								    \
	}	while (BusyCnt < MAX_BUSY_COUNT);		    \
	if (BusyCnt < MAX_BUSY_COUNT)				    \
	{											    \
		RTMP_IO_WRITE32(_A, PHY_CSR4, _V);			\
	}											    \
}

// Read BBP register by register's ID
#define	RTMP_BBP_IO_READ8_BY_REG_ID(_A, _I, _pV)	    \
{												        \
	PHY_CSR3_STRUC	BbpCsr;                             \
	INT             i, k;                               \
	for (i=0; i<MAX_BUSY_COUNT; i++)                    \
	{                                                   \
        RTMP_IO_READ32(_A, PHY_CSR3, &BbpCsr.word);     \
        if (BbpCsr.field.Busy == BUSY)                  \
        {                                               \
            continue;                                   \
        }                                               \
    	BbpCsr.word = 0;							    \
    	BbpCsr.field.fRead = 1;		                    \
    	BbpCsr.field.Busy = 1;					        \
    	BbpCsr.field.RegNum = _I;				        \
    	RTMP_IO_WRITE32(_A, PHY_CSR3, BbpCsr.word);     \
    	for (k=0; k<MAX_BUSY_COUNT; k++)                \
    	{                                               \
            RTMP_IO_READ32(_A, PHY_CSR3, &BbpCsr.word); \
            if (BbpCsr.field.Busy == IDLE)              \
                break;                                  \
    	}                                               \
    	if ((BbpCsr.field.Busy == IDLE) &&              \
            (BbpCsr.field.RegNum == _I))                \
    	{                                               \
            *(_pV) = (UCHAR)BbpCsr.field.Value;         \
        	break;                                      \
    	}                                               \
	}                                                   \
	if (BbpCsr.field.Busy == BUSY)                      \
	{                                                   \
        DBGPRINT_ERR("BBP read R%d fail\n", _I);        \
        *(_pV) = (_A)->BbpWriteLatch[_I];               \
	}                                                   \
}

// Write BBP register by register's ID & value
#define	RTMP_BBP_IO_WRITE8_BY_REG_ID(_A, _I, _V)	    \
{												        \
	PHY_CSR3_STRUC	BbpCsr;						        \
	INT             BusyCnt;                            \
    for (BusyCnt=0; BusyCnt<MAX_BUSY_COUNT; BusyCnt++)  \
    {                                                   \
        RTMP_IO_READ32(_A, PHY_CSR3, &BbpCsr.word);     \
        if (BbpCsr.field.Busy == BUSY)                  \
            continue;                                   \
        BbpCsr.word = 0;                                \
        BbpCsr.field.fRead = 0;                         \
        BbpCsr.field.Busy = 1;                          \
        BbpCsr.field.Value = _V;                        \
        BbpCsr.field.RegNum = _I;                       \
        RTMP_IO_WRITE32(_A, PHY_CSR3, BbpCsr.word);     \
        (_A)->BbpWriteLatch[_I] = _V;                   \
        break;                                          \
    }                                                   \
}

#define     MAP_CHANNEL_ID_TO_KHZ(ch, khz)  {               \
                switch (ch)                                 \
                {                                           \
                    case 1:     khz = 2412000;   break;     \
                    case 2:     khz = 2417000;   break;     \
                    case 3:     khz = 2422000;   break;     \
                    case 4:     khz = 2427000;   break;     \
                    case 5:     khz = 2432000;   break;     \
                    case 6:     khz = 2437000;   break;     \
                    case 7:     khz = 2442000;   break;     \
                    case 8:     khz = 2447000;   break;     \
                    case 9:     khz = 2452000;   break;     \
                    case 10:    khz = 2457000;   break;     \
                    case 11:    khz = 2462000;   break;     \
                    case 12:    khz = 2467000;   break;     \
                    case 13:    khz = 2472000;   break;     \
                    case 14:    khz = 2484000;   break;     \
                    case 36:  /* UNII */  khz = 5180000;   break;     \
                    case 40:  /* UNII */  khz = 5200000;   break;     \
                    case 44:  /* UNII */  khz = 5220000;   break;     \
                    case 48:  /* UNII */  khz = 5240000;   break;     \
                    case 52:  /* UNII */  khz = 5260000;   break;     \
                    case 56:  /* UNII */  khz = 5280000;   break;     \
                    case 60:  /* UNII */  khz = 5300000;   break;     \
                    case 64:  /* UNII */  khz = 5320000;   break;     \
                    case 149: /* UNII */  khz = 5745000;   break;     \
                    case 153: /* UNII */  khz = 5765000;   break;     \
                    case 157: /* UNII */  khz = 5785000;   break;     \
                    case 161: /* UNII */  khz = 5805000;   break;     \
					case 165: /* UNII */  khz = 5825000;   break;     \
                    case 100: /* HiperLAN2 */  khz = 5500000;   break;     \
                    case 104: /* HiperLAN2 */  khz = 5520000;   break;     \
                    case 108: /* HiperLAN2 */  khz = 5540000;   break;     \
                    case 112: /* HiperLAN2 */  khz = 5560000;   break;     \
                    case 116: /* HiperLAN2 */  khz = 5580000;   break;     \
                    case 120: /* HiperLAN2 */  khz = 5600000;   break;     \
                    case 124: /* HiperLAN2 */  khz = 5620000;   break;     \
                    case 128: /* HiperLAN2 */  khz = 5640000;   break;     \
                    case 132: /* HiperLAN2 */  khz = 5660000;   break;     \
                    case 136: /* HiperLAN2 */  khz = 5680000;   break;     \
                    case 140: /* HiperLAN2 */  khz = 5700000;   break;     \
                    case 34:  /* Japan MMAC */   khz = 5170000;   break;   \
                    case 38:  /* Japan MMAC */   khz = 5190000;   break;   \
                    case 42:  /* Japan MMAC */   khz = 5210000;   break;   \
                    case 46:  /* Japan MMAC */   khz = 5230000;   break;   \
                    default:    khz = 2412000;   break;     \
                }                                           \
            }

#define     MAP_KHZ_TO_CHANNEL_ID(khz, ch)  {               \
                switch (khz)                                \
                {                                           \
                    case 2412000:    ch = 1;     break;     \
                    case 2417000:    ch = 2;     break;     \
                    case 2422000:    ch = 3;     break;     \
                    case 2427000:    ch = 4;     break;     \
                    case 2432000:    ch = 5;     break;     \
                    case 2437000:    ch = 6;     break;     \
                    case 2442000:    ch = 7;     break;     \
                    case 2447000:    ch = 8;     break;     \
                    case 2452000:    ch = 9;     break;     \
                    case 2457000:    ch = 10;    break;     \
                    case 2462000:    ch = 11;    break;     \
                    case 2467000:    ch = 12;    break;     \
                    case 2472000:    ch = 13;    break;     \
                    case 2484000:    ch = 14;    break;     \
                    case 5180000:    ch = 36;  /* UNII */  break;     \
                    case 5200000:    ch = 40;  /* UNII */  break;     \
                    case 5220000:    ch = 44;  /* UNII */  break;     \
                    case 5240000:    ch = 48;  /* UNII */  break;     \
                    case 5260000:    ch = 52;  /* UNII */  break;     \
                    case 5280000:    ch = 56;  /* UNII */  break;     \
                    case 5300000:    ch = 60;  /* UNII */  break;     \
                    case 5320000:    ch = 64;  /* UNII */  break;     \
                    case 5745000:    ch = 149; /* UNII */  break;     \
                    case 5765000:    ch = 153; /* UNII */  break;     \
                    case 5785000:    ch = 157; /* UNII */  break;     \
                    case 5805000:    ch = 161; /* UNII */  break;     \
					case 5825000:    ch = 165; /* UNII */  break;     \
                    case 5500000:    ch = 100; /* HiperLAN2 */  break;     \
                    case 5520000:    ch = 104; /* HiperLAN2 */  break;     \
                    case 5540000:    ch = 108; /* HiperLAN2 */  break;     \
                    case 5560000:    ch = 112; /* HiperLAN2 */  break;     \
                    case 5580000:    ch = 116; /* HiperLAN2 */  break;     \
                    case 5600000:    ch = 120; /* HiperLAN2 */  break;     \
                    case 5620000:    ch = 124; /* HiperLAN2 */  break;     \
                    case 5640000:    ch = 128; /* HiperLAN2 */  break;     \
                    case 5660000:    ch = 132; /* HiperLAN2 */  break;     \
                    case 5680000:    ch = 136; /* HiperLAN2 */  break;     \
                    case 5700000:    ch = 140; /* HiperLAN2 */  break;     \
                    case 5170000:    ch = 34;  /* Japan MMAC */   break;   \
                    case 5190000:    ch = 38;  /* Japan MMAC */   break;   \
                    case 5210000:    ch = 42;  /* Japan MMAC */   break;   \
                    case 5230000:    ch = 46;  /* Japan MMAC */   break;   \
                    default:         ch = 1;     break;     \
                }                                           \
            }


#define NIC_MAX_PHYS_BUF_COUNT              8

typedef struct _RTMP_SCATTER_GATHER_ELEMENT {
    PVOID		Address;
    ULONG		Length;
    PULONG		Reserved;
} RTMP_SCATTER_GATHER_ELEMENT, *PRTMP_SCATTER_GATHER_ELEMENT;

typedef struct _RTMP_SCATTER_GATHER_LIST {
    ULONG  NumberOfElements;
    PULONG Reserved;
    RTMP_SCATTER_GATHER_ELEMENT Elements[NIC_MAX_PHYS_BUF_COUNT];
} RTMP_SCATTER_GATHER_LIST, *PRTMP_SCATTER_GATHER_LIST;

//
//  Some utility macros
//
#ifndef min
#define min(_a, _b)     (((_a) < (_b)) ? (_a) : (_b))
#endif

#ifndef max
#define max(_a, _b)     (((_a) > (_b)) ? (_a) : (_b))
#endif

#define INC_COUNTER(Val)        (Val.QuadPart++)
#define	INC_COUNTER64(Val)	    (Val.QuadPart++)

#define INFRA_ON(_p)            (OPSTATUS_TEST_FLAG(_p, fOP_STATUS_INFRA_ON))
#define ADHOC_ON(_p)            (OPSTATUS_TEST_FLAG(_p, fOP_STATUS_ADHOC_ON))

#define MIN_NET_DEVICE_FOR_AID			0x00		//0x00~0x3f
#define MIN_NET_DEVICE_FOR_MBSSID		0x00		//0x00,0x10,0x20,0x30
#define MIN_NET_DEVICE_FOR_WDS			0x10		//0x40,0x50,0x60,0x70

#define RTMP_SET_PACKET_FRAGMENTS(_p, number)       ((_p)->cb[10] = number)
#define RTMP_GET_PACKET_FRAGMENTS(_p)               ((_p)->cb[10])
#define RTMP_SET_PACKET_RTS(_p, number)             ((_p)->cb[11] = number)
#define RTMP_GET_PACKET_RTS(_p)                     ((_p)->cb[11])
#define RTMP_SET_PACKET_SOURCE(_p, pktsrc)          ((_p)->cb[12] = pktsrc)
#define RTMP_GET_PACKET_SOURCE(_p)                  ((_p)->cb[12])
#define RTMP_SET_PACKET_TXRATE(_p, rate)            ((_p)->cb[13] = rate)
#define RTMP_GET_PACKET_TXRATE(_p)                  ((_p)->cb[13])
#define RTMP_SET_PACKET_UP(_p, _prio)               ((_p)->cb[14] = _prio)
#define RTMP_GET_PACKET_UP(_p)                      ((_p)->cb[14])
#define RTMP_SET_PACKET_CTS(_p, number)             ((_p)->cb[17] = number)
#define RTMP_GET_PACKET_CTS(_p)                     ((_p)->cb[17])

//#define RTMP_SET_PACKET_NET_DEVICE_MBSSID(_p, idx)  ((_p)->cb[15] = idx)
//#define RTMP_SET_PACKET_NET_DEVICE_WDS(_p, idx)     ((_p)->cb[15] = (idx + MIN_NET_DEVICE_FOR_WDS))
#define RTMP_GET_PACKET_NET_DEVICE(_p)              ((_p)->cb[15])
#define RTMP_SET_PACKET_AID(_p, idx)				((_p)->cb[16] = idx)
#define RTMP_GET_PACKET_AID(_p)						((_p)->cb[16])

#define PKTSRC_NDIS             0x7f
#define PKTSRC_DRIVER           0x0f

#if ( LINUX_VERSION_CODE < KERNEL_VERSION(2,4,22) )
#define pci_name(x) 	((x)->slot_name)
#endif

#define	MAKE_802_3_HEADER(_p, _pMac1, _pMac2, _pType)					\
{																		\
	NdisMoveMemory(_p, _pMac1, MAC_ADDR_LEN);							\
	NdisMoveMemory((_p + MAC_ADDR_LEN), _pMac2, MAC_ADDR_LEN);	        \
	NdisMoveMemory((_p + MAC_ADDR_LEN * 2), _pType, LENGTH_802_3_TYPE);	\
}

// if pData has no LLC/SNAP (neither RFC1042 nor Bridge tunnel), keep it that way.
// else if the received frame is LLC/SNAP-encaped IPX or APPLETALK, preserve the LLC/SNAP field 
// else remove the LLC/SNAP field from the result Ethernet frame
// Note:
//     _pData & _DataSize may be altered (remove 8-byte LLC/SNAP) by this MACRO
//     _pRemovedLLCSNAP: pointer to removed LLC/SNAP; NULL is not removed
#define CONVERT_TO_802_3(_p8023hdr, _pDA, _pSA, _pData, _DataSize, _pRemovedLLCSNAP)      \
{                                                                       \
    char LLC_Len[2];                                                    \
                                                                        \
    _pRemovedLLCSNAP = NULL;                                            \
	if (NdisEqualMemory(SNAP_802_1H, _pData, 6)  ||                     \
	    NdisEqualMemory(SNAP_BRIDGE_TUNNEL, _pData, 6))                 \
	{                                                                   \
	    PUCHAR pProto = _pData + 6;                                     \
					                                                    \
		if ((NdisEqualMemory(IPX, pProto, 2) || NdisEqualMemory(APPLE_TALK, pProto, 2)) &&  \
		    NdisEqualMemory(SNAP_802_1H, _pData, 6))                    \
		{                                                               \
			LLC_Len[0] = (UCHAR)(_DataSize / 256);                      \
			LLC_Len[1] = (UCHAR)(_DataSize % 256);                      \
			MAKE_802_3_HEADER(_p8023hdr, _pDA, _pSA, LLC_Len);          \
		}                                                               \
		else                                                            \
		{                                                               \
			MAKE_802_3_HEADER(_p8023hdr, _pDA, _pSA, pProto);           \
			_pRemovedLLCSNAP = _pData;                                  \
			_DataSize -= LENGTH_802_1_H;                                \
			_pData += LENGTH_802_1_H;                                   \
		}                                                               \
	}                                                                   \
	else                                                                \
	{                                                                   \
		LLC_Len[0] = (UCHAR)(_DataSize / 256);                          \
		LLC_Len[1] = (UCHAR)(_DataSize % 256);                          \
		MAKE_802_3_HEADER(_p8023hdr, _pDA, _pSA, LLC_Len);              \
	}                                                                   \
}

#define RECORD_LATEST_RX_DATA_RATE(_pAd, _pRxD)                                 \
{                                                                               \
    if ((_pRxD)->Ofdm)                                                          \
        (_pAd)->LastRxRate = OfdmSignalToRateId[(_pRxD)->PlcpSignal & 0x0f];    \
    else if ((_pRxD)->PlcpSignal == 10)                                         \
        (_pAd)->LastRxRate = RATE_1;                                            \
    else if ((_pRxD)->PlcpSignal == 20)                                         \
        (_pAd)->LastRxRate = RATE_2;                                            \
    else if ((_pRxD)->PlcpSignal == 55)                                         \
        (_pAd)->LastRxRate = RATE_5_5;                                          \
    else                                                                        \
        (_pAd)->LastRxRate = RATE_11;                                           \
	if ((_pAd->RfIcType == RFIC_5325) || (_pAd->RfIcType == RFIC_2529))	        \
    {                                                                           \
         _pAd->Mlme.bTxRateReportPeriod = FALSE;                               	\
         RTMP_IO_WRITE32(_pAd, TXRX_CSR1, 0x9eb39eb3);                       	\
    }                                                                           \
}

// INFRA mode- Address 1 - AP, Address 2 - this STA, Address 3 - DA
// ADHOC mode- Address 1 - DA, Address 2 - this STA, Address 3 - BSSID
#define MAKE_802_11_HEADER(_pAd, _80211hdr, _pDA, _seq)                         \
{                                                                               \
    NdisZeroMemory(&_80211hdr, sizeof(HEADER_802_11));                          \
    if (INFRA_ON(_pAd))                                                         \
    {                                                                           \
        COPY_MAC_ADDR(_80211hdr.Addr1, _pAd->PortCfg.Bssid);                    \
        COPY_MAC_ADDR(_80211hdr.Addr3, _pDA);                                   \
        _80211hdr.FC.ToDs = 1;                                                  \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        COPY_MAC_ADDR(_80211hdr.Addr1, _pDA);                                   \
        COPY_MAC_ADDR(_80211hdr.Addr3, _pAd->PortCfg.Bssid);                    \
    }                                                                           \
    COPY_MAC_ADDR(_80211hdr.Addr2, _pAd->CurrentAddress);                       \
    _80211hdr.Sequence = _seq;                                                  \
    _80211hdr.FC.Type = BTYPE_DATA;                                             \
                                                                                \
    if (_pAd->PortCfg.bAPSDForcePowerSave)									    \
    {																			\
    	_80211hdr.FC.PwrMgmt = PWR_SAVE;									    \
    }																			\
    else																		\
    {																			\
        _80211hdr.FC.PwrMgmt = (_pAd->PortCfg.Psm == PWR_SAVE);                 \
    }																			\
}

//Need to collect each ant's rssi concurrently
//rssi1 is report to pair2 Ant and rss2 is reprot to pair1 Ant when 4 Ant
#define COLLECT_RX_ANTENNA_AVERAGE_RSSI(_pAd, _rssi1, _rssi2)					\
{																				\
	SHORT	AvgRssi;															\
	UCHAR	UsedAnt;															\
	if (_pAd->RfIcType == RFIC_2529)											\
	{																			\
		if (_pAd->RxAnt.EvaluatePeriod == 0)									\
		{																		\
			UsedAnt = _pAd->RxAnt.Pair2PrimaryRxAnt;							\
			AvgRssi = _pAd->RxAnt.Pair2AvgRssi[UsedAnt];						\
			if (AvgRssi < 0)													\
				AvgRssi = AvgRssi - (AvgRssi >> 3) + _rssi1;					\
			else																\
				AvgRssi = _rssi1 << 3;											\
			_pAd->RxAnt.Pair2AvgRssi[UsedAnt] = AvgRssi;						\
		}																		\
		else																	\
		{																		\
			UsedAnt = _pAd->RxAnt.Pair2SecondaryRxAnt;							\
			AvgRssi = _pAd->RxAnt.Pair2AvgRssi[UsedAnt];						\
			if ((AvgRssi < 0) && (_pAd->RxAnt.FirstPktArrivedWhenEvaluate))		\
				AvgRssi = AvgRssi - (AvgRssi >> 3) + _rssi1;					\
			else																\
			{																	\
				AvgRssi = _rssi1 << 3;											\
			}																	\
			_pAd->RxAnt.Pair2AvgRssi[UsedAnt] = AvgRssi;						\
			_pAd->RxAnt.RcvPktNumWhenEvaluate++;								\
		}																		\
		if ((!_pAd->Mlme.bTxRateReportPeriod))									\
		{																		\
			if (_pAd->RxAnt.EvaluatePeriod == 0)								\
			{																	\
				UsedAnt = _pAd->RxAnt.Pair1PrimaryRxAnt;						\
				AvgRssi = _pAd->RxAnt.Pair1AvgRssi[UsedAnt];					\
				if (AvgRssi < 0)												\
					AvgRssi = AvgRssi - (AvgRssi >> 3) + _rssi2;				\
				else															\
					AvgRssi = _rssi2 << 3;										\
				_pAd->RxAnt.Pair1AvgRssi[UsedAnt] = AvgRssi;					\
			}																	\
			else																\
			{																	\
				UsedAnt = _pAd->RxAnt.Pair1SecondaryRxAnt;						\
				AvgRssi = _pAd->RxAnt.Pair1AvgRssi[UsedAnt];					\
				if ((AvgRssi < 0) && (_pAd->RxAnt.FirstPktArrivedWhenEvaluate))	\
					AvgRssi = AvgRssi - (AvgRssi >> 3) + _rssi2;				\
				else															\
				{																\
					_pAd->RxAnt.FirstPktArrivedWhenEvaluate = TRUE;				\
					AvgRssi = _rssi2 << 3;										\
				}																\
				_pAd->RxAnt.Pair1AvgRssi[UsedAnt] = AvgRssi;					\
			}																	\
		}																		\
	}																			\
	else																		\
	{																			\
		if (_pAd->RxAnt.EvaluatePeriod == 0)									\
		{																		\
			UsedAnt = _pAd->RxAnt.Pair1PrimaryRxAnt;							\
			AvgRssi = _pAd->RxAnt.Pair1AvgRssi[UsedAnt];						\
			if (AvgRssi < 0)													\
				AvgRssi = AvgRssi - (AvgRssi >> 3) + _rssi1;					\
			else																\
				AvgRssi = _rssi1 << 3;											\
			_pAd->RxAnt.Pair1AvgRssi[UsedAnt] = AvgRssi;						\
		}																		\
		else																	\
		{																		\
			UsedAnt = _pAd->RxAnt.Pair1SecondaryRxAnt;							\
			AvgRssi = _pAd->RxAnt.Pair1AvgRssi[UsedAnt];						\
			if ((AvgRssi < 0) && (_pAd->RxAnt.FirstPktArrivedWhenEvaluate))		\
				AvgRssi = AvgRssi - (AvgRssi >> 3) + _rssi1;					\
			else																\
			{																	\
				_pAd->RxAnt.FirstPktArrivedWhenEvaluate = TRUE;					\
				AvgRssi = _rssi1 << 3;											\
			}																	\
			_pAd->RxAnt.Pair1AvgRssi[UsedAnt] = AvgRssi;						\
			_pAd->RxAnt.RcvPktNumWhenEvaluate++;								\
		}																		\
	}																			\
}

#define MAC_ADDR_EQUAL(pAddr1,pAddr2)           RTMPEqualMemory((PVOID)(pAddr1), (PVOID)(pAddr2), MAC_ADDR_LEN)
#define COPY_MAC_ADDR(Addr1, Addr2)             memcpy((Addr1), (Addr2), MAC_ADDR_LEN)
#define SSID_EQUAL(ssid1, len1, ssid2, len2)    ((len1==len2) && (RTMPEqualMemory(ssid1, ssid2, len1)))

#define RELEASE_NDIS_PACKET(_pAd, _pSkb)                             	\
{                                                                       \
    if (RTMP_GET_PACKET_SOURCE(_pSkb) == PKTSRC_NDIS)                	\
    {                                                                   \
        RTMPFreeSkbBuffer(_pSkb);                                    	\
        _pAd->RalinkCounters.PendingNdisPacketCount --;                 \
    }                                                                   \
    else                                                                \
        RTMPFreeSkbBuffer(_pSkb);                                    	\
}

//
// Register set pair for initialzation register set definition
//
typedef struct  _RTMP_REG_PAIR
{
    ULONG   Register;
    ULONG   Value;
}   RTMP_REG_PAIR, *PRTMP_REG_PAIR;

typedef	struct	_BBP_REG_PAIR
{
	UCHAR	Register;
	UCHAR	Value;
}	BBP_REG_PAIR, *PBBP_REG_PAIR;

//
// Register set pair for initialzation register set definition
//
typedef struct  _RTMP_RF_REGS
{
    UCHAR   Channel;
    ULONG   R1;
    ULONG   R2;
    ULONG   R3;
    ULONG   R4;
}   RTMP_RF_REGS, *PRTMP_RF_REGS;

//
//  Data buffer for DMA operation, the buffer must be contiguous physical memory
//  Both DMA to / from CPU use the same structure.
//
typedef struct  _RTMP_DMABUF
{
    ULONG                   AllocSize;
    PVOID                   AllocVa;            // TxBuf virtual address
    dma_addr_t              AllocPa;            // TxBuf physical address
    struct sk_buff			*pSkb;
}   RTMP_DMABUF, *PRTMP_DMABUF;

//
// Control block (Descriptor) for all ring descriptor DMA operation, buffer must be 
// contiguous physical memory. NDIS_PACKET stored the binding Rx packet descriptor
// which won't be released, driver has to wait until upper layer return the packet
// before giveing up this rx ring descriptor to ASIC. NDIS_BUFFER is assocaited pair
// to describe the packet buffer. For Tx, NDIS_PACKET stored the tx packet descriptor
// which driver should ACK upper layer when the tx is physically done or failed.
//
typedef struct _RTMP_DMACB
{
    ULONG                   AllocSize;          // Control block size
    PVOID                   AllocVa;            // Control block virtual address
    dma_addr_t              AllocPa;            // Control block physical address
    RTMP_DMABUF             DmaBuf;             // Associated DMA buffer structure
	struct sk_buff          *pSkb;
	struct sk_buff          *pNextSkb;
}   RTMP_DMACB, *PRTMP_DMACB;

typedef struct _RTMP_TX_BUF
{
    PQUEUE_ENTRY    Next;
    UCHAR           Index;
    ULONG           AllocSize;          // Control block size
    PVOID           AllocVa;            // Control block virtual address
    dma_addr_t      AllocPa;            // Control block physical address
}   RTMP_TXBUF, *PRTMP_TXBUF;

typedef struct _RTMP_TX_RING
{
    RTMP_DMACB  Cell[TX_RING_SIZE];
    UINT        CurTxIndex;
    UINT        NextTxDmaDoneIndex;
}   RTMP_TX_RING, *PRTMP_TX_RING;

typedef struct _RTMP_RX_RING
{
    RTMP_DMACB  Cell[RX_RING_SIZE];
    UINT        CurRxIndex;
}   RTMP_RX_RING, *PRTMP_RX_RING;

typedef struct _RTMP_MGMT_RING
{
    RTMP_DMACB  Cell[MGMT_RING_SIZE];
    UINT        CurTxIndex;
    UINT        NextTxDmaDoneIndex;
} RTMP_MGMT_RING, *PRTMP_MGMT_RING;

//
//  Statistic counter structure
//
typedef struct _COUNTER_802_3
{
    // General Stats
	ULONG		GoodTransmits;
	ULONG		GoodReceives;
	ULONG		TxErrors;
	ULONG		RxErrors;
	ULONG		RxNoBuffer;

    // Ethernet Stats
	ULONG		RcvAlignmentErrors;
	ULONG		OneCollision;
	ULONG		MoreCollisions;

}   COUNTER_802_3, *PCOUNTER_802_3;

typedef struct _COUNTER_802_11 {
    ULONG           Length;
    LARGE_INTEGER	TransmittedFragmentCount;
    LARGE_INTEGER	MulticastTransmittedFrameCount;
    LARGE_INTEGER	FailedCount;
    LARGE_INTEGER	RetryCount;
    LARGE_INTEGER	MultipleRetryCount;
    LARGE_INTEGER	RTSSuccessCount;
    LARGE_INTEGER	RTSFailureCount;
    LARGE_INTEGER	ACKFailureCount;
    LARGE_INTEGER	FrameDuplicateCount;
    LARGE_INTEGER	ReceivedFragmentCount;
    LARGE_INTEGER	MulticastReceivedFrameCount;
    LARGE_INTEGER	FCSErrorCount;
} COUNTER_802_11, *PCOUNTER_802_11;

typedef struct _COUNTER_RALINK {
    ULONG           TransmittedByteCount;   // both successful and failure, used to calculate TX throughput
    ULONG           ReceivedByteCount;      // both CRC okay and CRC error, used to calculate RX throughput
    ULONG           BeenDisassociatedCount;
    ULONG           BadCQIAutoRecoveryCount;
    ULONG           PoorCQIRoamingCount;
    ULONG           MgmtRingFullCount;
    ULONG           RxCount;
    ULONG           RxRingErrCount;
    ULONG           KickTxCount;
    ULONG           TxRingErrCount;
	LARGE_INTEGER	RealFcsErrCount;
    ULONG           PendingNdisPacketCount;

    ULONG           OneSecOsTxCount[NUM_OF_TX_RING];
    ULONG           OneSecDmaDoneCount[NUM_OF_TX_RING];
    ULONG           OneSecTxDoneCount;
    ULONG           OneSecTxAggregationCount;
    ULONG           OneSecRxAggregationCount;

    ULONG           OneSecTxNoRetryOkCount;
    ULONG           OneSecTxRetryOkCount;
    ULONG           OneSecTxFailCount;
	ULONG		    OneSecFalseCCACnt;		// CCA error count, for debug purpose, might move to global counter
    ULONG           OneSecRxOkCnt;          // RX without error
    ULONG           OneSecRxOkDataCnt;      // unicast-to-me DATA frame count
	ULONG           OneSecRxFcsErrCnt;      // CRC error
	ULONG           OneSecBeaconSentCnt;
	ULONG           LastOneSecTotalTxCount; // OneSecTxNoRetryOkCount + OneSecTxRetryOkCount + OneSecTxFailCount
	ULONG           LastOneSecRxOkDataCnt;  // OneSecRxOkDataCnt
} COUNTER_RALINK, *PCOUNTER_RALINK;

typedef struct _COUNTER_DRS {
    // to record the each TX rate's quality. 0 is best, the bigger the worse.
    USHORT          TxQuality[MAX_LEN_OF_SUPPORTED_RATES];
    UCHAR           PER[MAX_LEN_OF_SUPPORTED_RATES];
    UCHAR           TxRateUpPenalty;      // extra # of second penalty due to last unstable condition
    ULONG           CurrTxRateStableTime; // # of second in current TX rate
    BOOLEAN         fNoisyEnvironment;
    UCHAR           LastSecTxRateChangeAction; // 0: no change, 1:rate UP, 2:rate down
} COUNTER_DRS, *PCOUNTER_DRS;

//
//  Arcfour Structure Added by PaulWu
//
typedef struct _ARCFOUR
{
    UINT            X;
    UINT            Y;
    UCHAR           STATE[256];
}   ARCFOURCONTEXT, *PARCFOURCONTEXT;

typedef	struct PACKED	_IV_CONTROL_
{
	union PACKED
	{
		struct PACKED 
		{
			UCHAR		rc0;
			UCHAR		rc1;
			UCHAR		rc2;

			union PACKED
			{
				struct PACKED
				{
#ifdef BIG_ENDIAN
					UCHAR	KeyID:2;
					UCHAR	ExtIV:1;
					UCHAR	Rsvd:5;
#else
					UCHAR	Rsvd:5;
					UCHAR	ExtIV:1;
					UCHAR	KeyID:2;
#endif
				}	field;
				UCHAR		Byte;
			}	CONTROL;
		}	field;
		
		ULONG	word;
	}	IV16;
	
	ULONG	IV32;
}	TKIP_IV, *PTKIP_IV;

typedef struct _CIPHER_KEY {
	UCHAR	Mac[6];	            // as lookup key, for pairwise key only
	UCHAR   BssId[6];
	UCHAR   CipherAlg;          // 0-none, 1:WEP64, 2:WEP128, 3:TKIP, 4:AES, 5:CKIP64, 6:CKIP128
	UCHAR	KeyLen;				// Key length for each key, 0: entry is invalid
	UCHAR	Key[16];			// right now we implement 4 keys, 128 bits max
	UCHAR	RxMic[8];
	UCHAR	TxMic[8];
	UCHAR	TxTsc[6];			// 48bit TSC value
	UCHAR	RxTsc[6];			// 48bit TSC value
	UCHAR   Type;               // Indicate Pairwise/Group when reporting MIC error
}	CIPHER_KEY, *PCIPHER_KEY;

typedef struct _SOFT_RX_ANT_DIVERSITY_STRUCT {
    UCHAR     EvaluatePeriod;		 // 0:not evalute status, 1: evaluate status, 2: switching status
    UCHAR     Pair1PrimaryRxAnt;     // 0:Ant-E1, 1:Ant-E2
    UCHAR     Pair1SecondaryRxAnt;   // 0:Ant-E1, 1:Ant-E2
    UCHAR     Pair2PrimaryRxAnt;     // 0:Ant-E3, 1:Ant-E4
    UCHAR     Pair2SecondaryRxAnt;   // 0:Ant-E3, 1:Ant-E4
    SHORT     Pair1AvgRssi[2];       // AvgRssi[0]:E1, AvgRssi[1]:E2
    SHORT     Pair2AvgRssi[2];       // AvgRssi[0]:E3, AvgRssi[1]:E4
    SHORT     Pair1LastAvgRssi;
    SHORT     Pair2LastAvgRssi;
    ULONG     RcvPktNumWhenEvaluate;
    BOOLEAN   FirstPktArrivedWhenEvaluate;
	RALINK_TIMER_STRUCT    RxAntDiversityTimer;
} SOFT_RX_ANT_DIVERSITY, *PSOFT_RX_ANT_DIVERSITY;

typedef struct {
    BOOLEAN     Enable;
    UCHAR       Delta;
    BOOLEAN     PlusSign;
} CCK_TX_POWER_CALIBRATE, *PCCK_TX_POWER_CALIBRATE;

//
// Receive Tuple Cache Format
//
typedef struct  _TUPLE_CACHE    {
    BOOLEAN         Valid;
    UCHAR           MacAddress[MAC_ADDR_LEN];
    USHORT          Sequence; 
    USHORT          Frag;
}   TUPLE_CACHE, *PTUPLE_CACHE;

//
// Fragment Frame structure
//
typedef struct  _FRAGMENT_FRAME {
    UCHAR       Header802_11[LENGTH_802_11];  // add by johnli, fix WPS & WPAPSK/WPA2PSK bugs for receiving EAPoL fragmentation packets
    UCHAR       Header802_3[LENGTH_802_3];
    UCHAR       Header_LLC[LENGTH_802_1_H];
    UCHAR       Buffer[LENGTH_802_3 + MAX_FRAME_SIZE];  // Add header to prevent NETBUEI continuous buffer isssue
    ULONG       RxSize;
    USHORT      Sequence;
    USHORT      LastFrag;
    ULONG       Flags;          // Some extra frame information. bit 0: LLC presented
}   FRAGMENT_FRAME, *PFRAGMENT_FRAME;

//---> Add by Zero:Jul.05.2007
//
// Packet information for NdisQueryPacket
//
typedef struct  _PACKET_INFO    {
	UINT            PhysicalBufferCount;    // Physical breaks of buffer descripor chained
	UINT            BufferCount ;           // Number of Buffer descriptor chained
	UINT            TotalPacketLength ;     // Self explained
	PNDIS_BUFFER    pFirstBuffer;           // Pointer to first buffer descriptor
} PACKET_INFO, *PPACKET_INFO;
//<---

//
// Tkip Key structure which RC4 key & MIC calculation
//
typedef struct _TKIP_KEY_INFO  {
    UINT        nBytesInM;  // # bytes in M for MICKEY
    ULONG       IV16;
    ULONG       IV32;   
    ULONG       K0;         // for MICKEY Low
    ULONG       K1;         // for MICKEY Hig
    ULONG       L;          // Current state for MICKEY
    ULONG       R;          // Current state for MICKEY
    ULONG       M;          // Message accumulator for MICKEY
    UCHAR       RC4KEY[16];
    UCHAR       MIC[8];
}   TKIP_KEY_INFO, *PTKIP_KEY_INFO;

//
// Private / Misc data, counters for driver internal use
//
typedef struct  __PRIVATE_STRUC {
    ULONG       SystemResetCnt;         // System reset counter
    ULONG       TxRingFullCnt;          // Tx ring full occurrance number
    ULONG       ResetCountDown;         // Count down before issue reset
    ULONG       CCAErrCnt;              // CCA error count, for debug purpose, might move to global counter
    ULONG       PhyRxErrCnt;            // PHY Rx error count, for debug purpose, might move to global counter
    ULONG       PhyTxErrCnt;            // PHY Tx error count, for debug purpose, might move to global counter
    // Variables for WEP encryption / decryption in rtmp_wep.c
    ULONG           FCSCRC32;
    ULONG           RxSetCnt;
    ULONG           DecryptCnt;
    ARCFOURCONTEXT  WEPCONTEXT;
    // Tkip stuff
    TKIP_KEY_INFO   Tx;
    TKIP_KEY_INFO   Rx;
}   PRIVATE_STRUC, *PPRIVATE_STRUC;

#ifdef RALINK_ATE
typedef	struct _ATE_INFO {
	UCHAR	Mode;
	CHAR	TxPower;
	UCHAR	Addr1[6];
	UCHAR	Addr2[6];
	UCHAR	Addr3[6];
    UCHAR	Channel;
	ULONG	TxLength;
	ULONG	TxCount;
	ULONG	TxDoneCount;
	ULONG	TxRate;
	ULONG	RFFreqOffset;
}	ATE_INFO, *PATE_INFO;
#endif	// RALINK_ATE

// structure to tune BBP R17 "RX AGC VGC init"
typedef struct _BBP_R17_TUNING {
    BOOLEAN     bEnable;
    UCHAR       R17LowerBoundG;
    UCHAR       R17LowerBoundA;
    UCHAR       R17UpperBoundG;
    UCHAR       R17UpperBoundA;
//    UCHAR       LastR17Value;
//    SHORT       R17Dec;     // R17Dec = 0x79 - RssiToDbm, for old version R17Dec = 0.
//                            // This is signed value
    USHORT      FalseCcaLowerThreshold;  // default 100
    USHORT      FalseCcaUpperThreshold;  // default 512
    UCHAR       R17Delta;                // R17 +- R17Delta whenever false CCA over UpperThreshold or lower than LowerThreshold
    UCHAR       R17CurrentValue;
    BOOLEAN		R17LowerUpperSelect; //Before LinkUp, Used LowerBound or UpperBound as R17 value.
} BBP_R17_TUNING, *PBBP_R17_TUNING;

// structure to store channel TX power
typedef struct _CHANNEL_TX_POWER {
    UCHAR      Channel;
    UCHAR       Power;
}	CHANNEL_TX_POWER, *PCHANNEL_TX_POWER;

typedef enum _ABGBAND_STATE_ {
	UNKNOWN_BAND,
	BG_BAND,
	A_BAND,
} ABGBAND_STATE;

typedef struct _MLME_STRUCT {
    STATE_MACHINE           CntlMachine;
    STATE_MACHINE           AssocMachine;
    STATE_MACHINE           AuthMachine;
    STATE_MACHINE           AuthRspMachine;
    STATE_MACHINE           SyncMachine;
    STATE_MACHINE           WpaPskMachine;
    STATE_MACHINE_FUNC      AssocFunc[ASSOC_FUNC_SIZE];
    STATE_MACHINE_FUNC      AuthFunc[AUTH_FUNC_SIZE];
    STATE_MACHINE_FUNC      AuthRspFunc[AUTH_RSP_FUNC_SIZE];
    STATE_MACHINE_FUNC      SyncFunc[SYNC_FUNC_SIZE];
    STATE_MACHINE_FUNC      WpaPskFunc[WPA_PSK_FUNC_SIZE];
    ULONG                   ChannelQuality;  // 0..100, Channel Quality Indication for Roaming
    ULONG                   Now32;           // latch the value of NdisGetSystemUpTime() 

    BOOLEAN                 bRunning;
    spinlock_t              TaskLock;
    spinlock_t              MemLock; 
    MLME_QUEUE              Queue;

    UINT                    ShiftReg;

	BOOLEAN					bTxRateReportPeriod;
	RALINK_TIMER_STRUCT	RssiReportTimer;  
	ULONG                   RssiReportRound;  
	
	RALINK_TIMER_STRUCT	PeriodicTimer;
    ULONG                   PeriodicRound;
	RALINK_TIMER_STRUCT	LinkDownTimer;
    
} MLME_STRUCT, *PMLME_STRUCT;

// structure for radar detection and channel switch
typedef struct _RADAR_DETECT_STRUCT {
	UCHAR		CSCount;			//Channel switch counter
	UCHAR		CSPeriod;			//Channel switch period (beacon count)
	UCHAR		RDCount;			//Radar detection counter
	UCHAR		RDMode;				//Radar Detection mode
	UCHAR		BBPR16;
	UCHAR		BBPR17;
	UCHAR		BBPR18;
	UCHAR		BBPR21;
	UCHAR		BBPR22;
	UCHAR		BBPR64;
	ULONG		InServiceMonitorCount; // unit: sec
} RADAR_DETECT_STRUCT, *PRADAR_DETECT_STRUCT;

//  configuration and status
typedef struct _PORT_CONFIG {

    // MIB:ieee802dot11.dot11smt(1).dot11StationConfigTable(1)
    USHORT      Psm;                  // power management mode   (PWR_ACTIVE|PWR_SAVE)
    USHORT      DisassocReason;
    UCHAR       DisassocSta[MAC_ADDR_LEN];
    USHORT      DeauthReason;
    UCHAR       DeauthSta[MAC_ADDR_LEN];
    USHORT      AuthFailReason;
    UCHAR       AuthFailSta[MAC_ADDR_LEN];

    NDIS_802_11_AUTHENTICATION_MODE     AuthMode;       // This should match to whatever microsoft defined
    NDIS_802_11_WEP_STATUS              WepStatus;
	NDIS_802_11_WEP_STATUS				OrigWepStatus;	// Original wep status set from OID

#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
	int     	wx_auth_alg;	 // OPEN, SHARED, LEAP
	int     	wx_key_mgmt;	 // WPA-PSK, WPA-EAP, IEEE8021X, NONE
	int		wx_wpa_version; // WPA(IEEE802.11i/D3), RSN(WPA2/IEEE802.11i)
	int		wx_pairwise;    // CCMP, TKIP, NONE;
	int		wx_groupCipher; // CCMP, TKIP, WEP104, WEP40
//	UCHAR       wx_RSN_IE[MAX_WPA_IE_LEN];
//	UCHAR       wx_RSNIE_Len;		 // Used to save WPA/RSN IE temporary.
//	UCHAR		wx_psk[65];     // WPA pre-shared key.(64 hex-digitial or 8~63 ascii)
//	int			wx_eapol_flags; // bit(0): require dynamically generated unicast WEP key
								     // bit(1):require dynamically generated broadcast WEP key
									 // bit(2): Require both.
	BOOLEAN		wx_need_sync;	 //Notify that need to sync the wpa_supplicant configuration with our driver.
	BOOLEAN		bNativeWpa;	 // add by johnli, enable/disable native wpa supplicant
#endif // NATIVE_WPA_SUPPLICANT_SUPPORT //
    
    // Add to support different cipher suite for WPA2/WPA mode
	NDIS_802_11_ENCRYPTION_STATUS		GroupCipher;		// Multicast cipher suite
	NDIS_802_11_ENCRYPTION_STATUS		PairCipher;			// Unicast cipher suite
	BOOLEAN								bMixCipher;			// Indicate current Pair & Group use different cipher suites
	USHORT								RsnCapability;
	
    // MIB:ieee802dot11.dot11smt(1).dot11WEPDefaultKeysTable(3)
    CIPHER_KEY  PskKey;                 // WPA PSK mode PMK
    UCHAR       PTK[64];                // WPA PSK mode PTK
	BSSID_INFO	SavedPMK[PMKID_NO];
	ULONG		SavedPMKNum;			// Saved PMKID number

    // WPA 802.1x port control, WPA_802_1X_PORT_SECURED, WPA_802_1X_PORT_NOT_SECURED
    UCHAR       PortSecured;
    UCHAR		RSN_IE[44];
	UCHAR		RSN_IELen;

//#if WPA_SUPPLICANT_SUPPORT	
	BOOLEAN     IEEE8021X;				// Enable or disable IEEE 802.1x 
	CIPHER_KEY	DesireSharedKey[4];		// Record user desired WEP keys	
	BOOLEAN		IEEE8021x_required_keys;				// Enable or disable dynamic wep key updating
	BOOLEAN     WPA_Supplicant;         // Enable or disable WPA_SUPPLICANT 
//#endif

    // For WPA countermeasures
    unsigned long	LastMicErrorTime;   // record last MIC error time
    ULONG       MicErrCnt;          // Should be 0, 1, 2, then reset to zero (after disassoiciation).
    BOOLEAN     bBlockAssoc;        // Block associate attempt for 60 seconds after counter measure occurred.
    // For WPA-PSK supplicant state
    WPA_STATE   WpaState;           // Default is SS_NOTUSE and handled by microsoft 802.1x
    UCHAR       ReplayCounter[8];
    UCHAR       ANonce[32];         // ANonce for WPA-PSK from aurhenticator
    UCHAR       SNonce[32];         // SNonce for WPA-PSK
    
    // MIB:ieee802dot11.dot11smt(1).dot11PrivacyTable(5)
    UCHAR                               DefaultKeyId;
    NDIS_802_11_PRIVACY_FILTER          PrivacyFilter;  // PrivacyFilter enum for 802.1X


    // MIB:ieee802dot11.dot11mac(2).dot11OperationTable(1)
    USHORT      RtsThreshold;           // in unit of BYTE
    USHORT      FragmentThreshold;      // in unit of BYTE
    BOOLEAN     bFragmentZeroDisable;   // Microsoft use 0 as disable 
    
    // MIB:ieee802dot11.dot11phy(4).dot11PhyTxPowerTable(3)
    UCHAR       TxPower;                // in unit of mW
    ULONG       TxPowerPercentage;      // 0~100 %
    ULONG       TxPowerDefault;         // keep for TxPowerPercentage

    // MIB:ieee802dot11.dot11phy(4).dot11PhyDSSSTable(5)
    UCHAR       Channel;              	// current (I)BSS channel used in the station
    UCHAR       AdHocChannel;          	// current (I)BSS channel used in the station
    UCHAR       CountryRegion;    	// Enum of country region, 0:FCC, 1:IC, 2:ETSI, 3:SPAIN, 4:France, 5:MKK, 6:MKK1, 7:Israel
    UCHAR       CountryRegionForABand;	// Enum of country region for A band

    
    // Copy supported rate from desired AP's beacon. We are trying to match
	// AP's supported and extended rate settings.
	UCHAR       SupRate[MAX_LEN_OF_SUPPORTED_RATES];
    UCHAR       SupRateLen;
    UCHAR       ExtRate[MAX_LEN_OF_SUPPORTED_RATES];
    UCHAR       ExtRateLen;
    UCHAR       ExpectedACKRate[MAX_LEN_OF_SUPPORTED_RATES];

    ULONG       BasicRateBitmap;        // backup basic ratebitmap

    //
    // other parameters not defined in standard MIB
    //
    UCHAR       DesireRate[MAX_LEN_OF_SUPPORTED_RATES];      // OID_802_11_DESIRED_RATES
    UCHAR       MaxDesiredRate;

    UCHAR       TxRate;                 // RATE_1, RATE_2, RATE_5_5, RATE_11, ...
    UCHAR       MaxTxRate;              // RATE_1, RATE_2, RATE_5_5, RATE_11
    UCHAR       MinTxRate;              // RATE_1, RATE_2, RATE_5_5, RATE_11
    UCHAR       RtsRate;                // RATE_xxx
    UCHAR       MlmeRate;
    UCHAR       BasicMlmeRate;          // Default Rate for sending MLME frames

    UCHAR       Bssid[MAC_ADDR_LEN];
    USHORT      BeaconPeriod; 
    CHAR        Ssid[MAX_LEN_OF_SSID+1];      // NOT NULL-terminated
    UCHAR       SsidLen;                    // the actual ssid length in used
    UCHAR       LastSsidLen;                // the actual ssid length in used
    CHAR        LastSsid[MAX_LEN_OF_SSID+1];  // NOT NULL-terminated
    UCHAR		LastBssid[MAC_ADDR_LEN];

    UCHAR       BssType;                // BSS_INFRA or BSS_ADHOC
    USHORT      AtimWin;                // used when starting a new IBSS
    
    UCHAR       RssiTrigger;
    UCHAR       RssiTriggerMode;        // RSSI_TRIGGERED_UPON_BELOW_THRESHOLD or RSSI_TRIGGERED_UPON_EXCCEED_THRESHOLD
    USHORT      DefaultListenCount;     // default listen count;
    ULONG       WindowsPowerMode;           // Power mode for AC power
    ULONG       WindowsBatteryPowerMode;    // Power mode for battery if exists
    BOOLEAN     bWindowsACCAMEnable;        // Enable CAM power mode when AC on
    BOOLEAN     bAutoReconnect;         // Set to TRUE when setting OID_802_11_SSID with no matching BSSID
    
    UCHAR       LastRssi;               // last received BEACON's RSSI
    UCHAR	    LastRssi2;	            // last received BEACON's RSSI for smart antenna
    USHORT      AvgRssi;                // last 8 BEACON's average RSSI
	USHORT      AvgRssi2;              // last 8 BEACON's average RSSI
    USHORT      AvgRssiX8;              // sum of last 8 BEACON's RSSI
	USHORT      AvgRssi2X8;            // sum of last 8 BEACON's RSSI
    ULONG       NumOfAvgRssiSample;

    unsigned long	LastBeaconRxTime;       // OS's timestamp of the last BEACON RX time
    unsigned long	Last11bBeaconRxTime;    // OS's timestamp of the last 11B BEACON RX time
//    ULONG       IgnoredScanNumber;      // Ignored BSSID_SCAN_LIST requests
    unsigned long	LastScanTime;           // Record last scan time for issue BSSID_SCAN_LIST
    ULONG       ScanCnt;              // Scan counts since most recent SSID, BSSID, SCAN OID request
    BOOLEAN     bSwRadio;               // Software controlled Radio On/Off, TRUE: On
    BOOLEAN     bHwRadio;               // Hardware controlled Radio On/Off, TRUE: On
    BOOLEAN     bRadio;                 // Radio state, And of Sw & Hw radio state
    BOOLEAN     bHardwareRadio;         // Hardware controlled Radio enabled
    BOOLEAN     bShowHiddenSSID;      // Show all known SSID in SSID list get operation


    // PHY specification
    UCHAR     PhyMode;          // PHY_11A, PHY_11B, PHY_11BG_MIXED, PHY_ABG_MIXED
    USHORT    Dsifs;            // in units of usec
    USHORT    TxPreamble;       // Rt802_11PreambleLong, Rt802_11PreambleShort, Rt802_11PreambleAuto

    // New for WPA, windows want us to to keep association information and
    // Fixed IEs from last association response
    NDIS_802_11_ASSOCIATION_INFORMATION     AssocInfo;
	USHORT					ReqVarIELen;				// Length of next VIE include EID & Length
	UCHAR					ReqVarIEs[MAX_VIE_LEN];
	USHORT					ResVarIELen;				// Length of next VIE include EID & Length
	UCHAR					ResVarIEs[MAX_VIE_LEN];

    ULONG                   EnableTurboRate;      // 1: enable 72/100 Mbps whenever applicable, 0: never use 72/100 Mbps
    ULONG                   UseBGProtection;      // 0:AUTO, 1-always ON,2-always OFF
    ULONG                   UseShortSlotTime;     // 0: disable, 1 - use short slot (9us)


	// EDCA Qos
    BOOLEAN                 bWmmCapable;        // 0:disable WMM, 1:enable WMM
    QOS_CAPABILITY_PARM		APQosCapability;    // QOS capability of the current associated AP
    EDCA_PARM				APEdcaParm;         // EDCA parameters of the current associated AP
    QBSS_LOAD_PARM			APQbssLoad;         // QBSS load of the current associated AP

    // APSD
    BOOLEAN		            bAPSDCapable;
	BOOLEAN		            bAPSDAC_BE;
	BOOLEAN		            bAPSDAC_BK;
	BOOLEAN		            bAPSDAC_VI;
	BOOLEAN		            bAPSDAC_VO;
//	BOOLEAN		            bNeedSendTriggerFrame;  //Driver should not send trigger frame, it should be send by application layer
	BOOLEAN		            bAPSDForcePowerSave;	// Force power save mode, should only use in APSD-STAUT
	UCHAR		            MaxSPLength;

    UCHAR           	    DtimCount;      // 0.. DtimPeriod-1
	UCHAR           	    DtimPeriod;     // default = 3


    BOOLEAN					bEnableTxBurst;         // 0: disable, 1: enable TX PACKET BURST
    BOOLEAN					bAggregationCapable;    // 1: enable TX aggregation when the peer supports it
    BOOLEAN                 bPiggyBackCapable;		// 1: enable TX piggy-back according MAC's version
    BOOLEAN                 bIEEE80211H;			// 1: enable IEEE802.11h spec.

    //Fast Roaming
    BOOLEAN		    bFastRoaming;	// 0: disable, 1: enable Fast Roaming
    ULONG		    dBmToRoam:8;      // fast roaming offset value
    
	// a bitmap of BOOLEAN flags. each bit represent an operation status of a particular 
    // BOOLEAN control, either ON or OFF. These flags should always be accessed via
    // OPSTATUS_TEST_FLAG(), OPSTATUS_SET_FLAG(), OP_STATUS_CLEAR_FLAG() macros.
    // see fOP_STATUS_xxx in RTMP_DEF.C for detail bit definition
    ULONG					OpStatusFlags;

	UCHAR					AckPolicy[4];       // ACK policy of the specified AC. see ACK_xxx
		
    ABGBAND_STATE		    BandState;		    // For setting BBP used on B/G or A mode

    ULONG                   AdhocMode;          // 0:WIFI mode (11b rates only), 1: b/g mixed, 2: 11g only
    
	RALINK_TIMER_STRUCT	QuickResponeForRateUpTimer;
	BOOLEAN				    QuickResponeForRateUpTimerRunning;
	RALINK_TIMER_STRUCT	WpaDisassocAndBlockAssocTimer;

    RADAR_DETECT_STRUCT	    RadarDetect;
    
    BOOLEAN                 bGetAPConfig;

	ULONG BlockChannelBitMap;
	
    
} PORT_CONFIG, *PPORT_CONFIG;

// This data structure keep the current active BSS/IBSS's configuration that this STA
// had agreed upon joining the network. Which means these parameters are usually decided
// by the BSS/IBSS creator instead of user configuration. Data in this data structurre 
// is valid only when either ADHOC_ON(pAd) or INFRA_ON(pAd) is TRUE.
// Normally, after SCAN or failed roaming attempts, we need to recover back to
// the current active settings.
typedef struct _ACTIVE_CONFIG {
    USHORT      Aid;
    USHORT      AtimWin;                // in kusec; IBSS parameter set element
    USHORT      CapabilityInfo;
    USHORT      CfpMaxDuration;
    USHORT      CfpPeriod;
    
    // Copy supported rate from desired AP's beacon. We are trying to match
    // AP's supported and extended rate settings.
    UCHAR       SupRate[MAX_LEN_OF_SUPPORTED_RATES];
    UCHAR       ExtRate[MAX_LEN_OF_SUPPORTED_RATES];
    UCHAR       SupRateLen;
    UCHAR       ExtRateLen;
} ACTIVE_CONFIG, *PACTIVE_CONFIG;


//
//  The miniport adapter structure
//
typedef struct _RTMP_ADAPTER
{
	// linux specific
	struct pci_dev          *pPci_Dev;
	struct net_device       *net_dev;
	ULONG  VendorDesc;      // VID/PID
	
#if WIRELESS_EXT >= 12
    struct iw_statistics iw_stats;
#endif
    struct net_device_stats stats;
    
    CHAR                    nickn[IW_ESSID_MAX_SIZE+1]; // nickname, only used in the iwconfig i/f 
    INT						chip_id;
    PVOID                   CSRBaseAddress;

    // resource for DMA operation
    RTMP_TX_RING            TxRing[NUM_OF_TX_RING];                  // AC0~4 + HCCA
    RTMP_RX_RING            RxRing;
    RTMP_MGMT_RING          MgmtRing;
    RTMP_DMABUF             TxDescRing[NUM_OF_TX_RING]; // Shared memory for Tx descriptors
    RTMP_DMABUF             TxBufSpace[NUM_OF_TX_RING]; // Shared memory of all 1st pre-allocated TxBuf associated with each TXD
    RTMP_DMABUF             RxDescRing;                 // Shared memory for RX descriptors
    RTMP_DMABUF             MgmtDescRing;               // Shared memory for MGMT descriptors

    // resource for software backlog queues
    QUEUE_HEADER            TxSwQueue[NUM_OF_TX_RING];  // 4 AC + 1 HCCA

    // SpinLocks
	spinlock_t				TxRingLock;					// Tx Ring spinlock
	spinlock_t				MgmtRingLock;				// Prio Ring spinlock
	spinlock_t				RxRingLock;					// Rx Ring spinlock
	spinlock_t				TxSwQueueLock;				// TxSwQueue spinlock

    // outgoing BEACON frame buffer and corresponding TXD 
    TXD_STRUC               BeaconTxD;
    CHAR                    BeaconBuf[256]; // NOTE: BeaconBuf should be 4-byte aligned

    // pre-build PS-POLL and NULL frame upon link up. for efficiency purpose.
    PSPOLL_FRAME            PsPollFrame;
    HEADER_802_11           NullFrame;

    // configuration: read from Registry & E2PROM
    BOOLEAN                 bLocalAdminMAC;                     // Use user changed MAC
    UCHAR                   PermanentAddress[MAC_ADDR_LEN];     // Factory default MAC address
    UCHAR                   CurrentAddress[MAC_ADDR_LEN];       // User changed MAC address

    MLME_STRUCT             Mlme;

    // -----------------------------------------------
    // STA specific configuration
    // -----------------------------------------------
    PORT_CONFIG             PortCfg;            // user desired settings
    ACTIVE_CONFIG           ActiveCfg;             // valid only when ADHOC_ON(pAd) || INFRA_ON(pAd)
    MLME_AUX                MlmeAux;            // temporary settings used during MLME state machine
    BSS_TABLE               ScanTab;            // store the latest SCAN result

    // encryption/decryption KEY tables
    CIPHER_KEY              SharedKey[4];
//  CIPHER_KEY              PairwiseKey[64];        // for AP only

    // Boolean control for packet filter
    BOOLEAN                 bAcceptDirect;
    BOOLEAN                 bAcceptMulticast;
    BOOLEAN                 bAcceptBroadcast;
    BOOLEAN                 bAcceptAllMulticast;
    
    // 802.3 multicast support
    ULONG                   NumberOfMcastAddresses;     // Number of mcast entry exists
    UCHAR                   McastTable[MAX_MCAST_LIST_SIZE][MAC_ADDR_LEN];      // Mcast list


    // RX Tuple chahe for duplicate frame check
	TUPLE_CACHE				TupleCache[MAX_CLIENT];		// Maximum number of tuple caches, only useful in Ad-Hoc
	UCHAR                   TupleCacheLastUpdateIndex;  // 0..MAX_CLIENT-1

	// RX re-assembly buffer for fragmentation
	FRAGMENT_FRAME			FragFrame;					// Frame storage for fragment frame
	
    // various Counters 
    COUNTER_802_3           Counters8023;               // 802.3 counters
    COUNTER_802_11          WlanCounters;				// 802.11 MIB counters
    COUNTER_RALINK          RalinkCounters;				// Ralink propriety counters
    COUNTER_DRS             DrsCounters;                // counters for Dynamic TX Rate Switching
	PRIVATE_STRUC			PrivateInfo;				// Private information & counters

    // flags, see fRTMP_ADAPTER_xxx flags
    ULONG                   Flags;                      // Represent current device status

    // current TX sequence #
	USHORT					Sequence;

    // Control disconnect / connect event generation
    UCHAR		     LinkDownReason;
    ULONG                   LinkDownTime;
    ULONG                   LastRxRate;
    BOOLEAN                 bConfigChanged;         // Config Change flag for the same SSID setting

    ULONG                   ExtraInfo;              // Extra information for displaying status
    ULONG                   SystemErrorBitmap;      // b0: E2PROM version error
    
    // ---------------------------
    // E2PROM
    // ---------------------------
    ULONG                   EepromVersion;          // byte 0: version, byte 1: revision, byte 2~3: unused
    UCHAR					EEPROMAddressNum;		// 93c46=6	93c66=8
    USHORT					EEPROMDefaultValue[NUM_EEPROM_BBP_PARMS];
    ULONG                   FirmwareVersion;        // byte 0: Minor version, byte 1: Major version, otherwise unused.

    // ---------------------------
    // BBP Control
    // ---------------------------
    UCHAR                   BbpWriteLatch[110];     // record last BBP register value written via BBP_IO_WRITE/BBP_IO_WRITE_VY_REG_ID
    UCHAR                   BbpRssiToDbmDelta;
    BBP_R17_TUNING          BbpTuning;

    // ----------------------------
    // RFIC control
    // ----------------------------
    UCHAR                   RfIcType;       // RFIC_xxx
    ULONG					RfFreqOffset;	// Frequency offset for channel switching
	BOOLEAN                 bAutoTxAgc;		// Enable driver auto Tx Agc control
    RTMP_RF_REGS            LatchRfRegs;    // latch th latest RF programming value since RF IC doesn't support READ
//    CCK_TX_POWER_CALIBRATE  CckTxPowerCalibrate;    // 2004-05-25 add CCK TX power caliberation based on E2PROM settings
	RALINK_TIMER_STRUCT	RfTuningTimer;  // some RF IC requires multiple steps programming sequence with

    
	UCHAR					RFProgSeq;
	EEPROM_ANTENNA_STRUC	Antenna;							// Since ANtenna definition is different for a & g. We need to save it for future reference.
	EEPROM_NIC_CONFIG2_STRUC    NicConfig2;
	CHANNEL_TX_POWER		TxPower[MAX_NUM_OF_CHANNELS];		// Store Tx power value for all channels.
	CHANNEL_TX_POWER		ChannelList[MAX_NUM_OF_CHANNELS];   // list all supported channels for site survey
	UCHAR                   ChannelListNum;                     // number of channel in ChannelList[]
    EEPROM_TXPOWER_DELTA_STRUC  TxPowerDeltaConfig;				// Compensate the Tx power BBP94 with this configurate value
	UCHAR					    Bbp94;
	BOOLEAN					    BbpForCCK;
    

    // This soft Rx Antenna Diversity mechanism is used only when user set 
    // RX Antenna = DIVERSITY ON
    SOFT_RX_ANT_DIVERSITY   RxAnt;

    
	BOOLEAN     bAutoTxAgcA;                // Enable driver auto Tx Agc control
	UCHAR	    TssiRefA;					// Store Tssi reference value as 25 tempature.	
	UCHAR	    TssiPlusBoundaryA[5];		// Tssi boundary for increase Tx power to compensate.
	UCHAR	    TssiMinusBoundaryA[5];		// Tssi boundary for decrease Tx power to compensate.
	UCHAR	    TxAgcStepA;					// Store Tx TSSI delta increment / decrement value
	CHAR		TxAgcCompensateA;			// Store the compensation (TxAgcStep * (idx-1))

	BOOLEAN     bAutoTxAgcG;                // Enable driver auto Tx Agc control
	UCHAR	    TssiRefG;					// Store Tssi reference value as 25 tempature.	
	UCHAR	    TssiPlusBoundaryG[5];		// Tssi boundary for increase Tx power to compensate.
	UCHAR	    TssiMinusBoundaryG[5];		// Tssi boundary for decrease Tx power to compensate.
	UCHAR	    TxAgcStepG;					// Store Tx TSSI delta increment / decrement value
	CHAR		TxAgcCompensateG;			// Store the compensation (TxAgcStep * (idx-1))

	UCHAR       IoctlIF;

    
    CHAR	    BGRssiOffset1;				// Store B/G RSSI#1 Offset value on EEPROM 0x9Ah
	CHAR	    BGRssiOffset2;				// Store B/G RSSI#2 Offset value 
	CHAR	    ARssiOffset1;				// Store A RSSI#1 Offset value on EEPROM 0x9Ch
	CHAR	    ARssiOffset2;

    // ----------------------------
    // LED control
    // ----------------------------
    MCU_LEDCS_STRUC     LedCntl;
    UCHAR				LedIndicatorStregth;

    // ----------------------------
    // DEBUG paramerts
    // ----------------------------
    ULONG                   DebugSetting[4];

#ifdef RALINK_ATE
	ATE_INFO				ate;
#endif	// RALINK_ATE

#ifdef THREAD_ISR
	UINT					Rtmp_Masked_Int;	/* global interrupt mask */

	struct tasklet_struct 	rx_done_task;
	struct tasklet_struct 	tx_done_task;
	struct tasklet_struct 	mgmt_done_task;
	struct tasklet_struct 	ac0_dma_done_task;
	struct tasklet_struct 	ac1_dma_done_task;
	struct tasklet_struct 	ac2_dma_done_task;
	struct tasklet_struct 	ac3_dma_done_task;
	struct tasklet_struct 	hcca_dma_done_task;

	BOOLEAN					rx_done_running;
	BOOLEAN					tx_done_running;
	BOOLEAN					mgmt_done_running;
	BOOLEAN					ac0_dma_done_running;
	BOOLEAN					ac1_dma_done_running;
	BOOLEAN					ac2_dma_done_running;
	BOOLEAN					ac3_dma_done_running;
	BOOLEAN					hcca_dma_done_running;

	BOOLEAN					Rx_Early_Done;
#endif /* THREAD_ISR */

	UCHAR scanTabCleanTime;

	UINT debugdata[10];

}   RTMP_ADAPTER, *PRTMP_ADAPTER;



//
// --------------------- AP related -----------------------
//
#define AP_MAX_LEN_OF_RSNIE         90

//for-wpa value domain of pMacEntry->WpaState  802.1i D3   p.114
typedef enum _ApWpaState {
    AS_NOTUSE,              // 0
    AS_DISCONNECT,          // 1
    AS_DISCONNECTED,        // 2
    AS_INITIALIZE,          // 3
    AS_AUTHENTICATION,      // 4
    AS_AUTHENTICATION2,     // 5
    AS_INITPMK,             // 6
    AS_INITPSK,             // 7
    AS_PTKSTART,            // 8
    AS_PTKINIT_NEGOTIATING, // 9
    AS_PTKINITDONE,         // 10
    AS_UPDATEKEYS,          // 11
    AS_INTEGRITY_FAILURE,   // 12
    AS_KEYUPDATE,           // 13
} AP_WPA_STATE;

// for-wpa value domain of pMacEntry->WpaState  802.1i D3   p.114
typedef enum _GTKState {
    REKEY_NEGOTIATING,
    REKEY_ESTABLISHED,
    KEYERROR,
} GTK_STATE;

// Value domain of pMacEntry->Sst
typedef enum _Sst {
    SST_NOT_AUTH,   // 0: equivalent to IEEE 802.11/1999 state 1
    SST_AUTH,       // 1: equivalent to IEEE 802.11/1999 state 2
    SST_ASSOC       // 2: equivalent to IEEE 802.11/1999 state 3
} SST;

// value domain of pMacEntry->AuthState
typedef enum _AuthState {
    AS_NOT_AUTH,
    AS_AUTH_OPEN,       // STA has been authenticated using OPEN SYSTEM
    AS_AUTH_KEY,        // STA has been authenticated using SHARED KEY
    AS_AUTHENTICATING   // STA is waiting for AUTH seq#3 using SHARED KEY
} AUTH_STATE;

typedef struct  _MAC_TABLE_ENTRY {
	BOOLEAN         Valid;
	//jan for wpa
	// record which entry revoke MIC Failure , if it leaves the BSS itself, AP won't update aMICFailTime MIB
	UCHAR           CMTimerRunning;
	UCHAR           RSNIE_Len;
	UCHAR           RSN_IE[AP_MAX_LEN_OF_RSNIE];
	UCHAR           ANonce[32];
	UCHAR           R_Counter[8];
	UCHAR           PTK[64];
	UCHAR           ReTryCounter;   
	RALINK_TIMER_STRUCT				RetryTimer;
	NDIS_802_11_AUTHENTICATION_MODE     AuthMode;   // This should match to whatever microsoft defined
	NDIS_802_11_WEP_STATUS              WepStatus;
	AP_WPA_STATE    WpaState;
	GTK_STATE       GTKState;
	USHORT          PortSecured;
	NDIS_802_11_PRIVACY_FILTER          PrivacyFilter;      // PrivacyFilter enum for 802.1X
	CIPHER_KEY      PairwiseKey;
	PVOID           pAd;

	UCHAR           Addr[MAC_ADDR_LEN];
	UCHAR           PsMode;
	SST             Sst;
	AUTH_STATE      AuthState; // for SHARED KEY authentication state machine used only
	USHORT          Aid;
	USHORT          CapabilityInfo;
	UCHAR           MaxSupportedRate;
	UCHAR           CurrTxRate;
	UCHAR           LastRssi;
	ULONG           NoDataIdleCount;
	ULONG           PsQIdleCount;
	QUEUE_HEADER    PsQueue;
	struct _MAC_TABLE_ENTRY *pNext;

	// to record the each TX rate's quality. 0 is best, the bigger the worse.
	USHORT          TxQuality[MAX_LEN_OF_SUPPORTED_RATES];
	UCHAR           PER[MAX_LEN_OF_SUPPORTED_RATES];
	USHORT          OneSecTxOkCount;
	USHORT          OneSecTxRetryOkCount;
	USHORT          OneSecTxFailCount;
	UCHAR           TxRateUpPenalty;      // extra # of second penalty due to last unstable condition
	ULONG           CurrTxRateStableTime; // # of second in current TX rate
	BOOLEAN         fNoisyEnvironment;
	UCHAR           LastSecTxRateChangeAction; // 0: no change, 1:rate UP, 2:rate down

	// a bitmap of BOOLEAN flags. each bit represent an operation status of a particular 
	// BOOLEAN control, either ON or OFF. These flags should always be accessed via
	// CLIENT_STATUS_TEST_FLAG(), CLIENT_STATUS_SET_FLAG(), CLIENT_STATUS_CLEAR_FLAG() macros.
	// see fOP_STATUS_xxx in RTMP_DEF.C for detail bit definition
	ULONG           ClientStatusFlags;
} MAC_TABLE_ENTRY, *PMAC_TABLE_ENTRY;
// --------------------------------------------------------


extern RTMP_RF_REGS RF5225RegTable[];
extern RTMP_RF_REGS RF5225RegTable_1[];
extern UCHAR NUM_OF_5225_CHNL;
extern UCHAR NUM_OF_5225_CHNL_1;


//
// Prototypes of function definition
//
INT     RT61_close(
    IN  struct net_device *net_dev);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
irqreturn_t RTMPIsr(
    IN  INT             irq, 
    IN  VOID            *dev_instance, 
    IN  struct pt_regs  *rgs);
#else
VOID    RTMPIsr(
    IN  INT             irq, 
    IN  VOID            *dev_instance, 
    IN  struct pt_regs  *rgs);
#endif

INT     RT61_open(
    IN  struct net_device *net_dev);

INT     RTMPSendPackets(
    IN  struct sk_buff *skb, 
    IN  struct net_device *net_dev);

INT     RT61_probe(
    IN  struct pci_dev              *pPci_Dev, 
    IN  const struct pci_device_id  *ent);

INT     RT61_ioctl(
    IN  struct net_device   *net_dev, 
    IN  OUT struct ifreq    *rq, 
    IN  INT                 cmd);

#if WIRELESS_EXT >= 12
struct iw_statistics *RT61_get_wireless_stats(
    IN  struct net_device *net_dev);
#endif

struct net_device_stats *RT61_get_ether_stats(
    IN  struct net_device *net_dev);

VOID    RT61_set_rx_mode(
    IN  struct net_device *net_dev);
    
long rt_abs(long arg);

    
//
// Routines in rtmp_init.c
//
NDIS_STATUS	RTMPAllocAdapterBlock(
	IN	PRTMP_ADAPTER	pAdapter);

NDIS_STATUS RTMPAllocDMAMemory(
    IN  PRTMP_ADAPTER   pAdapter);

VOID    RTMPFreeDMAMemory(
    IN  PRTMP_ADAPTER   pAdapter);

// Enable & Disable NIC interrupt via writing interrupt mask register
// Since it use ADAPTER structure, it have to be put after structure definition.
#ifdef BIG_ENDIAN
inline VOID
#else
VOID
#endif
NICDisableInterrupt(    
    IN  PRTMP_ADAPTER   pAdapter);

#ifdef BIG_ENDIAN
inline VOID
#else
VOID
#endif
NICEnableInterrupt(
    IN  PRTMP_ADAPTER   pAdapter,
    IN	UINT32			int_mask);

VOID	NICInitTxRxRingAndBacklogQueue(
	IN	PRTMP_ADAPTER	pAdapter);

VOID    NICReadEEPROMParameters(
    IN  PRTMP_ADAPTER   pAd);

VOID    NICInitAsicFromEEPROM(
    IN  PRTMP_ADAPTER   pAd);

NDIS_STATUS NICInitializeAdapter(
	IN PRTMP_ADAPTER   pAdapter);

VOID   NICInitializeAsic(
	IN    PRTMP_ADAPTER   pAdapter);

VOID    NICIssueReset(
    IN  PRTMP_ADAPTER   pAdapter);

BOOLEAN NICCheckForHang(
    IN  PRTMP_ADAPTER   pAd);

VOID NICUpdateRawCounters(
    IN PRTMP_ADAPTER pAdapter);

VOID    NICResetFromError(
    IN  PRTMP_ADAPTER   pAdapter);

PUCHAR  RTMPFindSection(
    IN  PCHAR   buffer);

char * rtstrstr(
    IN const char * s1,
    IN const char * s2);

INT RTMPGetKeyParameter(
    IN  PCHAR   key,
    OUT PCHAR   dest,   
    IN  INT     destsize,
    IN  PCHAR   buffer);

// add by johnli
VOID rtmp_read_key_parms_from_file(
    IN    PRTMP_ADAPTER pAd,
    OUT char *tmpbuf,
    IN    char *buffer);
// end johnli

VOID RTMPReadParametersFromFile(
    IN  PRTMP_ADAPTER pAd);

NDIS_STATUS NICLoadFirmware(
    IN PRTMP_ADAPTER pAd);

VOID    RTMPRingCleanUp(
    IN  PRTMP_ADAPTER   pAdapter,
    IN  UCHAR           RingType);
    
#ifndef BIG_ENDIAN
ULONG   RTMPEqualMemory(
    IN  PVOID   pSrc1,
    IN  PVOID   pSrc2,
    IN  ULONG   Length);
#endif

ULONG   RTMPCompareMemory(
    IN  PVOID   pSrc1,
    IN  PVOID   pSrc2,
    IN  ULONG   Length);

VOID    NdisZeroMemory(
    IN  PVOID   pSrc,
    IN  ULONG   Length);

VOID    NdisFillMemory(
    IN  PVOID   pSrc,
    IN  ULONG   Length,
    IN  UCHAR   Fill);

VOID    NdisMoveMemory(
    OUT PVOID   pDest,
    IN  PVOID   pSrc,
    IN  ULONG   Length);

VOID	PortCfgInit(
	IN	PRTMP_ADAPTER pAd);

UCHAR BtoH(
	IN CHAR		ch);

VOID AtoH(
	IN CHAR		*src,
	OUT UCHAR	*dest,
	IN INT		destlen);

VOID	RTMPusecDelay(
	IN	ULONG	usec);


VOID RTMPSetLED(
	IN PRTMP_ADAPTER 	pAd, 
	IN UCHAR			Status);

VOID RTMPSetSignalLED(
	IN PRTMP_ADAPTER 	pAd, 
	IN NDIS_802_11_RSSI Dbm);

//
// MLME routines
//

// Asic/RF/BBP related functions
VOID AsicSwitchChannel(
    IN PRTMP_ADAPTER pAd, 
    IN UCHAR Channel);

VOID AsicLockChannel(
    IN PRTMP_ADAPTER pAd, 
    IN UCHAR Channel);
    
VOID	AsicAntennaSelect(
    IN	PRTMP_ADAPTER	pAd,
    IN	UCHAR			Channel); 

VOID	AsicAntennaSetting(
    IN	PRTMP_ADAPTER	pAd,
    IN	ABGBAND_STATE	BandState);

VOID AsicRfTuningExec(
    IN	unsigned long data);
    
VOID AsicAdjustTxPower(
    IN PRTMP_ADAPTER pAd); 

VOID AsicSleepThenAutoWakeup(
    IN PRTMP_ADAPTER pAd, 
    IN USHORT TbttNumToNextWakeUp); 

VOID AsicForceSleep(
    IN PRTMP_ADAPTER pAd);

VOID AsicForceWakeup(
    IN PRTMP_ADAPTER pAd);

VOID AsicSetBssid(
    IN PRTMP_ADAPTER pAd, 
    IN PUCHAR pBssid); 

VOID AsicDisableSync(
    IN PRTMP_ADAPTER pAd); 

VOID AsicEnableBssSync(
    IN PRTMP_ADAPTER pAd); 

VOID AsicEnableIbssSync(
    IN PRTMP_ADAPTER pAd);

VOID AsicSetEdcaParm(
    IN PRTMP_ADAPTER pAd,
    IN PEDCA_PARM    pEdcaParm);

VOID AsicSetSlotTime(
    IN PRTMP_ADAPTER pAd,
    IN BOOLEAN bUseShortSlotTime); 

VOID AsicBbpTuning(
    IN PRTMP_ADAPTER pAd);

VOID AsicAddSharedKeyEntry(
    IN PRTMP_ADAPTER pAd,
    IN UCHAR         BssIndex,
    IN UCHAR         KeyIdx,
    IN UCHAR         CipherAlg,
    IN PUCHAR        pKey,
    IN PUCHAR        pTxMic,
    IN PUCHAR        pRxMic);

VOID AsicRemoveSharedKeyEntry(
    IN PRTMP_ADAPTER pAd,
    IN UCHAR         BssIndex,
    IN UCHAR         KeyIdx);

VOID AsicAddPairwiseKeyEntry(
    IN PRTMP_ADAPTER pAd,
    IN PUCHAR        pAddr,
    IN UCHAR         KeyIdx,
    IN UCHAR         CipherAlg,
    IN PUCHAR        pKey,
    IN PUCHAR        pTxMic,
    IN PUCHAR        pRxMic);

VOID AsicRemovePairwiseKeyEntry(
    IN PRTMP_ADAPTER pAd,
    IN UCHAR         KeyIdx);

BOOLEAN AsicSendCommandToMcu(
    IN PRTMP_ADAPTER pAd,
    IN UCHAR         Command,
    IN UCHAR         Token,
    IN UCHAR         Arg0,
    IN UCHAR         Arg1);

VOID	RTMPCheckRates(
	IN		PRTMP_ADAPTER	pAd,
	IN OUT	UCHAR			SupRate[],
	IN OUT	UCHAR			*SupRateLen);
 
VOID AsicSetRxAnt(
    IN PRTMP_ADAPTER	pAd,
    IN UCHAR			Pair1,
    IN UCHAR			Pair2);

VOID AsicEvaluateSecondaryRxAnt(
    IN PRTMP_ADAPTER pAd);

VOID AsicRxAntEvalTimeout(
    IN	unsigned long data);

VOID MlmeRssiReportExec(
    IN	unsigned long data);
    
VOID StaQuickResponeForRateUpExec(
    IN	unsigned long data);

VOID RadarDetectionStart(
    IN PRTMP_ADAPTER	pAd);

BOOLEAN RadarDetectionStop(
    IN PRTMP_ADAPTER	pAd);
	
BOOLEAN RadarChannelCheck(
    IN PRTMP_ADAPTER	pAd,
    IN UCHAR			Ch);

VOID RTMPSetPiggyBack(
	IN PRTMP_ADAPTER	pAd,
	IN PMAC_TABLE_ENTRY pEntry,
	IN BOOLEAN			bPiggyBack);

VOID BssTableInit(
    IN BSS_TABLE *Tab); 

ULONG BssTableSearch(
    IN BSS_TABLE *Tab, 
    IN PUCHAR    pBssid,
    IN UCHAR     Channel); 

ULONG BssSsidTableSearch(
	IN BSS_TABLE *Tab, 
	IN PUCHAR    pBssid,
	IN PUCHAR    pSsid,
	IN UCHAR     SsidLen,
	IN UCHAR     Channel); 

ULONG BssTableSearchWithSSID(
	IN BSS_TABLE *Tab, 
	IN PUCHAR    Bssid,
	IN PUCHAR    pSsid,
	IN UCHAR     SsidLen,
	IN UCHAR     Channel);
	
VOID BssTableDeleteEntry(
	IN OUT	BSS_TABLE *Tab, 
	IN		PUCHAR    pBssid,
	IN      UCHAR     Channel); 

VOID BssEntrySet(
    IN  PRTMP_ADAPTER   pAd, 
    OUT BSS_ENTRY *pBss, 
    IN PUCHAR pBssid, 
    IN CHAR Ssid[], 
    IN UCHAR SsidLen, 
    IN UCHAR BssType, 
    IN USHORT BeaconPeriod, 
    IN PCF_PARM pCfParm, 
    IN USHORT AtimWin, 
    IN USHORT CapabilityInfo, 
    IN UCHAR SupRate[], 
    IN UCHAR SupRateLen,
    IN UCHAR ExtRate[], 
    IN UCHAR ExtRateLen,
    IN UCHAR Channel,
    IN UCHAR Rssi,
    IN LARGE_INTEGER TimeStamp,
    IN UCHAR CkipFlag,
	IN PEDCA_PARM pEdcaParm,
	IN PQOS_CAPABILITY_PARM pQosCapability,
	IN PQBSS_LOAD_PARM pQbssLoad,
//	IN UCHAR LengthVIE,
	IN USHORT LengthVIE,  // edit by johnli, variable ie length could be > 256
    IN PNDIS_802_11_VARIABLE_IEs pVIE);  

ULONG BssTableSetEntry(
    IN  PRTMP_ADAPTER   pAd, 
    OUT BSS_TABLE *Tab, 
    IN PUCHAR pBssid, 
    IN CHAR Ssid[], 
    IN UCHAR SsidLen, 
    IN UCHAR BssType, 
    IN USHORT BeaconPeriod, 
    IN CF_PARM *CfParm, 
    IN USHORT AtimWin, 
    IN USHORT CapabilityInfo, 
    IN UCHAR SupRate[],
    IN UCHAR SupRateLen,
    IN UCHAR ExtRate[],
    IN UCHAR ExtRateLen,
    IN UCHAR ChannelNo,
    IN UCHAR Rssi,
    IN LARGE_INTEGER TimeStamp,
    IN UCHAR CkipFlag,
	IN PEDCA_PARM pEdcaParm,
	IN PQOS_CAPABILITY_PARM pQosCapability,
	IN PQBSS_LOAD_PARM pQbssLoad,
//	IN UCHAR LengthVIE,
	IN USHORT LengthVIE,  // edit by johnli, variable ie length could be > 256
    IN PNDIS_802_11_VARIABLE_IEs pVIE); 

VOID BssTableSsidSort(
    IN	PRTMP_ADAPTER	pAd, 
    OUT BSS_TABLE *OutTab, 
    IN  CHAR Ssid[], 
    IN  UCHAR SsidLen); 

VOID BssTableSortByRssi(
    IN OUT BSS_TABLE *OutTab); 

VOID BssCipherParse(
	IN OUT	PBSS_ENTRY	pBss);

VOID MacAddrRandomBssid(
    IN PRTMP_ADAPTER pAd, 
    OUT PUCHAR pAddr);

VOID MgtMacHeaderInit(
    IN	PRTMP_ADAPTER	pAd, 
    IN OUT PHEADER_802_11 pHdr80211, 
    IN UCHAR SubType, 
    IN UCHAR ToDs, 
    IN PUCHAR pDA, 
    IN PUCHAR pBssid); 

ULONG MakeOutgoingFrame(
    OUT CHAR *Buffer, 
    OUT ULONG *FrameLen, ...);

NDIS_STATUS MlmeQueueInit(
    IN MLME_QUEUE *Queue); 

BOOLEAN MlmeEnqueue(
	IN	PRTMP_ADAPTER	pAd,
    IN ULONG Machine, 
    IN ULONG MsgType, 
    IN ULONG MsgLen, 
    IN VOID *Msg); 

BOOLEAN MlmeEnqueueForRecv(
    IN	PRTMP_ADAPTER	pAd, 
    IN	PFRAME_802_11	p80211Frame,  // add by johnli, fix WPS & WPAPSK/WPA2PSK bugs for receiving EAPoL fragmentation packets
    IN ULONG TimeStampHigh, 
    IN ULONG TimeStampLow,
    IN UCHAR Rssi, 
    IN ULONG MsgLen, 
    IN VOID *Msg,
    IN UCHAR Signal);

BOOLEAN MlmeDequeue(
    IN MLME_QUEUE *Queue, 
    OUT MLME_QUEUE_ELEM **Elem); 

VOID	MlmeRestartStateMachine(
    IN	PRTMP_ADAPTER	pAd);

BOOLEAN MlmeQueueEmpty(
    IN MLME_QUEUE *Queue); 

BOOLEAN MlmeQueueFull(
    IN MLME_QUEUE *Queue); 

VOID MlmeQueueDestroy(
    IN MLME_QUEUE *pQueue); 

BOOLEAN MsgTypeSubst(
    IN PRTMP_ADAPTER  pAd,
    // edit by johnli, fix WPS & WPAPSK/WPA2PSK bugs for receiving EAPoL fragmentation packets
//    IN PFRAME_802_11 pFrame, 
    IN PFRAME_802_11 p80211hdr, 
    IN PUCHAR pFrame, 
    // end johnli
    OUT INT *Machine, 
    OUT INT *MsgType); 

VOID StateMachineInit(
    IN STATE_MACHINE *S, 
    IN STATE_MACHINE_FUNC Trans[], 
    IN ULONG StNr, 
    IN ULONG MsgNr, 
    IN STATE_MACHINE_FUNC DefFunc, 
    IN ULONG InitState, 
    IN ULONG Base); 

VOID StateMachineSetAction(
    IN STATE_MACHINE *S, 
    IN ULONG St, 
    IN ULONG Msg, 
    IN STATE_MACHINE_FUNC Func); 

VOID StateMachinePerformAction(
    IN	PRTMP_ADAPTER	pAd, 
    IN STATE_MACHINE *S, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID Drop(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID LfsrInit(
    IN PRTMP_ADAPTER pAd, 
    IN unsigned long Seed); 

UCHAR RandomByte(
    IN PRTMP_ADAPTER pAd);

NDIS_STATUS MlmeInit(
    IN PRTMP_ADAPTER pAd); 

VOID MlmeHandler(
    IN PRTMP_ADAPTER pAd); 

VOID MlmeHalt(
    IN PRTMP_ADAPTER pAd); 

VOID MlmePeriodicExec(
    IN	unsigned long data);

VOID STAMlmePeriodicExec(
    PRTMP_ADAPTER pAd);

VOID LinkDownExec(
    IN	unsigned long data);

VOID MlmeAutoScan(
    IN PRTMP_ADAPTER pAd);
    
VOID MlmeAutoRecoverNetwork(
    IN PRTMP_ADAPTER pAd);

VOID MlmeAutoReconnectLastSSID(
    IN PRTMP_ADAPTER pAd);

BOOLEAN	MlmeValidateSSID(
    IN PUCHAR   pSsid,
    IN UCHAR    SsidLen);

VOID MlmeCheckForRoaming(
    IN PRTMP_ADAPTER pAd,
    IN ULONG    Now32);

VOID MlmeCheckForFastRoaming(
	IN	PRTMP_ADAPTER	pAd,
	IN	unsigned long   Now);

VOID MlmeCalculateChannelQuality(
    IN PRTMP_ADAPTER pAd,
    IN ULONG Now32);

VOID MlmeDynamicTxRateSwitching(
    IN PRTMP_ADAPTER pAd);

VOID MlmeCheckPsmChange(
    IN PRTMP_ADAPTER pAd,
    IN ULONG    Now32);

VOID MlmeSetPsmBit(
    IN PRTMP_ADAPTER pAd, 
    IN USHORT psm);

VOID MlmeSetTxPreamble(
    IN PRTMP_ADAPTER pAd, 
    IN USHORT TxPreamble);

VOID MlmeUpdateTxRates(
    IN PRTMP_ADAPTER pAd,
    IN BOOLEAN		 bLinkUp);

VOID MlmeRadioOff(
    IN PRTMP_ADAPTER pAd);

VOID MlmeRadioOn(
    IN PRTMP_ADAPTER pAd);

VOID AssocStateMachineInit(
    IN	PRTMP_ADAPTER	pAd, 
    IN  STATE_MACHINE *S, 
    OUT STATE_MACHINE_FUNC Trans[]);
    
VOID AssocTimeout(
    IN	unsigned long data);

VOID ReassocTimeout(
    IN	unsigned long data);

VOID DisassocTimeout(
    IN	unsigned long data);

VOID MlmeAssocReqAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem);

VOID MlmeReassocReqAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem);

VOID MlmeDisassocReqAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem);

VOID PeerAssocRspAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem);

VOID PeerReassocRspAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem);

VOID AssocPostProc(
    IN PRTMP_ADAPTER pAd, 
    IN PUCHAR pAddr2, 
    IN USHORT CapabilityInfo, 
    IN USHORT Aid, 
    IN UCHAR SupRate[], 
    IN UCHAR SupRateLen,
    IN UCHAR ExtRate[],
    IN UCHAR ExtRateLen,
    IN PEDCA_PARM pEdcaParm);

VOID PeerDisassocAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem);

VOID AssocTimeoutAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem);

VOID ReassocTimeoutAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem);

VOID DisassocTimeoutAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem);

VOID InvalidStateWhenAssoc(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID InvalidStateWhenReassoc(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 
    
VOID InvalidStateWhenDisassociate(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID Cls3errAction(
    IN PRTMP_ADAPTER pAd, 
    IN PUCHAR      pAddr); 

VOID AuthStateMachineInit(
    IN PRTMP_ADAPTER pAd, 
    IN PSTATE_MACHINE Sm, 
    OUT STATE_MACHINE_FUNC Trans[]);

VOID AuthTimeout(
    IN	unsigned long data);

VOID MlmeAuthReqAction(
    IN  PRTMP_ADAPTER   pAd, 
    IN  MLME_QUEUE_ELEM *Elem);

VOID PeerAuthRspAtSeq2Action(
    IN  PRTMP_ADAPTER   pAd, 
    IN  MLME_QUEUE_ELEM *Elem);

VOID PeerAuthRspAtSeq4Action(
    IN  PRTMP_ADAPTER   pAd, 
    IN  MLME_QUEUE_ELEM *Elem);

VOID AuthTimeoutAction(
    IN  PRTMP_ADAPTER   pAd, 
    IN  MLME_QUEUE_ELEM *Elem);

VOID Cls2errAction(
    IN  PRTMP_ADAPTER   pAd, 
    IN  PUCHAR pAddr);

VOID MlmeDeauthReqAction(
    IN  PRTMP_ADAPTER   pAd, 
    IN  MLME_QUEUE_ELEM *Elem);

VOID InvalidStateWhenAuth(
    IN  PRTMP_ADAPTER   pAd, 
    IN  MLME_QUEUE_ELEM *Elem);

VOID AuthRspStateMachineInit(
    IN PRTMP_ADAPTER pAd, 
    IN PSTATE_MACHINE Sm, 
    IN STATE_MACHINE_FUNC Trans[]);
    
VOID PeerAuthSimpleRspGenAndSend(
    IN PRTMP_ADAPTER pAd, 
    IN PHEADER_802_11 pHdr80211,
    IN USHORT Alg, 
    IN USHORT Seq, 
    IN USHORT Reason, 
    IN USHORT Status);

VOID PeerDeauthAction(
    IN PRTMP_ADAPTER pAd, 
    IN PMLME_QUEUE_ELEM Elem);
    
VOID MlmeCntlInit(
    IN PRTMP_ADAPTER pAd, 
    IN STATE_MACHINE *S, 
    OUT STATE_MACHINE_FUNC Trans[]); 

VOID MlmeCntlMachinePerformAction(
    IN PRTMP_ADAPTER pAd, 
    IN STATE_MACHINE *S, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID CntlIdleProc(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID CntlOidScanProc(
    IN PRTMP_ADAPTER pAd,
    IN MLME_QUEUE_ELEM *Elem);
    
VOID CntlOidSsidProc(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM * Elem); 

VOID CntlOidRTBssidProc(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM * Elem); 

VOID CntlMlmeRoamingProc(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID CntlWaitDisassocProc(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID CntlWaitJoinProc(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID CntlWaitStartProc(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID CntlWaitAuthProc(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID CntlWaitAuthProc2(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID CntlWaitAssocProc(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID CntlWaitReassocProc(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID SendSingalToDaemon(
		IN PRTMP_ADAPTER	pAd);

VOID LinkUp(
    IN PRTMP_ADAPTER pAd,
    IN UCHAR BssType); 

VOID LinkDown(
    IN PRTMP_ADAPTER pAd,
    IN  BOOLEAN      IsReqFromAP);

VOID IterateOnBssTab(
    IN PRTMP_ADAPTER pAd); 

VOID IterateOnBssTab2(
    IN PRTMP_ADAPTER pAd); 

VOID JoinParmFill(
    IN PRTMP_ADAPTER pAd, 
    IN OUT MLME_JOIN_REQ_STRUCT *JoinReq, 
    IN ULONG BssIdx); 

VOID AssocParmFill(
    IN PRTMP_ADAPTER pAd, 
    IN OUT MLME_ASSOC_REQ_STRUCT *AssocReq, 
    IN PUCHAR                     pAddr, 
    IN USHORT                     CapabilityInfo, 
    IN ULONG                      Timeout, 
    IN USHORT                     ListenIntv); 

VOID ScanParmFill(
    IN PRTMP_ADAPTER pAd, 
    IN OUT MLME_SCAN_REQ_STRUCT *ScanReq, 
    IN CHAR Ssid[], 
    IN UCHAR SsidLen, 
    IN UCHAR BssType, 
    IN UCHAR ScanType); 

VOID DisassocParmFill(
    IN PRTMP_ADAPTER pAd, 
    IN OUT MLME_DISASSOC_REQ_STRUCT *DisassocReq, 
    IN PUCHAR pAddr, 
    IN USHORT Reason); 

VOID StartParmFill(
    IN PRTMP_ADAPTER pAd, 
    IN OUT MLME_START_REQ_STRUCT *StartReq, 
    IN CHAR Ssid[], 
    IN UCHAR SsidLen); 

VOID AuthParmFill(
    IN PRTMP_ADAPTER pAd, 
    IN OUT MLME_AUTH_REQ_STRUCT *AuthReq, 
    IN PUCHAR pAddr, 
    IN USHORT Alg); 

VOID ComposePsPoll(
    IN PRTMP_ADAPTER pAd);

VOID ComposeNullFrame(
    IN PRTMP_ADAPTER pAd);

ULONG MakeIbssBeacon(
    IN PRTMP_ADAPTER pAd);

//
// Private routines in Sync.c
//
VOID SyncStateMachineInit(
    IN PRTMP_ADAPTER pAd, 
    IN STATE_MACHINE *Sm, 
    OUT STATE_MACHINE_FUNC Trans[]); 

VOID BeaconTimeout(
    IN  unsigned long data); 

VOID ScanTimeout(
    IN  unsigned long data) ;

VOID MlmeScanReqAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem);

VOID MlmeJoinReqAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID MlmeStartReqAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID PeerBeaconAtScanAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID PeerBeaconAtJoinAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID PeerBeacon(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 
 
VOID PeerProbeReqAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID BeaconTimeoutAtJoinAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID ScanTimeoutAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID ScanNextChannel(
	IN PRTMP_ADAPTER pAd);

VOID InvalidStateWhenScan(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID InvalidStateWhenJoin(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID InvalidStateWhenStart(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem); 

VOID EnqueuePsPoll(
    IN PRTMP_ADAPTER pAd); 

VOID EnqueueBeaconFrame(
    IN PRTMP_ADAPTER pAd); 

VOID EnqueueProbeRequest(
    IN PRTMP_ADAPTER pAd); 

VOID BuildChannelList(
	IN PRTMP_ADAPTER pAd);

UCHAR NextChannel(
    IN PRTMP_ADAPTER pAd, 
    IN UCHAR channel);

UCHAR FirstChannel(
    IN PRTMP_ADAPTER pAd);

CHAR	ConvertToRssi(
    IN PRTMP_ADAPTER pAd,
	IN	UCHAR	Rssi,
	IN  UCHAR   RssiNumber);

//
// prototypes in sanity.c
//
BOOLEAN MlmeScanReqSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT UCHAR *pBssType, 
    OUT CHAR Ssid[], 
    OUT UCHAR *pSsidLen, 
    OUT UCHAR *pScanType); 

BOOLEAN MlmeStartReqSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT CHAR Ssid[], 
    OUT UCHAR *pSsidLen); 

BOOLEAN MlmeAssocReqSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pApAddr, 
    OUT USHORT *pCapabilityInfo, 
    OUT ULONG *pTimeout, 
    OUT USHORT *pListenIntv); 

BOOLEAN MlmeAuthReqSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr, 
    OUT ULONG *pTimeout, 
    OUT USHORT *pAlg); 

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
    OUT PEDCA_PARM pEdcaParm);

BOOLEAN PeerDisassocSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr2, 
    OUT USHORT *pReason); 

BOOLEAN PeerDeauthSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr2, 
    OUT USHORT *pReason); 

BOOLEAN PeerAuthSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr, 
    OUT USHORT *pAlg, 
    OUT USHORT *pSeq, 
    OUT USHORT *pStatus, 
    CHAR *pChlgText); 

BOOLEAN PeerProbeReqSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr2,
    OUT CHAR Ssid[], 
    OUT UCHAR *pSsidLen); 

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
    OUT	PNDIS_802_11_VARIABLE_IEs pVIE);

BOOLEAN GetTimBit(
    IN CHAR *Ptr, 
    IN USHORT Aid, 
    OUT UCHAR *TimLen, 
    OUT UCHAR *BcastFlag, 
    OUT UCHAR *DtimCount, 
    OUT UCHAR *DtimPeriod,
    OUT UCHAR *MessageToMe); 

UCHAR ChannelSanity(
    IN PRTMP_ADAPTER pAd, 
    IN UCHAR channel);

NDIS_802_11_NETWORK_TYPE NetworkTypeInUseSanity(
    IN PBSS_ENTRY pBss);

NDIS_STATUS	RTMPWPAWepKeySanity(
	IN	PRTMP_ADAPTER	pAd,
	IN	PVOID			pBuf);

UCHAR PeerTxTypeInUseSanity(
	IN UCHAR  Channel,
	IN UCHAR  SupRate[],
	IN UCHAR  SupRateLen,
	IN UCHAR  ExtRate[],
	IN UCHAR  ExtRateLen);
	
//
// prototypes in eeprom.c
//
VOID RaiseClock(
    IN  PRTMP_ADAPTER   pAd,
    IN  ULONG *x);

VOID LowerClock(
    IN  PRTMP_ADAPTER   pAd,
    IN  ULONG *x);

USHORT ShiftInBits(
    IN  PRTMP_ADAPTER   pAd);

VOID ShiftOutBits(
    IN  PRTMP_ADAPTER   pAd,
    IN  USHORT data,
    IN  USHORT count);

VOID EEpromCleanup(
    IN  PRTMP_ADAPTER   pAd);

VOID EWDS(
    IN  PRTMP_ADAPTER   pAd);

VOID EWEN(
    IN  PRTMP_ADAPTER   pAd);
    
USHORT RTMP_EEPROM_READ16(
    IN  PRTMP_ADAPTER   pAd,
    IN  USHORT Offset);

VOID RTMP_EEPROM_WRITE16(
    IN  PRTMP_ADAPTER   pAd,
    IN  USHORT Offset,
    IN  USHORT Data);

//
// Private routines in rtmp_data.c
//
VOID REPORT_ETHERNET_FRAME_TO_LLC(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			p8023hdr,
	IN	PUCHAR			pData,
	IN	ULONG			DataSize,
	IN	struct net_device	*net_dev);

VOID REPORT_ETHERNET_FRAME_TO_LLC_WITH_NON_COPY(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			p8023hdr,
	IN	PUCHAR			pData,
	IN	ULONG			DataSize,
	IN	struct net_device	*net_dev);

VOID REPORT_AGGREGATE_ETHERNET_FRAME_TO_LLC_WITH_NON_COPY(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			p8023hdr,
	IN	PUCHAR			pData,
	IN	ULONG			DataSize1,
	IN	ULONG			DataSize2,
	IN	struct net_device	*net_dev);

VOID	RTMPHandleRxDoneInterrupt(
	IN	PRTMP_ADAPTER	pAd);

VOID	RTMPHandleTxDoneInterrupt(
	IN	PRTMP_ADAPTER	pAd);

VOID    RTMPHandleTxRingDmaDoneInterrupt(
	IN	PRTMP_ADAPTER	pAdapter,
	IN  INT_SOURCE_CSR_STRUC TxRingBitmap);

VOID	RTMPHandleMgmtRingDmaDoneInterrupt(
	IN	PRTMP_ADAPTER	pAdapter);

VOID    RTMPHandleTBTTInterrupt(
    IN PRTMP_ADAPTER pAd);

VOID    RTMPHandleTwakeupInterrupt(
    IN PRTMP_ADAPTER pAd);

NDIS_STATUS	MiniportMMRequest(
	IN	PRTMP_ADAPTER	pAdapter,
	IN	PUCHAR			pData,
	IN	UINT			Length);

NDIS_STATUS	MlmeHardTransmit(
	IN	PRTMP_ADAPTER	pAdapter,
	IN  struct sk_buff	*pSkb);

BOOLEAN TxFrameIsAggregatible(
    IN  PRTMP_ADAPTER   pAd,
    IN  PUCHAR          pPrevAddr1,
    IN  PUCHAR          p8023hdr);

NDIS_STATUS Sniff2BytesFromNdisBuffer(
    IN  struct sk_buff	*pFirstSkb,
    IN  UCHAR           DesiredOffset,
    OUT PUCHAR          pByte0,
    OUT PUCHAR          pByte1);

VOID	RTMPDeQueuePacket(
	IN	PRTMP_ADAPTER	pAdapter,
	IN	UCHAR		Index);

NDIS_STATUS	RTMPSendPacket(
	IN	PRTMP_ADAPTER	pAd,
	IN  struct sk_buff  *pSkb);

NDIS_STATUS	RTMPFreeTXDRequest(
	IN		PRTMP_ADAPTER	pAdapter,
	IN		UCHAR			QueIdx,
	IN		UCHAR			NumberRequired,
	IN 		PUCHAR			FreeNumberIs);

VOID	RTMPSendNullFrame(
	IN	PRTMP_ADAPTER	pAd,
	IN  UCHAR           TxRate,
	IN	BOOLEAN			bQosNull);

VOID	RTMPSendRTSCTSFrame(
	IN	PRTMP_ADAPTER	pAd,
	IN	PUCHAR			pDA,
	IN	unsigned int	NextMpduSize,
	IN  UCHAR           TxRate,
	IN	UCHAR			RTSRate,
	IN  USHORT          AckDuration,
	IN  UCHAR           QueIdx,
	IN	UCHAR			FrameGap,
	IN	UCHAR			Type);
	
#ifndef BIG_ENDIAN
NDIS_STATUS	RTMPHardTransmit(
	IN	PRTMP_ADAPTER	pAd,
	IN	struct sk_buff  *pSkb,
	IN  UCHAR           QueIdx);
#endif

USHORT	RTMPCalcDuration(
	IN	PRTMP_ADAPTER	pAdapter,
	IN	UCHAR			Rate,
	IN	ULONG			Size);

VOID	RTMPWriteTxDescriptor(
    IN  PRTMP_ADAPTER pAd,
 	IN	PTXD_STRUC	pSourceTxD,
	IN	UCHAR		CipherAlg,
	IN  UCHAR       KeyTable,
	IN  UCHAR       KeyIdx,
	IN	BOOLEAN		Ack,
	IN	BOOLEAN		Fragment,
	IN  BOOLEAN     InsTimestamp,
	IN	UCHAR		RetryMode,
	IN	UCHAR		Ifs,
	IN	UINT		Rate,
	IN	ULONG		Length,
	IN  UCHAR       QueIdx,
	IN  UCHAR       PID,
	IN	BOOLEAN		bPiggyBack,
	IN	BOOLEAN		bAfterRTSCTS,
	IN	BOOLEAN		bBurstMode);

BOOLEAN	RTMPSearchTupleCache(
	IN	PRTMP_ADAPTER	pAdapter,
	IN	PHEADER_802_11	pHeader);

VOID	RTMPUpdateTupleCache(
	IN	PRTMP_ADAPTER	pAdapter,
	IN	PHEADER_802_11	pHeader);

VOID    RTMPSuspendMsduTransmission(
	IN	PRTMP_ADAPTER	pAd);

VOID    RTMPResumeMsduTransmission(
	IN	PRTMP_ADAPTER	pAd);

NDIS_STATUS	RTMPCheckRxError(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PHEADER_802_11	pHeader,
	IN	PRXD_STRUC		pRxD);

NDIS_STATUS	RTMPApplyPacketFilter(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PRXD_STRUC		pRxD, 
	IN	PHEADER_802_11	pHeader);

PQUEUE_HEADER	RTMPCheckTxSwQueue(
	IN	PRTMP_ADAPTER	pAdapter,
	OUT	PULONG		pNumber,
	OUT 	PUCHAR          pQueIdx);

VOID	RTMPReportMicError(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PCIPHER_KEY		pWpaKey);

NDIS_STATUS RTMPCloneNdisPacket(
    IN  PRTMP_ADAPTER   pAdapter,
    IN  struct sk_buff  *pInSkb,
    OUT struct sk_buff  **ppOutSkb);

NDIS_STATUS RTMPAllocateNdisPacket(
    IN  PRTMP_ADAPTER   pAdapter,
    OUT struct sk_buff  **ppSkb,
    IN  PUCHAR          pData,
    IN  UINT            DataLen);

VOID DBGPRINT_TX_RING(
    IN PRTMP_ADAPTER  pAdapter,
    IN UCHAR          QueIdx);

VOID RTMPFreeTXDUponTxDmaDone(
    IN PRTMP_ADAPTER    pAd, 
    IN UCHAR            QueIdx,
    IN UINT             RingSize);

BOOLEAN RTMPCheckDHCPFrame(
	IN	PRTMP_ADAPTER	pAd, 
	IN  struct sk_buff	*pSkb);

VOID RTMPCckBbpTuning(
	IN	PRTMP_ADAPTER	pAd, 
	IN	UINT			TxRate);


//
// prototypes in rtmp_wep.c
//
VOID    RTMPInitWepEngine(
    IN  PRTMP_ADAPTER   pAdapter,   
    IN  PUCHAR          pKey,
    IN  UCHAR           KeyId,
    IN  UCHAR           KeyLen, 
    IN OUT  PUCHAR      pDest);

VOID    RTMPEncryptData(
    IN  PRTMP_ADAPTER   pAdapter,   
    IN  PUCHAR          pSrc,
    IN  PUCHAR          pDest,
    IN  UINT            Len);

VOID    ARCFOUR_INIT(
    IN  PARCFOURCONTEXT Ctx,
    IN  PUCHAR          pKey,
    IN  UINT            KeyLen);

UCHAR   ARCFOUR_BYTE(
    IN  PARCFOURCONTEXT     Ctx);

VOID    ARCFOUR_DECRYPT(
    IN  PARCFOURCONTEXT Ctx,
    IN  PUCHAR          pDest, 
    IN  PUCHAR          pSrc,
    IN  UINT            Len);

VOID    ARCFOUR_ENCRYPT(
    IN  PARCFOURCONTEXT Ctx,
    IN  PUCHAR          pDest,
    IN  PUCHAR          pSrc,
    IN  UINT            Len);

ULONG   RTMP_CALC_FCS32(
    IN  ULONG   Fcs,
    IN  PUCHAR  Cp,
    IN  INT     Len);

VOID    RTMPSetICV(
    IN  PRTMP_ADAPTER   pAdapter,
    IN  PUCHAR  pDest);
    
//
// prototypes in rtmp_tkip.c
//
VOID    RTMPInitTkipEngine(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          pTKey,
    IN  UCHAR           KeyId,
    IN  PUCHAR          pTA,
    IN  PUCHAR          pMICKey,
    IN  PUCHAR          pTSC,
    OUT PULONG          pIV16,
    OUT PULONG          pIV32);

VOID	RTMPInitMICEngine(
	IN	PRTMP_ADAPTER	pAdapter,	
	IN	PUCHAR			pKey,
	IN	PUCHAR			pDA,
	IN	PUCHAR			pSA,
	IN  UCHAR           UserPriority,
	IN	PUCHAR			pMICKey);

BOOLEAN RTMPTkipCompareMICValue(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          pSrc,
    IN  PUCHAR          pDA,
    IN  PUCHAR          pSA,
    IN  PUCHAR          pMICKey,
    IN  UINT            Len);

VOID	RTMPCalculateMICValue(
	IN	PRTMP_ADAPTER	pAdapter,
	IN	struct sk_buff  *pSkb,
	IN	PUCHAR			pEncap,
	IN	PCIPHER_KEY		pKey,
	IN	UCHAR			apidx);  //Add by Zero:Jul.05.2007 "apidx for ETH_CONVERT_SUPPORT"

BOOLEAN	RTMPTkipCompareMICValueWithLLC(
	IN	PRTMP_ADAPTER	pAdapter,
	IN	PUCHAR			pLLC,
	IN	PUCHAR			pSrc,
	IN	PUCHAR			pDA,
	IN	PUCHAR			pSA,
	IN	PUCHAR			pMICKey,
	IN	UINT			Len);

VOID    RTMPTkipAppend( 
    IN  PTKIP_KEY_INFO  pTkip,  
    IN  PUCHAR          pSrc,
    IN  UINT            nBytes);

VOID    RTMPTkipGetMIC( 
    IN  PTKIP_KEY_INFO  pTkip);

//
// prototypes in wpa.c
//
BOOLEAN WpaMsgTypeSubst(
    IN  UCHAR   EAPType,
    OUT ULONG   *MsgType);

VOID WpaPskStateMachineInit(
    IN  PRTMP_ADAPTER       pAd, 
    IN  STATE_MACHINE       *S, 
    OUT STATE_MACHINE_FUNC Trans[]);

VOID WpaEAPOLKeyAction(
    IN  PRTMP_ADAPTER   pAd, 
    IN  MLME_QUEUE_ELEM *Elem);

VOID    WpaPairMsg1Action(
    IN  PRTMP_ADAPTER   pAd, 
    IN  MLME_QUEUE_ELEM *Elem);

VOID    WpaPairMsg3Action(
    IN  PRTMP_ADAPTER   pAd, 
    IN  MLME_QUEUE_ELEM *Elem); 

VOID    WpaGroupMsg1Action(
    IN  PRTMP_ADAPTER   pAd, 
    IN  MLME_QUEUE_ELEM *Elem);

VOID    WpaMacHeaderInit(
    IN      PRTMP_ADAPTER   pAd, 
    IN OUT  PHEADER_802_11  pHdr80211, 
    IN      UCHAR           wep, 
    IN      PUCHAR          pAddr1); 

VOID    Wpa2PairMsg1Action(
    IN  PRTMP_ADAPTER   pAd, 
    IN  MLME_QUEUE_ELEM *Elem);

VOID    Wpa2PairMsg3Action(
    IN  PRTMP_ADAPTER   pAd, 
    IN  MLME_QUEUE_ELEM *Elem);

VOID ParseKeyData(
	IN	PRTMP_ADAPTER	pAd,
	IN	PUCHAR			pKeyData,
	IN	UCHAR			KeyDataLen,
	IN	UCHAR			KeyIdx,
	IN	BOOLEAN			IsGroupMsg);
    
VOID WPAMake8023Hdr(
    IN PRTMP_ADAPTER    pAd, 
    IN PCHAR            pDAddr, 
    IN OUT PCHAR        pHdr);

VOID RTMPToWirelessSta(
    IN  PRTMP_ADAPTER   pAd,
    IN  PUCHAR          pFrame,
    IN  UINT            FrameLen);    

VOID    HMAC_SHA1(
    IN  UCHAR   *text,
    IN  UINT    text_len,
    IN  UCHAR   *key,
    IN  UINT    key_len,
    IN  UCHAR   *digest);

VOID    PRF(
    IN  UCHAR   *key,
    IN  INT     key_len,
    IN  UCHAR   *prefix,
    IN  INT     prefix_len,
    IN  UCHAR   *data,
    IN  INT     data_len,
    OUT UCHAR   *output,
    IN  INT     len);

VOID WpaCountPTK(
    IN  UCHAR   *PMK,
    IN  UCHAR   *ANonce,
    IN  UCHAR   *AA,
    IN  UCHAR   *SNonce,
    IN  UCHAR   *SA,
    OUT UCHAR   *output,
    IN  UINT    len);

VOID    GenRandom(
    IN  PRTMP_ADAPTER   pAd, 
    OUT UCHAR           *random);

VOID    AES_GTK_KEY_UNWRAP( 
    IN  UCHAR   *key,
    OUT UCHAR   *plaintext,
    IN UCHAR    c_len,
    IN  UCHAR   *ciphertext);	

//#if WPA_SUPPLICANT_SUPPORT
INT	RTMPCheckWPAframeForEapCode(
	IN PRTMP_ADAPTER   		pAd,
	IN PUCHAR				pFrame,
	IN ULONG				FrameLen,
	IN ULONG				OffSet);	
//#endif

VOID	WpaMicFailureReportFrame(
	IN  PRTMP_ADAPTER   pAd,
	IN MLME_QUEUE_ELEM *Elem);

VOID    WpaDisassocApAndBlockAssoc(
    IN  PVOID SystemSpecific1, 
    IN  PVOID FunctionContext, 
    IN  PVOID SystemSpecific2, 
    IN  PVOID SystemSpecific3);

//
// prototypes for *iwpriv* in rtmp_info.c
//
char * rstrtok(char * s,const char * ct);

INT RTMPSetInformation(
    IN  PRTMP_ADAPTER pAdapter,
    IN  OUT struct ifreq    *rq,
    IN  INT             cmd);

INT RTMPQueryInformation(
    IN  PRTMP_ADAPTER pAdapter,
    IN  OUT struct ifreq    *rq,
    IN  INT             cmd);

NDIS_STATUS	RTMPWPAAddKeyProc(
	IN	PRTMP_ADAPTER	pAd,
	IN	PVOID			pBuf);

NDIS_STATUS	RTMPWPARemoveKeyProc(
	IN	PRTMP_ADAPTER	pAd,
	IN	PVOID			pBuf);

VOID RTMPIndicateWPA2Status(
	IN  PRTMP_ADAPTER  pAd);

VOID	RTMPWPARemoveAllKeys(
	IN	PRTMP_ADAPTER	pAd);

VOID	RTMPSetPhyMode(
	IN	PRTMP_ADAPTER	pAd,
	IN  ULONG phymode);

VOID    RTMPSetDesiredRates(
    IN  PRTMP_ADAPTER   pAdapter,
    IN  LONG            Rates);
    
INT Set_DriverVersion_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT Set_CountryRegion_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT Set_CountryRegionABand_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT Set_SSID_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_WirelessMode_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_TxRate_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_AdhocModeRate_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_Channel_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_BlockChannelBitMap_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

#ifdef DBG
INT	Set_Debug_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT	Set_R17D_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT	Set_ShowScanTab_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT	Set_ScanTabClean_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

#endif  //#ifdef DBG  

INT Set_BGProtection_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_StaWithEtherBridge_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);
    
INT Set_TxPreamble_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_RTSThreshold_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_FragThreshold_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_TxBurst_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_ShowTx_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

#ifdef AGGREGATION_SUPPORT
INT	Set_PktAggregate_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);
#endif  /* AGGREGATION_SUPPORT */

INT Set_TurboRate_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

#ifdef WMM_SUPPORT
INT	Set_WmmCapable_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);
	
INT	Set_APSDPsm_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);
#endif /* WMM_SUPPORT */

INT Set_ShortSlot_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT	Set_IEEE80211H_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT Set_NetworkType_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_AuthMode_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_EncrypType_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_DefaultKeyID_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_Key1_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_Key2_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_Key3_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_Key4_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_WPAPSK_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT	Set_ResetStatCounter_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);
	
INT Set_PSMode_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);
        
#ifdef DBG
VOID RTMPIoctlBBP(
	IN	PRTMP_ADAPTER	pAdapter,
	IN	struct iwreq	*wrq);

VOID RTMPIoctlMAC(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	struct iwreq	*wrq);

#ifdef RALINK_ATE
VOID RTMPIoctlE2PROM(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	struct iwreq	*wrq);
#endif // RALINK_ATE

VOID RTMPIoctlCHKICVER(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	struct iwreq	*wrq);

#endif //#ifdef DBG

#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
INT RTMPIoctlSetAuth(	
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	struct iwreq	*wrq);

INT RTMPIoctlSetKeyId(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	struct iwreq	*wrq);

INT RTMPIoctlSetEncryp(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	struct iwreq	*wrq);

INT RTMPIoctlSetWpapsk(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	struct iwreq	*wrq);

INT RTMPIoctlSetPsm(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	struct iwreq	*wrq);
#endif // NATIVE_WPA_SUPPLICANT_SUPPORT //

VOID RTMPIoctlStatistics(
	IN	PRTMP_ADAPTER	pAd, 
	IN	struct iwreq	*wrq);


VOID RTMPIoctlGetSiteSurvey(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	struct iwreq	*wrq);

VOID RTMPMakeRSNIE(
    IN  PRTMP_ADAPTER   pAdapter,
    IN	UCHAR	        GroupCipher);
    
NDIS_STATUS RTMPWPANoneAddKeyProc(
    IN  PRTMP_ADAPTER   pAd,
    IN	PVOID			pBuf);

#ifdef RALINK_ATE
INT	Set_ATE_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_ATE_DA_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_ATE_SA_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_ATE_BSSID_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_ATE_CHANNEL_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_ATE_TX_POWER_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_ATE_TX_FREQOFFSET_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);
	
INT	Set_ATE_TX_LENGTH_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_ATE_TX_COUNT_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_ATE_TX_RATE_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

VOID RTMPStationStop(
    IN  PRTMP_ADAPTER   pAd);

VOID RTMPStationStart(
    IN  PRTMP_ADAPTER   pAd);

VOID ATEAsicSwitchChannel(
    IN PRTMP_ADAPTER pAd, 
    IN UCHAR Channel);

#endif  // RALINK_ATE


VOID	RTMPInitTimer(
	IN	PRTMP_ADAPTER			pAd,
	IN	PRALINK_TIMER_STRUCT	pTimer,
	IN	PVOID					pTimerFunc);

VOID	RTMPSetTimer(
	IN	PRTMP_ADAPTER			pAd,	
	IN	PRALINK_TIMER_STRUCT	pTimer,
	IN	ULONG					Value);

VOID	RTMPCancelTimer(
	IN	PRALINK_TIMER_STRUCT	pTimer,
	OUT	BOOLEAN					*pCancelled);



//---> Add by Zero:Jul.05.2007
BOOLEAN rtstrmactohex(
	IN char *s1,
	IN char *s2);

BOOLEAN rtstrcasecmp(
	IN char *s1,
	IN char *s2);
//<---


#ifdef BIG_ENDIAN
static inline VOID	RTMPDescriptorEndianChange(
	IN	PUCHAR			pData,
	IN	ULONG			DescriptorType)
{
	int size = (DescriptorType == TYPE_TXD) ? TXD_SIZE : RXD_SIZE;
	int i;
	for (i=1; i<size/4; i++) {
		/*
		 * Handle IV and EIV with little endian
		 */
		if (DescriptorType == TYPE_TXD) {		
			 /* Skip Word 3 IV and Word 4 EIV of TXD */			
			if (i==3||i==4)  
				continue; 
		}
		else {	
			 /* Skip Word 2 IV and Word 3 EIV of RXD */	
			if (i==2||i==3)  
				continue; 
		}
		*((ULONG *)(pData + i*4)) = SWAP32(*((ULONG *)(pData + i*4))); 
	}
    *(ULONG *)pData = SWAP32(*(ULONG *)pData);  // Word 0; this must be swapped last

}

static inline VOID	WriteBackToDescriptor(
	IN  PUCHAR			Dest,
 	IN	PUCHAR			Src,
    IN  BOOLEAN			DoEncrypt,
	IN  ULONG           DescriptorType)
{
	// Shiang: change the data type to fix the problem in 64 bit big-endian platform.
	//PULONG p1, p2;
	UINT32 *p1, *p2;	// UINT32 defined as unsigned int.
	UCHAR i;
	int size = (DescriptorType == TYPE_TXD) ? TXD_SIZE : RXD_SIZE;

	//p1 = ((PULONG )Dest) + 1;
	//p2 = ((PULONG )Src) + 1;
	p1 = ((UINT32 *)Dest) + 1;
	p2 = ((UINT32 *)Src) + 1;
	for (i = 1; i < size/4 ; i++)
		*p1++ = *p2++;
	//*(PULONG )Dest = *(PULONG )Src;
	*(UINT32 *)Dest = *(UINT32 *)Src;		// Word 0; this must be written back last
}

static inline VOID	RTMPFrameEndianChange(
	IN  PRTMP_ADAPTER   pAdapter, 
	IN  PUCHAR          pData, 
	IN  ULONG           Dir,
	IN  BOOLEAN         FromRxDoneInt)
{
    PFRAME_802_11	pFrame;
    PUCHAR			pMacHdr;

    // swab 16 bit fields - Frame Control field
    if(Dir == DIR_READ)
    {
        *(USHORT *)pData = SWAP16(*(USHORT *)pData);
    }

    pFrame = (PFRAME_802_11) pData;
    pMacHdr = (PUCHAR) pFrame;

    // swab 16 bit fields - Duration/ID field
    *(USHORT *)(pMacHdr + 2) = SWAP16(*(USHORT *)(pMacHdr + 2));

    // swab 16 bit fields - Sequence Control field
    *(USHORT *)(pMacHdr + 22) = SWAP16(*(USHORT *)(pMacHdr + 22));

    if(pFrame->Hdr.FC.Type == BTYPE_MGMT)
    {
        switch(pFrame->Hdr.FC.SubType)
        {
            case SUBTYPE_ASSOC_REQ:
            case SUBTYPE_REASSOC_REQ:
                // swab 16 bit fields - CapabilityInfo field
                pMacHdr += LENGTH_802_11;
                *(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);

                // swab 16 bit fields - Listen Interval field
                pMacHdr += 2;
                *(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);
                break;

            case SUBTYPE_ASSOC_RSP:
            case SUBTYPE_REASSOC_RSP:
                // swab 16 bit fields - CapabilityInfo field
                pMacHdr += LENGTH_802_11;
                *(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);

                // swab 16 bit fields - Status Code field
                pMacHdr += 2;
                *(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);
                
                // swab 16 bit fields - AID field
                pMacHdr += 2;
                *(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);
                break;

            case SUBTYPE_AUTH:
                // If from RTMPHandleRxDoneInterrupt routine, it is still a encrypt format.
                // The convertion is delayed to RTMPHandleDecryptionDoneInterrupt.
                if(!FromRxDoneInt && pFrame->Hdr.FC.Wep != 1)
                {
                    // swab 16 bit fields - Auth Alg No. field
                    pMacHdr += LENGTH_802_11;
                    *(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);

                    // swab 16 bit fields - Auth Seq No. field
                    pMacHdr += 2;
                    *(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);

                    // swab 16 bit fields - Status Code field
                    pMacHdr += 2;
                    *(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);
            	}
            	break;

            case SUBTYPE_BEACON:
            case SUBTYPE_PROBE_RSP:
            	// swab 16 bit fields - BeaconInterval field
            	pMacHdr += LENGTH_802_11 + TIMESTAMP_LEN;
            	*(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);

            	// swab 16 bit fields - CapabilityInfo field
            	pMacHdr += sizeof(USHORT);
            	*(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);
            	break;

            case SUBTYPE_DEAUTH:
            case SUBTYPE_DISASSOC:
            	// swab 16 bit fields - Reason code field
            	pMacHdr += LENGTH_802_11;
            	*(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);
            	break;
        }
    }
    else if(pFrame->Hdr.FC.Type == BTYPE_DATA)
    {
    }
    else if(pFrame->Hdr.FC.Type == BTYPE_CNTL)
    {
    }
    else
    {
        DBGPRINT(RT_DEBUG_ERROR,"Invalid Frame Type!!!\n");
    }

    // swab 16 bit fields - Frame Control
    if(Dir == DIR_WRITE)
    {
        *(USHORT *)pData = SWAP16(*(USHORT *)pData);
    }
}

static inline ULONG   RTMPEqualMemory(
    IN  PVOID   pSrc1,
    IN  PVOID   pSrc2,
    IN  ULONG   Length)
{
    PUCHAR  pMem1;
    PUCHAR  pMem2;
    ULONG   Index = 0;

    pMem1 = (PUCHAR) pSrc1;
    pMem2 = (PUCHAR) pSrc2;

    for (Index = 0; Index < Length; Index++)
    {
        if (pMem1[Index] != pMem2[Index])
        {
            break;
        }
    }

    if (Index == Length)
    {
        return (1);
    }
    else
    {
        return (0);
	}
}

#endif

VOID RTMPIoctlGetRaAPCfg(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	struct iwreq	*wrq);

BOOLEAN BackDoorProbeRspSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen,
    OUT CHAR *pCfgDataBuf);

#define	RTMP_SEM_INIT(__lock)			\
{                                       \
	spin_lock_init(__lock);				\
}

#ifndef THREAD_ISR

#define RTMP_SEM_UNLOCK(__lock, __irqflag)		\
{												\
	spin_unlock_irqrestore(__lock, (__irqflag));	\
}

#define RTMP_SEM_LOCK(__lock, __irqflag)	    \
{												\
	spin_lock_irqsave(__lock, (__irqflag));		\
}

#else /* THREAD_ISR */

extern spinlock_t	Rtmp_Irq_Lock;

#define RTMP_SEM_UNLOCK(__lock, __irqflag)		\
{												\
	__irqflag = 0;								\
	spin_unlock_bh(__lock);						\
}

#define RTMP_SEM_LOCK(__lock, __irqflag)		\
{												\
	__irqflag = 0;								\
	spin_lock_bh(__lock);						\
}

#define RTMP_IRQ_LOCK(__irqflag)					\
{													\
	spin_lock_irqsave(&Rtmp_Irq_Lock, (__irqflag));	\
}

#define RTMP_IRQ_UNLOCK(__irqflag)						\
{														\
	spin_unlock_irqrestore(&Rtmp_Irq_Lock, ((unsigned long)__irqflag));	\
}
#endif /* THREAD_ISR */


#ifdef THREAD_ISR
void 	Rtmp_Init_Thread_Task(IN PRTMP_ADAPTER pAdapter);
void 	Rtmp_Kill_Thread_Task(IN PRTMP_ADAPTER pAdapter);
UINT32	Rtmp_Thread_Isr(IN PRTMP_ADAPTER pAdapter, 
					 	INT_SOURCE_CSR_STRUC	IntSource);
#endif

#endif /* __RTMP_H__ */
