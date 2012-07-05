#ifndef _ZDAPI_H_
#define _ZDAPI_H_

#include "zdtypes.h"


#define FPRINT(string)			printk(KERN_DEBUG "%s\n", string)
#define FPRINT_V(string, i)		printk(KERN_DEBUG "%s = %x\n", string, i)


//#define ZD_DEBUG 	//debug protocol stack

#ifdef ZD_DEBUG
#define ZDEBUG(string)			FPRINT(string)
#define ZDEBUG_V(string, i)		FPRINT_V(string, i)
#else
#define ZDEBUG(string) 			//do {} while (0)
#define ZDEBUG_V(string, i)		//do {} while (0)
#endif

//#define PS_DEBUG		//debug power save function

#ifdef PS_DEBUG
#define PSDEBUG(string)			FPRINT(string)
#define PSDEBUG_V(string, i)	FPRINT_V(string, i)	
#else
#define PSDEBUG(string) 		//do {} while (0)
#define PSDEBUG_V(string, i)	//do {} while (0)
#endif


//#define HASH_DEBUG	//debug hash function

#ifdef HASH_DEBUG
#define HSDEBUG(string)			FPRINT(string)
#define HSDEBUG_V(string, i)	FPRINT_V(string, i)	
#else
#define HSDEBUG(string) 		//do {} while (0)
#define HSDEBUG_V(string, i)	//do {} while (0)
#endif

//#define RATE_DEBUG	//debug rate adaption function

#ifdef RATE_DEBUG
#define RATEDEBUG(string)		FPRINT(string)
#define RATEDEBUG_V(string, i)	FPRINT_V(string, i)	
#else
#define RATEDEBUG(string) 		//do {} while (0)
#define RATEDEBUG_V(string, i)	//do {} while (0)
#endif


//#define DEFRAG_DEBUG	//debug defrag function

#ifdef DEFRAG_DEBUG
#define DFDEBUG(string)			FPRINT(string)
#define DFDEBUG_V(string, i)	FPRINT_V(string, i)	
#else
#define DFDEBUG(string) 		//do {} while (0)
#define DFDEBUG_V(string, i)	//do {} while (0)
#endif


#define CMD_RESET_80211			0x0001	//parm1: zd_80211Obj_t *
#define CMD_ENABLE				0x0002	//parm1: None
#define CMD_DISASOC				0x0003	//parm1: U8 *MacAddress, parm2: reasonCode
#define CMD_DEAUTH				0x0004	//parm1: U8 *MacAddress, parm2: reasonCode
#define CMD_PS_POLL				0x0005	//parm1: U8 *MacHeader
#define CMD_SCAN				0x0006	//parm1: None
#define CMD_DiSASOC_ALL			0x0007	//parm1: None, parm2: reasonCode


//Event Notify
#define EVENT_TBCN				0x0010
#define EVENT_DTIM_NOTIFY		0x0011
#define EVENT_TX_COMPLETE		0x0012 //parm1: tx status, parm2: msgId, parm3: aid
#define EVENT_TCHAL_TIMEOUT		0x0013
#define EVENT_SCAN_TIMEOUT		0x0014
#define EVENT_UPDATE_TX_RATE	0x0015 //parm1: rate, parm2: aid
#define EVENT_SW_RESET          0x0016
#define EVENT_BUF_RELEASE       0x0017
#define EVENT_ENABLE_PROTECTION	0x0018

#define DO_CHAL					0x0001
#define DO_SCAN					0x0002

#define SCAN_TIMEOUT			150 //ms
#define HOUSE_KEEPING_PERIOD	100	//ms


//reason code
#define ZD_UNSPEC_REASON 		1 
#define ZD_AUTH_NOT_VALID		2
#define ZD_DEAUTH_LEAVE_BSS		3 
#define ZD_INACTIVITY			4
#define ZD_AP_OVERLOAD			5 
#define ZD_CLASS2_ERROR			6
#define ZD_CLASS3_ERROR			7 
#define ZD_DISAS_LEAVE_CSS		8
#define ZD_ASOC_NOT_AUTH		9
#define ZD_INVALID_IE			13
#define ZD_MIC_FAIL				14
#define ZD_4WAY_SHAKE_TIMEOUT	15
#define ZD_GKEY_UPDATE_TIMEOUT	16
#define ZD_IE_IMCOMPABILITY		17
#define ZD_MC_CIPHER_INVALID	18
#define ZD_UNI_CIPHER_INVALID	19
#define ZD_AKMP_INVALID			20
#define ZD_UNSUP_RSNE_VERSION	21
#define ZD_INVALID_RSNE_CAP		22
#define ZD__8021X_AUTH_FAIL		23


