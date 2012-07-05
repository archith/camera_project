/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _SERIAL_BILK_OPS_H_
#define  _SERIAL_BILK_OPS_H_

#define COM_BULK_NAME		"SERIAL"

struct COM_DS
{
	int  SerialMode;
	int  RS485Addr;
	int  BaudRate;
	int  DataBit;
	int  StopBit;
	int  Parity;
};

int COM_BULK_ReadDS(void* ds);
int COM_BULK_CheckDS(void* ds, void* ds_org);
int COM_BULK_WriteDS(void* ds, void* ds_org);
int COM_BULK_RunDS(void* ds, void* ds_org);

#endif

