/*
 * mlme/mlme.c
 *
 * Copyright (C) 2004  Ivan Wang <ivan@faraday.com.tw>
 */ 
#include "mlme.h"
#include "hw_ctrl.h"

static void mlme_auth_req_action(mlme_data_t *mlme_p);
static UINT32 mlme_auth_even_rcv_action(mlme_data_t *mlme_p,parm_entry_t *parm);
static void mlme_auth_timeout_action(mlme_data_t *mlme_p);
static void mlme_deauth_rcv_action(mlme_data_t *mlme_p);
static void mlme_deauth_req_action(mlme_data_t *mlme_p);
static void mlme_auth_timeout_function(mlme_data_t *mlme_p);


void mlme_handle_auth_state(mlme_data_t *mlme_p,msg_entry_t *entry)
{
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"Handle AUTH State Machine...");
        
    switch(entry->message)
    {
        case MLME_STOP:
            mlme_p->mlme_auth_state=AUTH_IDLE;
            break;        
        case MLME_AUTH_REQ:
            if(mlme_p->mMode==MLME_ADHOC_MODE)
            {
                fw_showstr(MLME_DBG_LEVEL1,"ADHOC:not support...");
            }
            else
            {  
                if(mlme_p->mlme_auth_state==AUTH_IDLE)
                {
                    mlme_auth_req_action(mlme_p);
                    mlme_p->mlme_auth_state=WAIT_SEQ_2;
                }
            }
            break;
        case MLME_AUTH_ODD_RCV:
            if(mlme_p->mMode==MLME_ADHOC_MODE)
            {
                fw_showstr(MLME_DBG_LEVEL1,"ADHOC:not support...");
            }
            else
            {
                fw_showstr(MLME_DBG_LEVEL1,"BSS:AP only...");
            }
            break;
        case MLME_AUTH_EVEN_RCV:
            if(mlme_p->mMode==MLME_ADHOC_MODE)
            {
                fw_showstr(MLME_DBG_LEVEL1,"ADHOC:not support...");
            }
            else
            {
                if(mlme_p->mlme_auth_state==WAIT_SEQ_2)
                {                    
                    if(mlme_auth_even_rcv_action(mlme_p,&entry->parm))
                    {
                        if(mlme_p->mAuth_config==MLME_AUTH_ALG_KEY)
                            mlme_p->mlme_auth_state=WAIT_SEQ_4;
                        else
                        {
                            mlme_p->mAuth=1;
                            mlme_p->mlme_auth_state=AUTH_IDLE;
                        }
                    }
                }
                else if(mlme_p->mlme_auth_state==WAIT_SEQ_4)
                {
                    if(mlme_auth_even_rcv_action(mlme_p,&entry->parm))
                    {                        
                        mlme_p->mlme_auth_state=AUTH_IDLE;
                        mlme_p->mAuth=1;
                    }
                }
            }
            break;
        case MLME_AUTH_TIMEOUT:
            if( (mlme_p->mlme_auth_state==WAIT_SEQ_2) || (mlme_p->mlme_auth_state==WAIT_SEQ_4))
            {
                mlme_auth_timeout_action(mlme_p);
                mlme_p->mlme_auth_state=AUTH_IDLE;
            }
            break;
        case MLME_DEAUTH_RCV:
            if(mlme_p->mAuth)
                mlme_deauth_rcv_action(mlme_p);
            break;
        case MLME_DEAUTH_REQ:
            if(mlme_p->mAuth)
                mlme_deauth_req_action(mlme_p);
            break;        
        default:
            while(1)
                ;
    }
}

static void mlme_auth_timeout_function(mlme_data_t *mlme_p)
{
    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"mlme_auth_timeout_function...");
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_AUTH_TIMEOUT to AUTH state machine...");

    mlme_msg_enqueue(&mlme_p->mQueue,MLME_AUTH_STATE_MACHINE,MLME_AUTH_TIMEOUT,0,0,0,0);
    //printk("MLME_main() from mlme_auth_timeout_function\n");
    MLME_main(mlme_p); //see MLME_main()
}

