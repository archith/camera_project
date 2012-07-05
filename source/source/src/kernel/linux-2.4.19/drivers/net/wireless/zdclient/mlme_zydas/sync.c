/*
 * mlme/mlme.c
 *
 * Copyright (C) 2004  Ivan Wang <ivan@faraday.com.tw>
 */ 
#include "mlme.h"
#include "hw_ctrl.h"

static void mlme_join_req_action(mlme_data_t *mlme_p);
static UINT32 mlme_beacon_rcv_action(mlme_data_t *mlme_p,UINT32,parm_entry_t *parm);
static void mlme_join_timeout_action(mlme_data_t *mlme_p);
static void mlme_scan_req_action(mlme_data_t *mlme_p);
static void mlme_scan_complete_action(mlme_data_t *mlme_p);
static void mlme_probe_req_rcv_action(mlme_data_t *mlme_p,UINT8 *dest);
static void mlme_start_req_action(mlme_data_t *mlme_p);
static void mlme_join_timeout_function(mlme_data_t *mlme_p);

//extern int mlme_dbg;
void mlme_handle_sync_state(mlme_data_t *mlme_p,msg_entry_t *entry)
{

//if(mlme_dbg==1)
//    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
//        fw_showstr(MLME_DBG_LEVEL1,"Handle SYNC State Machine...");
        
    switch(entry->message)
    {
        case MLME_STOP:
            mlme_p->mlme_sync_state=SYNC_IDLE;
            break;        
        case MLME_START_REQ:
            mlme_start_req_action(mlme_p);
            break;
        case MLME_JOIN_REQ:
            if(mlme_p->mlme_sync_state==SYNC_IDLE)
            {
                mlme_join_req_action(mlme_p);
                mlme_p->mlme_sync_state=JOIN_WAIT_BEACON;
            }
            break;
        case MLME_PROBE_RSP_RCV:
//        printk("MLME_PROBE_RSP_RCV\n");
        case MLME_BEACON_RCV:
            if(mlme_p->mlme_sync_state==JOIN_WAIT_BEACON)
            {
                if(mlme_beacon_rcv_action(mlme_p,1,&entry->parm))
                {
                    mlme_p->mlme_sync_state=SYNC_IDLE;
                }
            }
            else
                mlme_beacon_rcv_action(mlme_p,0,&entry->parm);                
            break;
        case MLME_PROBE_REQ_RCV:
            if(mlme_p->mMode==MLME_ADHOC_MODE)
            {
                if(mlme_p->mlme_sync_state==SYNC_IDLE)
                    mlme_probe_req_rcv_action(mlme_p,entry->parm.parm1);
            }
        case MLME_JOIN_TIMEOUT:
            if(mlme_p->mlme_sync_state==JOIN_WAIT_BEACON)
            {
                mlme_join_timeout_action(mlme_p);
                mlme_p->mlme_sync_state=SYNC_IDLE;
            }
            break;
        case MLME_SCAN_REQ:
//            fw_printf(MLME_DBG_LEVEL1,"scan req...................\n");
            if(mlme_p->mlme_sync_state==SYNC_IDLE)
            {
                mlme_scan_req_action(mlme_p);
                mlme_p->mlme_sync_state=SCAN_LISTEN;
            }
            break;
        case MLME_SCAN_COMPLETE:
//        printk("scan complete:%d\n",mlme_p->mlme_sync_state);
            if(mlme_p->mlme_sync_state==SCAN_LISTEN)
            {
//                printk("scan complete2\n");
                mlme_scan_complete_action(mlme_p);
                mlme_p->mlme_sync_state=SYNC_IDLE;
            }
            break;
        default:
            while(1)
                ;
    }
}


static void mlme_join_req_action(mlme_data_t *mlme_p)
{
    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"mlme_join_req_action...");

    /* stop beacon to wait another beacon*/
    if(mlme_p->mMode==MLME_ADHOC_MODE)
    {
        if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
            fw_showstr(MLME_DBG_LEVEL1,"[HW] HW_CMD_STOP_BEACON");
        HW_ioctl(mlme_p->hw_private,HW_CMD_STOP_BEACON,0,0);
    }

    /* set channel */
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"[HW] HW_CMD_SET_CHANNLE by %d",mlme_p->bss.mChannel);
    HW_ioctl(mlme_p->hw_private,HW_CMD_SET_CHANNLE,&mlme_p->bss.mChannel,0);

