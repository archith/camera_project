#ifndef __NET_H__
#define __NET_H__

#include <skbuff.h>
//#include <inet.h>
#include <eth.h>
#include <arp.h>
#include <ip.h>
#include <udp.h>
#include <tftp.h>

/* address - l for local, r for remote */
extern unsigned char	lmac[ETH_ALEN], rmac[ETH_ALEN];
extern unsigned int	lip, rip;
extern unsigned short	lport, rport;

/* tftp */
extern unsigned char	*tftp_buf;
extern int		tftp_len, tftp_exit;

int eth_send (struct sk_buff *skb, unsigned short proto);
int eth_recv (struct sk_buff *skb);

int arp_send (int op);
int arp_recv (struct sk_buff *skb);


int ip_send (struct sk_buff *skb, int protocol);
int ip_recv (struct sk_buff *skb);

int udp_send (struct sk_buff *skb);
int udp_recv (struct sk_buff *skb);

int tftp_send_rrq (char *file);
int tftp_recv (struct sk_buff *skb, int len);

unsigned short checksum (void *buf, int len);

#endif // __NET_H__
