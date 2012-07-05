/*
 * mlme/mlme.c
 *
 * Copyright (C) 2004  Ivan Wang <ivan@faraday.com.tw>
 */ 
 
#include "mlme.h"
#include "hw_ctrl.h"

void mlme_set_beacon(mlme_data_t *mlme_p);
void mlme_set_mode(mlme_data_t *mlme_p);

UINT8  mBroadcast[MACADDR_LEN]={0xff,0xff,0xff,0xff,0xff,0xff}; 
UINT16 mlme_dbg_level= 0;


static UINT32 mlme_get_state(mlme_data_t *mlme_p,UINT32 machine)
{
    switch(machine)
    {
        case MLME_SYS_STATE_MACHINE:
            return mlme_p->mlme_sys_state;
        case MLME_SYNC_STATE_MACHINE:
            return mlme_p->mlme_sync_state;
        case MLME_AUTH_STATE_MACHINE:
            return mlme_p->mlme_auth_state;
        case MLME_ASSOC_STATE_MACHINE:
            return mlme_p->mlme_assoc_state;
    }
    return 0;
}


/* search table by essid/channel */
static void mlme_print_table(BSS_table_t *table)
{
    UINT32 i,j=0;
    BSS_entry_t *tmp;

    for(i=0;i<BSS_TABLE_SIZE;i++)
    {
        tmp=&(table->entry[i]);
        if(!(tmp->exist))
            continue;
        fw_printf(MLME_DBG_LEVEL1,"[%d]Chl:%x BSSID:%02x %02x %02x %02x %02x %02x Qual:%d WEP:%d SSID:%s\n",i,tmp->mChannel,tmp->mBssid[0],
            tmp->mBssid[1],tmp->mBssid[2],tmp->mBssid[3],tmp->mBssid[4],tmp->mBssid[5],tmp->quality,tmp->mPrivacy,tmp->mEssid);
        /* mode */
        if(tmp->mMode==MLME_ADHOC_MODE)
            fw_printf(MLME_DBG_LEVEL1,"   Mode: Ad-hoc");
        else if(tmp->mMode==MLME_INFRA_MODE)
            fw_printf(MLME_DBG_LEVEL1,"   Mode: Infra");
            
        /* support rate */
        fw_printf(MLME_DBG_LEVEL1,"   Support Rate(Mb):");
        j=0;
        while(1)
        {
            if(tmp->rate[j]==0)
                break;
            if(tmp->rate[j]==MLME_SR_1M)
                fw_printf(MLME_DBG_LEVEL1,"1 ");
            if(tmp->rate[j]==MLME_SR_2M)
                fw_printf(MLME_DBG_LEVEL1,"2 ");
            if(tmp->rate[j]==MLME_SR_5_5M)
                fw_printf(MLME_DBG_LEVEL1,"5.5 ");
            if(tmp->rate[j]==MLME_SR_11M)
                fw_printf(MLME_DBG_LEVEL1,"11 ");
            if(tmp->rate[j]==MLME_SR_16_5M)
                fw_printf(MLME_DBG_LEVEL1,"16.5 ");
            if(tmp->rate[j]==MLME_SR_22M)
                fw_printf(MLME_DBG_LEVEL1,"22 ");
            if(tmp->rate[j]==MLME_SR_27_5M)
                fw_printf(MLME_DBG_LEVEL1,"27.5 ");
            j++;
        }
        /* basic rate */
        fw_printf(MLME_DBG_LEVEL1,"  Basic Rate value=0x%x",tmp->basic_rate);
        fw_printf(MLME_DBG_LEVEL1,"\n");
//fw_printf(MLME_DBG_LEVEL1,"tmp->timestamp=0x%x\n",tmp->timestamp);
//fw_printf(MLME_DBG_LEVEL1,"   Quality:0x%x Stregth:0x%x timestamp:0x%x\n",tmp->quality,tmp->strength,tmp->timestamp);
#if 0
fw_printf(MLME_DBG_LEVEL1,"wpa_ie=");
for(i=0;i<tmp->wpa_ie[1]+2;i++)
    fw_printf(MLME_DBG_LEVEL1,"%02x ",tmp->wpa_ie[i]);
fw_printf(MLME_DBG_LEVEL1,"\n");
#endif
    }
    
    fw_printf(MLME_DBG_LEVEL1,"-----------------------------------------------------------------\n");
    fw_printf(MLME_DBG_LEVEL1,"         Debug Level=0x%x\n",mlme_dbg_level);
    fw_printf(MLME_DBG_LEVEL1,"MLME_DEBUG_RX_DUMP_PKT=%04x       MLME_DEBUG_TX_DUMP_PKT=%04x\n",MLME_DEBUG_RX_DUMP_PKT,MLME_DEBUG_TX_DUMP_PKT);
    fw_printf(MLME_DBG_LEVEL1,"MLME_DEBUG_TCB_DUMP_MSG=%04x      MLME_DEBUG_WEP_DATA=%04x\n",MLME_DEBUG_TCB_DUMP_MSG,MLME_DEBUG_WEP_DATA);
    fw_printf(MLME_DBG_LEVEL1,"MLME_DEBUG_WPA_DATA=%04x          MLME_DEBUG_HARDWARE_CTL=%04x\n",MLME_DEBUG_WPA_DATA,MLME_DEBUG_HARDWARE_CTL);
    fw_printf(MLME_DBG_LEVEL1,"MLME_DEBUG_MLME_BASIC_MSG=%04x    MLME_DEBUG_MLME_ADVANCE_MSG=%04x\n",MLME_DEBUG_MLME_BASIC_MSG,MLME_DEBUG_MLME_ADVANCE_MSG);
    fw_printf(MLME_DBG_LEVEL1,"MLME_DEBUG_MLME_BEACON_DUMP=%04x  MLME_DEBUG_MLME_RX_MSG=%04x\n",MLME_DEBUG_MLME_BEACON_DUMP,MLME_DEBUG_MLME_RX_MSG);
    fw_printf(MLME_DBG_LEVEL1,"MLME_DEBUG_MLME_TX_DUMP=%04x      MLME_DEBUG_ATTACKER=%04x\n",MLME_DEBUG_MLME_TX_DUMP,MLME_DEBUG_ATTACKER);
}


static void mlme_clean_table(mlme_data_t *mlme_p)
{
    fw_memset(&mlme_p->mTable, 0, sizeof(BSS_table_t));
    mlme_p->mTable.idx=0;    
}


static void mlme_aging_table(mlme_data_t *mlme_p)
{
    UINT32 i;
    UINT32 now=mlme_get_time(mlme_p);
    BSS_entry_t *tmp;
    BSS_table_t *table=&mlme_p->mTable;
    
    for(i=0;i<BSS_TABLE_SIZE;i++)
    {
        tmp=&(table->entry[i]);
        if(!(tmp->exist))
            continue;
        if(now > tmp->timestamp)
        {
            if(now-tmp->timestamp >BEACON_WAIT_TIME)  //wait for BEACON_WAIT_TIME ms.
            {
                tmp->exist=0;
                if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
                    fw_printf(MLME_DBG_LEVEL1,"\n******* BSSID:%x %x %x %x %x %x aged...\n",tmp->mBssid[0],
                        tmp->mBssid[1],tmp->mBssid[2],tmp->mBssid[3],tmp->mBssid[4],tmp->mBssid[5]);
            }
        }
        else
            tmp->timestamp=now;
    }
}


