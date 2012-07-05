/* ============================================================================
 * Copyright (C) 1998-2001 Angus Mackay. All rights reserved; 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * ============================================================================
 */

/*
 * ez-ipupdate
 *
 * a very simple dynDNS client for the ez-ip dynamic dns service 
 * (http://www.ez-ip.net).
 *
 * why this program when something like:
 *   curl -u user:pass http://www.ez-ip.net/members/update/?key=val&...
 * would do the trick? because there are nicer clients for other OSes and
 * I don't like to see UNIX get the short end of the stick.
 *
 * tested under Linux and Solaris.
 * 
 */
 
/*
 *  1.Filter un-necessary code out
 *  2.Directly read configuration from system.conf
 *  3.Add check external ip from http://checkip.dyndns.org
 *  Sand Tu<sand_tu@sercomm.com>
 *  2002/09/23 16:35 
 */

#include <stdio.h>
#include <conf_sec.h>
#include <SCHD_functions.h>
#include <ddns_bulk_ops.h>
#include <log_bulk_ops.h>
#include <time.h>
//#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <asm/sockios.h>
#ifdef _GNUDIP_DDNS_
#include "md5.h"
#define MD5_DIGEST_BYTES (16)
#endif


#define DEBUG 1
//#undef DEBUG
#define DDNS_NO_DDNS		0
#define DDNS_DYNDNS		1
#define DDNS_TZO		2
#define DDNS_DYNDNS_QDNS3322	3
#define DDNS_GNUDIP_MYDYNIP	4
#define DDNS_ATDDNS		5
#define DDNS_DP21		6

#define SYSLOG_SECTION	"DDNS"
#define VERSION "3.0.11b7"
#define OS "i686-pc-linux-gnu"
#define DEF_SERVICE "dyndns"

/*DYNDNS*/
#define DYNDNS_DEFAULT_SERVER "members.dyndns.org"
#define DYNDNS_DEFAULT_PORT "80"
#define DYNDNS_REQUEST "/nic/update"

/*CHANGEIP*/
#define CHANGEIP_DEFAULT_SERVER "www.changeip.com"
#define CHANGEIP_DEFAULT_PORT "80"
#define CHANGEIP_REQUEST "/nic/update"

/*DP-21 IvyNetwork*/
#define DP21_DEFAULT_SERVER "dp-21.net" //dns.pd-21.net
#define DP21_DEFAULT_PORT "80"
#define DP21_REQUEST "/DynDNS/dyndns.cgi"	

/*ATDDNS @NET HOME*/
#define ATDDNS_DEFAULT_SERVER "atddns.hs.home.ne.jp"
#define ATDDNS_DEFAULT_PORT "80"
#define ATDDNS_REQUEST "/DynDNS/dyndns.cgi"

/*TZO*/
#define TZO_DEFAULT_SERVER "cgi.tzo.com"
#define TZO_DEFAULT_PORT "80"
#define TZO_REQUEST "/webclient/signedon.html"

/*DHS*/
#define DHS_DEFAULT_SERVER "members.dhs.org"
#define DHS_DEFAULT_PORT "80"
#define DHS_REQUEST "/nic/hosts"
#define DHS_SUCKY_TIMEOUT 60

/* Yi (GNUDIP-TCP Protocol) */
#define YI_DEFAULT_SERVER "gnudip2.yi.org"
#define YI_DEFAULT_PORT "3495"
#define YI_REQUEST "0"

/* MyDynIp (GNUDIP-HTTP Proocol) */
#define MYDYNIP_DEFAULT_SERVER "www.mydynip.org"
#define MYDYNIP_DEFAULT_PORT "80"
#define MYDYNIP_REQUEST "/update/doupdate"

/* DynDSL (GNUDIP-HTTP Protocol)*/
#define DYNDSL_DEFAULT_SERVER "update.dyndsl.com"
#define DYNDSL_DEFAULT_PORT "80"
#define DYNDSL_REQUEST "/gnudip/cgi-bin/gdipupdt.cgi"

/* 3322.org (GYNDNS Protocol) */
#define QDNS_DEFAULT_SERVER "members.3322.org"
#define QDNS_DEFAULT_PORT "80"
#define QDNS_REQUEST "/dyndns/update"

#define DEFAULT_TIMEOUT 120
#define BUFFER_SIZE (5*1024-1) //reduce buffer size to about 1k

#define  snprintf(x, y, z...) sprintf(x, ## z)
#define ARRAY_LEN(x) (sizeof(x)/sizeof(x[0]))

// the max time we will wait if the server tells us to
#define MAX_WAITRESPONSE_WAIT (24*3600)
/**************************************************/

struct service_t
{
  char *title;
  int (*update_entry)(void);
  char *default_server;
  char *default_port;
  char *default_request;
  int  default_max_interval;
};

enum {
  UPDATERES_CONNECT_FAIL = 0,
  UPDATERES_REG_OK,
  UPDATERES_REG_FAIL,
  UPDATERES_UPDATE_SKIP,
  UPDATERES_UPDATE_FAIL,
  UPDATERES_UPDATE_OK,
};

#if 0  /* get it from cgi library */
/* service objects for  sololy dyndns services */
enum {
        NO_DDNS,
        DYNDNS,
        TZO,
        DHS,
        CHANGEIP,
        DP21,
        ATDDNS,
        GNUDIP_YI,
        GNUDIP_MYDYNIP,
        GNUDIP_DYNDSL,
        DYNDNS_QDNS3322
};
#endif

/**************************************************/
#if DEBUG
static char logmsg[128];
#define TIME_LEN        20
static void log_data(char *s)
{
   FILE *fh;
   char dt[TIME_LEN];
   time_t t;
   struct tm *ptm;
   time(&t);
   ptm = localtime(&t);
   strftime(dt,TIME_LEN,"%m/%d/%Y %T",ptm);
   fh=fopen("/tmp/ez-ipupdate.log","a+");
   if(fh){
        fprintf(fh,"%s",s);
        fclose(fh); return;
   }
   else
        printf("Open file /tmp/ez-ipupdate.log error\n");
}
#endif
/**************************************************/
char ip[20];
char srv[GENERIC_STRING_LEN+1];
char user_pwd[256];
char auth[512];
char user_name[128];
char password[128];
char address[20];
int date;  // to record the last update time and date
char *ip_address = &ip[0];
char *server = &srv[0];
char *port = NULL;
char *request = NULL;
int wildcard = 0;
char host[40];
struct timeval timeout;
static volatile int client_sockfd;

/* added for DHS */
char *mx = NULL;
char *url = NULL;
char *cloak_title = NULL;

int DYNDNS_update_entry(void);
int CHANGEIP_update_entry(void);
int DP21_update_entry(void);
int ATDDNS_update_entry(void);
int TZO_update_entry(void);
int DHS_update_entry(void);
#ifdef _GNUDIP_DDNS_
int GNUDIP_TCP_update_entry(void);
int GNUDIP_HTTP_update_entry(void);
#endif

static struct service_t service1 = { 
	"dyndns",
	DYNDNS_update_entry,
	DYNDNS_DEFAULT_SERVER,
	DYNDNS_DEFAULT_PORT,
	DYNDNS_REQUEST,
	(25*24*3600)
};
static struct service_t service2 = { 
	"tzo",
	TZO_update_entry,
	TZO_DEFAULT_SERVER,
	TZO_DEFAULT_PORT,
	TZO_REQUEST,
	(25*24*3600)
};

static struct service_t service3 = { 
	"dhs",
	DHS_update_entry,
	DHS_DEFAULT_SERVER,
	DHS_DEFAULT_PORT,
	DHS_REQUEST,
	(25*24*3600)
};

static struct service_t service4 = { 
	"changeip",
	CHANGEIP_update_entry,
	CHANGEIP_DEFAULT_SERVER,
	CHANGEIP_DEFAULT_PORT,
	CHANGEIP_REQUEST,
	(25*24*3600)
};
static struct service_t service5 = { 
	"dp21-IvyNetwork",
	DP21_update_entry,
	DP21_DEFAULT_SERVER,
	DP21_DEFAULT_PORT,
	DP21_REQUEST,
	(25*24*3600)
};
static struct service_t service6 = { 
	"atddns-@NetHome",
	ATDDNS_update_entry,
	ATDDNS_DEFAULT_SERVER,
	ATDDNS_DEFAULT_PORT,
	ATDDNS_REQUEST,
	(25*24*3600)
};
#ifdef _GNUDIP_DDNS_
static struct service_t service7 = { 
	"gnudip",
	GNUDIP_TCP_update_entry,
	YI_DEFAULT_SERVER,
	YI_DEFAULT_PORT,
	YI_REQUEST,
	(25*24*3600)
};
static struct service_t service8 = { 
	"gnudip",
	GNUDIP_HTTP_update_entry,
	MYDYNIP_DEFAULT_SERVER,
	MYDYNIP_DEFAULT_PORT,
	MYDYNIP_REQUEST,
	(25*24*3600)
};
static struct service_t service9 = { 
	"gnudip",
	GNUDIP_HTTP_update_entry,
	DYNDSL_DEFAULT_SERVER,
	DYNDSL_DEFAULT_PORT,
	DYNDSL_REQUEST,
	(25*24*3600)
};
#endif

