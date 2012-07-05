//#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/wait.h>			/* for waitpid function*/
#include <pthread.h>
#include <net/if.h>
#include <net/if_arp.h>
#ifdef __GLIBC__
#include <net/if_packet.h>
#else
#include <linux/if_packet.h>
#endif
#include <net/route.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <setjmp.h>
#include <resolv.h>
#include <time.h>
#include <dirent.h>
#include "autoip.h"
#include "autoipImpl.h"

#include <linux/version.h>


#include <netinet/in.h>
#include <arpa/inet.h>



#ifndef LINUX_VERSION_CODE
#error Unknown Linux kernel version
#endif

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,1)
#define OLD_LINUX_VERSION 1
#endif

#define DHCPCHECK_TIME 	300		//Wait time to look to for DHCP again
#define DHCP_CACHE_DIR	"/tmp/dhcpc"  //Cache directory

int Gratuitousarp(u_int32_t IPAddress);
int IPDefend(int IPAddressToCheck);
int SendARPtoCheckUniqueness(int IPAddressToProbe); 

static char			IfName[IFNAMSIZ];
static int			IfName_len;
static unsigned char		bAutoConfiguredIP = 0;
static unsigned char		bStartUP;
static u_int32_t		AutoConfiguredIPAddress = 0;
static unsigned char 	MAC_StationID[3];
static pthread_t		thread;
static unsigned char		bCollision = 0;
static unsigned char		bthread = 0;
static int	       	dhcpSocket;
static int			dhcpSocket2;
static time_t       		NotifyTimeOut;
static unsigned char		ClientHwAddr[ETH_ALEN];
static unsigned char		bInitialize = 0;
static unsigned char  		bAddressIsSet = 0;


/* This function is called as by the DHCP clinet to pass on information */
int Initialize(unsigned char *HwAddr, char *pIfName, int IfName_length, time_t NotifyTime)
{
	if (bInitialize)
		return 1;

	memcpy(ClientHwAddr, HwAddr, ETH_ALEN);
	strncpy(IfName, pIfName, IfName_length);	
	IfName_len = IfName_length;
	NotifyTimeOut = NotifyTime;
	bInitialize = 1;
	return 0;
}
/* Deinitialize function */
int Deinitialize()
{
	if ((!bInitialize) || (bAutoConfiguredIP))
		return 1;

	bInitialize = 0;
	return 0;
}
/* This function is called to start the AutoIP process. When called it calls
dhcpAutoConfigureIP function to configure the system with auto-ip
*/
int StartAutoIP(int Sock, unsigned char StartUp)

{
	if (!bInitialize)
	   return -1;
	dhcpSocket = Sock;
	bStartUP = StartUp;
	return dhcpAutoConfigureIP(); 
}

/* This function is called to stop the Auto-IP. It basically Destroys the thread snooping for IP collision */
void StopAutoIP()
{
	DestroyThread();
	bAutoConfiguredIP = 0;
	bCollision = 0;
}


/*****************************************************************************/
//This function finds an unique link local address, runs scripts with stop 
//argument and then assign the link local address to the interface card. 
//It also creates thread to detect IP collision. It also sends Gratuitous ARP 
//and runs the script with start argument.
/*******************************************/

