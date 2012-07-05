/*
 * mlme/mlme.c
 *
 * Copyright (C) 2004  Ivan Wang <ivan@faraday.com.tw>
 */ 

#include "mlme.h"
#include "hw_ctrl.h"

UINT32 mAID;

static UINT32 mlme_get_assoc_rsp(mlme_data_t *mlme_p,UINT8 *pkt,UINT32 pktlen);
static void mlme_assoc_req_action(mlme_data_t *mlme_p);
static UINT32 mlme_assoc_rsp_rcv_action(mlme_data_t *mlme_p,parm_entry_t *parm);
static void mlme_disassoc_req_action(mlme_data_t *mlme_p,UINT32 *);
static UINT32 mlme_disassoc_rcv_action(mlme_data_t *mlme_p);
static void mlme_reassoc_req_action(mlme_data_t *mlme_p,parm_entry_t *parm);
static UINT32 mlme_reassoc_rsp_rcv_action(mlme_data_t *mlme_p,parm_entry_t *parm);
//static void mlme_reassoc_rcv_action(mlme_data_t *mlme_p);
static void mlme_assoc_cls3_err(mlme_data_t *mlme_p);
static void mlme_assoc_timeout_action(mlme_data_t *mlme_p);
//static void mlme_reassoc_timeout_action(mlme_data_t *mlme_p);
static void mlme_disassoc_timeout_action(mlme_data_t *mlme_p);
static void mlme_assoc_timeout_function(mlme_data_t *mlme_p);
static void mlme_reassoc_timeout_function(mlme_data_t *mlme_p);

void mlme_handle_assoc_state(mlme_data_t *mlme_p,msg_entry_t *entry)
{
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"Handle ASSOC State Machine...");
        
    switch(entry->message)
    {
        case MLME_STOP:
            mlme_p->mlme_assoc_state=ASSOC_IDLE;
            break;
        case MLME_ASSOC_REQ:
            if(mlme_p->mMode==MLME_INFRA_MODE)
            {
                if(mlme_p->mlme_assoc_state==ASSOC_IDLE)
                {
                    mlme_assoc_req_action(mlme_p);
                    mlme_p->mlme_assoc_state=ASSOC_WAIT;
                }
            }
            break;            
        case MLME_ASSOC_RSP_RCV:
            if(mlme_p->mMode==MLME_INFRA_MODE)
            {        
                if(mlme_p->mlme_assoc_state==ASSOC_WAIT)
                {
                    if(mlme_assoc_rsp_rcv_action(mlme_p,&entry->parm))
                    {
                        mlme_p->mlme_assoc_state=ASSOC_IDLE;
                    }
                }
            }
            break;
        case MLME_REASSOC_RSP_RCV:
            if(mlme_p->mMode==MLME_INFRA_MODE)
            {        
                if(mlme_p->mlme_assoc_state==REASSOC_WAIT)
                {
                    if(mlme_reassoc_rsp_rcv_action(mlme_p,&entry->parm))
                    {
                        mlme_p->mlme_assoc_state=ASSOC_IDLE;
                    }
                }
            }
            break;
        case MLME_DISASSOC_REQ:
            if(mlme_p->mAssoc)
            {
                mlme_p->mAssoc=0;
                if(mlme_p->mMode==MLME_INFRA_MODE)
                {        
                    if(mlme_p->mlme_assoc_state==ASSOC_IDLE)
                    {
                        mlme_disassoc_req_action(mlme_p,(UINT32 *)entry->parm.parm1);
                    }
                }
            }
            break;
        case MLME_DISASSOC_RCV:
            if(mlme_p->mAssoc)
            {
                mlme_p->mAssoc=0;
                if(mlme_p->mMode==MLME_INFRA_MODE)
                {        
                    if(mlme_disassoc_rcv_action(mlme_p))
                    {
                        if(mlme_p->mlme_assoc_state==ASSOC_WAIT)
                            mlme_p->mlme_assoc_state=ASSOC_IDLE;
                    }
                }
            }
            break;
        case MLME_REASSOC_REQ:
            if(mlme_p->mMode==MLME_INFRA_MODE)
            {        
                if(mlme_p->mlme_assoc_state==ASSOC_IDLE)
                {
                    mlme_reassoc_req_action(mlme_p,&entry->parm);
                    mlme_p->mlme_assoc_state=ASSOC_WAIT;
                }
            }
            break;
        case MLME_REASSOC_RCV:
            if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
                fw_showstr(MLME_DBG_LEVEL1,"BSS:AP only...");
            break;
        case MLME_CLS3_ERROR:
            if(mlme_p->mlme_assoc_state==ASSOC_IDLE)
                mlme_assoc_cls3_err(mlme_p);
            else if(mlme_p->mlme_assoc_state==ASSOC_WAIT)
            {
                mlme_assoc_cls3_err(mlme_p);
                mlme_p->mlme_assoc_state=ASSOC_IDLE;
            }
            break;
        case MLME_ASSOC_TIMEOUT:
            if(mlme_p->mlme_assoc_state==ASSOC_WAIT)
            {
                mlme_assoc_timeout_action(mlme_p);
                mlme_p->mlme_assoc_state=ASSOC_IDLE;
            }
            break;
        case MLME_REASSOC_TIMEOUT:
            if(mlme_p->mlme_assoc_state==ASSOC_WAIT)
            {
                mlme_assoc_timeout_action(mlme_p);
                mlme_p->mlme_assoc_state=ASSOC_IDLE;
            }
            break;
        case MLME_DISASSOC_TIMEOUT:
            if(mlme_p->mlme_assoc_state==ASSOC_WAIT)
            {
                mlme_disassoc_timeout_action(mlme_p);
                mlme_p->mlme_assoc_state=ASSOC_IDLE;
            }
            break;
        default:
            while(1)
                ;
    }
}


