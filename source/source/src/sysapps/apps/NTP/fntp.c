#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <strings.h>
#include <syslog.h>
#include "fntp.h"
#include "ntp.h"
#include "rtc.h"
#include <timechg_api.h>


/* SNTP Protocol Packet Reference 
                    1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     0+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |LI | VN  |Mode |    Stratum    |     Poll      |   Precision   |
     4+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                          Root Delay                           |
     8+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                       Root Dispersion                         |
    12+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                     Reference Identifier                      |
    16+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                   Reference Timestamp (64)                    |
      |                                                               |
    24+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                   Originate Timestamp (64)                    |
      |                                                               |
    32+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                    Receive Timestamp (64)                     |
      |                                                               |
    40+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                    Transmit Timestamp (64)                    |
      |                                                               |
    48+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       -- Authentication parms removed since we don't support them --
*/

// Return codes from UDP receiver
#define TIMEOUT 	0
#define GOOD_RESPONSE   1
#define BAD_PACKET      2
#define CONNECT_BAD     3	
#define SYNC_OK	        4
#define SYNC_ERROR     	5

// Miscellaneou default option standard settings
#define DEFAULT_TIMEOUT 10			// Default RX timeout
#define DEFAULT_RETRY_TIME 15			// Default retry time of 15 seconds
#define DEFAULT_ITC 3600			// Default 1 hour between checks
#define DEFAULT_CLIENT_VERS 3			// Default v3
#define DEFAULT_MAX_STRATUM 15			// Default maximum stratum
#define DEFAULT_ABORT_ON_ALARM 0		// Default abort on alarm (0=no)
#define DEFAULT_ABORT_ON_ZERO 0			// Default abort on strat 0 (0=no)
	
unsigned char tx_pkt[SNTP_PACKET_SIZE];		// Last TX packet
unsigned char rx_pkt[SNTP_RX_BUFFER];		// Last RX packet

struct timeval t1, t2, t3, t4;			// Transit timestampts as described in RFC2030
						// NT1 = Time request sent by client (local clock)
						// NT2 = Time request received by server (server clock)
						// NT3 = Time reply sent by server (server clock)
						// NT4 = Time reply received by client (local clock)
struct timeval t_orig;				// Originate timestamp as returned by server
				
struct timezone tz;				// Timezone in case anyone wants it

int clientVers=DEFAULT_CLIENT_VERS;		// Default client version
int maxStratum=DEFAULT_MAX_STRATUM;		// Minimum Acceptible Stratum
int abortOnAlarm=DEFAULT_ABORT_ON_ALARM;	// Default for aborting on LI alarm indication
int abortOnStratZero=DEFAULT_ABORT_ON_ZERO;	// Default for aborting on Strat 0 unsynchronized indication

#define NTP_PORT	123
//#define TZ_NO	75
#define TZ_NO	76

enum{
    	NEEDNOT_DST,    	/* Need not daylight saving time */
    	NA_DST,         	/* North America need DST */
    	EU_DST,         	/* Europe DST */
    	CHILE_DST,		/* Chile dst */
    	SA_DST,			/* Sourth america,for example: Brazil */
    	IRAQ_DST,		/* Raq and Iran */
    	AU2_DST,		/* Australia - Tasmania */
    	AU3_DST,		/* New Zealand, Chatham */
    	AF_DST,			/* Egypt */
    	AU_DST         		/* Australia DST */
};

struct TZ_S{
	char str[80];
	float  num;
	int   dst;
};