#ifdef MLME_ADS
    fw_setup_timer(MLME_TIMER_V1,MLME_JOIN_TIMEOUT_VALUE,(UINT32)mlme_p,mlme_join_timeout_function);
    fw_add_timer(MLME_TIMER_V1);
#else
    fw_setup_timer(&mlme_p->mlmetimer,MLME_JOIN_TIMEOUT_VALUE,mlme_p,mlme_join_timeout_function);
    fw_add_timer(&mlme_p->mlmetimer);
#endif
}


/* 0:normal beacon  1:join wait beacon  */
static UINT32 mlme_beacon_rcv_action(mlme_data_t *mlme_p,UINT32 mode,parm_entry_t *parm)
{
    UINT8           *pkt=(UINT8 *)parm->parm1;
    UINT8           *signal_data=(UINT8 *)parm->parm2;
    UINT32          pktlen=parm->len1;
    UINT32          ret=0,res;
    UINT32          tsf[2];
    BSS_entry_t     bss_entry;

    fw_memset(&bss_entry,0,sizeof(BSS_entry_t));
    
    /* take beacon information */
    //printk("(%d)",pktlen);
    if(!mlme_parsing_beacon_info(mlme_p,&bss_entry,pkt,pktlen))
        return 0;
    
    /* add quality/strength */
    bss_entry.quality=signal_data[0];
    bss_entry.strength=signal_data[1];

//    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
//        printk("bss_entry.mEssid[0]=%c\n",bss_entry.mEssid[0]);
    
    if((fw_strcmp(bss_entry.mEssid,mlme_p->bss.mEssid)==0) || (bss_entry.mEssid[0]==0))   //it is my ssid,NULL ssid
    {
        if(!fw_memcmp(mlme_p->bss.mBssid,bss_entry.mBssid,MACADDR_LEN)) //if my bssid
        {
            //if not specified essid, learn it from beacon (use at iwconfig eth1 ap 00:80..)
            //but can't work when AP is ssid broadcast disabled
            if( (mlme_p->bss.mEssid[0]==0) && (bss_entry.mEssid[0]!=0) )
            {
                fw_memcpy(mlme_p->bss.mEssid,bss_entry.mEssid,MACADDR_LEN);
                if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
                    printk("learn a bssid:%x %x %x\n",mlme_p->bss.mEssid[0],mlme_p->bss.mEssid[1],mlme_p->bss.mEssid[2]);
            }
            
            /* set beacon time */
            mlme_p->mLastBeaconTime=mlme_get_time(mlme_p);
            mlme_p->bss.quality=signal_data[0]; //update bss quality
            mlme_p->bss.strength=signal_data[1]; //update bss strength
            
            /* set support rate if need */
            if((mlme_p->mMode==MLME_INFRA_MODE) && (mlme_p->mAssoc))
            {
                if((res=fw_strcmp(mlme_p->bss.rate,bss_entry.rate))!=0)
                {
                    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
                    {
                        printk("(%d)mlme_p->bss.rate=%d %d %d %d %d(%d),",res,mlme_p->bss.rate[0],mlme_p->bss.rate[1],mlme_p->bss.rate[2],mlme_p->bss.rate[3],mlme_p->bss.rate[4],strlen(bss_entry.rate));
                        printk("bss_entry.rate=%d %d %d %d %d(%d)\n",bss_entry.rate[0],bss_entry.rate[1],bss_entry.rate[2],bss_entry.rate[3],bss_entry.rate[4],strlen(bss_entry.rate));
                        fw_showstr(MLME_DBG_LEVEL1,"[HW2] HW_CMD_SET_RATE_INFO by %d",mlme_p->bss.basic_rate);
                    }
                    HW_ioctl(mlme_p->hw_private,HW_CMD_SET_BASIC_RATE,&mlme_p->bss.basic_rate,0);
                }
            }

            if((mode==1) && (!mlme_p->mAssoc)) //join wait beacon
            {
                if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
                    printk("is wait beacon!!\n");
                fw_del_timer(&mlme_p->mlmetimer);
                if(mlme_p->mMode==MLME_ADHOC_MODE)
                {
                    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
                        fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_JOIN_REQ to SYS state machine...");
                    mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYS_STATE_MACHINE,MLME_JOIN_REQ,0,0,0,0);
                }
                else
                {
                    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
                        fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_AUTH_REQ to AUTH state machine...");
                    mlme_msg_enqueue(&mlme_p->mQueue,MLME_AUTH_STATE_MACHINE,MLME_AUTH_REQ,0,0,0,0);
                }
                ret=1;  //return for join wait beacon
            }
        }
        else //not my bssid
        {
            if((mlme_p->mMode==MLME_ADHOC_MODE) && (mlme_p->mAssoc))
            {
                int i;
                //choose small BSSID instead
                for(i=0;i<6;i++)
                {
                    if(mlme_p->bss.mBssid[i]>bss_entry.mBssid[i])
                    {
                        if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
                        {
                            printk("### Replace BSSID from ");
                            mlme_dump_data(mlme_p->bss.mBssid,6);
                            printk(" by ");
                            mlme_dump_data(bss_entry.mBssid,6);
                        }
                        
                        fw_memcpy(mlme_p->bss.mBssid,bss_entry.mBssid,MACADDR_LEN);
                        mlme_reset(mlme_p);//reset all state machine and restart by new bssid
                        if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
                            fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_JOIN_REQ to SYNC state machine...");
                        mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_JOIN_REQ,0,0,0,0);
                        break;
                    }
                }
            }
        }
    }
    else
    {
        //if not specified essid and specified bssid, learn it from beacon (use at iwconfig eth1 ap 00:80..)
        if(!fw_memcmp(mlme_p->bss.mBssid,bss_entry.mBssid,MACADDR_LEN)) //if my bssid
            if(mlme_p->bss.mEssid[0]==0)
                fw_strcpy(mlme_p->bss.mEssid,bss_entry.mEssid);        
        ret=0;        
    }
     

    HW_ioctl(mlme_p->hw_private,HW_CMD_GET_TIME,(UINT8 *)&tsf,0);
    bss_entry.timestamp=tsf[0];    //low part

     /* add to table */
    mlme_table_insert(&mlme_p->mTable,&bss_entry);

    return ret;
}

