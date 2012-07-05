
#ifndef DM9102_H
#define DM9102_H

#ifdef _LAN_DRV_DM9102_

void secdmfe_transmit(unsigned char *data,unsigned short len);
int secdmfe_open(void);
void secdmfe_receive(unsigned char *data,unsigned short *len);
void secdmfe_setmac(unsigned char *mac);

#endif

#endif

