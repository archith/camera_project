#ifndef __DIAG_H__
#define __DIAG_H__

extern unsigned char g_tftpip[16]; 
extern unsigned char g_tftpfile[255]; 
int diag_march(int cycle);
int diag_nor(int cycle);
int diag_tftp(int cycle);
int diag_m2m(int cycle);

#endif // __DIAG_H__
