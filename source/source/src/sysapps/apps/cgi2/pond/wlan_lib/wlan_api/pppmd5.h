/***************************************************************
 * (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
 *
 *      Use of this software is restricted to the terms and
 *      conditions of SerComm's software license agreement.
 *
 *                        www.sercomm.com
 ****************************************************************/
/*
 ***********************************************************************
 ** md5.h -- header file for implementation of MD5                    **
 ** RSA Data Security, Inc. MD5 Message-Digest Algorithm              **
 ** Created: 2/17/90 RLR                                              **
 ** Revised: 12/27/90 SRD,AJ,BSK,JT Reference C version               **
 ** Revised (for MD5): RLR 4/27/91                                    **
 **   -- G modified to have y&~z instead of y&z                       **
 **   -- FF, GG, HH modified to add in last register done             **
 **   -- Access pattern: round 2 works mod 5, round 3 works mod 3     **
 **   -- distinct additive constant for each step                     **
 **   -- round 4 added, working mod 7                                 **
 ***********************************************************************
 */

/*
 ***********************************************************************
 ** Copyright (C) 1990, RSA Data Security, Inc. All rights reserved.  **
 **                                                                   **
 ** License to copy and use this software is granted provided that    **
 ** it is identified as the "RSA Data Security, Inc. MD5 Message-     **
 ** Digest Algorithm" in all material mentioning or referencing this  **
 ** software or this function.                                        **
 **                                                                   **
 ** License is also granted to make and use derivative works          **
 ** provided that such works are identified as "derived from the RSA  **
 ** Data Security, Inc. MD5 Message-Digest Algorithm" in all          **
 ** material mentioning or referencing the derived work.              **
 **                                                                   **
 ** RSA Data Security, Inc. makes no representations concerning       **
 ** either the merchantability of this software or the suitability    **
 ** of this software for any particular purpose.  It is provided "as  **
 ** is" without express or implied warranty of any kind.              **
 **                                                                   **
 ** These notices must be retained in any copies of any part of this  **
 ** documentation and/or software.                                    **
 ***********************************************************************
 */

#ifndef __MD5_INCLUDE__

				/* Vincent_001: Start */
//#include "adm_comm.h"
#define VOID   void
#define CHAR   char
#define BYTE   unsigned char
#define WORD   unsigned short
#define INT16  short
#define DWORD  unsigned long
				/* Vincent_001: Stop  */

/* typedef a 32-bit type */
//typedef DWORD UINT4;

/* Data structure for MD5 (Message-Digest) computation */
typedef struct {
  DWORD i[2];                   /* number of _bits_ handled mod 2^64 */
  DWORD buf[4];                                    /* scratch buffer */
  BYTE in[64];                              /* input buffer */
  BYTE digest[16];     /* actual digest after MD5Final call */
} MD5_CTX;

VOID MD5Init (MD5_CTX *mdContext);
VOID MD5Update (MD5_CTX *mdContext, BYTE *inBuf, WORD inLen);
VOID MD5Final (MD5_CTX *mdContext);

#define __MD5_INCLUDE__
#endif /* __MD5_INCLUDE__ */
