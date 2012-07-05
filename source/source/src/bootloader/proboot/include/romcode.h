#ifndef __ROMCODE_H__
#define __ROMCODE_H__

#define ROMBASE			0x1fc00000
//#define ROMBASE			0x02000000

/*
 * PL1029
 */

#define SM_Read_Page		(ROMBASE + 0x80)
#define SM_Init			(ROMBASE + 0x88)
#define Show_Status		(ROMBASE + 0x90)
#define Show_Message		(ROMBASE + 0x98)
#define Show_Register		(ROMBASE + 0xa0)
//#define Show_Integer		(ROMBASE + 0xa8)
//#define Console_Status		(ROMBASE + 0xb0)
//#define FlashToRAM		(ROMBASE + 0xb8)
#define Version			(ROMBASE + 0xc0)
//#define Console_Serial_Break	(ROMBASE + 0xc8)
//#define Change_Freq		(ROMBASE + 0xd0)
#define SM_Get_Phy_Addr		(ROMBASE + 0xd8)
//#define Detect_Console		(ROMBASE + 0xe0)

#endif // __ROMCODE_H__