/* search table by essid/channel */
BSS_entry_t *mlme_search_table_by_essid(BSS_table_t *table,UINT8 *essid,UINT8 channel)
{
    UINT32      i;
    BSS_entry_t *tmp,*choose=0;
    
    for(i=0;i<BSS_TABLE_SIZE;i++)
    {
        tmp=&(table->entry[i]);
        if(!(tmp->exist))
            continue;        
        if(fw_strcmp(&essid[0],&(tmp->mEssid[0]))==0)
        {
            if( (channel==MLME_CHANNEL_NOCHECK) || (channel==tmp->mChannel))
            {
                /* choice by quality */
                if(!choose)
                    choose=tmp;
                else
                {
                    if(tmp->quality > choose->quality)
                        choose=tmp;
                }
            }
        }   
    }
    return choose;
}

BSS_entry_t *mlme_search_table_by_bssid(BSS_table_t *table,UINT8 *bssid)
{
    UINT32 i;
    BSS_entry_t *tmp;
    
    for(i=0;i<BSS_TABLE_SIZE;i++)
    {
        tmp=&(table->entry[i]);
        if(!(tmp->exist))
            continue;
        if(fw_memcmp(&bssid[0],&(tmp->mBssid[0]),sizeof(UINT8)*MACADDR_LEN)==0)
        {
            return tmp;
        }   
    }
    return 0;
}


UINT32 mlme_table_insert(BSS_table_t *table,BSS_entry_t *src)
{
    UINT32      i,idx=0x99;
    BSS_entry_t *tmp;
    UINT8       *bssid;
    UINT8       tmp_essid[MAX_ESSID_SIZE];
    
    bssid=src->mBssid;
    
    /* search if exist */
    for(i=0;i<BSS_TABLE_SIZE;i++)
    {
        tmp=&table->entry[i];
        if(!(tmp->exist) && (idx==0x99))
        {
            idx=i;
            continue;
        }
            
        if(fw_memcmp(&(bssid[0]),&(tmp->mBssid[0]),sizeof(UINT8)*MACADDR_LEN)==0)
        {
            idx=i;
            //check quality
            if(tmp->mChannel == src->mChannel)
                break;
            if(tmp->quality < src->quality) //smaller better
                break;
            else
                return 0;
        }
    }
    
    if(i==BSS_TABLE_SIZE && idx==0x99)
        return 0;
    
    if((src->mEssid[0]==0) && (table->entry[idx].mEssid[0]!=0)) //for NULL essid
    {
        fw_strcpy(tmp_essid,table->entry[idx].mEssid);
        fw_memcpy((UINT8 *)&(table->entry[idx]),(UINT8 *)src,sizeof(BSS_entry_t));
        fw_strcpy(table->entry[idx].mEssid,tmp_essid);
    }
    else    
        fw_memcpy((UINT8 *)&(table->entry[idx]),(UINT8 *)src,sizeof(BSS_entry_t));
    
    table->entry[idx].exist=1;


    //fw_printf(MLME_DBG_LEVEL1,"\ntable 0x%x insert:0x%x %s idx=0x%x exist:0x%x (0x%x)\n",table,i,src->mEssid,idx,table->entry[i].exist,(UINT8 *)&(table->entry[i]));
    
    return 1;
}

void mlme_clear_queue(mlme_data_t *mlme_p)
{
//    fw_printf(MLME_DBG_LEVEL1,"XXXXXXXX mlme_clear_queue\n");
    mlme_p->mQueue.head=0;
    mlme_p->mQueue.tail=0;
    fw_memset(mlme_p->mQueue.entry,0,sizeof(msg_entry_t)*MSG_ENTRY_SIZE);
}

UINT32 mlme_msg_enqueue(msg_queue_t *q,UINT32 machine,UINT32 msg,void *parm1,UINT32 len1,void *parm2,UINT32 len2)
{    
    if(q->head==q->tail+1)  //queue full
        return 0;//fw_error(FW_ERROR_QUEUE_FUL);

//ivan
//fw_showstr(MLME_DBG_LEVEL1,"enqueue index %d...",q->tail);
     
    q->entry[q->tail].machine=machine;
    q->entry[q->tail].message=msg;
    if(parm1)
    {
        fw_memcpy((q->entry[q->tail].parm.parm1),parm1,len1);
        q->entry[q->tail].parm.len1=len1;
    }
    if(parm2)
    {
        fw_memcpy((q->entry[q->tail].parm.parm2),parm2,len2);
        q->entry[q->tail].parm.len2=len2;
    }
    q->tail++;
    if(q->tail>=MSG_ENTRY_SIZE)
        q->tail=0;
    return 1;
}



UINT32 mlme_query_job(msg_queue_t *q)
{
    if(q->head==q->tail)  //empty
        return 0;
    else
        return 1;    
}

/* get pointer,need to protect */
msg_entry_t *mlme_msg_dequeue(msg_queue_t *q)
{
    msg_entry_t *q_now;
    
    if(q->head==q->tail)  //empty
        return 0;

//fw_showstr(MLME_DBG_LEVEL1,"dequeue index %d...",q->head);
        
    q_now = &q->entry[q->head];
    q->head++;
    if(q->head>=MSG_ENTRY_SIZE)
        q->head=0;    
    return q_now;
}


/* 
    Initialization of MLME 
    Given memory allocation
*/
void MLME_init(mlme_data_t *mlme_p)
{   
    int i=0;

//printk("MLME_init=0x%x\n",MLME_init); 
    
    spin_lock_init(&(macp->mlme_lock));
    
    /* scan type */
    //mlme_p->mScan_type=MLME_PASSIVE_SCAN;
    mlme_p->mScan_type=MLME_ACTIVE_SCAN;
    
    /* init hw layer */
    HW_init(mlme_p);
//printk("get channel info\n");    
    /* get channel info */
    HW_ioctl(mlme_p->hw_private,HW_CMD_GET_CHANNEL_INFO,mlme_p->mChannel_info,0);
//printk("get support rate\n");    
    /* get support rate */
    HW_ioctl(mlme_p->hw_private,HW_CMD_GET_RATE_INFO,mlme_p->mRate_info,0);
    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
        printk("mlme_init:support rate=%x %x %x %x %x\n",mlme_p->mRate_info[0],mlme_p->mRate_info[1],
            mlme_p->mRate_info[2],mlme_p->mRate_info[3],mlme_p->mRate_info[4]);

    /* authention method */
    mlme_p->mAuth_config=MLME_AUTH_ALG_OPEN;
//printk("basic rate\n");        
  	/* basic rate */
  	mlme_p->basic_rate_config=0;
  	while(1)
  	{  	    
        //printk("i=%d\n",i);
  	    if(mlme_p->mRate_info[i]==0)
  	        break;
  	    if(mlme_p->mRate_info[i]>mlme_p->basic_rate_config)
  	        mlme_p->basic_rate_config=mlme_p->mRate_info[i];
  	    i++;
    }

  	if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
        printk("mlme_init:basic_rate=0x%x\n",mlme_p->basic_rate_config);
//printk("clean table\n");    
    /* init BSS table */
    mlme_clean_table(mlme_p);
            
    mlme_p->mChannel_config=MLME_CHANNEL_NOCHECK;        //set config channel to NOCHECK
    mlme_p->mMode=MLME_INFRA_MODE;    //with BSS mode
    mlme_p->mLastBeaconTime=0;
    fw_memset(mlme_p->mBssid_config,0xff,MACADDR_LEN);
//printk("mlme_reset\n");    
    /* state machine reset */    
    mlme_reset(mlme_p);
       
    fw_memset(mlme_p->bss.mEssid,0,MACADDR_LEN);
    
    mlme_p->bss.mPrivacy=MLME_AUTH_ALG_OPEN;
    mlme_p->mInterval_config=BEACON_INTERVAL;
//    mlme_p->mCap_config=MLME_CAP_NULL; //with ESS bit
    //fw_printf(MLME_DBG_LEVEL1,"init ok:[0x%x]%d\n",&mlme_p->bss.mPrivacy,mlme_p->bss.mPrivacy);
//printk("init timer\n");
    /* init timer */
    fw_init_timer(&mlme_p->mlmetimer);
    fw_init_timer(&mlme_p->routinetimer);

    /* init MLME status */
    mlme_p->mInit=1;
}