static struct TZ_S TimeZone[TZ_NO]={
{"(GMT-12:00) International Date Line West",-12,NEEDNOT_DST},
{"(GMT-11:00) Midway Island, Samoa",-11,NEEDNOT_DST},
{"(GMT-10:00) Hawaii",-10,NEEDNOT_DST},
{"(GMT-09:00) Alaska",-9,NA_DST},
{"(GMT-08:00) Pacific Time (US &amp; Canada); Tijuana",-8,NA_DST}, //default tz
{"(GMT-07:00) Arizona",-7,NEEDNOT_DST},
{"(GMT-07:00) Chihuahua, La Paz, Mazatlan",-7,NA_DST},
{"(GMT-07:00) Mountain Time (US &amp; Canada)",-7,NA_DST},
{"(GMT-06:00) Central America",-6,NEEDNOT_DST},
{"(GMT-06:00) Central Time (US &amp; Canda)",-6,NA_DST},
{"(GMT-06:00) Guadalajara, Mexico City,Monterrey",-6,NA_DST},
{"(GMT-06:00) Saskatchewan",-6,NEEDNOT_DST},
{"(GMT-05:00) Bogota, Lima, Quito",-5,NEEDNOT_DST},
{"(GMT-05:00) Eastern Time (US &amp; Canda)",-5,NA_DST},
{"(GMT-05:00) Indiana (East)",-5,NEEDNOT_DST},
{"(GMT-04:00) Atlantic Time (Canada)",-4,NA_DST},
{"(GMT-04:00) La Paz",-4,NEEDNOT_DST},
{"(GMT-04:00) Santiago",-4,CHILE_DST},
{"(GMT-03:30) Newfoundland",-3.5,NA_DST},
{"(GMT-03:00) Brasilia",-3,SA_DST},
{"(GMT-03:00) Buenos Aires, Georgetown",-3,NEEDNOT_DST},
{"(GMT-03:00) Greenland",-3,EU_DST},
{"(GMT-02:00) Mid-Atlantic",-2,EU_DST},
{"(GMT-01:00) Azores",-1,EU_DST},
{"(GMT-01:00) Cape Verde Is.",-1,NEEDNOT_DST},
{"(GMT) Casablanca, Monrovia",0,NEEDNOT_DST},
{"(GMT) Greenwich Mean Time: Dublin,Edinburgh,Lisbon,London",0,EU_DST},
{"(GMT+01:00) Amsterdam, Berlin,Bern,Rome,Stockholm,Vienna",1,EU_DST},
{"(GMT+01:00) Belgrade, Bratislava,Budapest,Ljubljana,Prague",1,EU_DST},
{"(GMT+01:00) Brussels, Copenhagen, Madrid, Paris",1,EU_DST},
{"(GMT+01:00) Sarajevo, Skopje, Warsaw, Zagreb",1,EU_DST},
{"(GMT+01:00) West Central Africa",1,NEEDNOT_DST},
{"(GMT+02:00) Athens, Istanbul, Minsk",2,EU_DST},
{"(GMT+02:00) Bucharest",2,EU_DST},
{"(GMT+02:00) Cairo",2,AF_DST},
{"(GMT+02:00) Harare, Pretoria",2,NEEDNOT_DST},
{"(GMT+02:00) Helsinki, Kyiv, Riga, Sofia, Tallinn, Vilnius",2,EU_DST},
{"(GMT+02:00) Jerusalem",2,NEEDNOT_DST},
{"(GMT+03:00) Baghdad",3,IRAQ_DST},
{"(GMT+03:00) Kuwait, Riyadh",3,NEEDNOT_DST},
{"(GMT+03:00) Moscow, St. Petersburg, Volgograd",3,EU_DST},
{"(GMT+03:00) Nairobi",3,NEEDNOT_DST},
{"(GMT+03:30) Tehran",3.5,IRAQ_DST},
{"(GMT+04:00) Abu Dhabi, Muscat",4,NEEDNOT_DST},
{"(GMT+04:00) Baku, Tbilisi, Yerevan",4,EU_DST},
{"(GMT+04:30) Kabul",4.5,NEEDNOT_DST},
{"(GMT+05:00) Ekaterinburg",5,EU_DST},
{"(GMT+05:00) Islamabad, Karachi, Tashkent",5,NEEDNOT_DST},
{"(GMT+05:30) Chennai, Kolkata, Mumbai, New Delhi",5.5,NEEDNOT_DST},
{"(GMT+05:45) Kathmandu",5.75,NEEDNOT_DST},
{"(GMT+06:00) Almaty, Novosibirsk",6,EU_DST},
{"(GMT+06:00) Astana, Dhaka",6,NEEDNOT_DST},
{"(GMT+06:00) Sri Jayawardenepura",6,NEEDNOT_DST},
{"(GMT+06:30) Rangoon",6.5,NEEDNOT_DST},
{"(GMT+07:00) Bangkok, Hanoi, Jakarta",7,NEEDNOT_DST},
{"(GMT+07:00) Krasnoyarsk",7,EU_DST},
{"(GMT+08:00) Beijing, Chongqing, Hong Kong, Urumqi",8,NEEDNOT_DST},
{"(GMT+08:00) Irkutsk, Ulaan Bataar",8,EU_DST},
{"(GMT+08:00) Kuala Lumpur, Singapore",8,NEEDNOT_DST},
{"(GMT+08:00) Perth",8,NEEDNOT_DST},
{"(GMT+08:00) Taipei",8,NEEDNOT_DST},
{"(GMT+09:00) Osaka, Sapporo, Tokyo",9,NEEDNOT_DST},
{"(GMT+09:00) Seoul",9,NEEDNOT_DST},
{"(GMT+09:00) Yakutsk",9,EU_DST},
{"(GMT+09:30) Adelaide",9.5,AU_DST},
{"(GMT+09:30) Darwin",9.5,NEEDNOT_DST},
{"(GMT+10:00) Brisbane",10,NEEDNOT_DST},
{"(GMT+10:00) Canberra, Melbourne, Sydney",10,NEEDNOT_DST},
{"(GMT+10:00) Guam, Port Moresby",10,NEEDNOT_DST},
{"(GMT+10:00) Hobart",10,AU2_DST},
{"(GMT+10:00) Vladivostok",10,EU_DST},
{"(GMT+11:00) Magadan, Solomon Is., New Caledonia",11,NEEDNOT_DST},
{"(GMT+12:00) Auckland, Wellington",12,AU3_DST},
{"(GMT+12:00) Fiji, Kamchatka, Marshall Is.",12,NEEDNOT_DST},
{"(GMT+13:00) Nuku'alofa",13,NEEDNOT_DST},
{"(GMT-04:30) Caracas",-4.5,NEEDNOT_DST}
};
static int tzone = 4;

