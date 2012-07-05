#include <stdio.h>
#include <kdef.h>
#include <profile.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <conf_sec.h>
#include <string.h>
#include <asfparser.h>
#include <unistd.h>
#include <libsmbclient.h>
#include <sys/file.h>
#include <smb_bulk_ops.h>
#include <time.h>
#include <log_api.h>
#include <signal.h>
#include <stdlib.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
int gStopNow = 0;
int gSleepTime = 15;
#define SMBC_BUF_SIZE 2048
#define AV_HEADER_OBJECT_HEADER_SIZE sizeof(ASF_HEADER)+sizeof(ASF_FILE_PROPERTY)+sizeof(ASF_VIDEO_STREAM_PROPERTY)\
	+sizeof(ASF_AUDIO_STREAM_PROPERTY1)+sizeof(ASF_AUDIO_STREAM_PROPERTY2)+sizeof(ASF_AUDIO_STREAM_PROPERTY3)\
	+sizeof(ASF_HEADER_EXTENSION)

#define VIDEO_HEADER_OBJECT_HEADER_SIZE sizeof(ASF_HEADER)+sizeof(ASF_FILE_PROPERTY)+sizeof(ASF_VIDEO_STREAM_PROPERTY)\
	+sizeof(ASF_HEADER_EXTENSION)

#define DATA_OBJECT_HEADER_SIZE sizeof(ASF_DATA)
#define FIRST_PACKET_HEADER_SIZE sizeof(ASF_ERROR_CORRECTION)+sizeof(ASF_PAYLOAD_INFORMATION)+sizeof(struct PayloadFirstPacketHeader)
#define OTHER_PACKET_HEADER_SIZE sizeof(ASF_ERROR_CORRECTION)+sizeof(ASF_PAYLOAD_INFORMATION)+sizeof(struct PayloadOtherPacketHeader)
enum
{
	SMBC_STATE_HOH,
	SMBC_STATE_FPH,
	SMBC_STATE_OPH,
};
typedef struct {
	char ip[32];			// ip address for the log file
	unsigned short port;		// port for the log file
	char name[32];			// user name for the log file
	char agent[8];			// user agent for the MJPEG server push - "mozilla", "ocx" and "ie"
	char video_type[8];	// video type - "mjpeg", "mpeg" and "3gp"
	char resolution[8];	// resolution - "160x128", "320x240" and "640x480"
	char quality[16];		// quality - very low to very high - "1" to "5"
	int framerate;		// 1-30
	char x_session_cookie[32];	// for rtp/rtsp over http
	char post_data[1024];	// post data queue
	int post_data_len;	// post data length
} client_message;
struct smbc_asf_ds
{
	//for state machine
	int state_size;
	int state;
	int r_frame_size;

	//for seekable
	int hoh_size;
	int packet_count;
	int done_offset;
	int prev_done_offset;
	int PresentTime;
	int prev_PresentTime;