static struct service_t service10 ={
	"qdns",
	DYNDNS_update_entry,
	QDNS_DEFAULT_SERVER,
	QDNS_DEFAULT_PORT,
	QDNS_REQUEST,
	(25*24*3600)
};

static struct service_t *service = NULL;
static char provider = NULL;

/************************************************/
static void do_log(int retval)
{
    if( retval == UPDATERES_CONNECT_FAIL)
        AddLog(SYSLOG_SECTION, SYSLOG_ERROR, 1);
    else if( retval == UPDATERES_UPDATE_FAIL)
        AddLog(SYSLOG_SECTION, SYSLOG_ERROR, 2);
    else if( retval == UPDATERES_REG_OK)
        AddLog(SYSLOG_SECTION, SYSLOG_NORMAL, 3);
    else if( retval == UPDATERES_REG_FAIL)
#ifndef _SC_MOTO_LOG_SPEC_
        AddLog(SYSLOG_SECTION, SYSLOG_ERROR, 4);
#else
        AddLog(SYSLOG_SECTION, SYSLOG_ERROR, 7);	// update fail incorrect user name / password
#endif
    else if( retval == UPDATERES_UPDATE_SKIP)
        AddLog(SYSLOG_SECTION, SYSLOG_ERROR, 5);
    else if( retval == UPDATERES_UPDATE_OK)
#ifndef _SC_MOTO_LOG_SPEC_
        AddLog(SYSLOG_SECTION, SYSLOG_NORMAL, 6);
#else
	AddLog(SYSLOG_SECTION, SYSLOG_NORMAL, 6, host, ip_address);
#endif
}
/**************************************************/
int do_connect(int *sock, char *host, char *port);
void base64Encode(char *intext, char *output);
#ifdef  _SC_DDNS_MUST_UPDATE_AFTER_CGI_CHG_
int main(int argc, char *argv[]);
#else
int main(void);
#endif
/**************************************************/

/*
 * do_connect
 * connect a socket and return the file descriptor
 */
int do_connect(int *sock, char *host, char *port)
{
  struct sockaddr_in address;
  int len;
  int result;
  struct hostent *hostinfo;
  struct servent *servinfo;
  
  // set up the socket
  if((*sock=socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    return(-1);
  }
  address.sin_family = AF_INET;

  // get the host address
  hostinfo = gethostbyname(host);
  if(!hostinfo)
  {
    close(*sock);
    return(-1);
  }
  address.sin_addr = *(struct in_addr *)*hostinfo -> h_addr_list;

  // get the host port
  servinfo = getservbyname(port, "tcp");
  if(servinfo)
  {
    address.sin_port = servinfo -> s_port;
  }
  else
  {
    address.sin_port = htons(atoi(port));
  }

  // connect the socket
  len = sizeof(address);
  if((result=connect(*sock, (struct sockaddr *)&address, len)) == -1) 
  {
    close(*sock);
    return(-1);
  }

  return 0;
}

static char table64[]=
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
void base64Encode(char *intext, char *output)
{
  unsigned char ibuf[3];
  unsigned char obuf[4];
  int i;
  int inputparts;

  while(*intext) {
    for (i = inputparts = 0; i < 3; i++) { 
      if(*intext) {
        inputparts++;
        ibuf[i] = *intext;
        intext++;
      }
      else
        ibuf[i] = 0;
    }

    obuf [0] = (ibuf [0] & 0xFC) >> 2;
    obuf [1] = ((ibuf [0] & 0x03) << 4) | ((ibuf [1] & 0xF0) >> 4);
    obuf [2] = ((ibuf [1] & 0x0F) << 2) | ((ibuf [2] & 0xC0) >> 6);
    obuf [3] = ibuf [2] & 0x3F;

    switch(inputparts) {
      case 1: /* only one byte read */
        sprintf(output, "%c%c==", 
            table64[obuf[0]],
            table64[obuf[1]]);
        break;
      case 2: /* two bytes read */
        sprintf(output, "%c%c%c=", 
            table64[obuf[0]],
            table64[obuf[1]],
            table64[obuf[2]]);
        break;
      default:
        sprintf(output, "%c%c%c%c", 
            table64[obuf[0]],
            table64[obuf[1]],
            table64[obuf[2]],
            table64[obuf[3]] );
        break;
    }
    output += 4;
  }
  *output=0;
}


void output(void *buf)
{
  fd_set writefds;
  int max_fd;
  struct timeval tv;
  int ret;

#if DEBUG
  printf("I say: %s\n", (char *)buf);
#endif
  // set up our fdset and timeout
  FD_ZERO(&writefds);
  FD_SET(client_sockfd, &writefds);
  max_fd = client_sockfd;
  memcpy(&tv, &timeout, sizeof(struct timeval));

  ret = select(max_fd + 1, NULL, &writefds, NULL, &tv);

  if(ret == -1)
  {
#if DEBUG  
    printf("select:\n");    
#endif   
  }
  else if(ret == 0)
  {
#if DEBUG  
    printf("timeout\n");
#endif    
  }
  else
  {
    /* if we woke up on client_sockfd do the data passing */
    if(FD_ISSET(client_sockfd, &writefds))
    {
      if(send(client_sockfd, buf, strlen(buf), 0) == -1)
      {
#if DEBUG      
        printf("error send()ing request");        
#endif        
      }
    }
    else
    {
#if DEBUG
      printf("error: case not handled.");
#endif      
    }
  }
}

int read_input(char *buf, int len)
{
  fd_set readfds;
  int max_fd;
  struct timeval tv;
  int ret;
  int bread = -1;
  
  // set up our fdset and timeout
  FD_ZERO(&readfds);
  FD_SET(client_sockfd, &readfds);
  max_fd = client_sockfd;
  memcpy(&tv, &timeout, sizeof(struct timeval));

  ret = select(max_fd + 1, &readfds, NULL, NULL, &tv);

#if DEBUG  
  printf("ret: %d\n", ret);
#endif  

  if(ret == -1)
  {
#if DEBUG
    printf("select:\n");
#endif    
  }
  else if(ret == 0)
  {
#if DEBUG
    printf("timeout\n");    
#endif    
  }
  else
  {
    /* if we woke up on client_sockfd do the data passing */
    if(FD_ISSET(client_sockfd, &readfds))
    {
      bread = recv(client_sockfd, buf, len-1, 0);
      buf[bread] = '\0';
#if DEBUG
      printf("got: %s\n", buf);
#endif
      if(bread == -1)
      {
#if DEBUG
        printf("error recv()ing reply");
#endif        
      }
    }
    else
    {
#if DEBUG    
      printf("error: case not handled.");
#endif      
    }
  }

  return(bread);
}
#define OUTIP_TOKEN	"Current IP Address:"
#define OUTIP_TOKEN_SZ	sizeof(OUTIP_TOKEN)
int DYNDNS_check_ip(char *addr)
{
  char buf[BUFFER_SIZE+1];
  char buf2[128];
  char *bp = buf;
  char *pbuf = buf2;
  int bytes;
  int btot;
  char *server1="checkip.dyndns.org";
  char *port1="8245";
	int ip1=0,ip2=0,ip3=0,ip4=0;

	// buffer initialize 
  buf[BUFFER_SIZE] = '\0';

  // here connect to checkip.dyndns.org:80
  if(do_connect((int*)&client_sockfd, server1, port1) != 0)
  {
        return 1;
  }

  snprintf(buf, BUFFER_SIZE, "GET / HTTP/1.0\015\012");
  output(buf);

  snprintf(buf, BUFFER_SIZE, "Accept : */*\015\012");
  output(buf);

  snprintf(buf, BUFFER_SIZE, "User-Agent: %s %s [%s] (%s)\015\012", 
      "ez-update", OS, "daemon" , "by Angus Mackay");
  output(buf);

  snprintf(buf, BUFFER_SIZE, "Host: %s\015\012", server1);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "\015\012");
  output(buf);

  bp = buf;
  bytes = 0;
  btot = 0;
  while((bytes=read_input(bp, BUFFER_SIZE-btot)) > 0)
  {
    bp += bytes;
    btot += bytes;
  }
  close(client_sockfd);
  buf[btot] = '\0';

#if DEBUG
  printf("server output: %s\n", buf);
#endif

	// grep the outer ip address from buf
	pbuf=strstr(buf,OUTIP_TOKEN);
	
  	sscanf(pbuf+OUTIP_TOKEN_SZ,"%d.%d.%d.%d%*s",&ip1,&ip2,&ip3,&ip4);
	sprintf(addr,"%d.%d.%d.%d",ip1,ip2,ip3,ip4);
	

  return 0;
	
}

extern char * SkipHeader( char *response );
extern char * ScanAddr( char *body );