static void mlme_join_timeout_action(mlme_data_t *mlme_p)
{
    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"mlme_join_timeout_action...");
        
    if(mlme_p->mMode==MLME_ADHOC_MODE)
    {
        if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
            fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_START_REQ to SYNC state machine...");
        mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_START_REQ,0,0,0,0);
    }
    else
    {
        if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
            fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_STOP to SYNC state machine...");
        mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_STOP,0,0,0,0);
    }    
}


static void mlme_join_timeout_function(mlme_data_t *mlme_p)
{
    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"mlme_join_timeout_function...");

    if(mlme_p->mChannel_config!=MLME_CHANNEL_NOCHECK)
    {
        if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
            fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_SCAN_REQ to SYNC state machine...");
        mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_SCAN_REQ,0,0,0,0);
    }
    else
    {
        if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
            fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_JOIN_TIMEOUT to SYNC state machine...");
        mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_JOIN_TIMEOUT,0,0,0,0);
    }
    
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
        printk("MLME_main() from mlme_join_timeout_function\n");
    MLME_main(mlme_p); //see MLME_main()
    return;
}


static void mlme_scan_timeout_function(mlme_data_t *mlme_p)
{   
    I802_11_pkt_t   pkt;
    UINT32          i,len;
    
    //fw_printf(MLME_DBG_LEVEL1,"mlme_scan_timeout_function\n");
    
    //fix scan channel
    if(mlme_p->mChannel_config!=MLME_CHANNEL_NOCHECK)
    {
        fw_del_timer(&mlme_p->mlmetimer);
        
        if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
            fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_SCAN_COMPLETE to SYNC state machine...");
        mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_SCAN_COMPLETE,0,0,0,0);
        return;
    }
    
    //select next channel => i+1
    for(i=0;mlme_p->mChannel_info[i]!=0;i++)
    {
        if(mlme_p->mChannel_info[i]==mlme_p->bss.mChannel)
            break;
    }

    if(mlme_p->mChannel_info[i+1]==0)   //scan end
    {
        fw_del_timer(&mlme_p->mlmetimer);
        if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
            fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_SCAN_COMPLETE to SYNC state machine...");
        mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_SCAN_COMPLETE,0,0,0,0);
    }
    else
    {
        mlme_p->bss.mChannel=mlme_p->mChannel_info[i+1];
    }
    
    /* set channel */
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"[HW] HW_CMD_SET_CHANNLE by %d",mlme_p->bss.mChannel);
    HW_ioctl(mlme_p->hw_private,HW_CMD_SET_CHANNLE,&mlme_p->bss.mChannel,0);
    
    /* send probe request packet */    
    if(mlme_p->mScan_type==MLME_ACTIVE_SCAN)
    {
        if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
            fw_showstr(MLME_DBG_LEVEL1,"Send probe request packet by channel %d",mlme_p->bss.mChannel);
        len=mlme_make_probe_req(mlme_p,(UINT8 *)&pkt);
        HW_tx((UINT8 *)&pkt,len);
    }
    
    if(mlme_p->mChannel_info[i+2]!=0)   //scan next next?
    {
#ifdef MLME_ADS
        fw_setup_timer(MLME_TIMER_V1,MLME_MAX_CHANNEL_SCAN_TIME,(UINT32)mlme_p,mlme_scan_timeout_function);
        fw_add_timer(MLME_TIMER_V1);
#else    
        fw_setup_timer(&mlme_p->mlmetimer,MLME_MAX_CHANNEL_SCAN_TIME,mlme_p,mlme_scan_timeout_function);
        fw_add_timer(&mlme_p->mlmetimer);
#endif    
    }
    else
    {
        fw_del_timer(&mlme_p->mlmetimer);
        if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
            fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_SCAN_COMPLETE to SYNC state machine...");
        mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_SCAN_COMPLETE,0,0,0,0);
    }
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
        printk("MLME_main() from mlme_scan_timeout_function\n");
    MLME_main(mlme_p); //see MLME_main()
}