	//header
	char header_buf[AV_HEADER_OBJECT_HEADER_SIZE+DATA_OBJECT_HEADER_SIZE+1];
};
int pass_fd_and_message(int fd, client_message *p_msg) {
	int ret;
	int s=0;			/* socket */
	struct iovec iov[1];
	struct msghdr msg;
	struct cmsghdr *cmsgp = NULL;
	char buf[CMSG_SPACE (sizeof fd)];
	struct sockaddr_un a_srvr;	/* serv. adr */

	/* Clear message area */

	memset (&a_srvr, 0, sizeof a_srvr);
	memset (&msg, 0, sizeof (msg));
	memset (buf, 0, sizeof (buf));

	/* Create a Unix Socket */
	s = socket (PF_LOCAL, SOCK_STREAM, 0);
	if(s == -1)
	{
		fprintf(stderr, "create socket error\n");
		return -1;	/* Failed: check errno */
	}

	/* Set the socket to non-blocking to prevent blocking in the connect() function. */
	ret = fcntl(s, F_SETFL, O_NONBLOCK);
	if(ret == -1)
	{
		fprintf(stderr, "fcntl error, ret: %d, errno: (%d)%s\n", ret, errno, strerror(errno));
		goto quit;
	}

	/* Create the abstract address of the socket server */
	a_srvr.sun_family = AF_LOCAL;
	strncpy (a_srvr.sun_path, "./zSOCKET-SERVER",
		 sizeof a_srvr.sun_path - 1);
	a_srvr.sun_path[0] = 0;

	/* Connect to the sock server */
	ret = connect (s, (struct sockaddr *) &a_srvr, sizeof a_srvr);
	if(ret == -1)
	{
		fprintf(stderr, "connect failed: (%d) %s\n", errno, strerror(errno));
		goto quit;	/* Failed: check errno */
	}

	/* supply socket addr (if any) */
	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	/* init I/O vector to send the value in er. 
	 * This is done because data must be transmitted to send 
	 * the fd 
	 */
	iov[0].iov_base = p_msg;
	iov[0].iov_len = sizeof(client_message);
	msg.msg_iov = &iov[0];
	msg.msg_iovlen = 1;

	/* Establish control buffer */
	msg.msg_control = buf;
	msg.msg_controllen = sizeof buf;

	/* configure the message to send a fd */
	cmsgp = CMSG_FIRSTHDR (&msg);
	cmsgp->cmsg_level = SOL_SOCKET;
	cmsgp->cmsg_type = SCM_RIGHTS;
	cmsgp->cmsg_len = CMSG_LEN (sizeof fd);

	/* install the file descriptor value */
	*((int *) CMSG_DATA (cmsgp)) = fd;
	//msg.msg_controllen = cmsgp->cmsg_len;

	/* send it to client process */
	do {
		ret = sendmsg (s, &msg, 0);
	}
	while (ret == -1 && errno == EINTR);

quit:
	// close socket before exit 
	close (s);
	return (ret < 0) ? -1 : 0;
}
int get_asf_streaming_fd(void)
{
	int ret;
	int fd[2];
	client_message msg;

	//init values
	memset(&msg, 0, sizeof(client_message));
	fd[0]=-1;
	fd[1]=-1;

	//send fd to streaming server
	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);EQ_ERR(ret, -1);
	//ret = pipe(fd);EQ_ERR(ret, -1);
	strcpy(msg.video_type, "asf");
	strcpy(msg.ip, "samba recording");
	strcpy(msg.name, "admin");
	strcpy(msg.agent, "SMB");
	ret = pass_fd_and_message(fd[1], &msg);EQ_ERR(ret, -1);
	close(fd[1]);
	return fd[0];
errout:
	if(fd[0]!=-1)
		close(fd[0]);
	if(fd[1]!=-1)
		close(fd[1]);
	return -1;
}
char smb_username[SMB_USER_LEN+1];
char smb_password[SMB_PASSWD_LEN+1];
static void smb_auth_fn(const char *server, const char *share, char *workgroup, 
	int wgmaxlen, char *username, int unmaxlen, char *password, int pwmaxlen)
{
	
	memset(workgroup, 0, wgmaxlen);
	memset(username, 0, unmaxlen);
	memset(password, 0, pwmaxlen);
	strcpy(username, smb_username);
	strcpy(password, smb_password);
}
int get_smb_server_fd(char* url, char* username, char* password)
{
	int ret, fd;
	ret = smbc_init(smb_auth_fn, 0);NE_ERR(ret, 0);
	
	if(username[0]!='\0')
		strcpy(smb_username, username);
	else
		strcpy(smb_username, "dummy");
	strcpy(smb_password, password);

	fd = smbc_open(url, O_RDWR | O_CREAT | O_TRUNC, 0666);EQ_ERR(fd, -1);
	return fd;
errout:
        if(errno == 13)
                AddLog(SAMBA_LOGFILE_GROUP, 2, 4);//Permission denied.
        else if(errno == 2 || errno == 22 )
                AddLog(SAMBA_LOGFILE_GROUP, 2, 5);//No such remote path.
        else if(errno == 101)
                AddLog(SAMBA_LOGFILE_GROUP, 2, 3);//Unknown SAMBA server.
        else
                AddLog(SAMBA_LOGFILE_GROUP, 2, 2);//Failed to upload file.
	if(fd != -1)
		smbc_close(fd);
        gSleepTime = 60*10;
	return -1;
}
#define CHANGE_STATE(next_state, next_state_size, this_state_done_size) \
	asf_ds->state = next_state;\
	asf_ds->state_size = next_state_size;\
	done_size += this_state_done_size;\
	asf_ds->prev_done_offset = asf_ds->done_offset;\
	asf_ds->done_offset += this_state_done_size;\
	asf_ds->packet_count++;\