int dhcpAutoConfigureIP()
{
  int 			i=0;
  u_int32_t 		IPAddress;
  struct ifreq	ifr;
  struct sockaddr_in	*p = (struct sockaddr_in *)&(ifr.ifr_addr);
  struct rtentry	rtent;
  int 			ret;
#ifdef OLD_LINUX_VERSION
  struct sockaddr_pkt	sap;
#endif


  if ((!bAutoConfiguredIP) || (bCollision))
  {
  	if (bStartUP == 0)
	    DestroyThread();
  		
	for(i=0; i< AUTOIP_RETRY; i++)
	{
	    	//Get the IP address from the file only first time or if there is no collision with the previous address
	    if ((i) || (bCollision) || (ReadFromAutoIPCacheFile(&IPAddress)))
		     IPAddress = GetNewIP();

	    if (!SendARPtoCheckUniqueness(IPAddress))
	        break;
	} 
  
	if (i < AUTOIP_RETRY)
	{	bAutoConfiguredIP = 1;
		//Create Thread to look for IPAddress collision.
		bCollision = 0;
		bAddressIsSet = 0;
		CreateThreadToLookForIPCollision(IPAddress);
 	        RunScript(NOTIFY_STOP_SCRIPT, NotifyTimeOut);
		
 	     syslog(LOG_INFO,"AutoConfiguring IP address: \n");
//	     dhcpStop();
//	     dhcpStart();
	     memset(&ifr,0,sizeof(struct ifreq));
  	     memcpy(ifr.ifr_name,IfName,IfName_len);
  	     p->sin_family = AF_INET;
  	     p->sin_addr.s_addr = IPAddress;
	     if ( ioctl(dhcpSocket,SIOCSIFADDR,&ifr) == -1 )  //setting IP address 
    	     {
	         syslog(LOG_ERR,"AutoConfigureIP: ioctl SIOCSIFADDR: %m\n");
	         return -1;
	     }
	     // Adding default routing
	    
	    bAddressIsSet = 1; 
            memset(&rtent,0,sizeof(struct rtentry));
            p                   =   (struct sockaddr_in *)&rtent.rt_dst;
            p->sin_family	     =	  AF_INET;
	     p->sin_addr.s_addr   =   0;
            p		            =	  (struct sockaddr_in *)&rtent.rt_genmask;
            p->sin_family	     =   AF_INET;
	     p->sin_addr.s_addr   =   0;
            rtent.rt_dev	     =	  IfName;
            rtent.rt_metric      =	  0;
	     rtent.rt_flags       =	  RTF_UP;
            if ( ioctl(dhcpSocket,SIOCADDRT,&rtent) )
	         syslog(LOG_ERR,"AutoConfigureIP: ioctl SIOCADDRT: %m\n");

	     // Adding routing for multicast
            memset(&rtent,0,sizeof(struct rtentry));
            p                  =   (struct sockaddr_in *)&rtent.rt_dst;
            p->sin_family	    =	  AF_INET;
	     ret = inet_aton(MULTICAST_DEST, &p->sin_addr.s_addr);
            p		           =	  (struct sockaddr_in *)&rtent.rt_genmask;
            p->sin_family	    =    AF_INET;
	     p->sin_addr.s_addr  =    htonl(0xFF000000);;
            rtent.rt_dev	     =	  IfName;
            rtent.rt_metric     =	  1;
	     rtent.rt_flags      =	  RTF_UP;
	     if (ret)
	     {
	            if ( ioctl(dhcpSocket,SIOCADDRT,&rtent) )
		         syslog(LOG_ERR,"AutoConfigureIP: ioctl SIOCADDRT: Failed to add multicast route %m\n");
	     }
	     else
	         syslog(LOG_ERR,"Failed to add multicast route"); 
		 
		  
  /* rebind dhcpSocket after changing ip address to avoid problems with 2.0 kernels */
#ifdef OLD_LINUX_VERSION
	     memset(&sap,0,sizeof(sap));
  	     sap.spkt_family = AF_INET;
            sap.spkt_protocol = htons(ETH_P_ALL);
            memcpy(sap.spkt_device,IfName,IfName_len);
  	    if ( bind(dhcpSocket,(void*)&sap,sizeof(struct sockaddr)) == -1 )
    		syslog(LOG_ERR,"bind: %m\n");
#endif  
	     Gratuitousarp(IPAddress);
	     sleep(1);
	     Gratuitousarp(IPAddress);
	     AutoConfiguredIPAddress = IPAddress; 
                RunScript(NOTIFY_START_SCRIPT, 0);
	     Write2AutoIPCacheFile(IPAddress);
	  if (bStartUP)
	     DestroyThread();
	}
	else
	     return -1;
  }
  return 0;
}
/**********************************************************/
//This function make the DHCP/Auto-IP wait for 5 Secs. When this function
//returns after timeout, DHCP should look for presence DHCP server in the
//network.During its 5 secs wait, it also keep looking for IP collision.
/*********************************************************/