void MLME_close(mlme_data_t *mlme_p)
{
    fw_del_timer(&mlme_p->mlmetimer);
    fw_del_timer(&mlme_p->routinetimer);
    mlme_p->mInit=0;
    return;
}

void mlme_state_reset(mlme_data_t *mlme_p)
{
    //reset state machine
    mlme_p->mlme_sys_state=MLME_CLASS_1;
    mlme_p->mlme_ctl_state=MLME_CTL_IDLE;
    mlme_p->mlme_sync_state=SYNC_IDLE;
    mlme_p->mlme_auth_state=AUTH_IDLE;
    mlme_p->mlme_assoc_state=ASSOC_IDLE;    
}


/* 
1.reset mlme state machine 
2.stop timer
3.start routine timer
*/
void mlme_reset(mlme_data_t *mlme_p)
{
//    fw_showstr(MLME_DBG_LEVEL1,"mlme_reset");
    mlme_p->mAssoc=0;
    mlme_p->mAuth=0;
    mlme_state_reset(mlme_p);   //state reset
    mlme_stop_timer(mlme_p);          //stop all timer
    mlme_restart_routine_timer(mlme_p);
    mlme_clear_queue(mlme_p);
}

/****************************************
    State Machine Main program
        mode=0 (ad-hoc)
             1 (BSS)
    MLME_main called situation:
    1.ISR routine
    2.MLME routine
    3.MLME timer routine
****************************************/
void MLME_main(mlme_data_t *mlme_p)
{
    int         flags;
    msg_entry_t *entry;

    spin_lock_irqsave(&mlme_p->mlme_lock,flags);
    
    entry=mlme_msg_dequeue(&mlme_p->mQueue);
    if(!entry)
        return; 
   
    while(1)
    {
//        fw_printf(MLME_DBG_LEVEL1,"<%d %d>",entry->machine,entry->message);
    
        switch(entry->machine)
        {
            case MLME_NULL_STATE_MACHINE:
                //something wrong if get here            
                break;
            case MLME_SYS_STATE_MACHINE:
                mlme_handle_sys_state(mlme_p,entry);
                break;
            case MLME_SYNC_STATE_MACHINE:
                mlme_handle_sync_state(mlme_p,entry);
                break;
            case MLME_AUTH_STATE_MACHINE:
                mlme_handle_auth_state(mlme_p,entry);
                break;
            case MLME_ASSOC_STATE_MACHINE:
                mlme_handle_assoc_state(mlme_p,entry);
                break;
            default:
                fw_showstr(MLME_DBG_LEVEL1,"Error State Machine %d...",entry->machine);
                while(1)
                    ;
        }        
        entry=mlme_msg_dequeue(&mlme_p->mQueue);
        if(!entry)
            break;        
    }
    spin_unlock_irqrestore(&mlme_p->mlme_lock,flags);
}


/****************************************
    SYS state Machine
****************************************/
void mlme_handle_sys_state(mlme_data_t *mlme_p,msg_entry_t *entry)
{
//    fw_showstr(MLME_DBG_LEVEL1,"Handle SYS State Machine...");
    
    switch(entry->message)
    {
        case MLME_STOP:
            mlme_p->mlme_sys_state=MLME_CLASS_1;
            break;
        case MLME_JOIN_REQ:
        case MLME_START_REQ:
            if(mlme_p->mMode==MLME_INFRA_MODE)
            {
                mlme_up(mlme_p);
                mlme_p->mlme_sys_state=MLME_CLASS_1;
//                fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_AUTH_REQ to AUTH state machine...");
                mlme_msg_enqueue(&mlme_p->mQueue,MLME_AUTH_STATE_MACHINE,MLME_AUTH_REQ,0,0,0,0);
            }
            if(mlme_p->mMode==MLME_ADHOC_MODE)
            {
                if(mlme_p->mlme_sys_state==MLME_CLASS_1 || mlme_p->mlme_sys_state==MLME_CLASS_2)
                {
                    mlme_up(mlme_p);
                    mlme_set_beacon(mlme_p);
                    mlme_set_mode(mlme_p);
                    mlme_p->mlme_sys_state=MLME_CLASS_3;
                    mlme_p->mAssoc=1;
                }
            }
            break;
        case MLME_DEAUTH_REQ:
        case MLME_DEAUTH_RCV:
            if(mlme_p->mMode==MLME_INFRA_MODE)
            {                
                mlme_p->mAuth=0;
                mlme_p->mAssoc=0;
                mlme_clear_queue(mlme_p);
                mlme_state_reset(mlme_p);
            }
            break;
        case MLME_DISASSOC_REQ:
            if(mlme_p->mMode==MLME_INFRA_MODE)
            {
                if(mlme_p->mlme_sys_state==MLME_CLASS_3)
                    mlme_p->mlme_sys_state=MLME_CLASS_2;
            }
            else
            {
                mlme_p->mlme_sys_state=MLME_CLASS_1;
                mlme_down(mlme_p);
            }
            mlme_p->mAssoc=0;
            break;
        case MLME_DISASSOC_RCV:
            if(mlme_p->mMode==MLME_INFRA_MODE)
            {
                //mlme_p->mlme_sys_state=MLME_CLASS_2;
                mlme_p->mlme_sys_state=MLME_CLASS_1;    //back to class 1
                //mlme_startdown(mlme_p);
            }
            mlme_p->mAssoc=0;
            break;
        case MLME_REASSOC_RSP_RCV:
        case MLME_ASSOC_RSP_RCV:
            if(mlme_p->mMode==MLME_INFRA_MODE)
            {
                if(mlme_p->mlme_sys_state==MLME_CLASS_2)
                {
                    mlme_p->mlme_sys_state=MLME_CLASS_3;
                    mlme_up(mlme_p);
                    mlme_set_mode(mlme_p);
                    mlme_p->mAssoc=1;
                    
                    printk("Association Successful!\n");
                    
                    if(mlme_dbg_level&MLME_DEBUG_MLME_ADVANCE_MSG)
                        printk("Association Successful!\n");
                }                
            }
            break;
        case MLME_AUTH_EVEN_RCV:
            if(mlme_p->mMode==MLME_INFRA_MODE)
            {
                if(mlme_p->mlme_sys_state==MLME_CLASS_1)
                {
                    mlme_p->mlme_sys_state=MLME_CLASS_2;
                    
                    //start to do associaton
//                    fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_ASSOC_REQ to ASSOC state machine...");
                    mlme_msg_enqueue(&mlme_p->mQueue,MLME_ASSOC_STATE_MACHINE,MLME_ASSOC_REQ,0,0,0,0);
                }
            }
            break;  
    }    
}


