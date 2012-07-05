#ifndef _DHCP_AUTOIP__IMPL_H
#define _DHCP_AUTOIP__IMPL_H
#include <sys/types.h>
#ifdef __GLIBC__
#include <net/ethernet.h>
#else
#include <linux/if_ether.h>
#define ETHERTYPE_IP			0x0800
#define ETHERTYPE_ARP		0x0806
#endif
#include <paths.h>

#define AUTOIP_RETRY		10			/* Retry count for auto-ip */
#define NETWORK_ID		((169 << 8) + 254)	/* Link-Local Network ID. 169.254 */
#define START_ADDR		(1 << 8)		/* Valid start addr. host id part. 1.0 */
#define END_ADDR		((254 << 8) + 255)	/* Valid end addr. host id part. 254.255 */
#define MULTICAST_DEST 	"239.0.0.0"

#define AUTOIPD_CACHE_FILE	"/tmp/dhcpc/autoip-%s.cache"

//const int inaddr_broadcast = INADDR_BROADCAST;
#define BROADCAST_FLAG		0x8000
#define MAC_BCAST_ADDR		"\xff\xff\xff\xff\xff\xff"

//For notification
struct packed_ether_header {
  u_int8_t  ether_dhost[ETH_ALEN];      /* destination eth addr */
  u_int8_t  ether_shost[ETH_ALEN];      /* source ether addr    */
  u_int16_t ether_type;                 /* packet type ID field */
} __attribute__((packed));

typedef struct arpMessage
{
  struct packed_ether_header	ethhdr;
  u_short htype;		/* hardware type (must be ARPHRD_ETHER) */
  u_short ptype;		/* protocol type (must be ETHERTYPE_IP) */
  u_char  hlen;		/* hardware address length (must be 6) */
  u_char  plen;		/* protocol address length (must be 4) */
  u_short operation;		/* ARP opcode */
  u_char  sHaddr[ETH_ALEN];	/* sender's hardware address */
  u_char  sInaddr[4];	/* sender's IP address */
  u_char  tHaddr[ETH_ALEN];	/* target's hardware address */
  u_char  tInaddr[4];	/* target's IP address */
  u_char  pad[18];		/* pad for min. Ethernet payload (60 bytes) */
} __attribute__((packed)) arpMessage;


u_int32_t GetNewIP(void);
int ReadFromAutoIPCacheFile(u_int32_t* pnValue);
int Write2AutoIPCacheFile(u_int32_t nValue);
void CreateThreadToLookForIPCollision(u_int32_t IPAddress);
void * WatchForIPCollision(void *Param);
void DestroyThread();
int CreateSencondSocketForIPDefend();
int dhcpAutoConfigureIP();	 
int Select(int s, time_t tv_usec);
void RunScriptFiles(const char *Dir, const char * FileFormat, char * Argument);
int SendReplyForArpProbe(u_int32_t IPAddress, void *TargetDeviceMACAddress);
#endif
