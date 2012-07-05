#ifndef __IP_H__
#define __IP_H__

#define IPPROTO_ICMP	1
#define IPPROTO_TCP	6
#define IPPROTO_UDP	17

struct iphdr
{
	unsigned char	ihl:4;
	unsigned char	version:4;
	unsigned char	tos;
	unsigned short	tot_len;
	unsigned short	id;
	unsigned short	frag_off;
	unsigned char	ttl;
	unsigned char	protocol;
	unsigned short	check;
	unsigned int	saddr;
	unsigned int	daddr;
};

#define IP_HLEN sizeof (struct iphdr)

#endif // __IP_H__