UINT32 mlme_set_ssid_action(mlme_data_t *mlme_p)
{
    UINT32 reason;
    BSS_entry_t *bss_entry;
    
    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
        fw_showstr(MLME_DBG_LEVEL1,"mlme_set_ssid_action");
                 
    mlme_state_reset(mlme_p);   //state reset
    mlme_stop_timer(mlme_p);          //stop all timer
    mlme_restart_routine_timer(mlme_p); //restart routine timer
        
    if((mlme_p->mAssoc) && (mlme_p->mMode==MLME_INFRA_MODE))
    {
        /* do disassoc and return */
//        fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_DISASSOC_REQ to ASSOC state machine...");
        reason=MLME_REASON_DISASSOC_STA_LEAVING;
        mlme_msg_enqueue(&mlme_p->mQueue,MLME_ASSOC_STATE_MACHINE,MLME_DISASSOC_REQ,&reason,2,0,0);
        return 1;
    }

    if(mlme_p->mScan_type!=MLME_ACTIVE_SCAN)
    {
        //search table for specified ssid/ssid
        bss_entry=mlme_search_table_by_essid(&mlme_p->mTable,mlme_p->bss.mEssid,mlme_p->mChannel_config);
        if(bss_entry)
        {
            /* do join */
            fw_memcpy(&mlme_p->bss,bss_entry,sizeof(BSS_entry_t));
//            fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_JOIN_REQ to SYNC state machine...");
            mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_JOIN_REQ,0,0,0,0);
            return 1;
        }
    }

    /* do scan */
//    fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_SCAN_REQ to SYNC state machine...");
    mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_SCAN_REQ,0,0,0,0);
    return 1;
}

UINT32 mlme_set_bssid_action(mlme_data_t *mlme_p,UINT8 *parm)
{
    UINT32 reason;
//    msg_queue_t q;
    BSS_entry_t *bss_entry;
    
    fw_showstr(MLME_DBG_LEVEL1,"mlme_set_bssid_action...");
            
    if(parm==0)
        return 0;
    
    if(fw_memcmp(mlme_p->bss.mBssid,parm,sizeof(UINT8)*MACADDR_LEN)!=0) //not equal
        fw_memcpy(mlme_p->bss.mBssid,parm,sizeof(UINT8)*MACADDR_LEN);

    if(mlme_p->mMode==MLME_ADHOC_MODE)
    {
        printk("Should use specified ssid and create a random BSSID\n");
    }
    else    //BSS mode
    {    
        if(mlme_p->mAssoc)
        {
            /* do disassoc */
//            fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_DISASSOC_REQ to ASSOC state machine...");
            reason=MLME_REASON_DISASSOC_STA_LEAVING;
            mlme_msg_enqueue(&mlme_p->mQueue,MLME_ASSOC_STATE_MACHINE,MLME_DISASSOC_REQ,&reason,2,0,0);
            return 1;
        }
        else
        {
            //search table for specified ssid/ssid
            bss_entry=mlme_search_table_by_bssid(&mlme_p->mTable,parm);
            if(bss_entry && (bss_entry->mEssid[0]!=0)) //if null essid, it should do scan
            {
                if(mlme_p->mScan_type!=MLME_ACTIVE_SCAN)
                {
                    /* do join */
                    fw_memcpy(&mlme_p->bss,bss_entry,sizeof(BSS_entry_t));
//                    fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_JOIN_REQ to SYNC state machine...");
                    mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_JOIN_REQ,0,0,0,0);
                }
                else
                {
                    /* do scan */
//                    fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_SCAN_REQ to SYNC state machine...");
                    mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_SCAN_REQ,0,0,0,0);
                }
            }
            else
            {
                /* do scan */
//                fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_SCAN_REQ to SYNC state machine...");
                mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_SCAN_REQ,0,0,0,0);
            }
        }
    }
    return 1;
}


void mlme_scan_action(mlme_data_t *mlme_p)
{
    fw_showstr(MLME_DBG_LEVEL1,"mlme_scan_action...");
}

void mlme_disassoc_action(mlme_data_t *mlme_p)
{
    fw_showstr(MLME_DBG_LEVEL1,"mlme_disassoc_action...");
}

//no implmenet at non-AP mode
UINT32 mlme_chk_cls2_error(mlme_data_t *mlme_p)
{
    if(!mlme_p->mAuth)
    {
        //class 2 error
//        fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_CLS2_ERROR to AUTH state machine...");
        mlme_msg_enqueue(&mlme_p->mQueue,MLME_AUTH_STATE_MACHINE,MLME_CLS2_ERROR,0,0,0,0);                
        return 1;
    }
    return 0;
}

//no implmenet at non-AP mode
UINT32 mlme_chk_cls3_error(mlme_data_t *mlme_p)
{
    if(!mlme_p->mAssoc)
    {
        //class 3 error
//        fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_CLS3_ERROR to ASSOC state machine...");
        mlme_msg_enqueue(&mlme_p->mQueue,MLME_ASSOC_STATE_MACHINE,MLME_CLS3_ERROR,0,0,0,0);
        return 1;
    }
    return 0;
}


void mlme_dump_data(UINT8 *data, UINT32 data_len)
{
    int i;
	
	for (i=0; i<data_len; i++)
	{
		fw_printf(MLME_DBG_LEVEL1,"%02x", data[i]);
		fw_printf(MLME_DBG_LEVEL1," ");
		if ((i>0) && ((i+1)%16 == 0))

			fw_printf(MLME_DBG_LEVEL1,"\n");
	}
	fw_printf(MLME_DBG_LEVEL1,"\n");
}

