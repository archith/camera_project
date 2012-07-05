#ifndef CONFIG_H
#define CONFIG_H

#define		FOUR_ZERO_ZERO			0
#define		FOUR_TWO_ZERO			1
#define		FOUR_TWO_TWO			2
#define		FOUR_FOUR_FOUR			3
#define		RGB						4

#define		BLOCK_SIZE				64

#define SRC_W		640
#define SRC_H		480
#define RES_128X96	1
#define RES_64X48	2
#define FACTOR_640to128	5
#define FACTOR_640to64	10


#define DEFAULT_QUALITY 1024

#define Y_SIZE          (0x14000 * 4)
#define U_SIZE          (0x14000)
#define V_SIZE          (0x14000)

#define U_OFSET Y_SIZE			//0x50000
#define V_OFSET (Y_SIZE + U_SIZE)	//0x64000

//#define YUV_SIZE      (Y_SIZE+U_SIZE+V_SIZE)
//char ScaledYUV[YUV_SIZE];
#define YUV400_SIZE	(128*96)
#define YUV422_SIZE	(128*96*4)
#define YUV420_SIZE	((128*96*3) >> 2)

//extern char ScaledYUV[YUV422_SIZE];


#if 0
#define CY(...)		printf("%x \n")
#define CCb(...)	printf("%x \n")
#define CCr(...)  	printf("%x \n")
#endif
extern UINT8	Lqt [BLOCK_SIZE];
extern UINT8	Cqt [BLOCK_SIZE];
extern UINT16	ILqt [BLOCK_SIZE];
extern UINT16	ICqt [BLOCK_SIZE];

extern INT16	Y1 [BLOCK_SIZE];
extern INT16	Y2 [BLOCK_SIZE];
extern INT16	Y3 [BLOCK_SIZE];
extern INT16	Y4 [BLOCK_SIZE];
extern INT16	CB [BLOCK_SIZE];
extern INT16	CR [BLOCK_SIZE];
extern INT16	Temp [BLOCK_SIZE];

extern UINT32 lcode;
extern UINT16 bitindex;


#endif
