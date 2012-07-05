/*
 * mlme/mlme.h
 *
 * Copyright (C) 2004  Ivan Wang <ivan@faraday.com.tw>
 */ 
#ifndef _MLME_H_
#define _MLME_H_
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/timer.h>

#include "fw_os.h"

#ifdef MLME_ADS
#define MLME_TIMER_V1        1
#define MLME_TIMER_V2        2
#endif

#define MLME_DEBUG_LEVEL        3
#define MLME_DBG_LEVEL1         1
#define MLME_DBG_LEVEL2         2
#define MLME_DBG_LEVEL3         3

/* MLME definition */
#define MACADDR_LEN             6
#define BEACON_INTERVAL         100     //ms
#define MAX_ESSID_SIZE          (32+1)  //with \0 end
#define BEACON_WAIT_TIME        1000    //wait beacon
#define CAPABILITY_LEN          2
#define RATE_LEN                10      //with 0 to represent empty
#define MLME_MAX_CHANNEL        20
#define MLME_MAX_RATE           10
#define MLME_CHANNEL_NOCHECK    0x99

#define MLME_JOIN_TIMEOUT_VALUE       1000      //ms
#define MLME_ASSOC_TIMEOUT_VALUE      300       //ms
#define MLME_REASSOC_TIMEOUT_VALUE    300       //ms
#define MLME_AUTH_LONG_TIMEOUT        500       // ms (for share-key)
#define MLME_AUTH_SHORT_TIMEOUT       200       // ms (for open-system)
#define MLME_MAX_CHANNEL_SCAN_TIME    20       // ms
#define MLME_ROUTINE_TIME             800      //ms

#define MLME_MGMT_HDR_LEN             24
#define MLME_AUTH_BODY_LEN            6
#define MLME_ASSOC_BODY_LEN           50
#define MLME_BEACON_BODY_LEN          100

#define MLME_AUTH_ALG_OPEN            0
#define MLME_AUTH_ALG_KEY             1

#define MLME_PASSIVE_SCAN               0
#define MLME_ACTIVE_SCAN                1

#define MLME_QUALITY_OFFSET             25    //differnece 30 quality should roaming

typedef enum 
{
    MLME_CAP_NULL    =0x0,
	MLME_CAP_ESS 	= 0x01,
	MLME_CAP_IBSS	= 0x02,
	MLME_CAP_POLLABLE = 0x04,
	MLME_CAP_POLLREQ = 0x08,
	MLME_CAP_PRIVACY = 0x10,
	MLME_CAP_SHORT_PREAMBLE = 0x20,
	MLME_CAP_PBCC_ENABLE	= 0x40,
	MLME_CAP_SHORT_SLOT_TIME = 0x0400,
	MLME_CAP_DSSS_OFDM_BIT = 0x2000
} MLME_CAP;


/***********************************************************
        Packet Header
***********************************************************/
typedef struct I802_11_hdr
{
    UINT16      ver:2;
    UINT16      type:2;
    UINT16      subtype:4;
    UINT16      toDS:1;
    UINT16      fromDS:1;
    UINT16      moreFrag:1;
    UINT16      retry:1;
    UINT16      pwrMgmt:1;
    UINT16      moreData:1;
    UINT16      wep:1;
    UINT16      order:1;
    
    UINT16      duration;
    UINT8       addr1[MACADDR_LEN];
    UINT8       addr2[MACADDR_LEN];
    UINT8       addr3[MACADDR_LEN];
    
    UINT16      frag:4;
    UINT16      seq:12;    
} I802_11_hdr_t;

#define MLME_MAX_FRAME_BODY_LEN 2312
typedef struct I802_11_pkt
{
    I802_11_hdr_t   header;  
    UINT8           fb[MLME_MAX_FRAME_BODY_LEN];
} I802_11_pkt_t;


#define MLME_ELEMENT_SSID   0
#define MLME_ELEMENT_RATE   1
#define MLME_ELEMENT_FH     2
#define MLME_ELEMENT_DS     3
#define MLME_ELEMENT_CF     4
#define MLME_ELEMENT_TIM    5
#define MLME_ELEMENT_IBSS   6
#define MLME_ELEMENT_CHAL   16
#define MLME_ELEMENT_WPA    221

#define	MLME_SR_1M		    2
#define	MLME_SR_2M		    4
#define	MLME_SR_5_5M		11
#define	MLME_SR_11M		    22
#define	MLME_SR_16_5M		33
#define	MLME_SR_22M		    44
#define	MLME_SR_27_5M		55

#define MLME_MAX_WPA_LEN        64
/* Information Element */
typedef struct info_element
{
    UINT8       id;
    UINT8       len;
    UINT8       info[1];
} info_element_t;