int ATDDNS_check_ip(char *addr)
{
	  char buf[BUFFER_SIZE+1024+1];
	  char *bp = buf;
	  int bytes;
	  int btot;
	  char *body;				  /* temporaly pointer */
	  int ip1=0,ip2=0,ip3=0,ip4=0;
	  char cur_addr[16];
	  
	  buf[BUFFER_SIZE] = '\0';
	  if(do_connect((int*)&client_sockfd, server, port) != 0)
	  {
		AddLog(SYSLOG_SECTION, SYSLOG_ERROR, 1);
		return(UPDATERES_CONNECT_FAIL);
	  }
	  
	  snprintf(buf, BUFFER_SIZE, "GET %s", ATDDNS_REQUEST);
	  output(buf);
	  snprintf(buf, BUFFER_SIZE, " HTTP/1.0\n");
	  output(buf);
	  snprintf(buf, BUFFER_SIZE, "Host: %s\r\n", server);
	  output(buf);
	  snprintf(buf, BUFFER_SIZE, "Authorization: Basic %s\n\r\n", auth);
	  output(buf);
	
	  bp = buf;
	  bytes = 0;
	  btot = 0;
	  while((bytes=read_input(bp, BUFFER_SIZE-btot)) > 0)
	  {
		bp += bytes;
		btot += bytes;
	  }
	  close(client_sockfd);
	  buf[btot] = '\0';
	
#if DEBUG
	  printf("server output: %s\n", buf);
#endif
	/* filled with ZERO */
	   bzero( cur_addr, sizeof( cur_addr ) );
	
	   body = SkipHeader( buf );
	
	   strcpy( cur_addr, ScanAddr( body ) );
	   sscanf( cur_addr,"%d.%d.%d.%d%*s",&ip1,&ip2,&ip3,&ip4);
	   sprintf(addr,"%d.%d.%d.%d",ip1,ip2,ip3,ip4);
return 0;
}

#define REQUEST_CHKIP	"/DynDNS/yourip.cgi"
int IVY_check_ip(char *addr)
{
  char buf[BUFFER_SIZE+1024+1];
  char *bp = buf;
  int bytes;
  int btot;
  int ip1=0,ip2=0,ip3=0,ip4=0;
  char    *body = NULL;
  char    cur_addr[16];

  buf[BUFFER_SIZE] = '\0';
  if(do_connect((int*)&client_sockfd, server, port) != 0)
  {
    AddLog(SYSLOG_SECTION, SYSLOG_ERROR, 1);
    return(UPDATERES_CONNECT_FAIL);
  }
  snprintf(buf, BUFFER_SIZE, "GET %s",REQUEST_CHKIP);
  output(buf);
  snprintf(buf, BUFFER_SIZE, " HTTP/1.0\r\n");
  output(buf);
  snprintf(buf, BUFFER_SIZE, "Host: %s\r\n", server);//dp-21.net
  output(buf);
  snprintf(buf, BUFFER_SIZE, "Authorization: Basic %s\r\n", auth);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "User-Agent: %s- %s [%s] (%s)\r\n",
      "ez-update", OS, "daemon" , "by Angus Mackay");
  output(buf);
  snprintf(buf, BUFFER_SIZE, "\r\n");
  output(buf);

  bp = buf;
  bytes = 0;
  btot = 0;
  while((bytes=read_input(bp, BUFFER_SIZE+1024-btot)) > 0)
  {
    bp += bytes;
    btot += bytes;
#ifdef DEBUG    
    printf("btot: %d\n", btot);
#endif    
  }
  close(client_sockfd);
  buf[btot] = '\0';

#if DEBUG
  printf("server output: %s\n", buf);
#endif
     /* filled with ZERO */
        bzero( cur_addr, sizeof( cur_addr ) );

        body = SkipHeader( buf );

        strcpy( cur_addr, ScanAddr( body ) );
        sscanf( cur_addr,"%d.%d.%d.%d%*s",&ip1,&ip2,&ip3,&ip4);
        sprintf(addr,"%d.%d.%d.%d",ip1,ip2,ip3,ip4);
   return(0);
}

int DYNDNS_update_entry(void)
{
  char buf[BUFFER_SIZE+1];
  char *bp = buf;
  int bytes;
  int btot;
  int ret;
  int retval = UPDATERES_UPDATE_OK;

  buf[BUFFER_SIZE] = '\0';

  if(do_connect((int*)&client_sockfd, server, port) != 0)
  {
    AddLog(SYSLOG_SECTION, SYSLOG_ERROR, 1);
    return(UPDATERES_CONNECT_FAIL);
  }
  snprintf(buf, BUFFER_SIZE, "GET %s?", request);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "%s=%s&", "hostname", host);
  output(buf);
  if(ip_address != NULL)
  {
	snprintf(buf, BUFFER_SIZE, "%s=%s&", "myip", ip_address);
	output(buf);
  }
  snprintf(buf, BUFFER_SIZE, "%s=%s&", "wildcard", wildcard ? "ON" : "OFF");
  output(buf);
  if(mx != NULL && *mx != '\0')
  {
    snprintf(buf, BUFFER_SIZE, "%s=%s&", "mx", mx);
    output(buf);
  }
  snprintf(buf, BUFFER_SIZE, " HTTP/1.0\015\012");
  output(buf);
  snprintf(buf, BUFFER_SIZE, "Authorization: Basic %s\015\012", auth);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "User-Agent: %s- %s [%s] (%s)\015\012", 
      "ez-update", OS, "daemon" , "by Angus Mackay");
  output(buf);
  snprintf(buf, BUFFER_SIZE, "Host: %s\015\012", server);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "\015\012");
  output(buf);

  bp = buf;
  bytes = 0;
  btot = 0;
  while((bytes=read_input(bp, BUFFER_SIZE-btot)) > 0)
  {
    bp += bytes;
    btot += bytes;
#ifdef DEBUG    
    printf("btot: %d\n", btot);
#endif    
  }
  close(client_sockfd);
  buf[btot] = '\0';

#ifdef DEBUG
  printf("server output: %s\n", buf);
#endif
  if(sscanf(buf, " HTTP/1.%*c %3d", &ret) != 1)
  {
    ret = -1;
  }

  switch(ret)
  {
    case -1:
      retval = UPDATERES_UPDATE_FAIL;
      break;

    case 200:
      if(strstr(buf, "\ngood ") != NULL || strstr(buf, "\nnochg") != NULL)
      {
      }
      else
      {
        if(strstr(buf, "\nnohost") != NULL)
        {
          retval = UPDATERES_UPDATE_FAIL;
        }
        else if(strstr(buf, "\nnotfqdn") != NULL)
        {
          retval = UPDATERES_UPDATE_FAIL;
        }
        else if(strstr(buf, "\n!yours") != NULL)
        {
          retval = UPDATERES_UPDATE_FAIL;
        }
        else if(strstr(buf, "\nabuse") != NULL)
        {
          retval = UPDATERES_UPDATE_FAIL;
        }
        else if(strstr(buf, "\nbadauth") != NULL)
        {
          retval = UPDATERES_UPDATE_FAIL;
        }
        else if(strstr(buf, "\nbadsys") != NULL)
        {
          retval = UPDATERES_UPDATE_FAIL;
        }
        else if(strstr(buf, "\nbadagent") != NULL)
        {
          retval = UPDATERES_UPDATE_FAIL;
        }
        else if(strstr(buf, "\nnumhost") != NULL)
        {
          retval = UPDATERES_UPDATE_FAIL;
        }
        else if(strstr(buf, "\ndnserr") != NULL)
        {
          retval = UPDATERES_UPDATE_FAIL;
        }
        else if(strstr(buf, "\n911") != NULL)
        {
          retval = UPDATERES_UPDATE_FAIL;
        }
        else if(strstr(buf, "\n999") != NULL)
        {
          retval = UPDATERES_UPDATE_FAIL;
        }
        else if(strstr(buf, "\n!donator") != NULL)
        {
          retval = UPDATERES_UPDATE_OK;
        }
        // this one should be last as it is a stupid string to signify waits
        // with as it is so short
        else if(strstr(buf, "\nw") != NULL)
        {
          int howlong = 0;
          char *p = strstr(buf, "\nw");
          char reason[256];
          char mult;

          // get time and reason
          if(strlen(p) >= 2)
          {
            sscanf(p, "%d%c %255[^\r\n]", &howlong, &mult, reason);
            if(mult == 'h')
            {
              howlong *= 3600;
            }
            else if(mult == 'm')
            {
              howlong *= 60;
            }
            if(howlong > MAX_WAITRESPONSE_WAIT)
            {
              howlong = MAX_WAITRESPONSE_WAIT;
            };
          }
          else
          {
            sprintf(reason, "problem parsing reason for wait response");
          }

          /* Wait response received, waiting for ? seconds before next update*/
          sleep(howlong);
          retval = UPDATERES_UPDATE_FAIL;
        }
        else
        {
          retval = UPDATERES_UPDATE_FAIL;
        }
      }
      break;

    case 401:
      retval = UPDATERES_REG_FAIL;
      break;

    default:
      retval = UPDATERES_UPDATE_FAIL;
      break;
  }

  do_log(retval);
  return(retval);
}

