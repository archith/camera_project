#ifndef __INET__
#define __INET__

typedef unsigned short	u16;
typedef unsigned int	u32;

#define htons(x) \
({ \
	u16 __x = (x); \
	((u16)( (((u16)(__x) & (u16)0x00ffU) << 8) | \
		  (((u16)(__x) & (u16)0xff00U) >> 8) )); \
})

#define ntohs(x) \
({ \
	u16 __x = (x); \
	((u16)( (((u16)(__x) & (u16)0x00ffU) << 8) | \
		  (((u16)(__x) & (u16)0xff00U) >> 8) )); \
})

#define htonl(x) \
({ \
	u32 __x = (x); \
	((u32)( (((u32)(__x) & (u32)0x000000ffUL) << 24) | \
		  (((u32)(__x) & (u32)0x0000ff00UL) <<  8) | \
		  (((u32)(__x) & (u32)0x00ff0000UL) >>  8) | \
		  (((u32)(__x) & (u32)0xff000000UL) >> 24) )); \
})

#define ntohl(x) \
({ \
	u32 __x = (x); \
	((u32)( (((u32)(__x) & (u32)0x000000ffUL) << 24) | \
		  (((u32)(__x) & (u32)0x0000ff00UL) <<  8) | \
		  (((u32)(__x) & (u32)0x00ff0000UL) >>  8) | \
		  (((u32)(__x) & (u32)0xff000000UL) >> 24) )); \
})


unsigned int inet_addr (char *cp);
char *inet_ntoa (unsigned int cp);

#endif // __INET__
