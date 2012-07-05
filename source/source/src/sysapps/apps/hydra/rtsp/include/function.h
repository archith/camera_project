#ifndef	FUNCTION_H
#define FUNCTION_H


/* network.c */

int net_set_ttl (int sock, int ttl);

int net_set_loop (int sock, int loop);

int net_create_sk (void);

int net_get_ip (char *ifname, char *ip);

/* stat.c */

int update_session (int cid, int bytecount);

void update_session_seq (int cid);

void update_session_time (int cid);

void update_session_byte (int cid, int bytecount);

/* util.c */

unsigned long random32 (unsigned int seed);

void dbg (int level, char *fmt, ...);
#define DBGALL		0
#define DBGALL		0
#define DBGINFO		1
#define DBGWARN		2
#define DBGERR		3
#define DBGINFO		1
#define DBGWARN		2
#define DBGERR		3

int b64_decode (const char *str, unsigned char *space, int size);

/* sdp.c */
size_t sdp_generate (char *response, int index);

/* response.c */

int rtsp_sk_pass (int index);


/* pv_spec.c */

int pv_listen (int port);

int pv_reply (int fd);

#endif
