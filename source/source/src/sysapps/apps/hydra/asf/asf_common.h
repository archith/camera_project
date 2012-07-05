#ifndef COMMON_H
#define COMMON_H

#define IN
#define OUT

typedef unsigned char BYTE;
typedef unsigned short WORD;   // 2 bytes
typedef unsigned int DWORD;  // 4 bytes
typedef double QWORD;  // 8 bytes
typedef long LONG;   // 4 bytes
#define  ASF_RES_160  160  /* 128 x 160 */
#define  ASF_RES_320  320  /* 240 x 320, default */
#define  ASF_RES_640  640  /* 480 x 640 */
#define  ASF_RES_D1   704  /* 480 x 704 */

#define LE_ERR(a, b) if(a<=b){ fprintf(stderr,"err:%d <= %d:%d\n", a,b,__LINE__);exit(1);}
#define EQ_ERR(a, b) if(a==b){ fprintf(stderr,"err:%d == %d:%d\n", a,b,__LINE__); exit(1);}
#define NE_ERR(a, b) if(a!=b){ fprintf(stderr,"err:%d != %d:%d\n", a,b,__LINE__); exit(1);}

#define DEBUG
#ifdef DEBUG
#define DBG(a...)	fprintf(stderr, a);
#else
#define DBG(a...)
#endif
#define ERROUT(a...)	{DBG(a);goto errout;}

#define ASF_RET_BUSY	1
#define ASF_RET_OK	0
#define ASF_RET_ERR	-1
void dump(unsigned long addr, int size, char* name);
char* ASF_GetMP4Header(int width, int height);

#endif

