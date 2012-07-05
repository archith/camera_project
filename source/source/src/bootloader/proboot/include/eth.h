#ifndef __ETH_H_
#define __ETH_H_

#define ETH_ALEN	6	// Octets in one ethernet addr
#define ETH_HLEN	14	// Total octets in header
#define ETH_ZLEN	60	// Min octets in frame sans FCS
#define ETH_FRAME_LEN	1514	// Max octets in frame sans FCS

#define ETH_P_IP	0x0800	// Internet Protocol packet
#define ETH_P_ARP	0x0806	// Address Resolution packet

struct ethhdr
{
	unsigned char	dst[ETH_ALEN];
	unsigned char	src[ETH_ALEN];
	unsigned short	protocol;
} __attribute__((packed));

#endif // __ETH_H_
