/*
 * IC+ - IP100A driver header file
 */

#ifndef __IP100A_H__
#define __IP100A_H__

#ifdef _LAN_DRV_IP100A_

void ip100a_sd_receive(unsigned char *data,unsigned short* len);
void ip100a_sd_setmac(unsigned char *mac);
void ip100a_sd_transmit(unsigned char *data,unsigned short len);
int ip100a_sd_open(void);

#endif

#endif