static int daylight_saving(time_t time) {
    	int 		dst_region=0; 
    	int 		last_left_day,last_left_hour; 
    	int 		current_year = 2000; 
 	int 		startDay;
 	int 		endDay;
 	int 		bSaveFlag = 0;
//	time_t 		t;
	struct tm 	*st;
	
//	t = time(NULL);
	st = localtime(&time);
	
    	current_year = 1900 + st->tm_year;
    	last_left_day = st->tm_yday;
	last_left_hour = st->tm_hour;
    	
	dst_region = TimeZone[tzone].dst;
    		
	switch(dst_region){
	        case NA_DST:
			//======================================================================
                	// Daylight Saving Time in the United States and Canada are changing. The
                	// period of Daylight Savings Time will be a total of 4 weeks longer than
                	// before - beginning 3 weeks earlier in Spring and ending 1 week later
                	// in the Fall.                                       (sync with wag325n)
		       {
				int is_leap_year=0;       
				if( (current_year%4==0 && current_year%100!=0) 
					||(current_year%100==0 && current_year%400==0))
					is_leap_year=1;
					startDay = 72 + is_leap_year - (current_year-2002 + (current_year-2004)/4 -2 )%7;										   
					endDay = 309 + is_leap_year - (current_year-2002 + (current_year-2004)/4 - 2)%7;				        
                 	}
  	        	//======================================================================
	            	break;
	        case EU_DST:
			{
			       	int is_leap_year=0;
				if( (current_year%4==0 && current_year%100!=0) 
					||(current_year%100==0 && current_year%400==0))
					is_leap_year=1;

	       	    		startDay = 90 + is_leap_year - (current_year - 2002 + ((current_year - 2000) / 4)) % 7;
	       	    		endDay = 304 +is_leap_year - (current_year - 1999 + ((current_year - 1996) / 4)) % 7;
			}
	            	break;      
	        case AU_DST:
	       	    	startDay = 304 - (current_year - 1999 + ((current_year - 1996) / 4)) % 7;
	       	    	endDay = 90 - (current_year - 2002 + ((current_year - 2000) / 4)) % 7;
	            	break;
	        case CHILE_DST:
	            	startDay = 287 - (current_year - 2000 + ((current_year - 2000) / 4)) % 7;
		    	endDay = 73 - (current_year - 1998 + ((current_year - 1996) / 4)) % 7;
		    	break;  
	        case SA_DST:
	            	startDay = 311 - (current_year-2004 + ((current_year - 2004) / 4)) % 7;
		    	endDay = 52 - (current_year - 1999 + ((current_year - 1996) / 4)) % 7;
		    	break;  
	        case AF_DST:
	            	startDay = 120 - (current_year - 2004 + ((current_year - 2004) / 4)) % 7;
		    	endDay = 273 - (current_year - 2004 + ((current_year - 2004) / 4)) %7;
		    	break;  
	        case IRAQ_DST:
	        	if (current_year%4 != 0){
	            		startDay = 91;
		    		endDay = 274;
		    	}else{
	            		startDay = 92;
		    		endDay = 275;
		    	}
		    	break;  
	        case AU2_DST:
	            	startDay = 280 - (current_year - 2001 + ((current_year - 2000) / 4)) % 7;
		    	endDay = 90 - (current_year - 2002 + ((current_year - 2000) / 4)) % 7;
		    	break;  
	        case AU3_DST:
	            	startDay = 280 - (current_year - 2001 + ((current_year - 2000) / 4)) % 7;
		    	endDay = 80 - (current_year - 1999 + ((current_year - 1996) / 4)) % 7;
		    	break;  
	        default:
	            break;                    
	}
	
	bSaveFlag = 0;	/* The other area can't use daylight saving, so default is 0 */
       	if(dst_region == AU_DST || dst_region == AU2_DST || dst_region == AU3_DST) {    /* AU in south */            
	   	if(current_year == 2006) {
	   		last_left_day++;
	   		if(dst_region == AU_DST)
		   		last_left_day += 2;	   			   	
	  	}
	
	  	if(current_year == 2007 && last_left_day+2 > endDay) {
	  	 	last_left_day++;	   	
	  	}

	  	if(last_left_day >= startDay || last_left_day <= endDay)
    			bSaveFlag = 1;
            	else
    			bSaveFlag = 0;
	} else if (dst_region == IRAQ_DST || dst_region == AF_DST || dst_region == SA_DST
			|| dst_region == CHILE_DST){
	    	if(last_left_day >= startDay-1 && last_left_day <= endDay-1)
			bSaveFlag = 1;
	    	else
	        	bSaveFlag = 0;
	} else if(dst_region == EU_DST) {
	    		if((last_left_day >= (startDay-1)) && (last_left_day <= (endDay-1)))
			{
				bSaveFlag = 1;
				if(((last_left_day == (startDay-1)) && (last_left_hour < 1)) || ((last_left_day == (endDay-1)) && (last_left_hour >= 1)))
				{
					bSaveFlag = 0;
				}
			}
			else {
				bSaveFlag = 0;
			}
	} else if (dst_region == NA_DST) {// GMT-8 --> USA & Canada
	  		if(current_year == 2005) {
				startDay = 92;
				endDay = 302;	
			} else if(current_year == 2006) {
				startDay = 91;
				endDay = 301;	
			} else {
				endDay += 1;	
			}

			
	    		if(last_left_day >= startDay && last_left_day <= endDay)
			{
				bSaveFlag = 1;
				if(((last_left_day == startDay) && (last_left_hour < 2)) || ((last_left_day == endDay) && (last_left_hour >= 2)))
				{
					bSaveFlag = 0;
				}
			}
			else {
				bSaveFlag = 0;
			}
	}

    	return  bSaveFlag;               
}


