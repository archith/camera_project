#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>

#include <linux/sockios.h>

#define	ZDAPIOCTL	SIOCDEVPRIVATE

struct zdap_ioctl {
	unsigned short cmd;                /* Command to run */
	unsigned long addr;                /* Length of the data buffer */
	unsigned long value;              /* Pointer to the data buffer */
	unsigned char data[0x100];
};


#define ZD_IOCTL_REG_READ	0x01
#define ZD_IOCTL_REG_WRITE	0x02
#define ZD_IOCTL_MEM_DUMP	0x03
#define ZD_IOCTL_REGS_DUMP	0x04
#define ZD_IOCTL_DBG_MESSAGE 0x05
#define ZD_IOCTL_DBG_COUNTER 0x06
#define ZD_IOCTL_LB_MODE    0x07
#define ZD_IOCTL_TX_ONE     0x08
#define ZD_IOCTL_TX_ON      0x09
#define ZD_IOCTL_WEP        0x0A
#define ZD_IOCTL_TX         0x0B
#define ZD_IOCTL_TX1        0x0C
#define ZD_IOCTL_RATE       0x0D
#define ZD_IOCTL_PORT       0x0E
#define ZD_IOCTL_SNIFFER    0x0F
#define ZD_IOCTL_WEP_ON_OFF 0x10
#define ZD_IOCTL_CAM        0x11
#define ZD_IOCTL_VAP        0x12
#define ZD_IOCTL_APC        0x13
#define ZD_IOCTL_CAM_DUMP   0x14
#define ZD_IOCTL_CAM_CLEAR  0x15
#define ZD_IOCTL_WDS        0x16
#define ZD_IOCTL_FIFO       0x17
#define ZD_IOCTL_FRAG       0x18
#define ZD_IOCTL_RTS        0x19
#define ZD_IOCTL_PREAMBLE   0x1A
#define ZD_IOCTL_DUMP_PHY   0x1B
#define ZD_IOCTL_DBG_PORT   0x1C
#define ZD_IOCTL_CARD_SETTING 0x1D
#define ZD_IOCTL_RESET      0x1F
#define ZD_IOCTL_SW_CIPHER	0x20
#define ZD_IOCTL_HASH_DUMP	0x21



char *prgname;

int set_ioctl(int sock, struct ifreq *req)
{
	 if (ioctl(sock, ZDAPIOCTL, req) < 0) {
        fprintf(stderr, "%s: ioctl(SIOCGIFMAP): %s\n",
                prgname, strerror(errno));
        return -1;
    }
	
	return 0;
}


int read_reg(int sock, struct ifreq *req)
{
	struct zdap_ioctl *zdreq = 0;
	
	if (!set_ioctl(sock, req))
		return -1;

    //zdreq = (struct zdap_ioctl *)req->ifr_data;
    //printf( "reg = %4x, value = %4x\n", zdreq->addr, zdreq->value);

    return 0;
}


int read_mem(int sock, struct ifreq *req)
{
	struct zdap_ioctl *zdreq = 0;
	int i;
	
	if (!set_ioctl(sock, req))
		return -1;

    /*zdreq = (struct zdap_ioctl *)req->ifr_data;
	printf( "dump mem from %x, length = %x\n", zdreq->addr, zdreq->value);
	for (i=0; i<zdreq->value; i++){
		printf("%02x", zdreq->data[i]);
		printf(" ");
		if ((i>0) && ((i+1)%16 == 0))
			printf("\n");
	}*/

    return 0;
}