int WaitInAutoIPBeforeLookforDHCP()
{
  u_int32_t  wait_time = 0;

  if ((bStartUP) && (!bCollision))
	  CreateThreadToLookForIPCollision(AutoConfiguredIPAddress);

  bStartUP = 0;	  

  while(wait_time < DHCPCHECK_TIME)
  {
	if (bCollision)
		return -1;
  	wait_time += 1;
  	sleep(1);	
  }
  syslog(LOG_INFO,"AutoConfigureIP: looking for DHCP again\n");
  
  return 0;
}

/*****************************************************************************/
//This function generates a new link local address. This function is called
// when the previouly used link local address is in use by some other node.
/*****************************************************************************/
u_int32_t GetNewIP(void)
{
	u_int32_t	nNewIP		= 0;
	u_short 	nHostID		=
	nHostID = START_ADDR + (u_short)(((float)END_ADDR)*rand()/(RAND_MAX+1.0));
 	nNewIP 	= htonl((NETWORK_ID << 16) | nHostID);

	return nNewIP;
}


/*****************************************************************************
//This function writes link local IP address in the cache.
*****************************************************************************/
int Write2AutoIPCacheFile(u_int32_t nValue)
{
	int nfd = -1;
	int nRetval = -1;
	char cache_file[255];
	
	nRetval = mkdir(DHCP_CACHE_DIR,S_IREAD|S_IWRITE);
	
	if ( (nRetval != -1) || (errno == EEXIST) )
	{
		sprintf(cache_file,AUTOIPD_CACHE_FILE,IfName);
		nfd = open(cache_file,O_WRONLY|O_CREAT);
	
		if (nfd != -1)
		{
			char szBuffer[32];
			
			memset(&szBuffer,0,sizeof(szBuffer));
			sprintf(szBuffer,"%d",nValue);
		
			if (write(nfd,szBuffer,sizeof(szBuffer)) > 0)
			{
				close(nfd);
				return 0;
			}
		}
	}
	return 1;	
}
/*****************************************************************************/
//This function read the previous link local address from the cache file. If 
//there is no cache file then it generates the IP address from its MAC address.
/*****************************************************************************/
int ReadFromAutoIPCacheFile(u_int32_t* pnValue)
{
	char 	cache_file[100];
	int 	nfd;
       u_short nHostID;
       int     i =0;
	u_char  temp;
  	struct ifreq	ifr;
	  
	sprintf(cache_file,AUTOIPD_CACHE_FILE,IfName);

	nfd = open(cache_file,O_RDONLY);
	
	if (nfd != -1)
	{
		char szBuffer[32];
		int nRead = 0;

		memset(&szBuffer,0,sizeof(szBuffer));
		
		if ((nRead = read(nfd,&szBuffer,sizeof(szBuffer))) > 0)
		{
			*pnValue = (u_int32_t)atoi((const char*)&szBuffer);
			close(nfd);
			return 0;
		}
		close(nfd);
	}
	else 
        {
	     	memset(&ifr,0,sizeof(struct ifreq));
	  	memcpy(ifr.ifr_name,IfName,IfName_len);
		if ( ioctl(dhcpSocket,SIOCGIFHWADDR,&ifr) )
		{
      			syslog(LOG_ERR,"ReadFromAutoIPCacheFile: ioctl SIOCGIFHWADDR: %m\n");
			return 1;
		}	
		memcpy(MAC_StationID, &ifr.ifr_hwaddr.sa_data[3], 3);  
		
		nHostID = MAC_StationID[2];
		    
		for(i=2; i>0; i--)
		{
		    if ((MAC_StationID[i-1] >= 1) && (MAC_StationID[i-1] <= 254))
		    {
			nHostID |= ((u_short)MAC_StationID[i-1]) << 8;
			break;
		    }
		}
		if (i == 0)
		{
			temp = ((u_short)MAC_StationID[1] + (u_short)(MAC_StationID[0])) /2;
			if ((temp < 1) || (temp > 254))
			     temp = 1;
			nHostID |= ((u_short)temp) << 8;	
		}
 		*pnValue 	= htonl((NETWORK_ID << 16) | nHostID);
		return 0;
	}
	return 1;	
}