/***********************************************************
        BSS table definition
***********************************************************/
typedef struct BSS_entry
{
    UINT32      exist;      //0:no 1:exist    
    UINT32      timestamp;
    UINT16      mPrivacy;    
    UINT16      mInterval;
    UINT16      mChannel;               //BSS working channel        
    UINT16      atimWin;
    UINT16      beaconPeriod;
    UINT8       mBssid[MACADDR_LEN];
    UINT8       mEssid[MAX_ESSID_SIZE];
//    UINT8       mCap;
    UINT8       rate[RATE_LEN];     //1M:0x02 2M:0x04 5.5M:0x0B 11M:0x16 support rate
    UINT8       basic_rate;     //maximun basic rate
    UINT8       quality;        //should normolize to 0-100
    UINT8       strength;       //should normolize to 0-100
    UINT8       mMode;          //0:ad-hoc 1:intrastructure
    UINT8       wpa_ie[MLME_MAX_WPA_LEN];
} BSS_entry_t;

#define BSS_TABLE_SIZE  10
typedef struct BSS_table
{
    BSS_entry_t     entry[BSS_TABLE_SIZE];
    UINT32          idx;                //for mTable usage
} BSS_table_t;


/***********************************************************
        Message Queue
***********************************************************/
#define MSG_ENTRY_SIZE              10  //message queue size
#define QUEUE_PARM_SIZE             0x100


typedef struct parm_entry
{
    UINT8   parm1[QUEUE_PARM_SIZE];
    UINT32  len1;
    UINT8   parm2[QUEUE_PARM_SIZE];
    UINT32  len2;
} parm_entry_t;

typedef struct msg_entry
{
    UINT32          machine;
    UINT32          message;
    parm_entry_t    parm;    
} msg_entry_t;


/* Message Queue:
  0  1  2  3  4  5  6  7  8  9
     ^                    ^
    head(msg here)        tail(empty here, msg before here)
*/    
typedef struct msg_queue
{
    UINT32          head;
    UINT32          tail;    
    msg_entry_t     entry[MSG_ENTRY_SIZE];
} msg_queue_t;

/***********************************************************
        MLME definition
***********************************************************/
typedef unsigned char mlme_state_t;
typedef struct
{
    UINT32          base;              		//base address of MAC    
    UINT32          mLastBeaconTime;        //For ad-hoc mode to check if link down
    UINT32          mChannel_info[MLME_MAX_CHANNEL];
    UINT32          mAuth_config;           //authentication is open system/shared key
    UINT16          mInterval_config;                //listen/beacon interval
//    UINT16          mCap_config;                   //capability
    UINT16          mMode;                  //MLME_ADHOC_MODE:1 MLME_BSS_MODE:2
    UINT8           mInit;                  //0:no 1:yes
    UINT8           mAssoc;                 //0:No link 1:Start service
    UINT8           mAuth;                  //0:No auth 1:auth ok
    UINT8           mMAC[MACADDR_LEN];
    UINT8           mChannel_config;        //The setting channel config
    UINT8           mBssid_config[MACADDR_LEN]; 
    UINT8           mScan_type;              //0:pasive 1:active
    UINT8           mRate_info[MLME_MAX_CHANNEL];
    UINT8           basic_rate_config;      //basic rate
    BSS_entry_t     bss;                    //current BSS setup
    BSS_table_t     mTable;
    msg_queue_t     mQueue;
    
/* State machine declaration */
    mlme_state_t    mlme_sys_state;
    mlme_state_t    mlme_ctl_state;    
    mlme_state_t    mlme_sync_state;
    mlme_state_t    mlme_assoc_state;
    mlme_state_t    mlme_auth_state;

/* timer */
    struct timer_list mlmetimer;
    struct timer_list routinetimer;
/* spin lock */
    spinlock_t 		mlme_lock;
/* hw layer data */
    void            *hw_private;
} mlme_data_t;


/***********************************************************
        Packet Type
***********************************************************/
#define TYPE_MGMT               0
#define TYPE_CNTL               1
#define TYPE_DATA               2

// Reason code definitions
#define MLME_REASON_RESERVED                   0
#define MLME_REASON_UNSPECIFY                  1
#define MLME_REASON_NO_LONGER_VALID            2
#define MLME_REASON_DEAUTH_STA_LEAVING         3
#define MLME_REASON_DISASSOC_INACTIVE          4
#define MLME_REASON_DISASSPC_AP_UNABLE         5
#define MLME_REASON_CLS2ERR                    6
#define MLME_REASON_CLS3ERR                    7
#define MLME_REASON_DISASSOC_STA_LEAVING       8
#define MLME_REASON_STA_REQ_ASSOC_NOT_AUTH     9
#define MLME_REASON_MIC_FAILURE                14

