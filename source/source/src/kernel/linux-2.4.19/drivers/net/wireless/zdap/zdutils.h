#ifndef __ZDUTILS_H__
#define __ZDUTILS_H__
#include "zdsorts.h"
#include "zdsm.h"


#define isGroup(pMac)			(((U8*)pMac)[0] & 0x01)
#define eLen(elm)				((elm)->buf[1])
#define body(f, n) 				((f)->body[n])


#define addr1(f)				((MacAddr_t*)&((f)->header[4]))
#define addr2(f)				((MacAddr_t*)&((f)->header[10]))
#define addr3(f)				((MacAddr_t*)&((f)->header[16]))
#define addr4(f)				((MacAddr_t*)&((f)->header[24]))
#define setAddr1(f, addr)		(memcpy((char*)&((f)->header[4]),  (char*)addr, 6))
#define setAddr2(f, addr)		(memcpy((char*)&((f)->header[10]), (char*)addr, 6))
#define setAddr3(f, addr)		(memcpy((char*)&((f)->header[16]), (char*)addr, 6))
#define setAddr4(f, addr)		(memcpy((char*)&((f)->header[24]), (char*)addr, 6))
#define setFrameType(f, ft)		do {\
										f->header[0] = ft;\
										f->header[1] = 0;\
								} while (0)
#define baseType(f)				((f)->header[0] & 0x0C)
#define frmType(f)				((f)->header[0] & 0xFC)
#define setFrDs(f, frds)		((f)->header[1] = ((f)->header[1] & 0xFD) | ((frds)<<1))
#define setMoreFrag(f, mf)		((f)->header[1] = ((f)->header[1] & 0xFB) | ((mf)<<2))
#define setMoreData(f, md) 		((f)->header[1] = ((f)->header[1] & 0xDF) | ((md)<<5) )
#define wepBit(f) 				(((f)->header[1] & WEP_BIT) ? 1 : 0)
#define setWepBit(f, wep) 		((f)->header[1] = ((f)->header[1] & 0xBF) | ((wep)<<6))
#define orderBit(f) 			(((f)->header[1] & ORDER_BIT) ? 1 : 0)
#define durId(f)				(((f)->header[2]) + ((f)->header[3]*256))
#define setFrag(f, fr) 			((f)->header[22] = ((f)->header[22] & 0xF0) | (fr))

#define status(f)  				(body(f, 2) + (body(f, 3) * 256))				
#define authType(f) 			(body(f, 0) + (body(f, 1) * 256))
#define authSeqNum(f)			(body(f, 2) + (body(f, 3) * 256))
#define reason(f)				(body(f, 0) + (body(f, 1) * 256))		
#define listenInt(f)			(body(f, 2) + (body(f, 3) * 256))	
#define cap(f)					(body(f, 0) + (body(f, 1) * 256))	
#define setTs(f, loTm, hiTm)  	do {\
									body(f, 0) = (U8)loTm;\
									body(f, 1) = (U8)(loTm >> 8);\
									body(f, 2) = (U8)(loTm >> 16);\
									body(f, 3) = (U8)(loTm >> 24);\
									body(f, 4) = (U8)hiTm;\
									body(f, 5) = (U8)(hiTm >> 8);\
									body(f, 6) = (U8)(hiTm >> 16);\
									body(f, 7) = (U8)(hiTm >> 24);\
								} while (0)
#define trafficMap(trafficmap, aid)  (((trafficmap)->t[(aid/8)] & (1<<(7-(aid%8))) ) == 0 ? 0 : 1) 

#endif