int process_state(char* buf, int buf_size, struct smbc_asf_ds* asf_ds)
{
	ASF_HEADER *hp = (ASF_HEADER*)buf;
	struct PayloadFirstPacketHeader *fpp = 
		(struct PayloadFirstPacketHeader *)(buf + sizeof(ASF_PAYLOAD_INFORMATION) + sizeof(ASF_ERROR_CORRECTION));
	struct PayloadOtherPacketHeader *opp = 
		(struct PayloadOtherPacketHeader *)(buf + sizeof(ASF_PAYLOAD_INFORMATION) + sizeof(ASF_ERROR_CORRECTION));
	ASF_PAYLOAD_INFORMATION *pip = 
		(ASF_PAYLOAD_INFORMATION *)(buf + sizeof(ASF_ERROR_CORRECTION));

	int done_size = 0;

	if(buf_size < asf_ds->state_size)
		return -1;
		
	switch(asf_ds->state)
	{
	case SMBC_STATE_HOH:
		if(hp->objectNum == 4)
			asf_ds->hoh_size = AV_HEADER_OBJECT_HEADER_SIZE;
		else
			asf_ds->hoh_size = VIDEO_HEADER_OBJECT_HEADER_SIZE;
		memcpy(asf_ds->header_buf, buf, asf_ds->hoh_size + DATA_OBJECT_HEADER_SIZE);
		CHANGE_STATE(SMBC_STATE_FPH, FIRST_PACKET_HEADER_SIZE, asf_ds->hoh_size+DATA_OBJECT_HEADER_SIZE);
		break;
	case SMBC_STATE_FPH:
		asf_ds->r_frame_size = fpp->FrameSize;
		asf_ds->r_frame_size -= fpp->Len;		
		asf_ds->prev_PresentTime = asf_ds->PresentTime;
		asf_ds->PresentTime = fpp->PresentTime;
		if(asf_ds->r_frame_size)
		{
			CHANGE_STATE(SMBC_STATE_OPH, OTHER_PACKET_HEADER_SIZE, fpp->Len+FIRST_PACKET_HEADER_SIZE);
		}
		else
		{
			CHANGE_STATE(SMBC_STATE_FPH, FIRST_PACKET_HEADER_SIZE, fpp->Len+FIRST_PACKET_HEADER_SIZE + pip->paddingLength);			
		}
		break;
	case SMBC_STATE_OPH:
		asf_ds->r_frame_size -= opp->Len;
		if(asf_ds->r_frame_size)
		{
			CHANGE_STATE(SMBC_STATE_OPH, OTHER_PACKET_HEADER_SIZE, opp->Len+OTHER_PACKET_HEADER_SIZE);
		}
		else
		{
			CHANGE_STATE(SMBC_STATE_FPH, FIRST_PACKET_HEADER_SIZE, opp->Len+OTHER_PACKET_HEADER_SIZE + pip->paddingLength);
		}
		break;
	default:
		EQ_ERR(0, 0);
	}
	return done_size;
errout:
	return -1;
}
int process_asf_state(char* buf, int buf_size, struct smbc_asf_ds* asf_ds)
{
	
	int done_size;
	int total_done_size = 0;
	while(buf_size>0)
	{
		done_size = process_state(buf, buf_size, asf_ds);
		if(done_size == -1)
			break;
		buf += done_size;
		buf_size -= done_size;
		total_done_size += done_size;
	}
	return total_done_size;
}
int skip_http_header(int ifd, char* buf)
{
	char* p;
	int ret, buf_size = -1;
	ret = read(ifd, buf, 128);EQ_ERR(ret, -1);
	buf[ret]='\0';
	p = strstr(buf, "\r\n\r\n");EQ_ERR(p, NULL);
	p+=strlen("\r\n\r\n");
	buf_size = ret - (p - buf);
	memmove(buf, p, buf_size);
errout:
	return buf_size;
}
int make_it_seekable(int ofd, struct smbc_asf_ds* ds)
{
	ASF_SAMPLE_INDEX_OBJECT asfSampleIndexObject = 
	{{ 0x90, 0x08, 0x00, 0x33, 0xB1, 0xE5, 0xCF, 0x11, 0x89, 0xF4, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xCB },
		sizeof(ASF_SAMPLE_INDEX_OBJECT), 
		{ 0x01, 0xC0, 0xBA, 0xBA, 0x72, 0x2B, 0x07, 0xA9, 0xA8, 0xF9, 0x74, 0x23, 0xE9, 0x00, 0x6C, 0xD9 },
		10000000, 0, 0
	};
	ASF_FILE_PROPERTY* fp = (ASF_FILE_PROPERTY*)(ds->header_buf + sizeof(ASF_HEADER));
	ASF_DATA* dp = (ASF_DATA*)(ds->header_buf + ds->hoh_size);

	fp->broadcast = 0;
	fp->seekable = 1;
	fp->fileLen = ds->prev_done_offset;
	fp->playDuration = ds->prev_PresentTime * 10000;
	fp->playDuration2 = 0;
	fp->sendDuration = ds->prev_PresentTime * 10000;
	fp->sendDuration2 = 0;
	fp->dataPacketCount = ds->packet_count - 1;
	dp->objectLen = fp->fileLen - ds->hoh_size;
	dp->dummy = 0;
	dp->totalDataPackets = ds->packet_count - 1;

	smbc_lseek(ofd, 0, SEEK_SET);
	smbc_write(ofd, ds->header_buf, ds->hoh_size + sizeof(ASF_DATA));

	smbc_lseek(ofd, fp->fileLen, SEEK_SET);
	smbc_write(ofd, &asfSampleIndexObject, sizeof(ASF_SAMPLE_INDEX_OBJECT));
	
	
	return 0;
}
int send_asf(int ifd, int ofd, int asf_max_size)
{
	char buf[SMBC_BUF_SIZE+1];
	int ret;
	int buf_size =0;//data size on buf
	int done_size = 0;//processed data size
	struct smbc_asf_ds asf_ds;
        int errcode = -1;
	
	
	memset(&asf_ds, 0, sizeof(struct smbc_asf_ds));
	buf_size = skip_http_header(ifd, buf);EQ_ERR(buf_size, -1);
	while(asf_max_size > 0 && gStopNow == 0)
	{
		//get asf content
		ret = read(ifd, buf + buf_size, 
			asf_max_size>SMBC_BUF_SIZE?SMBC_BUF_SIZE-buf_size:
			asf_max_size-buf_size);EQ_ERR(ret, -1);
		EXP_ERR((ret == -1 && ret != EAGAIN) || ret == 0);
		buf_size+=ret;
		
		//process it
		if(buf_size > done_size)
			done_size += process_asf_state(buf + done_size, buf_size - done_size, &asf_ds);
		
		//send it out
		ret = smbc_write(ofd, buf, done_size>buf_size?buf_size:done_size);
		EXP_ERR((ret == -1 && ret != EAGAIN) || ret == 0);
		buf_size -= ret;
		done_size -= ret;
		asf_max_size -= ret;

		//move remaining
		if(buf_size)
			memmove(buf, buf + ret, buf_size);
	}
        errcode = 0;
errout:

	ret = make_it_seekable(ofd, &asf_ds);
        return errcode;
}
int lock_smb_fd(char* filename)
{
	int fd = -1, i;
	fd = open(filename, O_CREAT | O_RDWR, 0666);EQ_ERR(fd, -1);
	for(i=0;i<20;i++)
	{
		if( flock(fd, LOCK_EX | LOCK_NB) == 0)
			break;
		sleep(1);
	}
	if(i==20)
	{
		close(fd);
		fd = -1;
	}
errout:
	if(fd != -1)
		close(fd);
	return fd;
}
static int LAN_read_mac(unsigned char* eth_addr)
{
	
	int sockfd=-1, ret, errcode = -1, i;
	struct ifreq ifr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);EQ_ERR(sockfd, -1);

	strcpy(ifr.ifr_name, "eth0");
	ioctl(sockfd, SIOCGIFHWADDR, &ifr);EQ_ERR(ret, -1);
	for(i=0;i<6;i++)
		eth_addr[i] =(unsigned char)ifr.ifr_hwaddr.sa_data[i];
	errcode = 0;