/******************* *********************************************************/
//This function creates a child process and then run scripts in the
//background. The script is run before the IP address change with "stop"
//Argument and then it waits for WaitTime. Then funtion is called again after 
//the IP address is changed, with a "start" Argument.
/******************* *********************************************************/
void RunScript(char * Argument, u_int32_t WaitTime)
{
   pid_t			Childpid;

 
   Childpid = fork();

   if ( Childpid == 0 )
   {
// ¥i¥H¤£¶]¡H
   
//	RunScriptFiles(NOTIFY_SCRIPT_PATH, NOTIFY_SCRIPT_FILE, Argument);
//        RunScriptFiles(NOTIFY_GUAVA_SCRIPT_PATH, NOTIFY_GUAVA_SCRIPT_FILE, Argument);
	exit(0);
   }
   else
   {
       waitpid(Childpid, NULL, 0);
       if (WaitTime)
            sleep(WaitTime);
   }
}

/****************************************************************************/
//This function opens the directory and runs all the script files from the
//specfied directory.
/****************************************************************************/
void RunScriptFiles(const char *Directory, const char * FileFormat, char *Argument)
{	
   struct stat 		buf;
   struct dirent *	entry; 
   char 		FileName[255];
   DIR * 		dir;
	  
   dir = opendir(Directory);
   if (dir == NULL)
   {
#ifdef _DEBUG	
	    	syslog(LOG_ERR, "Failed to open the script directory %s, Err No:%d\n", FileName, errno);
#endif		
        return;
   }
   while((entry = readdir(dir)) != NULL)
   {
	sprintf(FileName, FileFormat, entry->d_name);
	stat(FileName, &buf);
	if (S_ISREG(buf.st_mode))
	{
	        strcat(FileName, Argument);
		syslog(LOG_INFO, "excuting %s \n", FileName);
		system(FileName);
        }
    } 
    closedir(dir);
} 
/******************* *********************************************************/
//This function creates a Thread to detect IP address conflict
/******************* *********************************************************/
void CreateThreadToLookForIPCollision(u_int32_t IPAddress)
{
    bthread = 1;
    if (pthread_create(&thread, NULL, WatchForIPCollision, (void *)IPAddress))
    {
    	syslog(LOG_ERR, "Failed to create thread to look for IP Collision\n");
	bthread = 0;
    }
}

/******************* *********************************************************/
//This is the thread function to detect IP address conflict. It creates a
//raw socket and keep looking for IP address conflict. If a collision is detected
//it sets the bCollision to true.
/******************* *********************************************************/
void * WatchForIPCollision(void *Param)
{
	u_int32_t	IPAddress = (u_int32_t) Param;
#ifdef _DEBUG	
	    syslog(LOG_ERR, "Looking for collision for %u.%u.%u.%u\n",
	    	((unsigned char *)&IPAddress)[0],
	    	((unsigned char *)&IPAddress)[1],
	    	((unsigned char *)&IPAddress)[2],
	    	((unsigned char *)&IPAddress)[3]);
#endif		
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	if (CreateSencondSocketForIPDefend())
		pthread_exit(0);

	if (!IPDefend(IPAddress))
	    bCollision = 1;

	syslog(LOG_INFO, "Exiting Thread\n");
	pthread_exit(0);
}