/* association_status_notify() <- status */
#define STA_ASOC_REQ			0x0001
#define STA_REASOC_REQ			0x0002
#define STA_ASSOCIATED			0x0003
#define STA_REASSOCIATED		0x0004
#define STA_DISASSOCIATED		0x0005
#define STA_AUTH_REQ			0x0006

//Tx complete event
#define ZD_TX_CONFIRM			0x0001
#define ZD_RETRY_FAILED			0x0002

#define DYN_KEY_WEP64			1
#define DYN_KEY_WEP128			2
#define DYN_KEY_TKIP			4


#define RATE_1M		0
#define	RATE_2M		1
#define	RATE_5M		2
#define	RATE_11M	3
#define	RATE_16M	4
#define	RATE_22M	5
#define	RATE_27M	6
#define	NUM_SUPPORTED_RATE	32


typedef struct card_Setting_s
{
	U8		EncryOnOff;		//0: encryption off, 1: encryption on 
	U8		OperationMode;	//reserved
	U8		PreambleType;	//0: long preamble, 1: short preamble
	U8		TxRate;			//0: 1M, 1: 2M, 2: 5.5M, 3: 11M, 4: 16.5M
	U8		FixedRate;		// fixed Tx Rate
	U8		CurrTxRate;		//
	U8		AuthMode;		//0: open system only, 1: shared key only, 2: auto
	U8		HiddenSSID;		//0: disable, 1:enable
	U8		LimitedUser;	//limited client number max to 32 user
	U8		RadioOn;		//0: radio off, 1: radio on
	U8		BlockBSS;		//0: don't block intra-bss traffic, 1: block
	U8		TxPowerLevel;	//0: 17dbm, 1: 14dbm, 2: 11dbm
	U8		BasicRate;		//
	U8		EncryMode;		//0: no wep, 2: wep63, 3:wep128
	U8		EncryKeyId;		//encryption key id
	U8		BcKeyId;		//broadcast key id for dynamic key
	U8		SwCipher;		//
	U8		WepKeyLen;		//WEP key length
	U8		BcKeyLen;		//Broadcast key length
	U8		DynKeyMode;		//Dynamic key mode, 1: WEP64, 2: WEP128, 4:TKIP
	U16		Channel;		//channel number
	U16		FragThreshold;	//fragment threshold, from 256~2432
	U16		RTSThreshold;	//RTS threshold, from 256~2432
	U16		BeaconInterval;	//default 100 ms
	U16		DtimPeriod;		//default 1
	U8		MacAddr[8];		
	// ElementID(1), Len(1), SSID
	U8		Info_SSID[34];	//include element ID, element Length, and element content
	// ElementID(1), Len(1), SupportedRates(1-8)
	U8		Info_SupportedRates[NUM_SUPPORTED_RATE];	//include element ID, element Length, and element content
	U8		keyVector[4][16];
	U8		BcKeyVector[16];
	U8		WPAIe[128];
	U8		WPAIeLen;
	U8		Rate275;
	U8		WpaBcKeyLen;
} card_Setting_t;


#define ZD_MAX_FRAG_NUM		8

typedef struct fragInfo_s{
	U8	*macHdr[ZD_MAX_FRAG_NUM];
	U8	*macBody[ZD_MAX_FRAG_NUM];
	U32	bodyLen[ZD_MAX_FRAG_NUM];
	U32	nextBodyLen[ZD_MAX_FRAG_NUM];
	U8	hdrLen;
	U8	totalFrag;
	U8	bIntraBss;
	U8	msgID;
	U8	rate;
	U8	preamble;
	U8	encryType;
	U8	burst;
	U16 vapId;
	U16 aid;
	void *buf;	
} fragInfo_t;


#define TX_QUEUE_SET			0x01
#define MGT_QUEUE_SET			0x02
#define	AWAKE_QUEUE_SET			0x04

#define ENABLE_PROTECTION_SET 	0x01
#define BARKER_PREAMBLE_SET 	0x02
#define CHANNEL_SCAN_SET		0x04