static time_t adjust_tz_time=0;

static void get_ntp_port(int *ntp_port){
	int ret;


	ret = PRO_GetInt(SEC_SYS, SYS_NTP_PORT, ntp_port);	
	if (ret != 0) {
		*ntp_port = NTP_PORT;
	}

	if((*ntp_port > 65535) || ((*ntp_port < 1024) && (*ntp_port != NTP_PORT)))
		*ntp_port = NTP_PORT;

}

static void get_timezone(void){
	int ret;

	ret = PRO_GetInt(SEC_SYS, SYS_TIME_ZONE, &tzone);	
	if (ret != 0) {
		adjust_tz_time = 0;
		return;
	}
	adjust_tz_time =(time_t)(TimeZone[tzone].num*3600);
}

static void get_daylightsaving(int* dsav){
	int ret;

	ret = PRO_GetInt(SEC_SYS, SYS_DAY_SAVING, dsav);	
	if (ret != 0) {
		*dsav = 0;
		return;
	}
}


static void convert(int range,u8 *hi,u8 *low){
	
	*hi=range/10;
	*low=range%10;
	return;
}

static int SETRTCTIME(void){

        time_t now;
	struct tm *now_tm;
	
	u8 rtc[16];
	u8 hi_byte,low_byte;
	

	time(&now);
	now_tm=localtime(&now);

	if(((now_tm->tm_year+1900) < 2000) || ((now_tm->tm_year+1900) >= 2038))
		return SYNC_ERROR;

	rtc[0]=2; /* year */
	rtc[1]=0;
	rtc[2]=0; /* week */
	rtc[3]=0;

	convert((now_tm->tm_year),&hi_byte,&low_byte);
	rtc[4]=hi_byte;
	rtc[5]=low_byte;

	convert((now_tm->tm_mon),&hi_byte,&low_byte);
	rtc[6]=hi_byte;
	rtc[7]=low_byte;

	convert((now_tm->tm_mday),&hi_byte,&low_byte);
	rtc[8]=hi_byte;
	rtc[9]=low_byte;

	convert((now_tm->tm_hour),&hi_byte,&low_byte);
	rtc[10]=hi_byte;
	rtc[11]=low_byte;

	convert((now_tm->tm_min),&hi_byte,&low_byte);
	rtc[12]=hi_byte;
	rtc[13]=low_byte;

	convert((now_tm->tm_sec),&hi_byte,&low_byte);
	rtc[14]=hi_byte;
	rtc[15]=low_byte;

//printf(" The RTC date %x%x%x%x / %x%x / %x%x , week %x%x, and RTC time %x%x:%x%x:%x%x\n",rtc[0],rtc[1],rtc[4],rtc[5],rtc[6],rtc[7],rtc[8],rtc[9],rtc[2],rtc[3],rtc[10],rtc[11],rtc[12],rtc[13],rtc[14],rtc[15]);

	set_RTC_time(rtc);
	timechg_stop();
	timechg_start();
#if 0
	PRO_SetInt(SEC_MAN, MAN_SUMMER_CHANGED, 0);	
	system("/usr/local/bin/sc_dst_setcron");
#endif
				
	return SYNC_OK;
}