errout:
	if(sockfd != -1)
		close(sockfd);

	return errcode;
}	
int rename_filename(char* tmp_filename, char* target_filename)
{
	int errcode = -1, ret;
	ret = smbc_rename(tmp_filename, target_filename);EQ_ERR(ret, -1);	
	errcode = 0;
errout:
        return errcode;
}
int generate_filename(char* tmp_filename, char* target_filename, char* mac_addr, struct SMB_DS *ds)
{
	time_t time_now = time(NULL);
	struct tm * t_s	= localtime(&time_now);
	char time[32], hostname[32];
	int i = 0;
	sprintf(time, "%02d%02d%04d-%02d%02d%02d",  
		t_s->tm_mon+1, t_s->tm_mday, t_s->tm_year+1900,
		t_s->tm_hour, t_s->tm_min, t_s->tm_sec);
	gethostname(hostname, 32);

	for(i=0;i<strlen(ds->rec_path);i++)
	{
		if(ds->rec_path[i]=='\\')
			ds->rec_path[i] = '/';
	}
	sprintf(tmp_filename, "smb://%s/%s/~%s-%s.asx", ds->rec_server, 
		ds->rec_path, hostname, time);
	if(ds->rec_mode == 1)
	{
		sprintf(target_filename, "smb://%s/%s/%s-%s.asx",
			ds->rec_server, ds->rec_path, hostname, time);
	}
	else
	{
		sprintf(target_filename, "smb://%s/%s/%s-SambaRecord.asx", 
			ds->rec_server, ds->rec_path, hostname);
        		
	}
	return 0;
}