//driver to provide callback functions for 802.11 protocol stack
typedef	struct zd_80211Obj_s
{
	void	*reg;			//Input
	U8		QueueFlag;		//Output
	U8		ConfigFlag; 	//Output
	U8		BasicRate;
	U8		bIntDisable;
	U32		rfMode;
	U32		RegionCode;
	U32		S_bit_cnt;
	U32		CR32Value;
	
	void	(* ReleaseBuffer)(void *buf);							// release rx buffer
	void	(* StartTimer)(U32 timeout, U32 event);					// start a chanllege timer (shared key authentication)
	void	(* StopTimer)(void);									// stop the challenge timer
	void	(* RxInd)(U8 *pData, U32 length, void *buf);			// rx indication
	void	(* TxCompleted)(void);									// tx completed
	BOOLEAN	(* SetupNextSend)(fragInfo_t *pFragInfo);				// send to HMAC
	void	(* SetReg)(void *reg, U32 offset, U32 value);			// set HMAC register
	U32		(* GetReg)(void *reg, U32 offset);						// get HMAC register			
	U16 	(* StatusNotify)(U16 status, U8 *StaAddr);				// association notify for bridge management
	void 	(* ExitCS)(U32 flags);								// enable interrupt
	U32		(* EnterCS)(void);								// disable interrupt
	U32 	(* Vir2PhyAddr)(U32 virtAddr);							// translate virtual address to physical address
	BOOLEAN	(* CheckTCBAvail)(U8 NumOfFrag);						// check TCB available
	void	(* DelayUs)(U16 ustime);								// delay function
	void *	(* AllocBuffer)(U16 dataSize, U8 **pData);				// allocate wireless forwarding buffer
	// wpa support
	void	(* MicFailure)(unsigned char *addr);
	int		(* AssocRequest)(U8 *addr, U8* data, U16 size);
	int 	(* WpaIe)(U8 *buffer, int length);
}zd_80211Obj_t;	


//802.11 export functions for driver use
extern void zd_SigProcess(void);									// protocol statck entry point
extern BOOLEAN zd_SendPkt(U8 *pHdr, U8 *pBody, U32 bodyLen, void *buf, U8 bEapol, void *pHash);	// tx request
extern void zd_ReceivePkt(U8 *pHdr, U32 hdrLen, U8 *pBody,			// rx indication
		U32 bodyLen, U8 rate, void *buf, U8 bDataFrm, U8 *pEthHdr);
extern BOOLEAN zd_CmdProcess(U16 CmdId, void *parm1, U32 parm2);	//command process		 
extern void zd_EventNotify(U16 EventId, U32 parm1, U32 parm2, U32 parm3);		//event notify
extern void zd_UpdateCardSetting(card_Setting_t *pSetting);
extern BOOLEAN zd_CleanupTxQ(void);
extern BOOLEAN zd_CleanupAwakeQ(void);
extern int zd_SetKeyInfo(U8 *addr, U8 encryMode, U8 keyLength, U8 KeyId, U8 *pKeyContent);
extern BOOLEAN zd_GetKeyInfo(U8 *addr, U8 *encryMode, U8 *keyLength, U8 *pKeyContent);
extern BOOLEAN zd_QueryStaTable(U8 *sta, void **ppHash);	
extern void zd_EncryptData(U8 Wep_Key_Len, U8 *Wep_Key, U8 *Wep_Iv, U16	Num_Bytes, //software encryption
    	U8 *Inbuf, U8 *Outbuf, U32 *Icv);
extern BOOLEAN zd_DecryptData(U8 Wep_Key_Len, U8 *Wep_Key, U8 *Wep_Iv, U16 Num_Bytes, //software decryption
		U8 *Inbuf, U8 *Outbuf, U32 *Icv); // Num_Bytes include ICV 
extern void zd_PerSecTimer(void);		
extern int zd_GetKeyInfo_ext(U8 *addr, U8 *encryMode, U8 *keyLength, U8 *pKeyContent, U16 iv16, U32 iv32);

extern int zd_SetTsc(U8 *addr, U8 KeyId, U8 direction, U32 tscHigh, U16 tscLow);
extern int zd_GetTsc(U8 *addr, U8 KeyId, U8 direction, U32 *tscHigh, U16 *tscLow);

#endif

