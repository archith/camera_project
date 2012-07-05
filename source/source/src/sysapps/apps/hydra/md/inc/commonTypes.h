/********************************************************************/
/*                   DM270 evaluation SW                            */
/*                                                                 	*/
/*                                    source: type define file      */
/********************************************************************/

#ifndef __COMMON_TYPES_H__
#define __COMMON_TYPES_H__

#ifndef NULL
#define NULL	0
#endif

#define BLKSIZE		16
#define BLKUINT_640	(639/BLKSIZE)	//39
#define BLKUINT_480	(479/BLKSIZE)	//29
#define BLKUINT_320	(319/BLKSIZE)	//19
#define BLKUINT_240	(239/BLKSIZE)	//14
#define BLKUINT_160	(159/BLKSIZE)	//9
#define BLKUINT_120	(119/BLKSIZE)	//7

#define LAST_PULL_H_EXPIRETM	2 	//5sec
typedef unsigned char   Uchar;
typedef unsigned char	Uint8;
typedef unsigned short  Uint16;
typedef unsigned long   Uint32;

typedef char		Char;
typedef char		Int8;

typedef short       	Int16;
typedef long        	Int32;
typedef void *      	Handle;

#define	STATUS		short

//typedef enum {FALSE, TRUE} 	BOOL;

/* useful macros for declaring peripheral registers */
#ifndef REG16
#define REG16(addr)     (*(volatile Uint16*)(addr))
#endif

#ifndef AND_DEFAULT
#define AND_DEFAULT      ((Uint16)0xFFFFu)
#endif
#ifndef OR_DEFAULT
#define OR_DEFAULT       ((Uint16)0x0000u)
#endif
#ifndef INV_DEFAULT
#define INV_DEFAULT      ((Uint16)0x0000u)
#endif
/* invalid pointer */
#define INV    ((void*)(-1))

typedef char            CHAR;
typedef unsigned char   UCHAR;
typedef int             INT;
typedef unsigned int    UINT;
typedef short           SHORT;
typedef unsigned short  USHORT;
//typedef long            LONG;
//typedef unsigned long   ULONG;
typedef double   		DOUBLE;
typedef unsigned char   UB;         /* Unsigned 8-bit integer       */

/* ***   Definitions   *** */
typedef char            INT8;
typedef short           INT16;
typedef long            INT32;
typedef unsigned char   UINT8;
typedef unsigned short  UINT16;
typedef unsigned long   UINT32;


//===
// Macros
//===
#define BYTESWAP(a)  (((UINT16)(a) <<  8) | ((UINT16)(a) >>  8))
#define WORDSWAP(a)  (((UINT32)(a) << 16) | ((UINT32)(a) >> 16))

#endif /* _COMMON_TYPES_H_ */