void MLME_rx(mlme_data_t *mlme_p,UINT8 *pkt,UINT32 pkt_len,UINT8 *parm)
{
    I802_11_hdr_t *mlme_hdr=(I802_11_hdr_t *)pkt;
    I802_11_pkt_t *mlme_pkt=(I802_11_pkt_t *)pkt;

//    fw_printf(MLME_DBG_LEVEL1,"\n-----------------------------------\n");
//    mlme_dump_data(pkt,50);
    if(mlme_dbg_level&MLME_DEBUG_ATTACKER)
        return;
    if(!mlme_checkinit(mlme_p))
        return;

    //check management type
    switch(mlme_hdr->subtype)
    {
        case SUBTYPE_ASSOC_REQ:     //AP mode only
            if(mlme_dbg_level&MLME_DEBUG_MLME_RX_MSG)
                fw_showstr(MLME_DBG_LEVEL1,"MLME_rx(),AP function only...");        
            break;
        case SUBTYPE_ASSOC_RSP:
            if(mlme_p->mMode==MLME_ADHOC_MODE)
            {
                if(mlme_dbg_level&MLME_DEBUG_MLME_RX_MSG)
                    fw_showstr(MLME_DBG_LEVEL1,"MLME_rx(),Adhoc not support MLME_ASSOC_RSP_RCV...");
            }
            else
            {
                if(mlme_dbg_level&MLME_DEBUG_MLME_RX_MSG)
                    fw_showstr(MLME_DBG_LEVEL1,"MLME_rx(),Send Message:MLME_ASSOC_RSP_RCV to ASSOC state machine...");
                mlme_msg_enqueue(&mlme_p->mQueue,MLME_ASSOC_STATE_MACHINE,MLME_ASSOC_RSP_RCV,(UINT8 *)pkt,pkt_len,parm,2);
            }
            break;
        case SUBTYPE_REASSOC_REQ:   //AP mode only
            if(mlme_dbg_level&MLME_DEBUG_MLME_RX_MSG)
                fw_showstr(MLME_DBG_LEVEL1,"MLME_rx(),AP function only...");        
            break;
        case SUBTYPE_REASSOC_RSP:               
            if(mlme_p->mMode==MLME_ADHOC_MODE)
            {
                if(mlme_dbg_level&MLME_DEBUG_MLME_RX_MSG)
                    fw_showstr(MLME_DBG_LEVEL1,"MLME_rx(),Adhoc not support MLME_REASSOC_RSP_RCV...");
            }
            else     
            {
                if(mlme_dbg_level&MLME_DEBUG_MLME_RX_MSG)
                    fw_showstr(MLME_DBG_LEVEL1,"MLME_rx(),Send Message:MLME_REASSOC_RSP_RCV to ASSOC state machine...");
                mlme_msg_enqueue(&mlme_p->mQueue,MLME_ASSOC_STATE_MACHINE,MLME_REASSOC_RSP_RCV,(UINT8 *)pkt,pkt_len,parm,2);
            }
            break;
        case SUBTYPE_PROBE_REQ:
            if(mlme_p->mMode==MLME_INFRA_MODE)
            {
                if(mlme_dbg_level&MLME_DEBUG_MLME_RX_MSG)
                    fw_showstr(MLME_DBG_LEVEL1,"MLME_rx(),BSS STA not support MLME_PROBE_REQ_RCV...");
            }
            else if(mlme_p->mMode==MLME_ADHOC_MODE)
            {
                if(mlme_dbg_level&MLME_DEBUG_MLME_RX_MSG)
                    fw_showstr(MLME_DBG_LEVEL1,"MLME_rx(),Send Message:MLME_PROBE_REQ_RCV to SYNC state machine...");
                mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_PROBE_REQ_RCV,mlme_hdr->addr2,6,0,0);
            }
            break;
        case SUBTYPE_PROBE_RSP:
            /* filter if ad-hoc/infrastrucautre packet */
//            printk("mlme_pkt->fb[10]=0x%x 0x%x mlme_p->mMode=%d\n",mlme_pkt->fb[10],mlme_pkt->fb[11],mlme_p->mMode);
            if(((mlme_pkt->fb[10]&0x2)==0x2) && (mlme_p->mMode==MLME_INFRA_MODE))  //Capability: Ad-hoc bit
                return;            
            if(((mlme_pkt->fb[10]&0x2)!=0x2) && (mlme_p->mMode==MLME_ADHOC_MODE))  //Capability: Ad-hoc bit
                return;
            if(mlme_dbg_level&MLME_DEBUG_MLME_RX_MSG)
                fw_showstr(MLME_DBG_LEVEL1,"MLME_rx(),Send Message:MLME_PROBE_RSP_RCV to SYNC state machine...");
            mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_PROBE_RSP_RCV,(UINT8 *)pkt,pkt_len,parm,2);
            break;
        case SUBTYPE_BEACON:
            /* filter if ad-hoc/infrastrucautre packet */
            //printk("mlme_pkt->fb[10]=0x%x 0x%x mlme_p->mMode=%d\n",mlme_pkt->fb[10],mlme_pkt->fb[11],mlme_p->mMode);
            if(((mlme_pkt->fb[10]&0x2)==0x2) && (mlme_p->mMode==MLME_INFRA_MODE))  //Capability: Ad-hoc bit
                return;            
            if(((mlme_pkt->fb[10]&0x2)!=0x2) && (mlme_p->mMode==MLME_ADHOC_MODE))  //Capability: Ad-hoc bit
                return;
            mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_BEACON_RCV,(UINT8 *)pkt,pkt_len,parm,2);
            break;
        case SUBTYPE_ATIM:  // not support yet
            if(mlme_dbg_level&MLME_DEBUG_MLME_RX_MSG)
                fw_showstr(MLME_DBG_LEVEL1,"MLME_rx(),Not Implement!");
            break;
        case SUBTYPE_DISASSOC:
            if(mlme_p->mMode==MLME_ADHOC_MODE)
            {
                if(mlme_dbg_level&MLME_DEBUG_MLME_RX_MSG)
                    fw_showstr(MLME_DBG_LEVEL1,"MLME_rx(),Adhoc not support MLME_DISASSOC_RCV...");
            }
            else     
            {       
                if(mlme_dbg_level&MLME_DEBUG_MLME_RX_MSG)
                    fw_showstr(MLME_DBG_LEVEL1,"MLME_rx(),Send Message:MLME_DISASSOC_RCV to ASSOC state machine...");
                mlme_msg_enqueue(&mlme_p->mQueue,MLME_ASSOC_STATE_MACHINE,MLME_DISASSOC_RCV,0,0,0,0);
            }
            break;                            
        case SUBTYPE_AUTH:
            if(mlme_p->mMode==MLME_ADHOC_MODE)
            {
                if(mlme_dbg_level&MLME_DEBUG_MLME_RX_MSG)
                    fw_showstr(MLME_DBG_LEVEL1,"MLME_rx(),Adhoc not support MLME_AUTH_RCV...");
            }
            else     
            {               
                UINT16 seq_num;
                fw_memcpy((UINT32 *)&seq_num,(UINT32 *)&(mlme_pkt->fb[2]),2);
                if(seq_num==1||seq_num==3)
                {
                    if(mlme_dbg_level&MLME_DEBUG_MLME_RX_MSG)
                        fw_printf(MLME_DBG_LEVEL1,"NOT support in STA(AUTH seq=%d)\n",seq_num);
                }
                else
                {
                    if(mlme_dbg_level&MLME_DEBUG_MLME_RX_MSG)
                        fw_showstr(MLME_DBG_LEVEL1,"MLME_rx(),Send Message:MLME_AUTH_EVEN_RCV to AUTH state machine(pkt_len=%d)...",pkt_len);
                    mlme_msg_enqueue(&mlme_p->mQueue,MLME_AUTH_STATE_MACHINE,MLME_AUTH_EVEN_RCV,(UINT8 *)pkt,pkt_len,0,0);
                }
            }
            break;        
        case SUBTYPE_DEAUTH:
            if(mlme_p->mMode==MLME_ADHOC_MODE)
            {
                if(mlme_dbg_level&MLME_DEBUG_MLME_RX_MSG)
                    fw_showstr(MLME_DBG_LEVEL1,"MLME_rx(),Adhoc not support MLME_DEAUTH_RCV...");
            }
            else     
            {
                if(mlme_dbg_level&MLME_DEBUG_MLME_RX_MSG)
                    fw_showstr(MLME_DBG_LEVEL1,"MLME_rx(),Send Message:MLME_DEAUTH_RCV to AUTH state machine...");
                mlme_msg_enqueue(&mlme_p->mQueue,MLME_AUTH_STATE_MACHINE,MLME_DEAUTH_RCV,0,0,0,0);
            }
            break;
        default:
            if(mlme_dbg_level&MLME_DEBUG_MLME_RX_MSG)
                fw_showstr(MLME_DBG_LEVEL1,"MLME_rx(),Undefined subtype");
            break;
    }    
}