static void mlme_scan_req_action(mlme_data_t *mlme_p)
{   
    I802_11_pkt_t   pkt;
    UINT32          len;

    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)            
        fw_showstr(MLME_DBG_LEVEL1,"mlme_scan_req_action...");
   
    if(mlme_p->mChannel_config==MLME_CHANNEL_NOCHECK)
        mlme_p->bss.mChannel=mlme_p->mChannel_info[0];
    else
        mlme_p->bss.mChannel=mlme_p->mChannel_config;

    /* set channel */
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"[HW] HW_CMD_SET_CHANNLE by %d",mlme_p->bss.mChannel);
    HW_ioctl(mlme_p->hw_private,HW_CMD_SET_CHANNLE,&mlme_p->bss.mChannel,0);

    /* send probe request packet */    
    if(mlme_p->mScan_type==MLME_ACTIVE_SCAN)
    {
        if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
            fw_showstr(MLME_DBG_LEVEL1,"Send probe request packet by channel %d",mlme_p->bss.mChannel);
        len=mlme_make_probe_req(mlme_p,(UINT8 *)&pkt);
        HW_tx((UINT8 *)&pkt,len);
    }

#ifdef MLME_ADS
    fw_setup_timer(MLME_TIMER_V1,MLME_MAX_CHANNEL_SCAN_TIME,(UINT32)mlme_p,mlme_scan_timeout_function);
    fw_add_timer(MLME_TIMER_V1);
#else    
    fw_setup_timer(&mlme_p->mlmetimer,MLME_MAX_CHANNEL_SCAN_TIME,mlme_p,mlme_scan_timeout_function);
    fw_add_timer(&mlme_p->mlmetimer);
#endif
}


/* it measn scan completely and automatically join BSS*/
static void mlme_scan_complete_action(mlme_data_t *mlme_p)
{
    BSS_entry_t     *bss_entry;

    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)    
        fw_showstr(MLME_DBG_LEVEL1,"mlme_scan_complete_action...");
    
    if(!mlme_p->mAssoc)
    {
        //do join it again
        bss_entry=mlme_search_table_by_essid(&mlme_p->mTable,mlme_p->bss.mEssid,mlme_p->mChannel_config);
        if(!bss_entry)
        {
            if(mlme_p->mMode==MLME_ADHOC_MODE)
            {
                if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
                    fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_START_REQ to SYNC state machine...");
                mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_START_REQ,0,0,0,0);
            }
        }
        else
        {            
            fw_memcpy(&mlme_p->bss,bss_entry,sizeof(BSS_entry_t));
            if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
                fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_JOIN_REQ to SYNC state machine...");
            mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_JOIN_REQ,0,0,0,0);
        }
    }
}