// Management frame
#define SUBTYPE_ASSOC_REQ       0
#define SUBTYPE_ASSOC_RSP       1
#define SUBTYPE_REASSOC_REQ     2
#define SUBTYPE_REASSOC_RSP     3
#define SUBTYPE_PROBE_REQ       4
#define SUBTYPE_PROBE_RSP       5
#define SUBTYPE_BEACON          8
#define SUBTYPE_ATIM            9
#define SUBTYPE_DISASSOC        10
#define SUBTYPE_AUTH            11
#define SUBTYPE_DEAUTH          12

// Control Frame
#define SUBTYPE_PS_POLL         10
#define SUBTYPE_RTS             11
#define SUBTYPE_CTS             12
#define SUBTYPE_ACK             13
#define SUBTYPE_CFEND           14
#define SUBTYPE_CFEND_CFACK     15

// Data Frame
#define SUBTYPE_DATA                0
#define SUBTYPE_DATA_CFACK          1
#define SUBTYPE_DATA_CFPOLL         2
#define SUBTYPE_DATA_CFACK_CFPOLL   3
#define SUBTYPE_NULL_FUNC           4
#define SUBTYPE_CFACK               5
#define SUBTYPE_CFPOLL              6
#define SUBTYPE_CFACK_CFPOLL        7

/***** MLME mode *****/
#define MLME_NULL_MODE              0
#define MLME_ADHOC_MODE             1
#define MLME_INFRA_MODE             2
#define MLME_AP_MODE                3

/********* State machine_definition *****/
#define MLME_NULL_STATE_MACHINE     0
#define MLME_SYS_STATE_MACHINE      1
#define MLME_CTL_STATE_MACHINE      2
#define MLME_SYNC_STATE_MACHINE     3
#define MLME_ASSOC_STATE_MACHINE    4
#define MLME_AUTH_STATE_MACHINE     5


/********* State Definition *************/
#define MLME_CLASS_1		    1   /* SYS state machine init state */
#define MLME_CLASS_2		    2
#define MLME_CLASS_3		    3

#define MLME_CTL_IDLE		    10  /* CTL state machine init state */
#define MLME_CTL_WAIT		    11

#define ASSOC_IDLE			    20  /* ASSOC state machine init state */
#define ASSOC_WAIT			    21
#define REASSOC_WAIT			22

#define AUTH_IDLE               30
#define WAIT_SEQ_2              31
#define WAIT_SEQ_4              32

#define SYNC_IDLE               40
#define JOIN_WAIT_BEACON        41
#define SCAN_LISTEN             42

/*
        Message Definition
*/
#define MLME_IOCTL_NULL_MSG 		  0
#define MLME_IOCTL_SET_SSID           1
#define MLME_IOCTL_GET_SSID           2
#define MLME_IOCTL_SET_BSSID          3
#define MLME_IOCTL_GET_BSSID          4
#define MLME_IOCTL_SET_CHANNEL        5
#define MLME_IOCTL_GET_CHANNEL        6
#define MLME_IOCTL_SET_MAC            7
#define MLME_IOCTL_GET_MAC            8
#define MLME_IOCTL_SET_MODE           9
#define MLME_IOCTL_GET_MODE           10
#define MLME_IOCTL_DEBUG_PRINT        11
#define MLME_IOCTL_RESTART            12
#define MLME_IOCTL_QUERY_JOB          13
#define MLME_IOCTL_QUERY_ASSOC        14
#define MLME_IOCTL_SET_AUTH_ALG       15
#define MLME_IOCTL_GET_AUTH_ALG       16
#define MLME_IOCTL_GET_SCAN_INFO      17
#define MLME_IOCTL_REQ_DISASSOC       18

/********** MLME request message ********/
#define MLME_ASSOC_REQ 	        20
#define MLME_REASSOC_REQ        21
#define MLME_DISASSOC_REQ 	    22
#define MLME_AUTH_REQ 	        23
#define MLME_DEAUTH_REQ 		24
#define MLME_SCAN_REQ 	        25
#define MLME_JOIN_REQ 	        26
#define MLME_START_REQ	        27
/********** MLME receive message ********/
#define MLME_DISASSOC_RCV       30
#define MLME_ASSOC_RCV		    31
#define MLME_ASSOC_RSP_RCV	    32
#define MLME_REASSOC_RCV		33
#define MLME_REASSOC_RSP_RCV	34
#define MLME_AUTH_EVEN_RCV	    35
#define MLME_AUTH_ODD_RCV	    36
#define MLME_DEAUTH_RCV		    37
#define MLME_BEACON_RCV		    38
#define MLME_PROBE_RSP_RCV	    39
#define MLME_ATIM_RCV			40
#define MLME_PROBE_REQ_RCV	    41