/******************* *********************************************************/
//Destroys the thread that was created to look for IP collision. It is called
//when StopAutoIP is called before it switches to DHCP mode.
/******************* *********************************************************/
void DestroyThread()
{
  if ((!bCollision) && (bthread))
  {
#ifdef _DEBUG	
  	    syslog(LOG_ERR, "Cancelling Thread\n");
#endif	    
	pthread_cancel(thread); 
  }
  pthread_join(thread, NULL);
  close(dhcpSocket2);
  bthread = 0;
}
/******************* *********************************************************/
//creates a raw socket to sniff on ARP packet to detect IP conflict
/******************* *********************************************************/
int CreateSencondSocketForIPDefend()
{
  int o = 1;
  struct ifreq	ifr;
  struct sockaddr_pkt sap;
  memset(&ifr,0,sizeof(struct ifreq));
  memcpy(ifr.ifr_name,IfName,IfName_len);
#ifdef OLD_LINUX_VERSION
  dhcpSocket2 = socket(AF_INET,SOCK_PACKET,htons(ETH_P_ALL));
#else
  dhcpSocket2 = socket(AF_PACKET,SOCK_PACKET,htons(ETH_P_ALL));
#endif
  if ( dhcpSocket2 == -1 )
    {
      syslog(LOG_ERR,"CreateSencondSocketForIPDefend: socket: %m\n");
      close(dhcpSocket2);
      return -1;
    }

  if ( ioctl(dhcpSocket,SIOCGIFHWADDR,&ifr) )
    {
      syslog(LOG_ERR,"CreateSencondSocketForIPDefend: ioctl SIOCGIFHWADDR: %m\n");
      close(dhcpSocket2);
      return -1;
    }
  if ( ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER )
    {
      syslog(LOG_ERR,"CreateSencondSocketForIPDefend: interface %s is not Ethernet\n",ifr.ifr_name);
      close(dhcpSocket2);
      return -1;
    }
  if ( setsockopt(dhcpSocket,SOL_SOCKET,SO_BROADCAST,&o,sizeof(o)) == -1 )
    {
      syslog(LOG_ERR,"CreateSencondSocketForIPDefend: setsockopt: %m\n");
      close(dhcpSocket2);
      return -1;
    }
  memset(&sap,0,sizeof(sap));
#ifdef OLD_LINUX_VERSION
  sap.spkt_family = AF_INET;
#else
  sap.spkt_family = AF_PACKET;
#endif
  sap.spkt_protocol = htons(ETH_P_ALL);
  memcpy(sap.spkt_device,IfName,IfName_len);
  if ( bind(dhcpSocket2,(void*)&sap,sizeof(struct sockaddr)) == -1 )
  {
    syslog(LOG_ERR,"CreateSencondSocketForIPDefend: bind: %m\n");
    close(dhcpSocket2);
  }
  return 0;
}

