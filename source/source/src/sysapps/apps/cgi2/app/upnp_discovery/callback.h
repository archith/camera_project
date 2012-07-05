/***************************************************************
*             Copyright (C) 2003 by SerComm Corp.
*                    All Rights Reserved.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef _CALLBACK_H
#define _CALLBACK_H

int http_ServerCallback( char * request, void *args );

#define CALLBACK_OK			0
#define CALLBACK_GET_FILE_ERR		-1
#define CALLBACK_ADD_MODIFY_ERR		-2
#define CALLBACK_ADD_DATE_ERR		-3
#define CALLBACK_SEND_HEADER_ERR	-4
#define CALLBACK_SEND_FILE_ERR		-5

#endif
