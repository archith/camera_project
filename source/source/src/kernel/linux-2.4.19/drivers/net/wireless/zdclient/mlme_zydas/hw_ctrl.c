/*
 * hw/hw_ctrl.c
 *
 * Copyright (C) 2004  Ivan Wang <ivan@faraday.com.tw>
 */ 

#include "mlme.h"
#include "hw_ctrl.h"
#include "../zdhw.h"
//#include "adapi.h"  //void definition

extern void zd1205_set_bss_rx_filter(void);
extern void zd1205_set_ibss_rx_filter(void);
extern void zd_setBasicRate(UINT8);
extern void HW_EnableSTA(U16 BecaonInterval, U16 DtimPeriod);
extern void HW_EnableIBSS(U16 BecaonInterval, U16 DtimPeriod);
extern void HW_MakeBeacon(U8 *pBeacon, U16 index);
extern void HW_StopBeacon(void);
extern int send_mgmt(unsigned char *pkt,unsigned int bodylen);
extern zd_80211Obj_t dot11Obj;

UINT32 HW_init(mlme_data_t *mlme_p)
{
//    mlme_p->hw_private=mlme_p - sizeof(zd_80211obj_t) + sizeof(mlme_data_t);
//    fw_printf(MLME_DBG_LEVEL1,"HW_open: hw_private:0x%x mlme_p:0x%x\n",mlme_p->hw_private,mlme_p);
return 1;
}

UINT32 HW_close(mlme_data_t *mlme_p)
{
//    fw_free(mlme_p->hw_private);
return 1;
}

/* call by OS driver to send management packet */
extern int send_auth_req(unsigned char *pkt);
UINT32 HW_tx(UINT8 *pkt,UINT32 bodylen)
{
    send_mgmt(pkt,bodylen);
    return 1;
}

void hw_cmd_reset(void *hw_private)
{
    return;
}

void hw_cmd_reset_mac(void *hw_private)
{
    return;
}

void hw_cmd_reset_bbp(void *hw_private)
{
    return;
}

void hw_cmd_reset_rf(void *hw_private)
{
    return;
}

void hw_cmd_stop_beacon(void *hw_private)
{
    //stop beacon
    HW_StopBeacon();
}

void hw_cmd_make_beacon(void *hw_private,UINT8 *pkt,UINT32 len)
{
//    printk("call HW_MakeBeacon:0x%x 0x%x len=0x%x\n",pkt[0],pkt[1],len);   
    len=len+4; /* CRC32 must increase at zd1205*/
    HW_MakeBeacon(pkt,len);
}

/*
UINT16
    parm[0]     => AP:0 STA:1 IBSS STA:2
    parm[1]     => beacon interval
    parm[2]     => DTIM interval
*/
void hw_cmd_set_mode(void *hw_private,void *data)
{
    UINT16  *parm=data;
//    printk("hw_cmd_set_mode:%d,%d,%d\n",parm[0],parm[1],parm[2]);
    switch(parm[0])
    {
        case MLME_ADHOC_MODE:
            HW_EnableIBSS((UINT16)parm[1],(UINT16)parm[2]);
            zd1205_set_ibss_rx_filter();
            break;
        case MLME_AP_MODE:            
            break;
        case MLME_INFRA_MODE:
            HW_EnableSTA(parm[1],parm[2]);
            zd1205_set_bss_rx_filter();
            break;
    }
}

void hw_cmd_set_mac(void *hw_private,void *data)
{
}

extern void zd1205_set_bssid(unsigned char *bssid);
void hw_cmd_set_bssid(void *hw_private,void *data)
{
    zd1205_set_bssid(data);
}

void hw_cmd_get_mac(void *hw_private,void *data)
{
    
}

void hw_cmd_get_time(void *hw_private,void *data)
{
    *(unsigned int *)data=jiffies;
}

void hw_cmd_set_tsf(void *hw_private,void *data)
{
}

void hw_cmd_get_channel(void *hw_private,void *data)
{
}

void hw_cmd_set_channel(void *hw_private,void *data)
{
    HW_SwitchChannel(&dot11Obj,*(UINT16 *)data);
}