/********** MLME timeout message ********/
#define MLME_ASSOC_TIMEOUT	    50
#define MLME_REASSOC_TIMEOUT	51
#define MLME_DISASSOC_TIMEOUT	52
#define MLME_AUTH_TIMEOUT		53
//#define MLME_AUTH_CHAL_TIMEOUT	54
#define MLME_SCAN_COMPLETE	    55
//#define MLME_BEACON_TIMEOUT	56
#define MLME_ATIM_TIMEOUT	    57
#define MLME_JOIN_TIMEOUT		58
/********** MLME error message ********/
#define MLME_CLS2_ERROR		    60
#define MLME_CLS3_ERROR		    61
#define MLME_SUCCESS			62
#define MLME_STOP               63


/* debug information */
//driver layer
#define MLME_DEBUG_RX_DUMP_PKT          0x0001  /* Dump Rx packet cotent */
#define MLME_DEBUG_TX_DUMP_PKT          0x0002  /* Dump Tx packet cotent */
#define MLME_DEBUG_TCB_DUMP_MSG         0x0004  
#define MLME_DEBUG_WEP_DATA             0x0008
#define MLME_DEBUG_WPA_DATA             0x0010
#define MLME_DEBUG_HARDWARE_CTL         0x0020

//mlme layer
#define MLME_DEBUG_MLME_BASIC_MSG       0x0100
#define MLME_DEBUG_MLME_ADVANCE_MSG     0x0200
#define MLME_DEBUG_MLME_BEACON_DUMP     0x0400
#define MLME_DEBUG_MLME_RX_MSG          0x0800
#define MLME_DEBUG_MLME_TX_DUMP         0x1000
#define MLME_DEBUG_ATTACKER             0x2000



/*
    inside MLME function call
*/
void        mlme_make_element(UINT8 *,UINT8,UINT8,UINT8 *);
BSS_entry_t *mlme_search_table_by_essid(BSS_table_t *table,UINT8 *essid,UINT8 channel);
BSS_entry_t *mlme_search_table_by_bssid(BSS_table_t *table,UINT8 *bssid);
UINT32      mlme_table_insert(BSS_table_t *table,BSS_entry_t *src);
UINT32      mlme_msg_enqueue(msg_queue_t *q,UINT32 machine,UINT32 msg,void *parm1,UINT32 len1,void *parm2,UINT32 len2);
msg_entry_t *mlme_msg_dequeue(msg_queue_t *q);

void        mlme_dump_data(UINT8 *data, UINT32 data_len);
void        mlme_handle_sys_state(mlme_data_t *mlme_p,msg_entry_t *entry);
void        mlme_handle_sync_state(mlme_data_t *mlme_p,msg_entry_t *entry);
void        mlme_handle_auth_state(mlme_data_t *mlme_p,msg_entry_t *entry);
void        mlme_handle_assoc_state(mlme_data_t *mlme_p,msg_entry_t *entry);
void        mlme_routine(mlme_data_t *mlme_p);
UINT32      mlme_checkinit(mlme_data_t *mlme_p);
void        mlme_state_reset(mlme_data_t *mlme_p);
void        mlme_up(mlme_data_t *mlme_p);
void        mlme_down(mlme_data_t *mlme_p);
UINT32      mlme_get_time(mlme_data_t *mlme_p);
void        mlme_restart_routine_timer(mlme_data_t *mlme_p);
void        mlme_reset(mlme_data_t *mlme_p);
void        mlme_stop_timer(mlme_data_t *mlme_p);
UINT32      mlme_set_ssid_action(mlme_data_t *mlme_p);
UINT32      mlme_make_header(UINT8 *pkt,UINT8 subtype,UINT8 toDS,UINT8 wep,UINT8 *addr1,UINT8 *addr2,UINT8 *addr3);
UINT32      mlme_make_auth(mlme_data_t *mlme_p,UINT8 *pkt,UINT16 seq,UINT16 status,UINT8 *);
UINT32      mlme_make_assoc(mlme_data_t *mlme_p,UINT8 *pkt);
UINT32      mlme_make_disassoc(mlme_data_t *mlme_p,UINT8 *pkt,UINT16 reason);
UINT32      mlme_make_probe_req(mlme_data_t *mlme_p,UINT8 *pkt);
UINT32      mlme_parsing_beacon_info(mlme_data_t *mlme_p,BSS_entry_t *bss_entry,UINT8 *pkt,UINT32 pktlen);
UINT32      mlme_make_probe_rsp(mlme_data_t *mlme_p,UINT8 *pkt,UINT8 *dest);
void        mlme_make_random_bssid(mlme_data_t *mlme_p);
void        sim_printmlme(mlme_data_t *mlme_p,int value);

/*
    exported function call
*/
void        MLME_main(mlme_data_t *mlme_p);
void        MLME_rx(mlme_data_t *mlme_p,UINT8 *,UINT32,UINT8 *);
void        MLME_init(mlme_data_t *mlme_p);
UINT32      MLME_ioctl(mlme_data_t *mlme_p,UINT32 cmd,void *data);
#endif