static void mlme_auth_req_action(mlme_data_t *mlme_p)
{
    UINT32  value;
    I802_11_pkt_t   pkt;
    UINT32 bodylen;
    
    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"mlme_auth_req_action...");
    
    //send auth odd packet
    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"send auth odd packet...");    

    bodylen=mlme_make_auth(mlme_p,(UINT8 *)&pkt,1,0,0);
    if(mlme_dbg_level&MLME_DEBUG_MLME_TX_DUMP)
    {
        mlme_dump_data((UINT8 *)&pkt,bodylen+MLME_MGMT_HDR_LEN);
        printk("bodylen=%d\n",bodylen);
    }
    HW_tx((UINT8 *)&pkt,bodylen);

    //wait auth even rcv timer
    if(mlme_p->mAuth_config==0) //open
        value=MLME_AUTH_SHORT_TIMEOUT;
    else
        value=MLME_AUTH_LONG_TIMEOUT;
        
#ifdef MLME_ADS
    fw_setup_timer(MLME_TIMER_V1,value,(UINT32)mlme_p,mlme_auth_timeout_function);
    fw_add_timer(MLME_TIMER_V1);
#else    
    fw_setup_timer(&mlme_p->mlmetimer,value,mlme_p,mlme_auth_timeout_function);
    fw_add_timer(&mlme_p->mlmetimer);
#endif
}


static UINT32 mlme_auth_even_rcv_action(mlme_data_t *mlme_p,parm_entry_t *parm)
{
    UINT32          value;
    I802_11_pkt_t   pkt;
    I802_11_pkt_t   *mpkt=(I802_11_pkt_t *)(parm->parm1);
    
    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"mlme_auth_even_rcv_action...");    
    
//    printk("parm->len1=%d\n",parm->len1);
    //mlme_dump_data(mpkt,30);
    //wait auth even rcv timer
    if(mlme_p->mAuth_config==0) //open
        value=MLME_AUTH_SHORT_TIMEOUT;
    else
        value=MLME_AUTH_LONG_TIMEOUT;

    if((mlme_p->mlme_auth_state==WAIT_SEQ_2) && (mpkt->fb[2]==2))
    {
        UINT32 bodylen;
//printk("mlme_p->mlme_auth_state==WAIT_SEQ_2\n");
        //check sucessful    
        if(mlme_p->mAuth_config==0)
        {
//printk("mlme_p->mAuth_config==0\n");
            if((mpkt->fb[4]==0)&&(mpkt->fb[5]==0))  //sucessful?
            {
                if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
                    fw_showstr(MLME_DBG_LEVEL1,"(successful)Send Message:MLME_AUTH_EVEN_RCV to SYS state machine...");
                mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYS_STATE_MACHINE,MLME_AUTH_EVEN_RCV,0,0,0,0);
                fw_del_timer(&mlme_p->mlmetimer);
                return 1;
            }
            else
            {
                if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
                    fw_showstr(MLME_DBG_LEVEL1,"Authentication2 Not Sucessful!!! %d %d %d\n",mpkt->fb[4],mpkt->fb[5],mlme_p->mAuth_config);
                mlme_dump_data((UINT8 *)mpkt,30);
                
                /* restart */
                mlme_reset(mlme_p);
                if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
                    fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_AUTH_REQ to AUTH state machine...");
                mlme_msg_enqueue(&mlme_p->mQueue,MLME_AUTH_STATE_MACHINE,MLME_AUTH_REQ,0,0,0,0);
            }
        }
        else
        {
            //send auth odd packet
            if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
                fw_showstr(MLME_DBG_LEVEL1,"send auth odd packet(challenge back)...");        
            bodylen=mlme_make_auth(mlme_p,(UINT8 *)&pkt,3,0,&mpkt->fb[6]);
            if(mlme_dbg_level&MLME_DEBUG_MLME_TX_DUMP)
            {
                printk("bodylen=%d\n",bodylen);
                mlme_dump_data((UINT8 *)&pkt,30);
            }
            HW_tx((UINT8 *)&pkt,bodylen);
#ifdef MLME_ADS
            fw_setup_timer(MLME_TIMER_V1,value,(UINT32)mlme_p,mlme_auth_timeout_function);
            fw_add_timer(MLME_TIMER_V1);
#else    
            fw_setup_timer(&mlme_p->mlmetimer,value,mlme_p,mlme_auth_timeout_function);
            fw_add_timer(&mlme_p->mlmetimer);
#endif
        }
    }
    else if(mlme_p->mlme_auth_state==WAIT_SEQ_4  && (mpkt->fb[2]==4))
    {
        if((mpkt->fb[4]==0)&&(mpkt->fb[5]==0))  //sucessful
        {

            if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
                fw_showstr(MLME_DBG_LEVEL1,"(successful)Send Message:MLME_AUTH_EVEN_RCV to SYS state machine...");
            mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYS_STATE_MACHINE,MLME_AUTH_EVEN_RCV,0,0,0,0);
#ifdef MLME_ADS        
            fw_del_timer(MLME_TIMER_V1);
#else
            fw_del_timer(&mlme_p->mlmetimer);
#endif        
        }
        else
        {
            if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
                printk("Authentication4 Not Sucessful!!! %d %d %d\n",mpkt->fb[4],mpkt->fb[5],mlme_p->mAuth_config);
            /* restart */
            mlme_reset(mlme_p);
            if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
                fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_AUTH_REQ to AUTH state machine...");
            mlme_msg_enqueue(&mlme_p->mQueue,MLME_AUTH_STATE_MACHINE,MLME_AUTH_REQ,0,0,0,0);            
        }            
    }
    return 1;
}