/*****************************************************************************/
//This function is called to send ARP probes. The main purpose is to make sure that
// the IP address is unique in the network. If it detects a collision then it will 
//return 1. And then a new address will be generated and checked again.
/*****************************************************************************/
int SendARPtoCheckUniqueness(int IPAddressToCheck) //Added IPAddressToCheck param
{
  arpMessage ArpMsgSend,ArpMsgRecv;
  struct sockaddr addr;
  int j,i=0;

  memset(&ArpMsgSend,0,sizeof(arpMessage));
  memcpy(ArpMsgSend.ethhdr.ether_dhost,MAC_BCAST_ADDR,ETH_ALEN);
  memcpy(ArpMsgSend.ethhdr.ether_shost,ClientHwAddr,ETH_ALEN);
  ArpMsgSend.ethhdr.ether_type = htons(ETHERTYPE_ARP);

  ArpMsgSend.htype	= htons(ARPHRD_ETHER);
  ArpMsgSend.ptype	= htons(ETHERTYPE_IP);
  ArpMsgSend.hlen	= ETH_ALEN;
  ArpMsgSend.plen	= 4;
  ArpMsgSend.operation	= htons(ARPOP_REQUEST);
  memcpy(ArpMsgSend.sHaddr,ClientHwAddr,ETH_ALEN);
  memcpy(&ArpMsgSend.tInaddr, &IPAddressToCheck, 4);
 
#ifdef _DEBUG	
  syslog(LOG_DEBUG,
    "broadcasting ARPOP_REQUEST for %u.%u.%u.%u\n",
    ArpMsgSend.tInaddr[0],ArpMsgSend.tInaddr[1],
    ArpMsgSend.tInaddr[2],ArpMsgSend.tInaddr[3]);
#endif    
  do
    {
      do
    	{
      	  if ( i++ > 4 ) return 0; /*  5 probes  */
      	  memset(&addr,0,sizeof(struct sockaddr));
      	  memcpy(addr.sa_data,IfName,IfName_len);
      	  if ( sendto(dhcpSocket,&ArpMsgSend,sizeof(arpMessage),0,
	   	&addr,sizeof(struct sockaddr)) == -1 )
	    {
	      syslog(LOG_ERR,"arpCheck: sendto: %m\n");
	      return -1;
	    }
    	}
      while ( Select(dhcpSocket,100000) ); /* 50 msec timeout */
      do
    	{
      	  memset(&ArpMsgRecv,0,sizeof(arpMessage));
      	  j=sizeof(struct sockaddr);
      	  if ( recvfrom(dhcpSocket,&ArpMsgRecv,sizeof(arpMessage),0,
		    (struct sockaddr *)&addr,&j) == -1 )
    	    {
      	      syslog(LOG_ERR,"arpCheck: recvfrom: %m\n");
      	      return -1;
    	    }
	  if ( ArpMsgRecv.ethhdr.ether_type != htons(ETHERTYPE_ARP) )
	    continue;
      	  if ( ArpMsgRecv.operation == htons(ARPOP_REPLY) )
	    {
#ifdef _DEBUG	
		      syslog(LOG_ERR,
	      "ARPOP_REPLY received from %u.%u.%u.%u for %u.%u.%u.%u\n",
	      ArpMsgRecv.sInaddr[0],ArpMsgRecv.sInaddr[1],
	      ArpMsgRecv.sInaddr[2],ArpMsgRecv.sInaddr[3],
	      ArpMsgRecv.tInaddr[0],ArpMsgRecv.tInaddr[1],
	      ArpMsgRecv.tInaddr[2],ArpMsgRecv.tInaddr[3]);
#endif	      
	    } // Also look for ARP probe from other host for the same IP address. 
      	  else if ((ArpMsgRecv.operation == htons(ARPOP_REQUEST)) && 
	  		memcmp(ArpMsgRecv.sHaddr,ClientHwAddr,ETH_ALEN) && 
			(!memcmp(&ArpMsgRecv.tInaddr, &IPAddressToCheck, 4)))
	    return 1;
	  else
	    continue;
// Do not need check the target address. Because if there is a ARP_REPLY for
//the same address then some system in the network is using the IP address
//we are trying assign to this one.
/*      	  if ( memcmp(ArpMsgRecv.tHaddr,ClientHwAddr,ETH_ALEN) )
	    {
	      if ( DebugFlag )	syslog(LOG_DEBUG,
	    	"target hardware address mismatch: %02X.%02X.%02X.%02X.%02X.%02X received, %02X.%02X.%02X.%02X.%02X.%02X expected\n",
	    	ArpMsgRecv.tHaddr[0],ArpMsgRecv.tHaddr[1],ArpMsgRecv.tHaddr[2],
	    	ArpMsgRecv.tHaddr[3],ArpMsgRecv.tHaddr[4],ArpMsgRecv.tHaddr[5],
	    	ClientHwAddr[0],ClientHwAddr[1],
	        ClientHwAddr[2],ClientHwAddr[3],
		ClientHwAddr[4],ClientHwAddr[5]);
	       continue;
	    }
*/	    
      	  if ( memcmp(&ArpMsgRecv.sInaddr,&IPAddressToCheck,4) )
	    {
#ifdef _DEBUG	
	    	syslog(LOG_DEBUG,
	    	"sender IP address mismatch: %u.%u.%u.%u received, %u.%u.%u.%u expected\n",
	    	ArpMsgRecv.sInaddr[0],ArpMsgRecv.sInaddr[1],ArpMsgRecv.sInaddr[2],ArpMsgRecv.sInaddr[3],
	    	((unsigned char *)&IPAddressToCheck)[0],
	    	((unsigned char *)&IPAddressToCheck)[1],
	    	((unsigned char *)&IPAddressToCheck)[2],
	    	((unsigned char *)&IPAddressToCheck)[3]);
#endif		
	      continue;
	    }
#ifdef _DEBUG	    
    	     syslog(LOG_ERR, "Failed to get the IP address %d", IPAddressToCheck);
      	     syslog(LOG_ERR,
	    	"sender IP address: %u.%u.%u.%u\n",
	    	ArpMsgRecv.sInaddr[0],ArpMsgRecv.sInaddr[1],ArpMsgRecv.sInaddr[2],ArpMsgRecv.sInaddr[3]);
           syslog(LOG_ERR,
	    	"sender MAC address: %u:%u:%u:%u:%u:%u:\n",
	    	ArpMsgRecv.ethhdr.ether_shost[0],
		ArpMsgRecv.ethhdr.ether_shost[1],
		ArpMsgRecv.ethhdr.ether_shost[2],
		ArpMsgRecv.ethhdr.ether_shost[3],
		ArpMsgRecv.ethhdr.ether_shost[4],
		ArpMsgRecv.ethhdr.ether_shost[5]);
#endif		
      	  return 1;
    	}
      while ( Select(dhcpSocket,100000) == 0 );
    }
  while ( 1 );
  return 0;
}