int CHANGEIP_update_entry(void)
{
  char buf[BUFFER_SIZE+1];
  char *bp = buf;
  int bytes;
  int btot;
  int ret;
  int retval = UPDATERES_UPDATE_OK;
  buf[BUFFER_SIZE] = '\0';
  if(do_connect((int*)&client_sockfd, server, port) != 0)
  {
    AddLog(SYSLOG_SECTION, SYSLOG_ERROR, 1);
    return(UPDATERES_CONNECT_FAIL);
  }

  snprintf(buf, BUFFER_SIZE, "GET %s?", request); 
  output(buf);
  snprintf(buf, BUFFER_SIZE, "%s=%s&", "hostname", host);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "%s=%s&", "myip", ip_address);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "%s", "backupMX");
  output(buf);
  snprintf(buf, BUFFER_SIZE, " HTTP/1.0\015\012");
  output(buf);
  snprintf(buf, BUFFER_SIZE, "User-Agent: %s- %s [%s] (%s)\015\012",
      "ez-update", OS, "daemon" , "by Angus Mackay");
  output(buf);
  snprintf(buf, BUFFER_SIZE, "Host: %s\015\012", server);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "Connection: Keep-Alive\015\012");
  output(buf);
  snprintf(buf, BUFFER_SIZE, "Authorization: Basic %s\015\012", auth);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "\015\012");
  output(buf);
  bp = buf;
  bytes = 0;
  btot = 0;
  while((bytes=read_input(bp, BUFFER_SIZE-btot)) > 0)
  {
    bp += bytes;
    btot += bytes;
#ifdef DEBUG    
    printf("btot: %d\n", btot);
#endif    
  }
  close(client_sockfd);
  buf[btot] = '\0';

#ifdef DEBUG
   printf("server output: %s\n", buf);
#endif


   if(sscanf(buf, " HTTP/1.%*c %3d", &ret) != 1)
   {
      ret = -1;
   }
#ifdef DEBUG
   printf("sscanf buf: %s\n", buf);
#endif

#if 1
    // Fixed the bug can't get the correct response from Server.
    // Sometimes there is no the "OK" or "!"
    if(strstr(buf, "HTTP/1.1 200") ||
      strstr(buf, "HTTP/1.0 200")) {
        if(strstr(buf, "Successful Update")) {
            retval = UPDATERES_UPDATE_OK;
        } else {
            retval = UPDATERES_UPDATE_FAIL;
        }
    }
#else
    if(strstr(buf, "Successful Update")) {
            retval = UPDATERES_UPDATE_OK;
        }
#endif
    else if(strstr(buf, "401")) {
        retval = UPDATERES_REG_FAIL;//user_name or password fail.
    } 
    else {
	retval = UPDATERES_UPDATE_FAIL;
        //"changeip.com: Internal Server Error"
    }
   do_log(retval);
   return(retval);
}

//IvyNetwork
#define ADDR_TOKEN_NUM	5
char * ScanAddr( char *body )
{
   char    *head;          /* IP address head pointer */
   char    *tail;          /* IP address tail pointer */
   int i;
   head=body;
   for(i=0 ; i< ADDR_TOKEN_NUM ; i++){
	head = index( head, ':' );
	if(head)
	   head++;
        else
	   break;
   }

   if (i<(ADDR_TOKEN_NUM-1)){
#if DEBUG
	printf("NO IP address found!\n");   
#endif
	return(NULL);
   }
   else{
         while( *head > '9' || *head < '0' ){
                head++;
         }
         tail = head;
         while( (*tail <= '9' && *tail >= '0') || *tail == '.' ){                                tail++;
         }
         *tail = '\0';
   }
   return(head);
}
char * SkipHeader( char *response )
{
        char    *cur;                   /* current position */
        char    *body=NULL;                  /* temporaly pointer */

        /* skip response header */
        for( cur = response ; cur != NULL ; cur++ ){
                cur = index( cur, '\n' );

                /* header/body separator is "\r\n\r\n" */
                if( *(cur-1) == '\r' && *(cur+1) == '\r' && *(cur+2) == '\n' ){
                        *(cur-1) = '\0';
                        body = cur + 3;
                        break;
                }
        }

        /* return body head */
        return( body );
}

int DP21_update_entry(void)
{
  char buf[BUFFER_SIZE+1];
  char *bp = buf;
  int bytes;
  int btot;
  int ret;
  int retval = UPDATERES_UPDATE_OK;
  char    *body;                  /* temporaly pointer */

  buf[BUFFER_SIZE] = '\0';
  if(do_connect((int*)&client_sockfd, server, port) != 0)
  {
    AddLog(SYSLOG_SECTION, SYSLOG_ERROR, 1);
    return(UPDATERES_CONNECT_FAIL);
  }
  snprintf(buf, BUFFER_SIZE, "GET %s?", request);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "ipaddr=%s", ip_address);
  output(buf);
  snprintf(buf, BUFFER_SIZE, " HTTP/1.0\r\n");
  output(buf);
  snprintf(buf, BUFFER_SIZE, "Host: %s\r\n", server);//dp-21.net
  output(buf);
  snprintf(buf, BUFFER_SIZE, "Authorization: Basic %s\r\n", auth);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "User-Agent: %s- %s [%s] (%s)\r\n",
      "ez-update", OS, "daemon" , "by Angus Mackay");
  output(buf);
  snprintf(buf, BUFFER_SIZE, "\r\n");
  output(buf);

  bp = buf;
  bytes = 0;
  btot = 0;
  while((bytes=read_input(bp, BUFFER_SIZE-btot)) > 0)
  {
    bp += bytes;
    btot += bytes;
#ifdef DEBUG    
    printf("btot: %d\n", btot);
#endif    
  }
  close(client_sockfd);
  buf[btot] = '\0';

#ifdef DEBUG
   printf("server output: %s\n", buf);
#endif

   if(sscanf(buf, " HTTP/1.%*c %3d", &ret) != 1)
   {
      ret = -1;
   }
#ifdef DEBUG
   printf("sscanf buf: %s\n", buf);
#endif

#if 1
    /* Japanese(EUC) Update Succeed Message */
    #define MSG_IPADDRESS   "\xa3\xc9\xa3\xd0\xa5\xa2\xa5\xc9\xa5\xec\xa5\xb9"
    if(strstr(buf, "HTTP/1.1 200 OK") ||
      strstr(buf, "HTTP/1.0 200 OK")) {
	body = SkipHeader( buf );
	if(strstr(body, MSG_IPADDRESS) && ScanAddr( body ) !=NULL){
   	         retval = UPDATERES_UPDATE_OK;
		//printf("UPDATE_OK %s\n");
	 }
         else{
		retval = UPDATERES_UPDATE_FAIL;
	 }
    } 
    else {
	retval = UPDATERES_UPDATE_FAIL;
    }

#else
    if(DP21_parse_result(buf)){
        retval = UPDATERES_UPDATE_OK;
    }
    else{
        retval = UPDATERES_UPDATE_FAIL;
    }
#endif

   do_log(retval);
   return(retval);

}

//@NetHome
int ATDDNS_update_entry(void)
{
  char buf[BUFFER_SIZE+1];
  char *bp = buf;
  int bytes;
  int btot;
  int ret;
  int retval = UPDATERES_UPDATE_OK;
  char    *body;                  /* temporaly pointer */

  buf[BUFFER_SIZE] = '\0';
  if(do_connect((int*)&client_sockfd, server, port) != 0)
  {
    AddLog(SYSLOG_SECTION, SYSLOG_ERROR, 1);
    return(UPDATERES_CONNECT_FAIL);
  }
  snprintf(buf, BUFFER_SIZE, "GET %s?", request);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "ipaddr=%s", ip_address);
  output(buf);
  snprintf(buf, BUFFER_SIZE, " HTTP/1.0\r\n");
  output(buf);
  snprintf(buf, BUFFER_SIZE, "Host: %s\r\n", server);//dp-21.net
  output(buf);
  snprintf(buf, BUFFER_SIZE, "Authorization: Basic %s\r\n", auth);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "User-Agent: %s- %s [%s] (%s)\r\n",
      "ez-update", OS, "daemon" , "by Angus Mackay");
  output(buf);
  snprintf(buf, BUFFER_SIZE, "\r\n");
  output(buf);

  bp = buf;
  bytes = 0;
  btot = 0;
  while((bytes=read_input(bp, BUFFER_SIZE-btot)) > 0)
  {
    bp += bytes;
    btot += bytes;
#ifdef DEBUG    
    printf("btot: %d\n", btot);
#endif    
  }
  close(client_sockfd);
  buf[btot] = '\0';

#ifdef DEBUG
   printf("server output: %s\n", buf);
#endif

   if(sscanf(buf, " HTTP/1.%*c %3d", &ret) != 1)
   {
      ret = -1;
   }
#ifdef DEBUG
   printf("sscanf buf: %s\n", buf);
#endif

#if 1
    /* Japanese(EUC) Update Succeed Message */
    #define MSG_IPADDRESS   "\xa3\xc9\xa3\xd0\xa5\xa2\xa5\xc9\xa5\xec\xa5\xb9"
    if(strstr(buf, "HTTP/1.1 200 OK") ||
      strstr(buf, "HTTP/1.0 200 OK")) {
	body = SkipHeader( buf );
	if(strstr(body, MSG_IPADDRESS) && ScanAddr( body ) !=NULL){
   	         retval = UPDATERES_UPDATE_OK;
		//printf("UPDATE_OK %s\n");
	 }
         else{
		retval = UPDATERES_UPDATE_FAIL;
	 }
    } 
    else {
	retval = UPDATERES_UPDATE_FAIL;
    }

#else
    if(DP21_parse_result(buf)){
        retval = UPDATERES_UPDATE_OK;
    }
    else{
        retval = UPDATERES_UPDATE_FAIL;
    }
#endif

   do_log(retval);
   return(retval);

}