// Adds x1 to x2 to give result
void tv_add(struct timeval *result, struct timeval *x1, struct timeval *x2)
{
	result->tv_sec=x1->tv_sec+x2->tv_sec;
	result->tv_usec=x1->tv_usec+x2->tv_usec;
	if (result->tv_usec>1000000l) {
		result->tv_usec-=1000000l;
		result->tv_sec++;
	}
	if (result->tv_usec<0) {
		result->tv_usec+=1000000l;
		result->tv_sec--;
	}
}

// Subtracts x2 from x1 to give result
void tv_sub(struct timeval *result, struct timeval *x1, struct timeval *x2)
{
	result->tv_sec=x1->tv_sec-x2->tv_sec;
	result->tv_usec=x1->tv_usec-x2->tv_usec;
	if (result->tv_usec>1000000l) {
		result->tv_usec-=1000000l;
		result->tv_sec++;
	}
	if (result->tv_usec<0) {
		result->tv_usec+=1000000l;
		result->tv_sec--;
	}
}

// Flips a long if an endian conversion is needed for use with the NTP packet
void endian_fix(uint32_t *data)
{
	uint32_t data2;
	data2=( (*data&0x000000FF)<<24 ) +
              ( (*data&0x0000FF00)<<8)  +
              ( (*data&0x00FF0000)>>8)  +
              ( (*data&0xFF000000)>>24);

	*data=data2;
}


// Convers an NTP 64 bit timestamp to timeval format
void ntp2timeval(struct timeval *tv, unsigned char *ntp)
{
	uint32_t sec, usec;
	memcpy(&sec, &ntp[0], 4);
	memcpy(&usec, &ntp[4], 4);
	endian_fix(&sec);
	endian_fix(&usec);
	usec=usec*NTP_OFFSET;
	sec-=EPOCH_OFFSET;
	tv->tv_usec=usec;
	tv->tv_sec=sec;
}

// Convers a timeval to 64 bit NTP timestamp
void timeval2ntp(struct timeval *tv, unsigned char *ntp)
{
	unsigned long sec, usec;

	sec=(unsigned long)tv->tv_sec+EPOCH_OFFSET;
	usec=(unsigned long)((double)tv->tv_usec/NTP_OFFSET);
	endian_fix((uint32_t *)&sec);
	endian_fix((uint32_t *)&usec);
	memcpy(&ntp[0], &sec, 4);
	memcpy(&ntp[4], &usec, 4);
}



