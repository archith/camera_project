#ifndef __ZDSM_H__
#define __ZDSM_H__

#include "zdos.h"
#include "zdsorts.h"


typedef struct FrmInfo_s
{
	MacAddr_t 	Sta;		//for Disassoc/Deauth STA
	ReasonCode	rCode;		//for Disassoc/Deauth Reason Code
	U8			fTot;		//total frag no.	
	U32			eol;		//timestamp for age	(PsQ)
	FrmDesc_t	*frmDesc;	//		
} FrmInfo_t;	


typedef struct Signal_s
{
	struct Signal_s 	*pNext;
	U8					id; 
	U8					block;
	U8					vapId;		//virtual AP id
	U8					bDataFrm;		//data frame
	//U8					dummy;
	void				*buf;		//buffer
	FrmInfo_t			frmInfo;
} Signal_t ;


typedef struct SignalQ_s
{
	Signal_t 	*first;		
	Signal_t 	*last;		
	U16			cnt;		
} SignalQ_t;


#define BLOCK_SYNCH					0x01
#define BLOCK_AUTH_REQ				0x02
#define BLOCK_AUTH_RSP				0x03
#define BLOCK_ASOC					0x04


//Auth_Req block, 1 state, 1 signals
#define SIG_DEAUTH_REQ				0x01	


//Asoc_Svc block, 1 state, 4 signals
#define SIG_DISASSOC				0x01	
#define SIG_DIASSOC_REQ				0x02	
#define SIG_ASSOC_REQ				0x03	
#define SIG_REASSOC_REQ				0x04	


//Auth_Rsp block, 2 state, 3 signals
#define STE_AUTH_RSP_IDLE			0x00
#define STE_AUTH_RSP_WAIT_CRSP 		0x01  


#define SIG_AUTH_ODD				0x01
#define SIG_DEAUTH					0x02
#define SIG_TO_CHAL					0x03


//Synch block, 1 state, 1 signals
#define SIG_PROBE_REQ				0x01	

#endif