static void mlme_auth_timeout_action(mlme_data_t *mlme_p)
{
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"mlme_auth_timeout_action...");
}

static void mlme_deauth_rcv_action(mlme_data_t *mlme_p)
{
    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"mlme_deauth_rcv_action...");
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_DEAUTH_RCV to SYS state machine...");
    mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYS_STATE_MACHINE,MLME_DEAUTH_RCV,0,0,0,0);    
}

static void mlme_deauth_req_action(mlme_data_t *mlme_p)
{
    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"mlme_deauth_req_action...");
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_DEAUTH_REQ to SYS state machine...");
    mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYS_STATE_MACHINE,MLME_DEAUTH_RCV,0,0,0,0);    
}


UINT32 mlme_make_auth(mlme_data_t *mlme_p,UINT8 *pkt,UINT16 seq,UINT16 status,UINT8 *chal_element)
{
    I802_11_pkt_t *mlme_pkt=(I802_11_pkt_t *)pkt;
    UINT16 alg=mlme_p->mAuth_config;
    UINT32 len=0,len2=0;

    if(chal_element)
        mlme_make_header(pkt,SUBTYPE_AUTH,0,1,mlme_p->bss.mBssid,mlme_p->mMAC,mlme_p->bss.mBssid);
    else
        mlme_make_header(pkt,SUBTYPE_AUTH,0,0,mlme_p->bss.mBssid,mlme_p->mMAC,mlme_p->bss.mBssid);
    
    /* auth algorithm */
    mlme_pkt->fb[len++]=alg&0xff;
    mlme_pkt->fb[len++]=(alg&0xff00)>>8;
    
    /* auth seq number */
    mlme_pkt->fb[len++]=seq&0xff;
    mlme_pkt->fb[len++]=(seq&0xff00)>>8;
    
    /* auth status */
    mlme_pkt->fb[len++]=status&0xff;
    mlme_pkt->fb[len++]=(status&0xff00)>>8;

    if(chal_element)
    {       
        len2=chal_element[1];
        if(len2>MLME_MAX_FRAME_BODY_LEN)
        {
            printk("Invalide len2 size=%d(>%d)\n",len2,MLME_MAX_FRAME_BODY_LEN);
            len2=MLME_MAX_FRAME_BODY_LEN;
        }
        //printk("Challenge len2=%d\n",len2);
        //mlme_dump_data(chal_element,len2);            
        len2+=2;
        fw_memcpy(&mlme_pkt->fb[len],chal_element,len2);
    }
    
    len+=len2;
    return len;
}


