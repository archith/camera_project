#ifndef __ARP_H__
#define __ARP_H__

#define ARPHRD_ETHER	1		// Ethernet

#define ARPOP_REQUEST	1		// ARP request
#define ARPOP_REPLY	2		// ARP reply

#include <eth.h>
struct arphdr
{
	unsigned short	hrd;		// format of hardware address
	unsigned short	pro;		// format of protocol address
	unsigned char	hln;		// length of hardware address
	unsigned char	pln;		// length of protocol address
	unsigned short	op;		// ARP opcode (command)

	unsigned char	sha[ETH_ALEN];	// sender hardware address
	unsigned int	sip;		// sender IP address
	unsigned char	tha[ETH_ALEN];	// target hardware address
	unsigned int	tip;		// target IP address
} __attribute__ ((packed));

#define ARP_HLEN sizeof (struct arphdr)

#endif // __ARP_H__