static void mlme_probe_req_rcv_action(mlme_data_t *mlme_p,UINT8 *dest)
{
    UINT32  len;
    I802_11_pkt_t   pkt;
    
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
    {
        fw_showstr(MLME_DBG_LEVEL1,"mlme_probe_req_rcv_action...");    
        fw_showstr(MLME_DBG_LEVEL1,"send probe response packet...");    
    }
    len=mlme_make_probe_rsp(mlme_p,(UINT8 *)&pkt,dest);
    HW_tx((UINT8 *)&pkt,len);
}

void mlme_make_random_bssid(mlme_data_t *mlme_p)
{    
    mlme_p->bss.mBssid[0]=(fw_rand_bytes()&0xfe)|0x02;
    mlme_p->bss.mBssid[1]=fw_rand_bytes();
    mlme_p->bss.mBssid[2]=fw_rand_bytes();
    mlme_p->bss.mBssid[3]=fw_rand_bytes();
    mlme_p->bss.mBssid[4]=fw_rand_bytes();
    mlme_p->bss.mBssid[5]=fw_rand_bytes();
    mlme_p->bss.mInterval=BEACON_INTERVAL;
//ivan
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
    {
        printk("Random BSSID:\n");
        mlme_dump_data(mlme_p->bss.mBssid,6);
    }
    return;
}

static void mlme_start_req_action(mlme_data_t *mlme_p)
{
    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"mlme_start_req_action...");
    
    if(mlme_p->mChannel_config!=MLME_CHANNEL_NOCHECK)
        mlme_p->bss.mChannel=mlme_p->mChannel_config;    
    mlme_make_random_bssid(mlme_p);
    
    /* set basic rate */
    mlme_p->bss.basic_rate=mlme_p->basic_rate_config;
    
    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"Send Message5:MLME_START_REQ to SYS state machine...");
    mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYS_STATE_MACHINE,MLME_START_REQ,0,0,0,0);
}


extern UINT8  mBroadcast[MACADDR_LEN];

/* return assoc body length */
UINT32 mlme_make_probe_req(mlme_data_t *mlme_p,UINT8 *pkt)
{
    I802_11_pkt_t   *mlme_pkt;
    UINT32          len,len2;
    
    mlme_pkt=(I802_11_pkt_t *)pkt;
    mlme_make_header(pkt,SUBTYPE_PROBE_REQ,0,0,mBroadcast,mlme_p->mMAC,mBroadcast);

    /* essid */
    mlme_pkt->fb[0]=MLME_ELEMENT_SSID;
    mlme_pkt->fb[1]=len2=(unsigned char)strlen(mlme_p->bss.mEssid);
  	fw_memcpy(&mlme_pkt->fb[2],mlme_p->bss.mEssid,len2);

    len=2+len2;
    /* support rate */
    mlme_pkt->fb[len]=MLME_ELEMENT_RATE;
    mlme_pkt->fb[len+1]=len2=(unsigned char)strlen(mlme_p->mRate_info);
    fw_memcpy(&mlme_pkt->fb[len+2],mlme_p->mRate_info,len2);
    
    len=len+2+len2;
    
    if(len>MLME_ASSOC_BODY_LEN)
    {
        fw_printf(MLME_DBG_LEVEL1,"\nerror assoc packet length-------------------\n");
        mlme_dump_data(pkt,len);
        while(1)
            ;
    }       
    return len;
}


