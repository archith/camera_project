/*
 * hw/hw_ctrl.c
 *
 * Copyright (C) 2004  Ivan Wang <ivan@faraday.com.tw>
 */ 

#include "mlme.h"
#include "fw_os.h"

#define HW_CMD_NULL             0
#define HW_CMD_RESET            1
#define HW_CMD_RESET_MAC        2
#define HW_CMD_RESET_BBP        3
#define HW_CMD_RESET_RF         4
#define HW_CMD_STOP_BEACON      5
#define HW_CMD_MAKE_BEACON      6
#define HW_CMD_SET_MODE         7
#define HW_CMD_SET_BSSID        8
#define HW_CMD_GET_TIME         10
#define HW_CMD_SET_TIME         11
#define HW_CMD_GET_CHANNEL      12
#define HW_CMD_SET_CHANNLE      13
#define HW_CMD_GET_CHANNEL_INFO 14
#define HW_CMD_GET_RATE         15
#define HW_CMD_SET_RATE         16
#define HW_CMD_GET_RATE_INFO    17
#define HW_CMD_SET_BASIC_RATE   18
#define HW_CMD_GET_PS           19
#define HW_CMD_SET_PS           20
#define HW_CMD_SET_MAC          21
#define HW_CMD_GET_MAC          22

#define HW_TX_MGMT          0
#define HW_TX_DATA          1

UINT32 HW_ioctl(void *hw_private,UINT32 cmd,void *parm1,void *parm2);
UINT32 HW_init(mlme_data_t *);
UINT32 HW_close(mlme_data_t *);
UINT32 HW_tx(UINT8 *pkt,UINT32 len);