#if 0
static char * select_ip(char *ptr)	//?? now we choose first ip of 4 ip sets
{	
	char tmp[128];
	char *ipp;

	strcpy(tmp, ptr);
	strtok(tmp, ",");
	ipp=strtok(NULL, ",");

	return ipp;
}
static int parse_server_ip(char *str, char *ip)
{
	char buf[128];
	char *pt1,*pt2;

	strcpy(buf, str);		//str -> "XXX\r\nYYY\r\n"

	strtok(buf, "\r\n");		
	pt1=strtok(NULL, "\r\n");	//ip -> "YYY"
	if( *pt1 == '0' )	
		return -1;		//"0\r\n" -> invalid key

	pt2=select_ip(pt1);
	strcpy(ip,pt2);

	return 0;
}
	

static int parse_resp(char *str)
{
	char buf[128];
	char *p;
	
	strcpy(buf, str);		//str -> "XXX\r\nYYY\r\n"

	strtok(buf, "\r\n");		
	p=strtok(NULL, "\r\n");		//ip -> "YYY"
	if(strcmp(p, "40 OK"))
		return -1;
	return 0;
}

int TZO_update_entry(void)
{
  char buf[BUFFER_SIZE+1];
  char *bp = buf;
  int bytes;
  int btot;
  int retval;
  char step2_ip[20];

  buf[BUFFER_SIZE] = '\0';

  if(do_connect((int*)&client_sockfd, server, port) != 0)
  {
    AddLog(SYSLOG_SECTION, SYSLOG_ERROR, 1);
    return(UPDATERES_ERROR);
  }

  snprintf(buf, BUFFER_SIZE, "I %s\r\n", password);
  output(buf);

  bp = buf;
  bytes = 0;
  btot = 0;
  while((bytes=read_input(bp, BUFFER_SIZE-btot)) > 0)
  {
    bp += bytes;
    btot += bytes;
  }
  close(client_sockfd);
  buf[btot] = '\0';

#ifdef DEBUG
  printf("server output: %s\n", buf);
#endif

  //parse 1 ip:
  if( parse_server_ip(buf,step2_ip) ){
    retval=UPDATERES_ERROR;
    goto err;
  }
#ifdef DEBUG
  printf("ip output: %s\n", step2_ip);
#endif

  //create 2nd session ,port=21347
  if(do_connect((int*)&client_sockfd, step2_ip, TZO_STEP2_PORT) != 0)
  {
    AddLog(SYSLOG_SECTION, SYSLOG_ERROR, 1);
    return(UPDATERES_ERROR);
  }

  //register
  snprintf(buf, BUFFER_SIZE, "R %s,%s,%s,%s\r\n", host, user_name, password,ip_address);
  output(buf);

  //read server response
  bp = buf;
  bytes = 0;
  btot = 0;
  while((bytes=read_input(bp, BUFFER_SIZE-btot)) > 0)
  {
    bp += bytes;
    btot += bytes;
  }
  close(client_sockfd);
  buf[btot] = '\0';

#ifdef DEBUG
  printf("server output: %s\n", buf);
#endif

  //parse server response , ex:"40 OK"
  if( parse_resp(buf) )
    retval=UPDATERES_ERROR;

  retval=UPDATERES_OK;

err:
  if ( retval == UPDATERES_OK)
    	AddLog(SYSLOG_SECTION, SYSLOG_NORMAL, 2);
  else if (retval == UPDATERES_ERROR)
	AddLog(SYSLOG_SECTION, SYSLOG_ERROR, 3);
  else if (retval == UPDATERES_SHUTDOWN)
    	AddLog(SYSLOG_SECTION, SYSLOG_ERROR, 4);

  return(retval);
}
#else
#ifdef DEBUG
#define show_message	printf
#else
#define show_message(...)    
#endif
int TZO_update_entry(void)
{
  char buf[BUFFER_SIZE+1];
  char *bp = buf;
  int bytes;
  int btot;
  int ret, retval = UPDATERES_UPDATE_OK;

  buf[BUFFER_SIZE] = '\0';

  if(do_connect((int*)&client_sockfd, server, port) != 0)
  {
    AddLog(SYSLOG_SECTION, SYSLOG_ERROR, 1);
    return(UPDATERES_CONNECT_FAIL);
  }
  snprintf(buf, BUFFER_SIZE, "GET %s?", request);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "%s=%s&", "TZOName", host);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "%s=%s&", "Email", user_name);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "%s=%s&", "TZOKey", password);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "%s=%s&", "IPAddress", ip_address);
  output(buf);
  snprintf(buf, BUFFER_SIZE, " HTTP/1.0\015\012");
  output(buf);
  snprintf(buf, BUFFER_SIZE, "User-Agent: %s-%s %s [%s] (%s)\015\012",
      "ez-update", VERSION, OS, "daemon", "by Angus Mackay");
  output(buf);
  snprintf(buf, BUFFER_SIZE, "Host: %s\015\012", server);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "\015\012");
  output(buf);

  bp = buf;
  bytes = 0;
  btot = 0;
  while((bytes=read_input(bp, BUFFER_SIZE-btot)) > 0)
  {
    bp += bytes;
    btot += bytes;
    show_message( "btot: %d\n", btot);
  }
  close(client_sockfd);
  buf[btot] = '\0';

  show_message("server output: %s\n", buf);

  if(strstr(buf,"Error=")==NULL)
  {
	show_message("request successful\n");
        retval = UPDATERES_UPDATE_OK;
  }
  else{
	retval = UPDATERES_UPDATE_FAIL;
  }      
  do_log(retval);
  return(retval);
}

/*
 * grrrrr, it seems that dhs.org requires us to use POST
 * also DHS doesn't update both the mx record and the address at the same
 * time, this service really stinks. go with justlinix.com (penguinpowered)
 * instead, the only advantage is short host names.
 */