int record_asf(char* url, char* username, char* password, int size)
{
	int ret = -1, ifd = -1, ofd = -1, errcode = -1;
	
	ifd = get_asf_streaming_fd();EQ_ERR(ifd, -1);
	ofd = get_smb_server_fd(url, username, password);EQ_ERR(ofd, -1);
	ret = send_asf(ifd, ofd, size);EQ_ERR(ret, -1);
	errcode = 0;
errout:
	if(ifd!=-1)
		close(ifd);
	if(ofd!=-1)
		smbc_close(ofd);
	return errcode;	
}
static void HandleSigHup(int sigraised)
{
	gStopNow = 1;
}
int main(int argc, char* argv[])
{
	int ret = -1, lock_fd = -1;
	struct SMB_DS smb_ds;
	char mac[32];
	char tmp_url[512+1], target_url[512+1];


	signal(SIGHUP,  HandleSigHup);
	lock_fd = lock_smb_fd("/var/lock/smbc_rec.lock");EQ_ERR(lock_fd, -1);
	ret = SMB_BULK_ReadDS(&smb_ds);EQ_ERR(ret, -1);
	ret = LAN_read_mac(mac);EQ_ERR(ret, -1);
	while(smb_ds.rec_enable && gStopNow == 0)
	{
                gSleepTime = 10;
		ret = generate_filename(tmp_url, target_url, mac, &smb_ds);EQ_ERR(ret, -1);
		ret = record_asf(tmp_url, smb_ds.rec_user, smb_ds.rec_passwd, smb_ds.rec_filesize*1024);
		if(ret == 0)
		        rename_filename(tmp_url, target_url);
                else
                        sleep(gSleepTime);
	}
errout:
	if(lock_fd != -1)
		close(lock_fd);
	return 0;
}