/*****************************************************************************/
// This function sends gratuitous ARP. 
/*****************************************************************************/
int Gratuitousarp(u_int32_t IPAddress)
{
  arpMessage ArpMsgSend;
  struct sockaddr addr;

  memset(&ArpMsgSend,0,sizeof(arpMessage));
  memcpy(ArpMsgSend.ethhdr.ether_dhost,MAC_BCAST_ADDR,ETH_ALEN);
  memcpy(ArpMsgSend.ethhdr.ether_shost,ClientHwAddr,ETH_ALEN);
  ArpMsgSend.ethhdr.ether_type = htons(ETHERTYPE_ARP);

  ArpMsgSend.htype	= htons(ARPHRD_ETHER);
  ArpMsgSend.ptype	= htons(ETHERTYPE_IP);
  ArpMsgSend.hlen	= ETH_ALEN;
  ArpMsgSend.plen	= 4;
  ArpMsgSend.operation	= htons(ARPOP_REQUEST);
  memcpy(ArpMsgSend.sHaddr,ClientHwAddr,ETH_ALEN);
  memcpy(ArpMsgSend.sInaddr,&IPAddress,4);
  memcpy(ArpMsgSend.tInaddr,&IPAddress,4);
 
  memset(&addr,0,sizeof(struct sockaddr));
  memcpy(addr.sa_data,IfName,IfName_len);
  if ( sendto(dhcpSocket,&ArpMsgSend,sizeof(arpMessage),0,
	      &addr,sizeof(struct sockaddr)) == -1 )
    {
      syslog(LOG_ERR,"arpAnouncement: sendto: %m\n");
      return -1;
    }
  return 0;
}
/*****************************************************************************/
// This function send a reply for a ARP probe.
/*****************************************************************************/

int SendReplyForArpProbe(u_int32_t IPAddress, void *TargetDeviceMACAddress)
{
  arpMessage ArpMsgSend;
  struct sockaddr addr;

  memset(&ArpMsgSend,0,sizeof(arpMessage));
  memcpy(ArpMsgSend.ethhdr.ether_dhost,TargetDeviceMACAddress,ETH_ALEN);
  memcpy(ArpMsgSend.ethhdr.ether_shost,ClientHwAddr,ETH_ALEN);
  ArpMsgSend.ethhdr.ether_type = htons(ETHERTYPE_ARP);
  ArpMsgSend.htype      = htons(ARPHRD_ETHER);
  ArpMsgSend.ptype      = htons(ETHERTYPE_IP);
  ArpMsgSend.hlen       = ETH_ALEN;
  ArpMsgSend.plen       = 4;
  ArpMsgSend.operation  = htons(ARPOP_REPLY);
  memcpy(ArpMsgSend.sHaddr,ClientHwAddr,ETH_ALEN);
  memcpy(ArpMsgSend.tHaddr,TargetDeviceMACAddress,ETH_ALEN); 
  memcpy(ArpMsgSend.sInaddr,&IPAddress,4);
  memset(&addr,0,sizeof(struct sockaddr));
  memcpy(addr.sa_data,IfName,IfName_len);
  if ( sendto(dhcpSocket,&ArpMsgSend,sizeof(arpMessage),0,
             &addr,sizeof(struct sockaddr)) == -1 )
  {
      syslog(LOG_ERR,"SendReplyForArpProbe: sendto: %m\n");
      return -1;
  }
  return 0;
}