UINT32 mlme_make_probe_rsp(mlme_data_t *mlme_p,UINT8 *pkt,UINT8 *dest)
{
    int             i;
    I802_11_pkt_t   *mlme_pkt;
    UINT32          len=0,len2;
    
    mlme_pkt=(I802_11_pkt_t *)pkt;
    mlme_make_header(pkt,SUBTYPE_PROBE_RSP,0,0,dest,mlme_p->mMAC,mlme_p->bss.mBssid);

	for (i=0; i<8; i++)
		mlme_pkt->fb[len++] = 0x0;

	/* BeaconInterval */
	mlme_pkt->fb[len++] = mlme_p->bss.mInterval;
	mlme_pkt->fb[len++] = mlme_p->bss.mInterval >> 8;

	/* Capability */
	mlme_pkt->fb[len++] = MLME_CAP_IBSS;
	mlme_pkt->fb[len++] = 0;
	
    /* essid */
    mlme_pkt->fb[len++]=MLME_ELEMENT_SSID;
    mlme_pkt->fb[len++]=len2=(unsigned char)strlen(mlme_p->bss.mEssid);
  	fw_memcpy(&mlme_pkt->fb[len],mlme_p->bss.mEssid,len2);
    len+=len2;
    
    /* support rate */
    mlme_pkt->fb[len++]=MLME_ELEMENT_RATE;
    mlme_pkt->fb[len++]=len2=(unsigned char)strlen(mlme_p->mRate_info);
    fw_memcpy(&mlme_pkt->fb[len],mlme_p->mRate_info,len2);
    len+=len2;
    
    /* Direct Set */
    mlme_pkt->fb[len++]=MLME_ELEMENT_DS;
    mlme_pkt->fb[len++]=1;
    mlme_pkt->fb[len++]=mlme_p->bss.mChannel;

    return len;
}


UINT32 mlme_parsing_beacon_info(mlme_data_t *mlme_p,BSS_entry_t *bss_entry,UINT8 *pkt,UINT32 pktlen)
{
    I802_11_pkt_t   *mpkt=(I802_11_pkt_t *)pkt;
    I802_11_hdr_t   *mhdr=(I802_11_hdr_t *)pkt;
    UINT32          len=0,elen=0;
    info_element_t  *ie_p,*tmp_p;

    /* privacy */
    if((mpkt->fb[10]&0x10)==0x10)
        bss_entry->mPrivacy=MLME_AUTH_ALG_KEY;
    
    /* mode */
    if((mpkt->fb[10]&0x2)==0x2)
        bss_entry->mMode=MLME_ADHOC_MODE;
    else
        bss_entry->mMode=MLME_INFRA_MODE;
        
    /* take bssid */
    memcpy(bss_entry->mBssid,mhdr->addr3,MACADDR_LEN);

    /* Information Element */    
    ie_p=(info_element_t  *)&mpkt->fb[12]; //first element at 12 bytes offset of body
    tmp_p=ie_p;
    
    elen=pktlen-24-12;  //element length (-header(24)-12(offset))
    
    while(1)
    {
        switch(tmp_p->id)
        {
            case MLME_ELEMENT_SSID:
                if(tmp_p->len>MAX_ESSID_SIZE)
                    return 0;
                fw_memcpy(bss_entry->mEssid,tmp_p->info,tmp_p->len);
                bss_entry->mEssid[tmp_p->len]=0;
                break;
            case MLME_ELEMENT_RATE:
                {
                    UINT8   tmp_rate;
                    UINT8   max=0;
                    UINT32  i;

                    for(i=0;i<tmp_p->len;i++)
                    {
                        tmp_rate=tmp_p->info[i];
                        if(tmp_rate>max)
                            max=tmp_rate;
                        tmp_rate&=0x7f;
                        bss_entry->rate[i]=tmp_rate;
//                        printk("==>0x%x\n",bss_entry->rate[i]);
                    }
                    bss_entry->basic_rate=max;
                    bss_entry->rate[i]=0;
                }
                break;
            case MLME_ELEMENT_FH:
                break;
            case MLME_ELEMENT_DS:
                bss_entry->mChannel=tmp_p->info[0];    //get channel
                break;
            case MLME_ELEMENT_CF:
                break;
            case MLME_ELEMENT_TIM:
                break;
            case MLME_ELEMENT_IBSS:
                break;
			case MLME_ELEMENT_WPA:
                if(tmp_p->len>MLME_MAX_WPA_LEN)
                    return 0;
                fw_memcpy(bss_entry->wpa_ie,tmp_p,(tmp_p->len)+2);
				break;
            default:
                break;
        }
        len+=2;
        len+=tmp_p->len;
//        printk("len=%d\n",len);   
        if(len>=elen)
            break;
        else
            tmp_p=(info_element_t *)((UINT8 *)ie_p+len);
    }
    return 1;
}