int DHS_update_entry(void)
{
  char buf[BUFFER_SIZE+1];
  char putbuf[BUFFER_SIZE+1];
  char *bp = buf;
  int bytes;
  int btot;
  int ret;
  char *domain = NULL;
  char *hostname = NULL;
  char *p;
  int limit;
  int retval = UPDATERES_UPDATE_OK;

  buf[BUFFER_SIZE] = '\0';
  putbuf[BUFFER_SIZE] = '\0';

  /* parse apart the domain and hostname */
  hostname = strdup(host);
  if((p=strchr(hostname, '.')) == NULL)
  {
      show_message("error parsing hostname from host %s\n", host);
    return(UPDATERES_UPDATE_FAIL);
  }
  *p = '\0';
  p++;
  if(*p == '\0')
  {
      show_message("error parsing domain from host %s\n", host);
    return(UPDATERES_UPDATE_FAIL);
  }
  domain = strdup(p);


  if(do_connect((int*)&client_sockfd, server, port) != 0)
  {
    AddLog(SYSLOG_SECTION, SYSLOG_ERROR, 1);
    return(UPDATERES_CONNECT_FAIL);
  }

  snprintf(buf, BUFFER_SIZE, "POST %s HTTP/1.0\015\012", request);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "Authorization: Basic %s\015\012", auth);
  output(buf);
  snprintf(buf, BUFFER_SIZE, "User-Agent: %s-%s %s [%s] (%s)\015\012", 
      "ez-update", VERSION, OS, "daemon", "by Angus Mackay");
  output(buf);
  snprintf(buf, BUFFER_SIZE, "Host: %s\015\012", server);
  output(buf);

  p = putbuf;
  *p = '\0';
  limit = BUFFER_SIZE - 1 - strlen(buf);
  snprintf(p, limit, "hostscmd=edit&hostscmdstage=2&type=4&");
  p += strlen(p);
  limit = BUFFER_SIZE - 1 - strlen(buf);
  snprintf(p, limit, "%s=%s&", "updatetype", "Online");
  p += strlen(p);
  limit = BUFFER_SIZE - 1 - strlen(buf);
  snprintf(p, limit, "%s=%s&", "ip", ip_address);
  p += strlen(p);
  limit = BUFFER_SIZE - 1 - strlen(buf);
  snprintf(p, limit, "%s=%s&", "mx", mx);
  p += strlen(p);
  limit = BUFFER_SIZE - 1 - strlen(buf);
  snprintf(p, limit, "%s=%s&", "offline_url", url);
  p += strlen(p);
  limit = BUFFER_SIZE - 1 - strlen(buf);
  if(cloak_title)
  {
    snprintf(p, limit, "%s=%s&", "cloak", "Y");
    p += strlen(p);
    limit = BUFFER_SIZE - 1 - strlen(buf);
    snprintf(p, limit, "%s=%s&", "cloak_title", cloak_title);
    p += strlen(p);
    limit = BUFFER_SIZE - 1 - strlen(buf);
  }
  else
  {
    snprintf(p, limit, "%s=%s&", "cloak_title", "");
    p += strlen(p);
    limit = BUFFER_SIZE - 1 - strlen(buf);
  }
  snprintf(p, limit, "%s=%s&", "submit", "Update");
  p += strlen(p);
  limit = BUFFER_SIZE - 1 - strlen(buf);
  snprintf(p, limit, "%s=%s&", "domain", domain);
  p += strlen(p);
  limit = BUFFER_SIZE - 1 - strlen(buf);
  snprintf(p, limit, "%s=%s", "hostname", hostname);
  p += strlen(p);
  limit = BUFFER_SIZE - 1 - strlen(buf);

  snprintf(buf, BUFFER_SIZE, "Content-length: %d\015\012", strlen(putbuf));
  output(buf);
  snprintf(buf, BUFFER_SIZE, "\015\012");
  output(buf);

  output(putbuf);
  snprintf(buf, BUFFER_SIZE, "\015\012");
  output(buf);

  bp = buf;
  bytes = 0;
  btot = 0;
  while((bytes=read_input(bp, BUFFER_SIZE-btot)) > 0)
  {
    bp += bytes;
    btot += bytes;
    //dprintf((stderr, "btot: %d\n", btot));
  }
  close(client_sockfd);
  buf[btot] = '\0';

  //dprintf((stderr, "server output: %s\n", buf));

  if(sscanf(buf, " HTTP/1.%*c %3d", &ret) != 1)
  {
    ret = -1;
  }

  switch(ret)
  {
    case -1:
      {
        show_message("strange server response, are you connecting to the right server?\n");
      }
      retval = UPDATERES_UPDATE_FAIL;
      break;

    case 200:
      {
        show_message("request successful\n");
	retval=UPDATERES_UPDATE_OK;
      }
      break;

    case 401:
      {
        show_message("authentication failure\n");
      }
      retval = UPDATERES_REG_FAIL;
      break;

    default:
      {
        // reuse the auth buffer
        *auth = '\0';
        sscanf(buf, " HTTP/1.%*c %*3d %255[^\r\n]", auth);
        show_message("unknown return code: %d\n", ret);
        show_message("server response: %s\n", auth);
      }
      retval = UPDATERES_UPDATE_FAIL;
      break;
  }

  // this stupid service requires us to do seperate request if we want to 
  // update the mail exchanger (mx). grrrrrr
  if(*mx != '\0')
  {
    // okay, dhs's service is incredibly stupid and will not work with two
    // requests right after each other. I could care less that this is ugly,
    // I personally will NEVER use dhs, it is laughable.
    sleep(DHS_SUCKY_TIMEOUT < timeout.tv_sec ? DHS_SUCKY_TIMEOUT : timeout.tv_sec);

    if(do_connect((int*)&client_sockfd, server, port) != 0)
    {
      AddLog(SYSLOG_SECTION, SYSLOG_ERROR, 1);
      return(UPDATERES_CONNECT_FAIL);
    }

    snprintf(buf, BUFFER_SIZE, "POST %s HTTP/1.0\015\012", request);
    output(buf);
    snprintf(buf, BUFFER_SIZE, "Authorization: Basic %s\015\012", auth);
    output(buf);
    snprintf(buf, BUFFER_SIZE, "User-Agent: %s-%s %s [%s] (%s)\015\012", 
        "ez-update", VERSION, OS, "daemon", "by Angus Mackay");
    output(buf);
    snprintf(buf, BUFFER_SIZE, "Host: %s\015\012", server);
    output(buf);

    p = putbuf;
    *p = '\0';
    limit = BUFFER_SIZE - 1 - strlen(buf);
    snprintf(p, limit, "hostscmd=edit&hostscmdstage=2&type=4&");
    p += strlen(p);
    limit = BUFFER_SIZE - 1 - strlen(buf);
    snprintf(p, limit, "%s=%s&", "updatetype", "Update+Mail+Exchanger");
    p += strlen(p);
    limit = BUFFER_SIZE - 1 - strlen(buf);
    snprintf(p, limit, "%s=%s&", "ip", ip_address);
    p += strlen(p);
    limit = BUFFER_SIZE - 1 - strlen(buf);
    snprintf(p, limit, "%s=%s&", "mx", mx);
    p += strlen(p);
    limit = BUFFER_SIZE - 1 - strlen(buf);
    snprintf(p, limit, "%s=%s&", "offline_url", url);
    p += strlen(p);
    limit = BUFFER_SIZE - 1 - strlen(buf);
    if(cloak_title)
    {
      snprintf(p, limit, "%s=%s&", "cloak", "Y");
      p += strlen(p);
      limit = BUFFER_SIZE - 1 - strlen(buf);
      snprintf(p, limit, "%s=%s&", "cloak_title", cloak_title);
      p += strlen(p);
      limit = BUFFER_SIZE - 1 - strlen(buf);
    }
    else
    {
      snprintf(p, limit, "%s=%s&", "cloak_title", "");
      p += strlen(p);
      limit = BUFFER_SIZE - 1 - strlen(buf);
    }
    snprintf(p, limit, "%s=%s&", "submit", "Update");
    p += strlen(p);
    limit = BUFFER_SIZE - 1 - strlen(buf);
    snprintf(p, limit, "%s=%s&", "domain", domain);
    p += strlen(p);
    limit = BUFFER_SIZE - 1 - strlen(buf);
    snprintf(p, limit, "%s=%s", "hostname", hostname);
    p += strlen(p);
    limit = BUFFER_SIZE - 1 - strlen(buf);

    snprintf(buf, BUFFER_SIZE, "Content-length: %d\015\012", strlen(putbuf));
    output(buf);
    snprintf(buf, BUFFER_SIZE, "\015\012");
    output(buf);

    output(putbuf);
    snprintf(buf, BUFFER_SIZE, "\015\012");
    output(buf);

    bp = buf;
    bytes = 0;
    btot = 0;
    while((bytes=read_input(bp, BUFFER_SIZE-btot)) > 0)
    {
      bp += bytes;
      btot += bytes;
      //dprintf((stderr, "btot: %d\n", btot));
    }
    close(client_sockfd);
    buf[btot] = '\0';

    //dprintf((stderr, "server output: %s\n", buf));

    if(sscanf(buf, " HTTP/1.%*c %3d", &ret) != 1)
    {
      ret = -1;
    }

    switch(ret)
    {
      case -1:
        {
          show_message("strange server response, are you connecting to the right server?\n");
        }
        retval = UPDATERES_UPDATE_FAIL;
        break;

      case 200:
//        if(!(options & OPT_QUIET))
        {
          show_message("request successful\n");
        }
        break;

      case 401:
        {
          show_message("authentication failure\n");
        }
        retval = UPDATERES_REG_FAIL;
        break;

      default:
        {
          // reuse the auth buffer
          *auth = '\0';
          sscanf(buf, " HTTP/1.%*c %*3d %255[^\r\n]", auth);
          show_message("unknown return code: %d\n", ret);
          show_message("server response: %s\n", auth);
        }
        retval = UPDATERES_UPDATE_FAIL;
        break;
    }
  }

  do_log(retval);
  return(retval);
}


#endif
void parse_service(char *str)
{

	int provider_no;
	provider_no=atoi(str);
	switch(provider_no){
	case 0:
                provider=DDNS_DYNDNS;
                service=&service1;
		break;
        case 1:
                provider=DDNS_TZO;
                service=&service2;
                break;
        case 2:
                provider=DDNS_DYNDNS_QDNS3322;
                service=&service10;
                break;
	default:
	        provider=DDNS_NO_DDNS;
                service=NULL;
                break;
	}
	return;
}

#ifdef _GNUDIP_DDNS_
//----------------------------------//
//--- START: GNUDIP -TCP Protocol --//
//----------------------------------//
enum {
  UPDATERES_OK = 0,
  UPDATERES_ERROR,
  UPDATERES_SHUTDOWN,
};
int options;
#define OPT_DEBUG       0x0001
#define OPT_DAEMON      0x0004
#define OPT_QUIET       0x0008
#define OPT_FOREGROUND  0x0010
#define OPT_OFFLINE     0x0020
/*
 * like "chomp" in perl, take off trailing newline chars
 */