/*****************************************************************************/
//This function is called to look for IP collision. And if a collision is detected 
//then the function will return.
/*****************************************************************************/
int IPDefend(int IPAddressToCheck) 
{
  arpMessage ArpMsgRecv;
  struct sockaddr addr;
  int j;
  
     
   do
   {
      while(Select(dhcpSocket2, -1));
      do
    	{
      	  memset(&ArpMsgRecv,0,sizeof(arpMessage));
      	  j=sizeof(struct sockaddr);
      	  if ( recvfrom(dhcpSocket2,&ArpMsgRecv,sizeof(arpMessage),0,
		    (struct sockaddr *)&addr,&j) == -1 )
    	    {
      	      syslog(LOG_ERR,"IPDefend: recvfrom: %m\n");
      	      return -1;
    	    }
	    
	  if ( ArpMsgRecv.ethhdr.ether_type != htons(ETHERTYPE_ARP) )
	    continue;
//If an packet is received from another sytem with the same IPaddress in the source field,
//then return address conflict. 
      	  if ( memcmp(ArpMsgRecv.sHaddr,ClientHwAddr,ETH_ALEN) && 
		memcmp(&ArpMsgRecv.sInaddr, &ArpMsgRecv.tInaddr, 4) &&	  
		(!memcmp(&ArpMsgRecv.sInaddr, &IPAddressToCheck, 4)))
	  {
	      syslog(LOG_ERR,
	    	"Address conflict with source hardware : %02X.%02X.%02X.%02X.%02X.%02X \n",
	    	ArpMsgRecv.sHaddr[0],ArpMsgRecv.sHaddr[1],ArpMsgRecv.sHaddr[2],
	    	ArpMsgRecv.sHaddr[3],ArpMsgRecv.sHaddr[4],ArpMsgRecv.sHaddr[5]);

	     return 0;
	  }

	  //If the address is not set then send a reply for ARP probe for the same address
	  if (memcmp(ArpMsgRecv.sHaddr,ClientHwAddr,ETH_ALEN) &&  (ArpMsgRecv.operation == htons(ARPOP_REQUEST))
	       && ((ArpMsgRecv.sInaddr == 0) || (!memcmp(&ArpMsgRecv.sInaddr, &ArpMsgRecv.tInaddr, 4))) && 
      	      (!memcmp(&ArpMsgRecv.tInaddr, &IPAddressToCheck, 4)) && (!bAddressIsSet))
	  {
	    syslog(LOG_INFO, "Sending Reply For Arp Request");  
	    SendReplyForArpProbe(IPAddressToCheck, ArpMsgRecv.sHaddr);
	  }  	
	    continue;
    	}
      while ( Select(dhcpSocket2, -1) == 0 );
  }while(1);
  return 1;
}

/*************************************************************/
//This calls select with or without a timeout. The main purpose is
// to wait for incoming data.
/************************************************************/
int Select(s,tv_usec)
int s;
time_t tv_usec;
{
  fd_set fs;
  struct timeval tv;

  FD_ZERO(&fs);
  FD_SET(s,&fs);
  if (tv_usec == -1)
  {
         if ( select(s+1,&fs,NULL,NULL,NULL) == -1 ) return -1;
  }
  else
  {
        tv.tv_sec=tv_usec/1000000;
        tv.tv_usec=tv_usec%1000000;
          if ( select(s+1,&fs,NULL,NULL,&tv) == -1 ) return -1;
  }
  if ( FD_ISSET(s,&fs) ) return 0;
  return 1;
}

