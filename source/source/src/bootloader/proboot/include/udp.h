#ifndef __UDPHDR__
#define __UDPHDR__

struct udphdr
{
	unsigned short	sport;
	unsigned short	dport;
	unsigned short	len;
	unsigned short	check;
} __attribute__((packed));

#define UDP_HLEN sizeof (struct udphdr)

#endif // __UDPHDR__
