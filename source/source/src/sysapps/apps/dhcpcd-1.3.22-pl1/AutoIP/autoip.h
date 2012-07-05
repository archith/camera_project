/* This file is the interface exposed by the autoip*/

#ifndef _DHCP_AUTOIP__H
#define _DHCP_AUTOIP__H

#define NOTIFY_SCRIPT_PATH	"/scripts"
#define NOTIFY_SCRIPT_FILE	"/scripts/%s"
#define NOTIFY_STOP_SCRIPT	" stop &"
#define NOTIFY_START_SCRIPT " start &"
#define NOTIFY_GUAVA_SCRIPT_PATH      "/guava/scripts"
#define NOTIFY_GUAVA_SCRIPT_FILE      "/guava/scripts/%s"

void RunScript(char * Argument, u_int32_t WaitTime);

int Initialize(unsigned char *HwAddr, char *pIfName, int IfName_length, time_t NotifyTime);
int Deinitialize();
int StartAutoIP(int Sock, unsigned char bStartUp);
  
void StopAutoIP();	

int WaitInAutoIPBeforeLookforDHCP();

	    
#endif