void mlme_stop_timer(mlme_data_t *mlme_p)
{
#ifdef MLME_ADS    
    fw_del_timer(MLME_TIMER_V1);
    fw_del_timer(MLME_TIMER_V2);
#else
    fw_del_timer(&mlme_p->mlmetimer);
    fw_del_timer(&mlme_p->routinetimer);
#endif
}


UINT32 MLME_ioctl(mlme_data_t *mlme_p,UINT32 cmd,void *data)
{
    switch(cmd)
    {
        case MLME_IOCTL_QUERY_JOB:  //query if queued mlme jobs
            if(mlme_query_job(&mlme_p->mQueue))
                *(UINT32 *)data=1;
            else
                *(UINT32 *)data=0;
            break;
        case MLME_IOCTL_SET_SSID:   //use string with 0x0 end
            if(fw_strcmp(mlme_p->bss.mEssid,(UINT8 *)data)!=0) //not equal
                fw_strcpy(mlme_p->bss.mEssid,(UINT8 *)data);
            mlme_reset(mlme_p); //should do mlme_reset because re-typing iwconfig ...
            mlme_set_ssid_action(mlme_p);
            break;
        case MLME_IOCTL_GET_SSID:
            fw_strcpy(data,mlme_p->bss.mEssid);
            break;
        case MLME_IOCTL_SET_BSSID:  //use string with 0x0 end
            mlme_reset(mlme_p); //should do mlme_reset because re-typing iwconfig ...
            mlme_set_bssid_action(mlme_p,data);
            break;
        case MLME_IOCTL_GET_BSSID:
            fw_memcpy(data,mlme_p->bss.mBssid,MACADDR_LEN);
            break;            
        case MLME_IOCTL_SET_MAC:
            fw_memcpy((UINT8 *)mlme_p->mMAC,(UINT8 *)data,MACADDR_LEN);
            break;
        case MLME_IOCTL_GET_MAC:
            fw_memcpy((UINT8 *)data,(UINT8 *)mlme_p->mMAC,MACADDR_LEN);
            break;
        case MLME_IOCTL_SET_MODE:        //UINT32 mode
            mlme_clean_table(mlme_p);
            mlme_p->mMode=*(UINT32 *)data;
//            fw_printf(MLME_DBG_LEVEL1,"MLME_SET_MODE:%d\n",mlme_p->mMode);
            mlme_down(mlme_p);
            mlme_reset(mlme_p); //should do mlme_reset because re-typing iwconfig ...            
            mlme_set_ssid_action(mlme_p);
            break;
        case MLME_IOCTL_GET_MODE:        //UINT32 mode
            *(UINT32 *)data=mlme_p->mMode;
//            fw_printf(MLME_DBG_LEVEL1,"MLME_GET_MODE:%d\n",*(UINT32 *)data);
            break;            
        case MLME_IOCTL_RESTART:
            mlme_reset(mlme_p);           
            break;
        case MLME_IOCTL_SET_CHANNEL:      //UINT32 channel
            mlme_p->mChannel_config=*(UINT32 *)data;
            mlme_p->bss.mChannel=mlme_p->mChannel_config;
            if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
                fw_printf(MLME_DBG_LEVEL1,"MLME_IOCTL_SET_CHANNEL:%d\n",mlme_p->bss.mChannel);
            mlme_reset(mlme_p); //should do mlme_reset because re-typing iwconfig ...
            mlme_set_ssid_action(mlme_p);
            break;
        case MLME_IOCTL_GET_CHANNEL:      //UINT32 channel
            *(UINT32 *)data=mlme_p->bss.mChannel;
            //fw_printf(MLME_DBG_LEVEL1,"MLME_GET_CHANNEL:%d\n",*(UINT32 *)data);            
            break;
        case MLME_IOCTL_QUERY_ASSOC:      //assoc status
            if(mlme_p->mAssoc)
                *(UINT32 *)data=1;
            else
                *(UINT32 *)data=0;
            break;
        case MLME_IOCTL_DEBUG_PRINT:
            sim_printmlme(mlme_p,*(UINT32 *)data);
            break;
        case MLME_IOCTL_SET_AUTH_ALG:
            mlme_p->mAuth_config=*(UINT32 *)data;
//            printk("MLME_IOCTL_SET_AUTH_ALG=%d\n",mlme_p->mAuth_config);
            break;
        case MLME_IOCTL_GET_AUTH_ALG:
            *(UINT32 *)data=mlme_p->mAuth_config;
//            printk("MLME_IOCTL_GET_AUTH_ALG=%d\n",mlme_p->mAuth_config);
            break;
        case MLME_IOCTL_GET_SCAN_INFO:
            *(UINT32 *)data=(UINT32)&mlme_p->mTable;
            break;
        case MLME_IOCTL_REQ_DISASSOC:
            if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)
                fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_DISASSOC_REQ to ASSOC state machine...");
            mlme_msg_enqueue(&mlme_p->mQueue,MLME_ASSOC_STATE_MACHINE,MLME_DISASSOC_REQ,data,2,0,0);
            break;            
        default:
            break;
    }    
    return 1;
}


UINT32 mlme_checkinit(mlme_data_t *mlme_p)
{
    if(mlme_p->mInit==0)
        return 0;
    return 1;
}