void hw_cmd_get_channel_info(void *hw_private,void *data)
{
    UINT32 *c;
    c=(UINT32 *)data;
    c[0]=1;c[1]=2;c[2]=3;c[3]=4;c[4]=5;
    c[5]=6;c[6]=7;c[7]=8;c[8]=9;c[9]=10;
    c[10]=11;c[11]=0;
}

void hw_cmd_get_rate_info(void *hw_private,void *data)
{
    UINT8 *c;
    c=(UINT8 *)data;
    c[0]=MLME_SR_1M|0x80;
    c[1]=MLME_SR_2M|0x80;
    c[2]=MLME_SR_5_5M|0x80;
    c[3]=MLME_SR_11M|0x80;
    c[4]=0;
}

void hw_cmd_set_basic_rate(void *hw_private,void *data)
{
    if(*(UINT8 *)data>(MLME_SR_11M|0x80)||(*(UINT8 *)data==0))
        *(UINT8 *)data=MLME_SR_11M|0x80;    //max basic rate<= 11M
    zd_setBasicRate(*(UINT8 *)data);
}

void hw_cmd_get_rate(void *hw_private,void *data)
{
    
}

void hw_cmd_set_rate(void *hw_private,void *data)
{
}

void hw_cmd_get_ps(void *hw_private,void *data)
{
}

void hw_cmd_set_ps(void *hw_private,void *data)
{
    
}

UINT32 HW_ioctl(void *hw_private,UINT32 cmd,void *parm1,void *parm2)
{
    switch(cmd)
    {
        case HW_CMD_NULL:
            return 0;
        case HW_CMD_RESET:
            hw_cmd_reset(hw_private);
            break;
        case HW_CMD_RESET_MAC:
            hw_cmd_reset_mac(hw_private);
            break;
        case HW_CMD_RESET_BBP:
            hw_cmd_reset_bbp(hw_private);
            break;
        case HW_CMD_RESET_RF:
            hw_cmd_reset_rf(hw_private);
            break;
        case HW_CMD_STOP_BEACON:   /* stop BSS service */
            hw_cmd_stop_beacon(hw_private);
            break;
        case HW_CMD_MAKE_BEACON:
            hw_cmd_make_beacon(hw_private,parm1,*(UINT32 *)parm2);
            break;
        case HW_CMD_SET_MODE:
            hw_cmd_set_mode(hw_private,parm1);
            break;
        case HW_CMD_SET_BSSID:
            hw_cmd_set_bssid(hw_private,parm1);
            break;
        case HW_CMD_SET_MAC:
            hw_cmd_set_mac(hw_private,parm1);
            break;
        case HW_CMD_GET_MAC:
            hw_cmd_get_mac(hw_private,parm1);
            break;
        case HW_CMD_GET_TIME:
        	hw_cmd_get_time(hw_private,parm1);
            break;
        case HW_CMD_SET_TIME:
            hw_cmd_set_tsf(hw_private,parm1);
            break;
        case HW_CMD_GET_CHANNEL:
            hw_cmd_get_channel(hw_private,parm1);
            break;
        case HW_CMD_SET_CHANNLE:
            hw_cmd_set_channel(hw_private,parm1);
            break;
        case HW_CMD_GET_CHANNEL_INFO:
            hw_cmd_get_channel_info(hw_private,parm1);
            break;
        case HW_CMD_GET_RATE:
            hw_cmd_get_rate(hw_private,parm1);
            break;
        case HW_CMD_SET_RATE:
            hw_cmd_set_rate(hw_private,parm1);
            break;
        case HW_CMD_GET_RATE_INFO:
            hw_cmd_get_rate_info(hw_private,parm1);
            break;
        case HW_CMD_SET_BASIC_RATE:
            hw_cmd_set_basic_rate(hw_private,parm1);
            break;
        case HW_CMD_GET_PS:
            hw_cmd_get_ps(hw_private,parm1);
            break;
        case HW_CMD_SET_PS:
            hw_cmd_set_ps(hw_private,parm1);
            break;
        default:
            break;
    }
    return 1;
}