char *chomp(char *buf)
{
  char *p;

  for(p=buf; *p != '\0'; p++);
  if(p != buf) { p--; }
  while(p>=buf && (*p == '\n' || *p == '\r'))
  {
    *p-- = '\0';
  }
  return(buf);
}
//TCP
int GNUDIP_TCP_update_entry(void)
{
  unsigned char digestbuf[MD5_DIGEST_BYTES];
  char buf[BUFFER_SIZE+1];
  char *p;
  int bytes;
  int ret;
  int i;
  char *domainname;
  char gnudip_request[2];

  // send an offline request if address 0.0.0.0 is used
  // otherwise, we ignore the address and send an update request
  gnudip_request[0] = strcmp(address, "0.0.0.0") == 0 ? '1' : '0';
  gnudip_request[1] = '\0';

  // find domainname
  for(p=host; *p != '\0' && *p != '.'; p++);
  if(*p != '\0') { p++; }
  if(*p == '\0')
  {
    ret=UPDATERES_UPDATE_FAIL;
    goto gnudiptcp_out;
  }

#if 0
  domainname = p;
#else
  domainname = host;
#endif
  if(do_connect((int*)&client_sockfd, server, port) != 0)
  {
    if(!(options & OPT_QUIET))
    {
      show_message("error connecting to %s:%s\n", server, port);
    }
    ret=UPDATERES_CONNECT_FAIL;
    goto gnudiptcp_out;
  }

  if((bytes=read_input(buf, BUFFER_SIZE)) <= 0)
  {
    close(client_sockfd);
    ret=UPDATERES_CONNECT_FAIL;
    goto gnudiptcp_out;
  }
  buf[bytes] = '\0';
  dprintf((stderr, "bytes: %d\n", bytes));
  dprintf((stderr, "server output: %s\n", buf));
  // buf holds the shared secret
  chomp(buf);

  // use the auth buffer
  md5_buffer(password, strlen(password), digestbuf);
  for(i=0, p=auth; i<MD5_DIGEST_BYTES; i++, p+=2)
  {
    sprintf(p, "%02x", digestbuf[i]);
  }
  strncat(auth, ".", 255-strlen(auth));
  strncat(auth, buf, 255-strlen(auth));
  dprintf((stderr, "auth: %s\n", auth));
  md5_buffer(auth, strlen(auth), digestbuf);
  for(i=0, p=buf; i<MD5_DIGEST_BYTES; i++, p+=2)
  {
    sprintf(p, "%02x", digestbuf[i]);
  }
  strcpy(auth, buf);

  dprintf((stderr, "auth: %s\n", auth));

  snprintf(buf, BUFFER_SIZE, "%s:%s:%s:%s\n", user_name, auth, domainname,
      gnudip_request);
  output(buf);

  bytes = 0;
  if((bytes=read_input(buf, BUFFER_SIZE)) <= 0)
  {
    close(client_sockfd);
    ret=UPDATERES_CONNECT_FAIL;
    goto gnudiptcp_out;
  }
  buf[bytes] = '\0';

  dprintf((stderr, "bytes: %d\n", bytes));
  dprintf((stderr, "server output: %s\n", buf));
  close(client_sockfd);

  if(sscanf(buf, "%d", &ret) != 1)
  {
    ret = -1;
  }
  switch(ret)
  {
    case -1:
      if(!(options & OPT_QUIET))
      {
        show_message("strange server response, are you connecting to the right server?\n");
      }
      ret=UPDATERES_UPDATE_FAIL;
      break;

    case 0:
      if(!(options & OPT_QUIET))
      {
        printf("update request successful\n");
      }
      ret=UPDATERES_UPDATE_OK;
      break;
    case 1:
      if(!(options & OPT_QUIET))
      {
        show_message("invalid login attempt\n");
      }
      ret=UPDATERES_UPDATE_FAIL;
      break;

    case 2:
      if(!(options & OPT_QUIET))
      {
        fprintf(stderr, "offline request successful\n");
      }
      ret=UPDATERES_UPDATE_OK;
      break;

    default:
      if(!(options & OPT_QUIET))
      {
        show_message("unknown return code: %d\n", ret);
      }
      ret=UPDATERES_REG_FAIL;
      break;
  }
gnudiptcp_out:
  do_log(ret);
  return(ret);
}
//----------------------------------//
//--- END: GNUDIP -TCP Protocol  ---//
//----------------------------------//

//----------------------------------//
//-- START: GNUDIP -HTTP Protocol --//
//----------------------------------//
#define META_FLAG       "meta name=\""
#define CONTENT_FLAG    "content=\""
#define TIME_FLAG       "time"
#define SALT_FLAG       "salt"
#define SIGN_FLAG       "sign"
#define FLAG_LEN        4
#define RETC_FLAG       "retc"
#define ADDR_FLAG       "addr"
//if success return 0, else return -1
//input, char * salt, time, sign should own >32 bytes memery. better 64 bytes
//the server's response will fill their memory with content
int GNUDIP_first_request_and_parse(char *salt, char *time, char * sign)
{
        char buf[BUFFER_SIZE+1];
        char temp[32];
        char *bp = buf;
        int bytes;
        int btot;
        int ret;
        buf[BUFFER_SIZE] = '\0';
        if((salt == NULL) || (time == NULL) || (sign == NULL))
        {
                return -1;
        }
	if(do_connect((int*)&client_sockfd, server, port) != 0)
          {
            return -1;
          }
        snprintf(buf, BUFFER_SIZE, "GET %s", request);
        output(buf);
        snprintf(buf, BUFFER_SIZE, " HTTP/1.0\015\012");
        output(buf);
        snprintf(buf, BUFFER_SIZE, "User-Agent: %s- %s [%s] (%s)\015\012",
        "ez-update", OS, "daemon" , "by Angus Mackay");
        output(buf);
        snprintf(buf, BUFFER_SIZE, "Host: %s\015\012", server);
        output(buf);
        snprintf(buf, BUFFER_SIZE, "\015\012");
        output(buf);

        bp = buf;
        bytes = 0;
        btot = 0;
        while((bytes=read_input(bp, BUFFER_SIZE-btot)) > 0)
        {
                bp += bytes;
                btot += bytes;
        }
        close(client_sockfd);
        buf[btot] = '\0';

        if(sscanf(buf, " HTTP/1.%*c %3d", &ret) != 1)
        {
                ret = -1;
        }

        switch(ret)
        {
                case 200:
                        bp = buf;
                        while(1)
                        {
                                 if((bp=strstr(bp, META_FLAG)))
                                {
                                        bp += strlen(META_FLAG) ;
                                        strncpy(temp, bp, FLAG_LEN);
                                        temp[FLAG_LEN] = 0;
                                        if(strcmp(temp, SALT_FLAG) == 0)
                                        {
                                                bp += FLAG_LEN+2; // + 2, because ''\"'' and signle space '_' before content
                                                 if(strncmp(bp, CONTENT_FLAG, strlen(CONTENT_FLAG)) == 0)
                                                 {
                                                        int i = 0;
                                                        bp += strlen(CONTENT_FLAG);
                                                        while(bp[i] != '"')
                                                        {
                                                                i++;
                                                        }
                                                        strncpy(salt, bp, i);
                                                        salt[i] = 0;
                                                 }
                                                 else
                                                 {
                                                        ret = -1;
                                                        break;
                                                 }
                                        }
                                        else if(strcmp(temp, TIME_FLAG) == 0)
                                        {
                                                bp += FLAG_LEN+2; // + 2, because ''\"'' and signle space '_' before content
                                                 if(strncmp(bp, CONTENT_FLAG, strlen(CONTENT_FLAG)) == 0)
                                                 {
                                                        int i = 0;
                                                        bp += strlen(CONTENT_FLAG);
                                                        while(bp[i] != '"')
                                                        {
                                                                i++;
                                                        }
                                                        strncpy(time, bp, i);
                                                        time[i] = 0;
                                                 }
                                                 else
                                                 {
                                                        ret = -1;
                                                        break;
                                                 }
                                        }
                                        else if(strcmp(temp, SIGN_FLAG) == 0)
                                        {
                                                bp += FLAG_LEN+2; // + 2, because ''\"'' and signle space '_' before content
                                                 if(strncmp(bp, CONTENT_FLAG, strlen(CONTENT_FLAG)) == 0)
                                                 {
                                                        int i = 0;
                                                        bp += strlen(CONTENT_FLAG);
                                                        while(bp[i] != '"')
                                                        {
                                                                i++;
                                                        }
                                                        strncpy(sign, bp, i);
                                                        sign[i] = 0;
                                                 }
                                                 else
                                                 {
                                                        ret = -1;
                                                        break;
                                                 }
                                        }
                                }
                                 else
                                 {
                                        ret = 0;
                                        break;
                                 }
                        }

                        break;
                default:
                      ret  = -1;
                      break;

        }

        return ret;
}



