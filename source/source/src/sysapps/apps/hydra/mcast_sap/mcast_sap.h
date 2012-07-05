/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/

#ifndef _MCAST_SAP_H_
#define _MCAST_SAP_H_

#include "model.h"

#define UDP_PORT  0x0396  /** valid range: 1024 - 5000 **/

/*------------------ Error Code Definition Start --------------------------*/

#define  Action_ok                  0     /* Successful */
#define  Action_fail               -1     /* Command error */
#define INVALID_SOCKET (int)     (~0)
#define Err_SOCKET                 -1     /* Note: the no cannot be change */
/*------------------- UPD Packet Definition -------------------------------*/
#if 0
struct UDP_Header{
       unsigned short int   command;
       short int   result;
       unsigned short int   sequenceNo;
       unsigned short int   length;
} __attribute__ ((packed));
#endif


struct SAP_Header {
        char    v;
        char    auth_len;
        char    msg_id_hash[2];
#ifdef _IPV4_
        //char    orig_source[4];
	in_addr_t orig_source; /*4 byte*/
#elif _IPV6_
        char    orig_source[16]
#endif
	//char    auth_data[4];
        char    payload_type[16]; //not sure
} __attribute__ ((packed));


struct UDP_Packet{
        struct  SAP_Header      sap_header;
        char    payload[1000];  //not sure

} __attribute__ ((packed));

int mcast_sap();

#endif /* _QUICKSET_H_ */