static void mlme_assoc_timeout_function(mlme_data_t *mlme_p)
{
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
    {
        fw_showstr(MLME_DBG_LEVEL1,"mlme_assoc_timeout_function...");
        fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_ASSOC_TIMEOUT to ASSOC state machine...");
    }
    mlme_msg_enqueue(&mlme_p->mQueue,MLME_ASSOC_STATE_MACHINE,MLME_ASSOC_TIMEOUT,0,0,0,0);
    MLME_main(mlme_p); //see MLME_main()
}

static void mlme_reassoc_timeout_function(mlme_data_t *mlme_p)
{
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
    {
        fw_showstr(MLME_DBG_LEVEL1,"mlme_reassoc_timeout_function...");
        fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_REASSOC_TIMEOUT to ASSOC state machine...");
    }
    mlme_msg_enqueue(&mlme_p->mQueue,MLME_ASSOC_STATE_MACHINE,MLME_REASSOC_TIMEOUT,0,0,0,0);
    MLME_main(mlme_p); //see MLME_main()
}


static void mlme_assoc_req_action(mlme_data_t *mlme_p)
{
    I802_11_pkt_t   pkt;
    UINT32  len;

    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
    {
        fw_showstr(MLME_DBG_LEVEL1,"mlme_assoc_req_action...");
        fw_showstr(MLME_DBG_LEVEL1,"send association requestion packet...");
    }
//    send_assoc_req(mlme_p->bss.mBssid,1,100,mlme_p->bss.mEssid);
    len=mlme_make_assoc(mlme_p,(UINT8 *)&pkt);
    //mlme_dump_data(pkt,len+MLME_MGMT_HDR_LEN);
    HW_tx((UINT8 *)&pkt,len);
    
#ifdef MLME_ADS
    fw_setup_timer(MLME_TIMER_V1,MLME_ASSOC_TIMEOUT_VALUE,(UINT32)mlme_p,mlme_assoc_timeout_function);
    fw_add_timer(MLME_TIMER_V1);
#else    
    fw_setup_timer(&mlme_p->mlmetimer,MLME_ASSOC_TIMEOUT_VALUE,mlme_p,mlme_assoc_timeout_function);
    fw_add_timer(&mlme_p->mlmetimer);
#endif    
}

