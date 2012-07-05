
/***************************************************************
 * (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
 *
 *      Use of this software is restricted to the terms and
 *      conditions of SerComm's software license agreement.
 *
 *                        www.sercomm.com
 ****************************************************************/
#ifndef _FL_CONF_H_
#define _FL_CONF_H_


//error code
#define	FL_RangeErr	1
#define FL_IOErr	2
#define FL_MemErr	3
#define FL_NullPara	4
#define FL_EraseErr	5
#define FL_FileErr	6
#define FL_InvalidMac	7

#define FLASH_SUCCESS	0
#define FLASH_FAILURE   FL_IOErr	//ioctl error

//-----------------------------------------------------------
// purpose: read configuration from flash to ramdisk file
// input: fname - ramdisk file name
// return: 0 - ok 
//         other values - read error
//-----------------------------------------------------------
int fl_read_conf(char *fname);

//-----------------------------------------------------------
// purpose: write configuration from file to flash
// input: fname - configuration file name
// return: 0 - ok 
//         other values - write error
//-----------------------------------------------------------
int fl_write_conf(char *fname);


//-----------------------------------------------------------
// purpose: read CA from flash to ramdisk file
// input: fname - ramdisk file name
//	  type	- 0: TLS root CA,1: TLS user CA+PrivateKey,2:TTLS root CA
// return: 0 - ok 
//         other values - read error
//-----------------------------------------------------------
int fl_read_CA(char *fname,int type);

//-----------------------------------------------------------
// purpose: write CA from file to flash
// input: fname - configuration file name
//	  type	- 0: TLS root CA,1: TLS user CA+PrivateKey,2:TTLS root CA
// return: 0 - ok 
//         other values - write error
//-----------------------------------------------------------
int fl_write_CA(char *fname,int type);


//--------------------------------------------------------------
// purpose: read mac address from flash
// output: mac - buffer to save mac address (6 bytes)
//               if the mac address==00c002012345 then mac[0]== 
//		 0x00c0, mac[1]==0x0201 and mac[2]==0x2345
// return: 0 - ok 
//         other values - write error
//--------------------------------------------------------------
//int fl_read_mac(unsigned short mac[3]);

//--------------------------------------------------------------
// purpose: write mac address to flash
// input: mac - mac address (6 bytes)
//              if the mac address==00c002012345 then mac[0]== 
//		0x00c0, mac[1]==0x0201 and mac[2]==0x2345
// return: 0 - ok 
//         other values - write error
//--------------------------------------------------------------
//int fl_write_mac(unsigned short mac[3]);

//---------------------------------------------------------
//purpuse: check system.conf magic number match to def_sys.conf
//return: 0 - magic number match
//        other value - magic number not match
//----------------------------------------------------------
int check_magic_number();


#endif