void mlme_infra_routine(mlme_data_t *mlme_p)
{
    BSS_entry_t     *bss_entry;
    UINT32  reason;
    
    if(mlme_p->mAssoc)
    {
        UINT32 now=mlme_get_time(mlme_p);
        if(now < mlme_p->mLastBeaconTime)
            return;
            
        if((now-mlme_p->mLastBeaconTime)>BEACON_WAIT_TIME)
        {
            fw_printf(MLME_DBG_LEVEL1,"*******Not accept belonged beacon: 0x%x from now 0x%x\n",mlme_p->mLastBeaconTime,now);
            mlme_reset(mlme_p);
            mlme_p->mLastBeaconTime=now;
            /* it will go through to if(!mlme_p->mAssoc) */
        }
        else
        {           
            mlme_p->mLastBeaconTime=0;
            
            //check romming quality
            bss_entry=mlme_search_table_by_essid(&mlme_p->mTable,mlme_p->bss.mEssid,mlme_p->mChannel_config);
            if(!bss_entry)
                return;
            if(!fw_memcmp(bss_entry->mBssid,mlme_p->bss.mBssid,MACADDR_LEN))
                return;

            printk("<not equal:%d %d diff:%d>",bss_entry->quality,mlme_p->bss.quality,bss_entry->quality-mlme_p->bss.quality);
            if((bss_entry->quality) - (mlme_p->bss.quality) > MLME_QUALITY_OFFSET)
            {                
                /* do disassoc */
//                fw_showstr(MLME_DBG_LEVEL1,"Send Message:MLME_DISASSOC_REQ to ASSOC state machine...");
                reason=MLME_REASON_DISASSOC_STA_LEAVING;
                mlme_msg_enqueue(&mlme_p->mQueue,MLME_ASSOC_STATE_MACHINE,MLME_DISASSOC_REQ,&reason,2,0,0);
//                printk("MLMA_main() from mlme_infra_routine\n");
                MLME_main(mlme_p);
                //join better AP
                fw_memcpy(&mlme_p->bss,bss_entry,sizeof(BSS_entry_t));
//                fw_showstr(MLME_DBG_LEVEL1,"(mlme_routine)Send Message:MLME_JOIN_REQ to SYNC state machine...");
                mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_JOIN_REQ,0,0,0,0);
            }
        }
    }
    
    if(!mlme_p->mAssoc) //not mAssoc
    {
//        printk("\n<mlme_routine_scan>\n");
        mlme_reset(mlme_p);

        if(mlme_p->mChannel_config==MLME_CHANNEL_NOCHECK)
        {
            /* do scan */
//            fw_showstr(MLME_DBG_LEVEL1,"(mlme_routine)Send Message:MLME_SCAN_REQ to SYNC state machine...");
            mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_SCAN_REQ,0,0,0,0);
//            printk("MLMA_main() from mlme_infra_routine\n");
            MLME_main(mlme_p);
        }
        else
        {
            //search table for specified ssid/ssid
            bss_entry=mlme_search_table_by_essid(&mlme_p->mTable,mlme_p->bss.mEssid,mlme_p->mChannel_config);
            if(bss_entry)
            {
                /* do join */                    
                fw_memcpy(&mlme_p->bss,bss_entry,sizeof(BSS_entry_t));
//                fw_showstr(MLME_DBG_LEVEL1,"(mlme_routine)Send Message:MLME_JOIN_REQ to SYNC state machine...");
                mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_JOIN_REQ,0,0,0,0);
            }
            else
            {
                /* do scan */
//                fw_showstr(MLME_DBG_LEVEL1,"(mlme_routine)Send Message:MLME_SCAN_REQ to SYNC state machine...");
                mlme_msg_enqueue(&mlme_p->mQueue,MLME_SYNC_STATE_MACHINE,MLME_SCAN_REQ,0,0,0,0);
            }
        }
    }    
}

void mlme_adhoc_routine(mlme_data_t *mlme_p)
{
    return;
}


/*
    MLME routine to check channel quality and some status
*/
void mlme_routine(mlme_data_t *mlme_p)
{
    if(mlme_dbg_level&MLME_DEBUG_ATTACKER)
        return;
    if(!mlme_checkinit(mlme_p))
        return;
    
    MLME_main(mlme_p); //clear state machine job
    
    /* check age of BSS entry */
    mlme_aging_table(mlme_p);

    if(mlme_p->mMode==MLME_INFRA_MODE)
        mlme_infra_routine(mlme_p);
    else if(mlme_p->mMode==MLME_ADHOC_MODE)
        mlme_adhoc_routine(mlme_p);
    mlme_restart_routine_timer(mlme_p);

    MLME_main(mlme_p); //clear state machine job
}

/* return beacon length */
UINT32 mlme_init_beacon(mlme_data_t *mlme_p,UINT8 *pkt)
{
    int     i;
    UINT32  len2;    
    UINT32  len;
    
    len=mlme_make_header(pkt,SUBTYPE_BEACON,0,0,mBroadcast,mlme_p->mMAC,mlme_p->bss.mBssid);
	
	/* Timestamp	HMAC will fill this field */
	for (i=0; i<8; i++)
		pkt[len++] = 0x0;
		
	/* BeaconInterval */
	pkt[len++] = mlme_p->bss.mInterval;
	pkt[len++] = mlme_p->bss.mInterval >> 8;
	
	/* Capability */
	pkt[len++] = MLME_CAP_IBSS;
	pkt[len++] = 0;
	
	/* SSID */
    pkt[len++]=MLME_ELEMENT_SSID;
    pkt[len++]=len2=(unsigned char)strlen(mlme_p->bss.mEssid);
  	fw_memcpy(&pkt[len],mlme_p->bss.mEssid,len2);
  	len=len+len2;
	
	/* Supported rates */
    pkt[len++]=MLME_ELEMENT_RATE;
    pkt[len++]=len2=(unsigned char)strlen(mlme_p->mRate_info);
    fw_memcpy(&pkt[len],mlme_p->mRate_info,len2);
    len=len+len2;
	
	/* DS parameter */
	pkt[len++] = MLME_ELEMENT_DS;
	pkt[len++] = 1;	
	pkt[len++] = mlme_p->bss.mChannel;

//printk("Beacon %d>>>>>>>>>>>>>>>>>>>>>>>>\n",len);
//mlme_dump_data(pkt,len);
	
    return len;
}


/* this function should call after HW_CMD_SET_BSSID in ad-hoc mode */
void mlme_set_beacon(mlme_data_t *mlme_p)
{
    UINT8   buf[128];
    UINT32  len;

    /* init beacon */
    len=mlme_init_beacon(mlme_p,buf);
    /* set beacon to hardware*/
    HW_ioctl(mlme_p->hw_private,HW_CMD_MAKE_BEACON,buf,&len);
}

void mlme_set_mode(mlme_data_t *mlme_p)
{
    UINT16 parm[3];
    
    parm[0]=mlme_p->mMode;
    parm[1]=mlme_p->mInterval_config;
    parm[2]=1;
//    fw_showstr(MLME_DBG_LEVEL1,"[HW] HW_CMD_SET_MODE by %d",mlme_p->mMode);
    HW_ioctl(mlme_p->hw_private,HW_CMD_SET_MODE,parm,0);
}


/* 
called by join/start action
1.Set BSSID/channel to hardware
*/
void mlme_up(mlme_data_t *mlme_p)
{   
//    UINT8   buf[128];
//    UINT16  len;

//    fw_showstr(MLME_DBG_LEVEL1,"mlme_startup");
    
    /* set max basic rate */
//    fw_showstr(MLME_DBG_LEVEL1,"[HW] HW_CMD_SET_BASIC_RATE by %d",mlme_p->bss.basic_rate);
    HW_ioctl(mlme_p->hw_private,HW_CMD_SET_BASIC_RATE,&mlme_p->bss.basic_rate,0);
    
    /* set channel */
//    fw_showstr(MLME_DBG_LEVEL1,"[HW] HW_CMD_SET_CHANNLE by %d",mlme_p->bss.mChannel);
    HW_ioctl(mlme_p->hw_private,HW_CMD_SET_CHANNLE,&mlme_p->bss.mChannel,0);

    /* set BSSID */
//    fw_showstr(MLME_DBG_LEVEL1,"[HW] HW_CMD_SET_BSSID by %x %x %x %x %x %x",mlme_p->bss.mBssid[0],
//        mlme_p->bss.mBssid[1],mlme_p->bss.mBssid[2],mlme_p->bss.mBssid[3],
//        mlme_p->bss.mBssid[4],mlme_p->bss.mBssid[5]);
    HW_ioctl(mlme_p->hw_private,HW_CMD_SET_BSSID,mlme_p->bss.mBssid,0);
   
    mlme_p->mLastBeaconTime=mlme_get_time(mlme_p);  //update beacon time

    /* restart routine timer */
    mlme_restart_routine_timer(mlme_p);
}


