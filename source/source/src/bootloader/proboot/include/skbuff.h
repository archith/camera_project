#ifndef __SKBUFF_H__
#define __SKBUFF_H__

#include <udp.h>
#include <ip.h>
#include <eth.h>

struct sk_buff
{
	unsigned char	buf[2048];
	unsigned char	*data;
	int		len;

	int		used;
} __attribute__ ((aligned (4)));

void skb_init (void);

struct sk_buff *alloc_skb (void);
void free_skb (struct sk_buff *skb);

void skb_reserve (struct sk_buff *skb, int len);
char *skb_put  (struct sk_buff *skb, int len);
char *skb_pull (struct sk_buff *skb, int len);
char *skb_push (struct sk_buff *skb, int len);

#endif // __SKBUFF_H__
