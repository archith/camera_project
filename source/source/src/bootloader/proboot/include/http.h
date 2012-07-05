//==========================================================================
//        http.h
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    Zero
// Contributors: Zero
// Date:         2006-11-8
// Purpose:      
// Description:  
//              
// This code is part of RedBoot ,porting from ecos
//
//####DESCRIPTIONEND####
//
//==========================================================================

#ifndef _HTTP_H_

#define _HTTP_H_

#include "cross.h"

//#define zero_debug

//#define STAR_PLATFORM
//#define printf diag_printf
//#define sprintf diag_sprintf

#ifdef zero_debug
#define debug_printf(fmt, args...) \
	{printf("%s(line %d):" , __FUNCTION__ ,__LINE__);printf( fmt,##args);}
#else
#define debug_printf printf
#endif

//#define BOOTP



#define ture 1
 /*length of ethernet header,src mac(6) + des mac(6) + protocol type(2)*/
#ifdef STAR_PLATFORM
#define ETHLEN 18 //4//include 4bytes padding
#else
#define ETHLEN 14
#endif

#define IPHLEN  20
#define TCPHLEN 20
#define UDPHLEN  8

#ifdef STAR_PLATFORM
#define ACKBUF_LEN 54+4 //4 //bytes padding
#else
#define ACKBUF_LEN 54
#endif
/*
 * Minimum ethernet packet length.
 */
#define ETH_MIN_PKTLEN  64//60
#define ETH_MAX_PKTLEN  (1540-14)


typedef unsigned char enet_addr_t[6];
typedef unsigned char ip_addr_t[4];

typedef unsigned char  octet;
typedef unsigned short word;
typedef unsigned int   dword;
typedef unsigned char BYTE;

#define WORD word
#define DWORD dword
#define CHAR char

#ifndef NULL
#define NULL 0
#endif

#if 1//this should reference your platform
#define __LITTLE_ENDIAN__
unsigned long  ntohl(unsigned long x);
unsigned long  ntohs(unsigned short x);
#else
#define	ntohl(x)	(x)
#define	ntohs(x)	(x)
#endif

#define	htonl(x)	ntohl(x)
#define	htons(x)	ntohs(x)

/*
 * Ethernet header.
 */
 #define ETH_TYPE_IP   0x800
#define ETH_TYPE_ARP  0x806
#define ETH_TYPE_RARP 0x8053
typedef struct {
    enet_addr_t   destination;
    enet_addr_t   source;
#ifdef STAR_PLATFORM
    dword		pad;//if  chip is different,this should remove
#endif
    word          type;
} __attribute__((packed)) eth_header_t;


/*
 * ARP/RARP header.
 */
#define ARP_HW_ETHER 1
#define ARP_HW_EXP_ETHER 2

#define	ARP_REQUEST	1
#define	ARP_REPLY	2
#define	RARP_REQUEST	3
#define	RARP_REPLY	4
typedef struct {
    word	hw_type;
    word	protocol;
    octet	hw_len;
    octet	proto_len;
    word	opcode;
    enet_addr_t	sender_enet;
    ip_addr_t	sender_ip;
    enet_addr_t	target_enet;
    ip_addr_t	target_ip;
} __attribute__((packed)) arp_header_t;


//efine ARP_PKT_SIZE  (sizeof(arp_header_t) + ETHLEN)

/*
 * Internet Protocol header.
 */
#define IP_PROTO_ICMP  1
#define IP_PROTO_TCP   6
#define IP_PROTO_UDP  17
typedef struct {
#ifdef __LITTLE_ENDIAN__
    octet       hdr_len:4, //IP header length
                version:4; 
#else
    octet       version:4,
                hdr_len:4;
#endif
    octet       tos;
    word        length;
    word        ident;
    word        fragment;
    octet       ttl;
    octet       protocol;
    word        checksum;
    ip_addr_t   source;
    ip_addr_t   destination;
} __attribute__((packed)) ip_header_t;


//efine IP_PKT_SIZE (60 + ETHLEN)


/*
 * A IP<->ethernet address mapping.
 */
typedef struct {
    ip_addr_t    ip_addr;
    enet_addr_t  enet_addr;
} ip_route_t;


/*
 * UDP header.
 */
typedef struct {
    word	src_port;
    word	dest_port;
    word	length;
    word	checksum;
} __attribute__((packed)) udp_header_t;


/*
 * TCP header.
 */
#define TCP_FLAG_FIN  1
#define TCP_FLAG_SYN  2
#define TCP_FLAG_RST  4
#define TCP_FLAG_PSH  8
#define TCP_FLAG_ACK 16
#define TCP_FLAG_URG 32
typedef struct {
    word	src_port;
    word	dest_port;
    dword	seqnum;
    dword	acknum;
#ifdef __LITTLE_ENDIAN__
    octet       reserved:4,
                hdr_len:4;
#else
    octet       hdr_len:4,
                reserved:4;
#endif
    octet	flags;
    word	window;
    word	checksum;
    word	urgent;
} __attribute__((packed)) tcp_header_t;


/*
 * ICMP header.
 */
#define ICMP_TYPE_ECHOREPLY   0
#define ICMP_TYPE_ECHOREQUEST 8
typedef struct {
    octet	type;
    octet	code;
    word	checksum;
    word	ident;
    word	seqnum;
} __attribute__((packed)) icmp_header_t;


#define BP_CHADDR_LEN	 16
#define BP_SNAME_LEN	 64
#define BP_FILE_LEN	128
#define BP_VEND_LEN	312
#define BP_MINPKTSZ	300	/* to check sizeof(struct bootp) */
#define BP_MIN_VEND_SIZE 64     /* minimum actual vendor area */

/*
 * UDP port numbers, server and client.
 */
#define	IPPORT_BOOTPS		67
#define	IPPORT_BOOTPC		68

#define BOOTP_REQUEST 0x1
#define BOOTP_REPLEY    0x2
#define DHCP_DISCOVER  0x1
#define DHCP_OFFER        0x2	
#define DHCP_REQUEST    0x3
#define DHCP_ACK 	      0x5
#define DHCP_NAK	      0x6

typedef struct bootp {
    unsigned char    bp_op;			/* packet opcode type */
    unsigned char    bp_htype;			/* hardware addr type */
    unsigned char    bp_hlen;			/* hardware addr length */
    unsigned char    bp_hops;			/* gateway hops */
    unsigned int      bp_xid;			/* transaction ID */
    unsigned short   bp_secs;			/* seconds since boot began */
    unsigned short   bp_flags;			/* RFC1532 broadcast, etc. */
   // struct in_addr   bp_ciaddr;			/* client IP address */
   // struct in_addr   bp_yiaddr;			/* 'your' IP address */
   // struct in_addr   bp_siaddr;			/* server IP address */
   // struct in_addr   bp_giaddr;			/* gateway IP address */
   ip_addr_t	     bp_ciaddr;			/* client IP address */
   ip_addr_t        bp_yiaddr;			/* 'your' IP address */
    ip_addr_t        bp_siaddr;			/* server IP address */
    ip_addr_t  	     bp_giaddr;			/* gateway IP address */
    unsigned char    bp_chaddr[BP_CHADDR_LEN];	/* client hardware address */
    char	     bp_sname[BP_SNAME_LEN];	/* server host name */
    char	     bp_file[BP_FILE_LEN];	/* boot file name */
    unsigned char    bp_vend[BP_VEND_LEN];	/* vendor-specific area */
    /* note that bp_vend can be longer, extending to end of packet. */
	}__attribute__((packed)) bootp_header_t;

#define _CLOSED      0
#define _LISTEN      1
#define _SYN_RCVD    2
#define _SYN_SENT    3
#define _ESTABLISHED 4
#define _CLOSE_WAIT  5
#define _LAST_ACK    6
#define _FIN_WAIT_1  7
#define _FIN_WAIT_2  8
#define _CLOSING     9
#define _TIME_WAIT  10
typedef struct _tcp_socket {
   // struct _tcp_socket *next;
    int		       state;       /* connection state */
    ip_route_t         his_addr;    /* address of other end of connection */
    ip_route_t         my_addr;
    word               our_port;
    word               his_port;
    word               data_bytes;   /* number of data bytes in pkt */
    char               reuse;        /* SO_REUSEADDR, no 2MSL */
   // timer_t            timer;
  //  pktbuf_t           pkt;         /* dedicated xmit packet */
  //  pktbuf_t           *rxlist;     /* list of unread incoming data packets */
   // char               *rxptr;      /* pointer to next byte to read */
    //int                rxcnt;       /* bytes left in current read packet */
    dword              ack;
    dword              seq;
    word		windowsize;
    word         flag;
    char 	err;
//    char               pktbuf[ETH_MAX_PKTLEN];
} tcp_socket_t;


#define strcmpi(d,s) lib_strcmpi(d,s)

int lib_strcmpi(const char *dest,const char *src);
char *gettoken(char **ptr);
char lib_toupper(char c);

void http_download_entry(void);
void tcp_send(tcp_socket_t *s, char* buf,int len,int flags, int resend);
void http_send_ack(tcp_socket_t *s);
void SendHttpDataAck(tcp_socket_t *s);
void print_packet( char * buf, int length );
void web_process(char * buf, int len,tcp_socket_t *s);
int  __http_enet_poll(unsigned char * pkt, unsigned short len);
void __arp_handler(unsigned char * pkt, int len);
 void __enet_send(char * buf,unsigned int len);
#endif // _HTTP_H_
