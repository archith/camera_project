#ifndef __SYSUTIL__
#define __SYSUTIL__


void show_LED(unsigned char a);
void Show_Status(unsigned char a);
int Version(void);

/* Error code definitions */
#define CPU_FAULT			0xE0
#define CPU_HW_Exception		0xE1
#define ERR_MCAS			0xE2
#define ERR_MRAS			0xE3
#define ERR_READ_SM			0xE4
#define ERR_UNBOOTABLE_SM		0xE5
#define BAUDRATE_TOO_HIGH		0xE6
#define ERR_CONSOLE_DETECT		0xE7
#define ERR_LESS_MEM			0xE8
#define ERR_MEM_TEST			0xE9
#define ERR_SERIAL_BREAK		0xEA
#define ERR_FRAME			0xEB
#define ERR_PARITY			0xEC
#define ERR_OVERRUN			0xED
#define ERR_READ_SEEPROM		0xEE
#define ERR_UNBOOTABLE_SEEPROM          0xEF
#define ERR_READ_NOR			0xF1	
#define ERR_UNBOOTABLE_NOR		0xF2	



/* Memory R/W */
#define Byte_RD(Addr)		( *(volatile unsigned char *)  (Addr) )
#define Word_RD(Addr)		( *(volatile unsigned short *) (Addr) )
#define DW_RD(Addr)		( *(volatile unsigned int *)   (Addr) )
//----------------------------------------------------------------------------   
#define Byte_WR(Addr, WData)	( *(volatile unsigned char *)  (Addr) ) = (unsigned char)WData
#define Word_WR(Addr, WData)	( *(volatile unsigned short *) (Addr) ) = (unsigned short)WData
#define DW_WR(Addr, WData)	( *(volatile unsigned int *)   (Addr) ) = (unsigned int)WData

//Global variables (memory from 0x100 to 0x200 ,should less than 256 bytes)
// used by flash.c
#define large		0x100	
#define maxblock	0x104
#define device_addr	0x108
// used by eeprom.c
#define MCR2		0x11C
#define DATA		0x110


#endif /* __SYSUTIL__ */