static UINT32 mlme_assoc_rsp_rcv_action(mlme_data_t *mlme_p,parm_entry_t *parm)
{
	UINT8           *pkt=(UINT8 *)parm->parm1;
    UINT32          pktlen=parm->len1;
    
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
    {
        fw_showstr(MLME_DBG_LEVEL1,"mlme_assoc_rsp_rcv_action...");
        fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_ASSOC_RSP_RCV to SYS state machine...");
    }
    mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYS_STATE_MACHINE,MLME_ASSOC_RSP_RCV,0,0,0,0);
                     
    mlme_get_assoc_rsp(mlme_p,pkt,pktlen);
           
#ifdef MLME_ADS
    fw_del_timer(MLME_TIMER_V1);
#else
    fw_del_timer(&mlme_p->mlmetimer);
#endif    
    return 1;
}

static void mlme_disassoc_req_action(mlme_data_t *mlme_p,UINT32 *reason)
{
    I802_11_pkt_t   pkt;
    UINT32  len;
    
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
    {
        fw_showstr(MLME_DBG_LEVEL1,"mlme_disassoc_req_action...");
        fw_showstr(MLME_DBG_LEVEL1,"Send disassoc packet...");
    }
    len=mlme_make_disassoc(mlme_p,(UINT8 *)&pkt,*reason);
    HW_tx((UINT8 *)&pkt,len);

    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_DISASSOC_REQ to SYS state machine...");
    mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYS_STATE_MACHINE,MLME_DISASSOC_REQ,reason,2,0,0);

    //reconnect by its essid
    mlme_set_ssid_action(mlme_p);
}

static UINT32 mlme_disassoc_rcv_action(mlme_data_t *mlme_p)
{
    UINT32 reason;
    
    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"mlme_disassoc_rcv_action...");

    //start to do associaton
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_DISASSOC_REQ to SYS state machine...");
        
    reason=MLME_REASON_DISASSOC_STA_LEAVING;
    mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYS_STATE_MACHINE,MLME_DISASSOC_REQ,&reason,2,0,0);

    //start to do associaton
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_ASSOC_REQ to ASSOC state machine...");
    mlme_msg_enqueue(&mlme_p->mQueue,MLME_ASSOC_STATE_MACHINE,MLME_ASSOC_REQ,0,0,0,0);
                        
    return 1;
}

static void mlme_reassoc_req_action(mlme_data_t *mlme_p,parm_entry_t *parm)
{
	UINT8           *pkt=(UINT8 *)parm->parm1;
    UINT32          pktlen=parm->len1;
    
    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
    {
        fw_showstr(MLME_DBG_LEVEL1,"mlme_reassoc_req_action...");
        fw_showstr(MLME_DBG_LEVEL1,"send reassociation requestion packet...");
    }
    
    mlme_get_assoc_rsp(mlme_p,pkt,pktlen);
    
#ifdef MLME_ADS
    fw_setup_timer(MLME_TIMER_V1,MLME_ASSOC_TIMEOUT_VALUE,(UINT32)mlme_p,mlme_reassoc_timeout_function);
    fw_add_timer(MLME_TIMER_V1);
#else    
    fw_setup_timer(&mlme_p->mlmetimer,MLME_REASSOC_TIMEOUT_VALUE,mlme_p,mlme_reassoc_timeout_function);
    fw_add_timer(&mlme_p->mlmetimer);
#endif    
}

static UINT32 mlme_reassoc_rsp_rcv_action(mlme_data_t *mlme_p,parm_entry_t *parm)
{
    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
    {
        fw_showstr(MLME_DBG_LEVEL1,"mlme_reassoc_rsp_rcv_action...");    
        fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_REASSOC_RSP_RCV to SYS state machine...");
    }
    mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYS_STATE_MACHINE,MLME_REASSOC_RSP_RCV,0,0,0,0);
                            
