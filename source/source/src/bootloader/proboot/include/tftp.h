#ifndef __TFTP_H__
#define __TFTP_H__

#define TFTP_RRQ	1	// read request
#define TFTP_WRQ	2	// write request
#define TFTP_DATA	3	// data packet
#define TFTP_ACK	4	// acknowledgement
#define TFTP_ERROR	5	// error code

struct tftp_req
{
	unsigned short	opcode;
	char		stuff[64];
};

struct tftphdr
{
	unsigned short	opcode;
	unsigned short	block;
};

#define TFTP_HLEN sizeof (struct tftphdr)


#endif // __TFTP_H__
