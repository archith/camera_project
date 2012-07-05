/* 
 * rtp.h  -- RTP header file ( RFC 1889 )
 */

#ifndef	RTP_H
#define RTP_H
#include <sys/socket.h>

/*
 * Structure Declaration
 */

#ifndef PACKED
#    define     PACKED  __attribute__((packed))
#endif  /* PACKED */

// RTP data header 
typedef struct 
{
	unsigned short	cc:4,	/* CSRC count			*/
			extension:1,	/* header extension flag	*/
			padding:1,	/* padding flag			*/
			version:2,	/* RTP version			*/
			pt:7,	/* payload type			*/
			marker:1;	/* marker bit			*/
	unsigned short	seq_no;		/* sequence number, in network byte order */
	unsigned int 	rtp_ts;		/* timestamp,in network byte order*/
	unsigned int	ssrc;		/* synchronization source, in network byte order */
/*	unsigned int	csrc[1];	   optional CSRC list		*/
}rtp_hdr_t;

/* RTCP source description packet data */
typedef struct rtcp_sdes
{
	unsigned short rc:5,
		     padding:1,
		     version:2,
		     pt:8;
	unsigned short length;
	unsigned int ssrc;
	unsigned short type:8,
		       textlen:8;
	char text[16];	//user@didadi.com
}rtcp_sdes_t;

struct rtp_jpeg_header
{
        unsigned long type_specific : 8, fragment_offset: 24;
        unsigned char type;
        unsigned char q;
        unsigned char width;
        unsigned char height;
};

// RTCP data packet
typedef struct 
{
	// header part
	unsigned short rc:5,
		       padding:1,
		       version:2,
		       pt:8;
	unsigned short length;
	unsigned int ssrc;

	// sender info
	unsigned int ntp_msw;
	unsigned int ntp_lsw;
	unsigned int rtp_ts;
	unsigned int send_packet;
	unsigned int send_octet;

	rtcp_sdes_t sdes;
	
}rtcp_hdr_t;

/*
 * Extenstion headers provide for experiments that require more header
 * information that that provided by the fixed RTP header. However, we 
 * will not need it to implement our product 
 */

typedef struct rtp_hdr_ext
{
	unsigned short	ext_type;
	unsigned short	ext_len;
	unsigned int	*ext_table;
}rtp_hdr_ext_t;
	

// RTP packet structure	
typedef struct
{
	rtp_hdr_t	*header		PACKED;
	size_t		hdr_len;	
	rtp_hdr_ext_t	*extension	PACKED;
	size_t		ext_len;
	unsigned char	*payload	PACKED;
	size_t		pay_len;
}rtp_pkt_t;

typedef struct
{
	/* RTP connection */
	rtp_hdr_t	hdr;		/* rtp header data structure */
	struct rtp_jpeg_header rtp_jpeg_hdr;
	unsigned int	rtsp_ready;
	unsigned int	rtspidx;	/* rtsp connection index */
	unsigned short init_seq_no;	/* sequence number of the initialization */
	unsigned long init_rtp_ts;	/* timestamp of the initialization      */
	unsigned long time_elapsed;	/* rtp_ts - init_rtp_ts         */

	char ebd[4];
	char byVOLHeader[64];
	
	/* RTCP connection fds*/
	int rtcp_fd;
	unsigned int send_octet;
	unsigned long time_get_rtcp_packet;

	struct msghdr pkt;
	int towrite;
	int sent;
	int tosend;
} rtp_conn_data;


#define	JAN_1970	0x83aa7e80	/* 2208988800 1970 - 1900 in seconds */

/*
 * Function Declaration
 */

//


#define E_LAN_TOO_BUSY            -0x02
#define E_UNDEFINE_ERROR          -0x01
#define R_NO_DATA_TO_SEND 	   0x01
#define R_SEND_DATA_FINISH	   0x02


//Return 0, success else failed.
int rtp_send (int rtpidx);
int rtp_setup(int rtpidx, int rtspidx);
int rtp_teardownEx(int rtpidx);
int rtp_teardown(int rtpidx);
int rtp_play(int rtspidx);
int rtp_pause(int rtspidx);
int rtp_recv(int rtpidx);

int rtcp_sr(int rtpidx);
int rtcp_recv(int rtpidx);
int rtcp_listen(int rtpidx, int rtspidx);
#endif