// Sends NTP query packet
void send_query(int sock, struct sockaddr_in *sa) 
{
	unsigned char status;
	// Construct our NTP Packet
	bzero(&tx_pkt, 48);

	status=(clientVers<<3) | 0x03;	// Set version and client (3)

	tx_pkt[0]=status;

	gettimeofday(&t1, &tz);
	t1.tv_sec -=  adjust_tz_time;
	timeval2ntp(&t1, &tx_pkt[24]);	// Set originate timestamp
	timeval2ntp(&t1, &tx_pkt[40]);	// Set terminate timestamp

	// Send tx_pkt
	socklen_t len=sizeof(struct sockaddr);
	sendto(sock, tx_pkt, 48, 0, (struct sockaddr *) sa, len);
}

// Receives NTP response
int get_response(int sock, int timeout)
{
	fd_set rx;
	int fdmax;
	int retval;

	int bytes;
	uint32_t usecs;

	struct timeval to;
	to.tv_sec=timeout;
	to.tv_usec=0;

	// Receive the packet or time out
	do {
		FD_ZERO(&rx);
		FD_SET(sock, &rx);
		retval=select(sock+1, &rx, NULL, NULL, &to);
	} while (retval<0 && errno==EINTR);

	gettimeofday(&t4, NULL);		// Immediately get current time
	t4.tv_sec -=  adjust_tz_time;

	if (retval<0)
	{	 		// Unhandled error in select
		return CONNECT_BAD;
	}
	if (retval==0) return TIMEOUT;		// Timeout occurred in select
	
	// Read the packet data
	bytes=recvfrom(sock, rx_pkt, SNTP_RX_BUFFER, 0, NULL, NULL);
	if (bytes<48) return BAD_PACKET;	// Must be >=48 bytes

	// Grab the t_orig (originating comparison timestamp) and
	// t2 (Server RX TIme) and t3 (Server TX Time)
	ntp2timeval(&t_orig, &rx_pkt[24]);
	ntp2timeval(&t2, &rx_pkt[32]);
	ntp2timeval(&t3, &rx_pkt[40]);

	// Information about the source for debugging
	MYPRINTF("Incoming Packet Details:\n");
	int stratum=rx_pkt[1];
	MYPRINTF("Stratum reported is %d\n", stratum);
	int mode=rx_pkt[0]&0x06;
	MYPRINTF("Mode reported is %d\n", mode);
	int version=(rx_pkt[0]&0x38)>>3;
	MYPRINTF("Version reported is %d\n", version);
	int li=(rx_pkt[0]&0xc0)>>6;
	MYPRINTF("LI reported is %d\n", li);

	if (li==3 && abortOnAlarm) 
	{
		fprintf(stderr,"NTP Server Free Running\n");
		return CONNECT_BAD;
	}

	if (stratum==0 && abortOnStratZero) 
	{
		fprintf(stderr,"Stratum Zero (unsynced/unspecified)");
		return CONNECT_BAD;
	}

	if (stratum>maxStratum) 
	{
		fprintf(stderr,"Max allowable stratum exceeded");
		return CONNECT_BAD;
	}

	// Got a good packet
	return GOOD_RESPONSE;
}