#ifdef MLME_ADS
    fw_del_timer(MLME_TIMER_V1);
#else
    fw_del_timer(&mlme_p->mlmetimer);
#endif
    return 1;
}

static void mlme_assoc_cls3_err(mlme_data_t *mlme_p)
{
    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"mlme_assoc_cls3_err...");
    mlme_p->mAssoc=0;
}

static void mlme_assoc_timeout_action(mlme_data_t *mlme_p)
{
    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"mlme_assoc_timeout_action...");
    
    //restart by set essid
    mlme_set_ssid_action(mlme_p);
}


static void mlme_disassoc_timeout_action(mlme_data_t *mlme_p)
{
    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"mlme_disassoc_timeout_action...");
}


/* return disassoc body length */
UINT32 mlme_make_disassoc(mlme_data_t *mlme_p,UINT8 *pkt,UINT16 reason)
{
    I802_11_pkt_t   *mlme_pkt=(I802_11_pkt_t *)pkt;
    
    mlme_make_header(pkt,SUBTYPE_DISASSOC,0,0,mlme_p->bss.mBssid,mlme_p->mMAC,mlme_p->bss.mBssid);

    mlme_pkt->fb[0]=reason&0xff;
    mlme_pkt->fb[1]=(reason&0xff00)>>8;    
    return 2;
}


/* return assoc body length */
UINT32 mlme_make_assoc(mlme_data_t *mlme_p,UINT8 *pkt)
{
    I802_11_pkt_t   *mlme_pkt=(I802_11_pkt_t *)pkt;
    UINT32          len,len2;
    UINT16          BcnInterval=mlme_p->mInterval_config;
    UINT16          cap=MLME_CAP_NULL;
    
    mlme_make_header(pkt,SUBTYPE_ASSOC_REQ,0,0,mlme_p->bss.mBssid,mlme_p->mMAC,mlme_p->bss.mBssid);

    /* capability info */
    mlme_pkt->fb[0]=cap&0xff;
    mlme_pkt->fb[1]=(cap&0xff00)>>8;
   	    
    /* beacon interval */
    mlme_pkt->fb[2]=BcnInterval&0xff;
    mlme_pkt->fb[3]=(BcnInterval&0xff00)>>8;

    /* essid */
    mlme_pkt->fb[4]=MLME_ELEMENT_SSID;
    mlme_pkt->fb[5]=len2=(unsigned char)strlen(mlme_p->bss.mEssid);
  	fw_memcpy(&mlme_pkt->fb[6],mlme_p->bss.mEssid,len2);

    len=6+len2;
    /* support rate */
    mlme_pkt->fb[len]=MLME_ELEMENT_RATE;
    mlme_pkt->fb[len+1]=len2=(unsigned char)strlen(mlme_p->mRate_info);
    fw_memcpy(&mlme_pkt->fb[len+2],mlme_p->mRate_info,len2);    
    len=len+2+len2;
    
    /* SSN support ?*/
    if(mlme_p->bss.wpa_ie[0]!=0)
    {
        fw_memcpy(&mlme_pkt->fb[len],mlme_p->bss.wpa_ie,mlme_p->bss.wpa_ie[1]+2);
        len=len+mlme_p->bss.wpa_ie[1]+2;
    }
    
    if(len>MLME_ASSOC_BODY_LEN)
    {
        fw_printf(MLME_DBG_LEVEL1,"\nerror assoc packet length-------------------\n");
        mlme_dump_data(pkt,len);
        while(1)
            ;
    }       
    return len;
}


/* return assoc response message */
static UINT32 mlme_get_assoc_rsp(mlme_data_t *mlme_p,UINT8 *pkt,UINT32 pktlen)
{ 
	//AID number
    mAID = (((UINT32)pkt[29] & 0x3F) << 8) + (UINT32)pkt[28];	
    //printk("\AID = 0x%x,",mAID);    
    
    return 1;  
}