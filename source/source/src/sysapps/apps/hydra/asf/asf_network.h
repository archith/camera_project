#ifndef ASF_NETWORK_H
#define ASF_NETWORK_H

#define NWL_IOVECTOR		576
#define NWL_PACKET_SIZE		1000
#define NWL_BUFFER_SIZE 	2048
#define MAX_WAIT_TIME		10
#define MAX_SC_PADDING_BUF_SIZE 32

int ASFProcessInit(void);
int ASFProcessConnection(int connidx);
int ASFProcessTearDown(int connidx);
#endif