void mlme_down(mlme_data_t *mlme_p)
{
//    UINT8   buf[30];

    if(mlme_dbg_level&MLME_DEBUG_MLME_BASIC_MSG)       
        fw_showstr(MLME_DBG_LEVEL1,"mlme_down");
       
    /* stop beacon */
    if(mlme_p->mMode==MLME_ADHOC_MODE)
    {
//        fw_showstr(MLME_DBG_LEVEL1,"[HW] HW_CMD_STOP_BEACON");
        HW_ioctl(mlme_p->hw_private,HW_CMD_STOP_BEACON,0,0);
    }
}


void mlme_restart_routine_timer(mlme_data_t *mlme_p)
{
    if(mlme_p->mInit!=1)
        return;
//    fw_printf(MLME_DBG_LEVEL1,"mlme_restart_routine_timer");
        
#ifdef MLME_ADS
    fw_setup_timer(MLME_TIMER_V2,MLME_ROUTINE_TIME,mlme_p,mlme_routine);
    fw_add_timer(MLME_TIMER_V2);
#else
    fw_setup_timer(&mlme_p->routinetimer,MLME_ROUTINE_TIME,mlme_p,mlme_routine);
    fw_add_timer(&mlme_p->routinetimer);
#endif
}


UINT32 mlme_get_time(mlme_data_t *mlme_p)
{
    UINT32  tsf[2];
    
    HW_ioctl(mlme_p->hw_private,HW_CMD_GET_TIME,tsf,0);
    return tsf[0];
}


UINT32 mlme_make_header(UINT8 *pkt,UINT8 subtype,UINT8 toDS,UINT8 wep,UINT8 *addr1,UINT8 *addr2,UINT8 *addr3)
{
    I802_11_hdr_t *mlme_hdr=(I802_11_hdr_t *)pkt;
    
//fw_showstr(MLME_DBG_LEVEL1,"mgt hdr size => %d",sizeof(I802_11_hdr_t));

    mlme_hdr->ver=0;            //version 0
    mlme_hdr->type=TYPE_MGMT;   //type management
    mlme_hdr->subtype=subtype;
    mlme_hdr->toDS=toDS;
    mlme_hdr->fromDS=0;
    mlme_hdr->moreFrag=0;
    mlme_hdr->retry=0;
    mlme_hdr->pwrMgmt=0;
    mlme_hdr->moreData=0;
    mlme_hdr->wep=wep;
    mlme_hdr->order=0;
    mlme_hdr->duration=0;   //given by hardware,init with 0
    memcpy(mlme_hdr->addr1,addr1,MACADDR_LEN);
    memcpy(mlme_hdr->addr2,addr2,MACADDR_LEN);
    memcpy(mlme_hdr->addr3,addr3,MACADDR_LEN);
    mlme_hdr->frag=0;       //given by hardware,init with 0
    mlme_hdr->seq=0;
    return sizeof(I802_11_hdr_t);
}



void sim_printmlme(mlme_data_t *mlme_p,int value)
{
    //mlme_dbg=value;
//    fw_printf(MLME_DBG_LEVEL1,"debug level=%d\n",mlme_dbg);
    
    //vt100_clear_line(2);
    fw_printf(MLME_DBG_LEVEL1,"\n");
    if(mlme_p->mAssoc)
        fw_printf(MLME_DBG_LEVEL1,"O");
    else
        fw_printf(MLME_DBG_LEVEL1,"X");
    if(mlme_p->mMode==MLME_INFRA_MODE)
        fw_printf(MLME_DBG_LEVEL1,"[BSS]");
    else if(mlme_p->mMode==MLME_ADHOC_MODE)
        fw_printf(MLME_DBG_LEVEL1,"[Adhoc]");
    else
        fw_printf(MLME_DBG_LEVEL1,"[NULL]");
    fw_printf(MLME_DBG_LEVEL1,"[%d]  ",mlme_p->bss.mChannel);

    if(!fw_memcmp(mlme_p->mBssid_config,mBroadcast,MACADDR_LEN))
        fw_printf(MLME_DBG_LEVEL1,"BSSID(Learning):");
    else
        fw_printf(MLME_DBG_LEVEL1,"BSSID(Config):");
        
    fw_printf(MLME_DBG_LEVEL1,"%02x %02x %02x %02x %02x %02x",mlme_p->bss.mBssid[0],
        mlme_p->bss.mBssid[1],mlme_p->bss.mBssid[2],mlme_p->bss.mBssid[3],
        mlme_p->bss.mBssid[4],mlme_p->bss.mBssid[5]);
        
        
    fw_printf(MLME_DBG_LEVEL1,"  MAC:%02x %02x %02x %02x %02x %02x\n",mlme_p->mMAC[0],
        mlme_p->mMAC[1],mlme_p->mMAC[2],mlme_p->mMAC[3],
        mlme_p->mMAC[4],mlme_p->mMAC[5]);
        
    fw_printf(MLME_DBG_LEVEL1,"  ESSID:\"%s\"",mlme_p->bss.mEssid);
    fw_printf(MLME_DBG_LEVEL1,"  Time(ms):0x%x",mlme_get_time(mlme_p));
    fw_printf(MLME_DBG_LEVEL1,"  Bcn:0x%x\n",mlme_p->mLastBeaconTime);
    
    fw_printf(MLME_DBG_LEVEL1,    "System State:  CLASS %d    ",mlme_get_state(mlme_p,MLME_SYS_STATE_MACHINE));

    if(mlme_get_state(mlme_p,MLME_ASSOC_STATE_MACHINE)==ASSOC_IDLE)
        fw_printf(MLME_DBG_LEVEL1,"Assoc State:   IDLE\n");
    else
        fw_printf(MLME_DBG_LEVEL1,"Assoc State:   WAIT\n");
    
    if(mlme_get_state(mlme_p,MLME_AUTH_STATE_MACHINE)==AUTH_IDLE)
        fw_printf(MLME_DBG_LEVEL1,"Auth State:    IDLE       ");
    else if(mlme_get_state(mlme_p,MLME_AUTH_STATE_MACHINE)==WAIT_SEQ_2)
        fw_printf(MLME_DBG_LEVEL1,"Auth State:    SEQ2       ");
    else
        fw_printf(MLME_DBG_LEVEL1,"Auth State:    SEQ4       ");
    
    if(mlme_get_state(mlme_p,MLME_SYNC_STATE_MACHINE)==SYNC_IDLE)
        fw_printf(MLME_DBG_LEVEL1,"Sync State:    IDLE      \n");
    else if(mlme_get_state(mlme_p,MLME_SYNC_STATE_MACHINE)==JOIN_WAIT_BEACON)
        fw_printf(MLME_DBG_LEVEL1,"Sync State:    WaitBeacon\n");
    else
        fw_printf(MLME_DBG_LEVEL1,"Sync State:    ScanListen\n");
 
 
    //fw_printf(MLME_DBG_LEVEL1,"q->head=%d q->tail=%d mlme_dbg_level=0x%x\n\n",mlme_p->mQueue.head,mlme_p->mQueue.tail,mlme_dbg_level);
    mlme_print_table(&mlme_p->mTable);

#if 0
{
    int i;
fw_printf(MLME_DBG_LEVEL1,"wpa_ie=");
for(i=0;i<mlme_p->bss.wpa_ie[1]+2;i++)
    fw_printf(MLME_DBG_LEVEL1,"%02x ",mlme_p->bss.wpa_ie[i]);
fw_printf(MLME_DBG_LEVEL1,"\n");    
}
#endif
}