// Main function
int NTP_update(char *serv)
{
	int i,port,dsav;
	struct timeval t4t1delta, t2t3delta,d;	// For calculating roundtrip delay (d)
	struct timeval t2t1delta, t3t4delta,tx2;// For calculating 2xlocal clock offset (tx2)
	struct timeval t;			// For calculating local clock offset (t)
	double tx2d, td;
	struct timeval now;			// Local timestamp
	struct timeval new;			// New time
	int retryTime=DEFAULT_RETRY_TIME;	// Retry Time Seconds
	int timeOut=DEFAULT_TIMEOUT;		// Timeout Seconds
	struct hostent *hptr;			// Hostname structure
	// Initialize socket
	struct sockaddr_in saDest;
	int sockfd;

	get_timezone();
	get_daylightsaving(&dsav);
	get_ntp_port(&port);

	if ((hptr=gethostbyname(serv))==NULL) 
	{
		fprintf(stderr,"Unknown or Bad IP/Hostname for -s (server) option");
	    	AddLog("NTP",SYSLOG_WARNING,4);
		return SYNC_ERROR;
	}

	bzero(&saDest, sizeof(saDest));
	saDest.sin_family = AF_INET;
	saDest.sin_port = htons(port);
	memcpy(&saDest.sin_addr, *(hptr->h_addr_list), sizeof(struct in_addr));
	if ((sockfd=socket(AF_INET, SOCK_DGRAM, 0))<0)
	{	 
		AddLog("NTP",SYSLOG_WARNING,2);
		return SYNC_ERROR;
	}

	// Endless time processing loop
	int result;
#ifdef _ONEDAY_
	for (i=0; i<3; i++) {
#endif
		// Send query and await response
		MYPRINTF("Sending Query\n");
		send_query(sockfd, &saDest);
		result=get_response(sockfd, timeOut);

		// We have a response
		if (result==TIMEOUT) {
			MYPRINTF("Timeout\n");
			sleep(retryTime);
		} else if (result==BAD_PACKET) {
			sleep(retryTime);
			MYPRINTF("Bad Response\n");
		} else if (result==CONNECT_BAD) {
			sleep(retryTime);
			MYPRINTF("Connect Bad\n");
		} else if (memcmp(&rx_pkt[24], &tx_pkt[24], 8)!=0) {
			// Failed to match originate timestamp in response against our
			// original request
			sleep(retryTime);
			MYPRINTF("Originate timestamp mismatch\n");
		} else {

			MYPRINTF("Response Received\n");
			// Calculate roundtrip delay and local offset
			MYPRINTF("T1   : %ld.%06ld (Client out)\n", t1.tv_sec, t1.tv_usec);
			MYPRINTF("T2   : %ld.%06ld (Server in)\n", t2.tv_sec, t2.tv_usec);
			MYPRINTF("T3   : %ld.%06ld (Server out)\n", t3.tv_sec, t3.tv_usec);
			MYPRINTF("T4   : %ld.%06ld (Client in)\n", t4.tv_sec, t4.tv_usec);
			tv_sub(&t4t1delta, &t4, &t1);
			tv_sub(&t2t3delta, &t2, &t3);
			tv_sub(&d, &t4t1delta, &t2t3delta);
			MYPRINTF("D    : %ld.%06ld (Roundtrip Delay)\n", d.tv_sec, d.tv_usec);
			tv_sub(&t2t1delta, &t2, &t1);
			tv_sub(&t3t4delta, &t3, &t4);
			tv_add(&t2, &t2t1delta, &t3t4delta);
			MYPRINTF("Tx2  : %ld.%06ld (Local Offset x2)\n", t2.tv_sec, t2.tv_usec);
			tx2d=(double)t2.tv_sec + ((double)t2.tv_usec/1000000.);
			MYPRINTF("Tx2d : %f (Local Offset x2 Intermediate Double)\n", tx2d);
			td=tx2d/2;
			MYPRINTF("Td   : %f (Local Offset Intermediate Double)\n", td);
			t.tv_sec=(int)td;
			t.tv_usec=(int) (1000000.*(td-(double)t.tv_sec));
			MYPRINTF("T    : %ld.%06ld (Local Offset)\n", t.tv_sec, t.tv_usec);
			MYPRINTF("RT   : %s\n", ctime(&t3));

			// If clock is >10 minutes out, we'll just do a gross adjustment to
			MYPRINTF("td:%f\n",td);
			// the TX time from the NTP server
			if (td> 10000.0 || td <-10000.0) {
				MYPRINTF("Gross Adjustment\n");
				t3.tv_sec +=  adjust_tz_time;
				if(dsav && daylight_saving(t3.tv_sec))
					t3.tv_sec += 3600; // daylight saving period
				if (settimeofday(&t3, NULL) || (SETRTCTIME() != SYNC_OK))
				{
					fprintf(stderr,"Unable to set clock\n");
#ifdef _ONEDAY_
					break;
#endif
				}
			        AddLog("NTP",SYSLOG_WARNING,1);
				close(sockfd);
				return SYNC_OK;
			} else {			
				MYPRINTF("Fine Adjustment\n");
				gettimeofday(&now, NULL);
				tv_add(&new, &now, &t);
				if(dsav && daylight_saving(new.tv_sec))
					new.tv_sec += 3600; // daylight saving period
				if (settimeofday(&new, NULL) || (SETRTCTIME() != SYNC_OK))
				{ 
					fprintf(stderr,"Unable to set clock");
#ifdef _ONEDAY_
					break;
#endif
				}
				AddLog("NTP",SYSLOG_WARNING,1);
				close(sockfd);
				return SYNC_OK;
			}
		}
#ifdef _ONEDAY_
	}
#endif

	if ((result==CONNECT_BAD) || (result==TIMEOUT)) 
		AddLog("NTP",SYSLOG_WARNING,2);
	else
		AddLog("NTP",SYSLOG_WARNING,3);
	
	close(sockfd);
	return SYNC_ERROR;
}