// input char * salt, time, sign,, their value  from the first request response
// by server.
// output update_ret, address
int GNUDIP_second_request_and_parse(char *salt, char *time, char *sign,  int * update_ret, char *addr)
{

        int i;
        unsigned char digestbuf[MD5_DIGEST_BYTES];
        char buf[BUFFER_SIZE+1];
        char temp[32];
        char *p;
        char *domainname;
        char *bp = buf;
        int bytes;
        int btot;
        int ret;
        if((salt == NULL) || (time == NULL) || (sign == NULL))
        {
                return -1;
        }

        domainname = strchr(host, '.') + 1;

          md5_buffer(password, strlen(password), digestbuf);
          for(i=0, p=auth; i<MD5_DIGEST_BYTES; i++, p+=2)
          {
            sprintf(p, "%02x", digestbuf[i]);
          }
          strncat(auth, ".", 255-strlen(auth));
          strncat(auth, salt, 255-strlen(auth));
          md5_buffer(auth, strlen(auth), digestbuf);
          for(i=0, p=buf; i<MD5_DIGEST_BYTES; i++, p+=2)
          {
            sprintf(p, "%02x", digestbuf[i]);
          }
          strcpy(auth, buf);

         if(do_connect((int*)&client_sockfd, server, port) != 0)
          {
            return -1;
          }
        snprintf(buf, BUFFER_SIZE, "GET %s?", request);
        output(buf);
        snprintf(buf, BUFFER_SIZE, "%s=%s&", "salt", salt);
        output(buf);
        snprintf(buf, BUFFER_SIZE, "%s=%s&", "time", time);
        output(buf);
        snprintf(buf, BUFFER_SIZE, "%s=%s&", "sign", sign);
        output(buf);
        snprintf(buf, BUFFER_SIZE, "%s=%s&", "user", user_name);
        output(buf);
        snprintf(buf, BUFFER_SIZE, "%s=%s&", "pass", auth);
        output(buf);
        snprintf(buf, BUFFER_SIZE, "%s=%s&", "domn", domainname);
        output(buf);
        snprintf(buf, BUFFER_SIZE, "%s=%d&", "reqc", 2);
        output(buf);
        snprintf(buf, BUFFER_SIZE, "%s=%s", "addr", ip_address);
        output(buf);
        snprintf(buf, BUFFER_SIZE, " HTTP/1.0\015\012");
        output(buf);
	snprintf(buf, BUFFER_SIZE, "Host: %s\015\012", server);

        output(buf);
        snprintf(buf, BUFFER_SIZE, "\015\012");
        output(buf);

        bp = buf;
        bytes = 0;
        btot = 0;
        while((bytes=read_input(bp, BUFFER_SIZE-btot)) > 0)
        {
                bp += bytes;
                btot += bytes;
#ifdef DEBUG
                printf("btot: %d\n", btot);
#endif
        }
        close(client_sockfd);
        buf[btot] = '\0';

        if(sscanf(buf, " HTTP/1.%*c %3d", &ret) != 1)
        {
                ret = -1;
        }

        switch(ret)
        {
                case 200:
                        bp=buf;
                        while(1)
                        {
                                 if((bp=strstr(bp, META_FLAG)))
                                {
                                        bp += strlen(META_FLAG) ;
                                        strncpy(temp, bp, FLAG_LEN);
                                        temp[FLAG_LEN] = 0;
                                        if(strcmp(temp, RETC_FLAG) == 0)
                                        {
                                                bp += FLAG_LEN+2; // + 2, because ''\"'' and signle space '_' before content
                                                 if(strncmp(bp, CONTENT_FLAG, strlen(CONTENT_FLAG)) == 0)
                                                 {
                                                        int i = 0;
                                                        bp += strlen(CONTENT_FLAG);
                                                        while(bp[i] != '"')
                                                        {
                                                                i++;
                                                        }
                                                        strncpy(temp, bp, i);
                                                        temp[i] = 0;
                                                        *update_ret = atoi(temp);
                                                 }
                                                 else
                                                 {
                                                        ret = -1;
                                                        break;
                                                 }
                                        }
                                        else if(strcmp(temp, ADDR_FLAG) == 0)
                                        {
                                                bp += FLAG_LEN+2; // + 2, because ''\"'' and signle space '_' before content
                                                 if(strncmp(bp, CONTENT_FLAG, strlen(CONTENT_FLAG)) == 0)
                                                 {
                                                        int i = 0;
                                                        bp += strlen(CONTENT_FLAG);
                                                        while(bp[i] != '"')
                                                        {
                                                                i++;
                                                        }
                                                        strncpy(addr, bp, i);
                                                        addr[i] = 0;
                                                 }
                                                 else
                                                 {
                                                        ret = -1;
                                                        break;
                                                 }
                                        }
                                }
                                 else
                                 {
                                        ret = 0;
                                        break;
                                 }
                        }

                        break;
                default :
                        ret = -1;
                        break;
        }
        return ret;
}
/*
 * like "chomp" in perl, take off trailing newline chars
 */

int GNUDIP_HTTP_update_entry(void)
{
        char salt[64];
        char time[64];
        char sign[64];
        int update_ret = -1;
        char addr[16];
        int ret;
        ret = GNUDIP_first_request_and_parse(salt, time, sign);
        if(ret == 0)
        {
                GNUDIP_second_request_and_parse(salt, time, sign, &update_ret, addr);
        }
  switch(update_ret)
  {
    case 0:
     ret = UPDATERES_UPDATE_OK;
      break;

    case 1:// invalid login or domain name
      ret = UPDATERES_UPDATE_FAIL;
      break;

//    case 2: // successful off line
//      break;

    default:
    ret = UPDATERES_UPDATE_FAIL;
      break;
  }
  do_log(ret);
  return(ret);
}
//----------------------------------//
//-- END: GNUDIP -HTTP Protocol --//
//----------------------------------//
#endif  //_GNUDIP_DDNS_

static int get_status(char *status)
{
   FILE *fp;
   char tmp[40];
   int  id;

   fp = fopen("/tmp/ddns_status","rt");
   if (fp == NULL)
   {
        fp = fopen("/tmp/ddns_status","wt");
        if (fp == NULL)
                return 2;
        fprintf(fp,"0,0.0.0.0");
	strcpy(status,"0,0.0.0.0");
        fclose(fp);
        return 1;
   }
   else
   {
        fgets(tmp,40,fp);
        fclose(fp);
	strcpy(status,tmp);
	return 0;
   }
}
static int set_status(char *status)
{
   FILE *fp;
   char tmp[40];
   int  id;

   fp = fopen("/tmp/ddns_status","rt");
   if (fp == NULL)
   {
	fp = fopen("/tmp/ddns_status","wt");
	if (fp == NULL)
		return 1;
	fprintf(fp,"%s", status);
	fclose(fp);
	return 0;
   }
   else
   {
	fclose(fp);
	fp = fopen("/tmp/ddns_status","wt");
        if (fp == NULL)
                return 1;
	//printf("writing ..%s\n",status);
	fprintf(fp,"%s",status);
	fclose(fp);
	return 1;
   }
   fclose(fp);
   return 0;  
}

#ifdef  _SC_DDNS_MUST_UPDATE_AFTER_CGI_CHG_
int main(int argc, char *argv[])
#else
int main(void)
#endif
{
  int retval = 1;
  FILE *fd;
  time_t now;
  int need_update = 0;
  char ipaddr[20];
  char *paddr = ipaddr;
  char buf[40];
  char *pStr; 
  ddns_param_t  ddns_conf;

  // some variables initializations
  *user_pwd = '\0';
  *user_name = '\0';
  *password = '\0';  

  timeout.tv_sec = DEFAULT_TIMEOUT;
  timeout.tv_usec = 0;  


  //read_ddns_config(&ddns_conf);
  ddns_ReadConf(&ddns_conf);
  strcpy(server,&ddns_conf.server[0]);
  strcpy(user_name,&ddns_conf.account[0]);
  strcpy(password,&ddns_conf.password[0]);
  strcpy(host,&ddns_conf.hostname[0]);
  
  get_status(buf);
  pStr=strtok(buf,",");
  date = atoi(pStr);
  pStr=strtok(NULL,",");
  strcpy(address,pStr);

  parse_service(server);
  server = strdup(service->default_server);
  port = strdup(service->default_port);
  request = strdup(service->default_request);

#ifdef DEBUG
  printf("update_server:%s port:%s cgi=%s\n",server,port,request);
#endif

  // process auth word.
  if(provider == DDNS_GNUDIP_MYDYNIP){
	char *ptr = NULL;
	ptr = strstr(user_name,"@");	
	if(ptr != NULL)
	   memset(ptr,'\0',1);
  }
  sprintf(user_pwd, "%s:%s", user_name, password);
  base64Encode(user_pwd, auth);  // to generate the authentication word

  // obtain external ip address
  switch(provider){
	case DDNS_DP21:            
		IVY_check_ip(paddr);
	     break;
	case DDNS_ATDDNS: 		 
		ATDDNS_check_ip(paddr);
		break;
	default:
                retval=DYNDNS_check_ip(paddr);
                break;
  }	
  if(retval){
	do_log(UPDATERES_CONNECT_FAIL);
	return 1;	
  }
  //paddr="12.34.56.03"; 
  strcpy(ip_address, paddr); //WAN IP
#ifdef DEBUG
  printf("WLAN IP=%s (max interval=%d)\n",ip_address,service->default_max_interval);
#endif

  time(&now);
  //if time now is x days later of the last update time. 
  if( now > date+(service->default_max_interval)){
#ifdef DEBUG
	printf("need_update=1:later of the last update time\n");
#endif
  	need_update = 1;
   }

  //if ip changed 

  //Here we check the system.conf to read the outer_ipadd
  //and compare with the ip we obtain from DYNDNS_check_ip().
  if(strcmp(ip_address, address)){
#ifdef DEBUG
	printf("need_update=1:WLAN IP changed\n");
#endif
  	need_update = 1;
  }

#ifdef  _SC_DDNS_MUST_UPDATE_AFTER_CGI_CHG_
  if(argc > 1)
  {
	need_update = atoi(argv[1]);
  }
#endif

  if(need_update==0)
  {
	AddLog(SYSLOG_SECTION, SYSLOG_NORMAL, 5);
	retval= 0;
  }
  else if(need_update ==1)
  {
     if(service->update_entry() == UPDATERES_UPDATE_OK){
#ifdef DEBUG
	  printf("UPDATE_OK(%d)\n",UPDATERES_UPDATE_OK);
#endif
	// Here we change write cache file into updating system.conf
	// the column "outer_ipadd" will be modified to record the 
	// current ip address of gateway device.

	strncpy(&address[0],ip_address, 20);
	
	*buf = '\0';
	sprintf(buf,"%d",now);
	strcat(buf,",");
	strcat(buf,ip_address);
	set_status(buf);
      
	if(PRO_SetInt(SEC_MAN, MAN_DDNS_LAST_UPDATE,now,CONF_FILE) != DDNS_OK)
		return DDNS_ERROR;      	
        retval=0;
     }
#ifdef DEBUG
     else
          printf("UPDATE_Fail\n");
#endif	
  }

  if(ip_address) { free(ip_address); }
  if(port) { free(port); }
  if(request) { free(request); }
  if(server) { free(server); }

  return(retval);
}