int main(int argc, char **argv)
{
    int sock;
    int addr, value;
    struct ifreq req;
    char *action = NULL;
    struct zdap_ioctl zdreq;
    
    prgname = argv[0];

    if (argc < 5) {
        fprintf(stderr,"%s: usage is \"%s <ifname> [<operation>] [<address>(0)] [<value>(0)]\"\n",
                prgname, prgname);
        fprintf(stderr,"valid operation: read, write, mem, regs, dbg, cnt, lb, txon, phy, port\n");
        fprintf(stderr,"wep, cam, vap, apc, camdump, camclear, wds, fifo, frag, rts, pre, card\n");
                    
        exit(1);
    }
    strcpy(req.ifr_name, argv[1]);
    zdreq.addr = 0;
	zdreq.value = 0;

    /* a silly raw socket just for ioctl()ling it */
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sock < 0) {
        fprintf(stderr, "%s: socket(): %s\n", argv[0], strerror(errno));
        exit(1);
    }

    sscanf(argv[3], "%x", &addr);
	sscanf(argv[4], "%x", &value);
    zdreq.addr = addr;
	zdreq.value = value;
    
	if (!strcmp(argv[2], "read")){
		zdreq.cmd = ZD_IOCTL_REG_READ;
		memcpy(req.ifr_data, (char *)&zdreq, sizeof(zdreq));
		read_reg(sock, &req);
	}
    else if (!strcmp(argv[2], "mem")){
		zdreq.cmd = ZD_IOCTL_MEM_DUMP;
		memcpy(req.ifr_data, (char *)&zdreq, sizeof(zdreq));
		read_mem(sock, &req);
	}		
	else if (!strcmp(argv[2], "write")){
		zdreq.cmd = ZD_IOCTL_REG_WRITE;
		goto just_set;
	}	
	else if (!strcmp(argv[2], "regs")){
		zdreq.cmd = ZD_IOCTL_REGS_DUMP;
		goto just_set;
	}
    else if (!strcmp(argv[2], "dbg")){
        zdreq.cmd = ZD_IOCTL_DBG_MESSAGE;
		goto just_set;
    }
    else if (!strcmp(argv[2], "cnt")){
		zdreq.cmd = ZD_IOCTL_DBG_COUNTER;
		goto just_set;
	}
    else if (!strcmp(argv[2], "lb")){
        zdreq.cmd = ZD_IOCTL_LB_MODE;
		goto just_set;
    }
    else if (!strcmp(argv[2], "txon")){
        zdreq.cmd = ZD_IOCTL_TX_ON;
		goto just_set;
    }
    else if (!strcmp(argv[2], "sniffer")){
        zdreq.cmd = ZD_IOCTL_SNIFFER;
		goto just_set;
    }
    else if (!strcmp(argv[2], "wep")){
        zdreq.cmd = ZD_IOCTL_WEP_ON_OFF;
		goto just_set;
    }
    else if (!strcmp(argv[2], "cam")){
        zdreq.cmd = ZD_IOCTL_CAM;
		goto just_set;
    }
    else if (!strcmp(argv[2], "vap")){
        zdreq.cmd = ZD_IOCTL_VAP;
		goto just_set;
    }
    else if (!strcmp(argv[2], "apc")){
        zdreq.cmd = ZD_IOCTL_APC;
		goto just_set;
    }
    else if (!strcmp(argv[2], "camdump")){
        zdreq.cmd = ZD_IOCTL_CAM_DUMP;
		goto just_set;
    }
    else if (!strcmp(argv[2], "camclear")){
        zdreq.cmd = ZD_IOCTL_CAM_CLEAR;
		goto just_set;
    }
#if 0    
    else if (!strcmp(argv[2], "wds")){
        zdreq.cmd = ZD_IOCTL_WDS;
		goto just_set;
    }
#endif
    else if (!strcmp(argv[2], "fifo")){
        zdreq.cmd = ZD_IOCTL_FIFO;
		goto just_set;
    }
    else if (!strcmp(argv[2], "frag")){
        zdreq.cmd = ZD_IOCTL_FRAG;
		goto just_set;
    }
    else if (!strcmp(argv[2], "rts")){
        zdreq.cmd = ZD_IOCTL_RTS;
		goto just_set;
    }
    else if (!strcmp(argv[2], "pre")){
        zdreq.cmd = ZD_IOCTL_PREAMBLE;
		goto just_set;
    }
    else if (!strcmp(argv[2], "phy")){
        zdreq.cmd = ZD_IOCTL_DUMP_PHY;
		goto just_set;
    }
    else if (!strcmp(argv[2], "port")){
        zdreq.cmd = ZD_IOCTL_DBG_PORT;
		goto just_set;
    }
    else if (!strcmp(argv[2], "card")){
        zdreq.cmd = ZD_IOCTL_CARD_SETTING;
		goto just_set;
    }
    else if (!strcmp(argv[2], "reset")){
        zdreq.cmd = ZD_IOCTL_RESET;
		goto just_set;
    }
    else if (!strcmp(argv[2], "sc")){
        zdreq.cmd = ZD_IOCTL_SW_CIPHER;
		goto just_set;
    }
    else if (!strcmp(argv[2], "hash")){
        zdreq.cmd = ZD_IOCTL_HASH_DUMP;
		goto just_set;
    }
	else{
		fprintf(stderr, "error action\n");
        exit(1);
	}

just_set:
    req.ifr_data = (char *)&zdreq;
    set_ioctl(sock, &req);
      
    exit(0);
}
