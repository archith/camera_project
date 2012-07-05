///////////////////////////////////////////////////////////////////////////////
//
//	File name: AP.c
//	Version: 1.0
//	Date: 2005/4/30
//
//	Author: Bruce
//	Company: Faraday Tech. Corp.
//
//	Description: 1.This AP is the interface between user and OTG Driver 
//                  
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
//#include <linux/config.h>
//#include <linux/delay.h>
//#include <linux/list.h>
//#include <linux/timer.h>
//#include <asm/delay.h>
//#include <linux/delay.h>
#include <time.h>


#define OTG_AP_CMD_GET_STATE           1
#define OTG_AP_CMD_SET_HOST            2
#define OTG_AP_CMD_SET_IDLE            3
#define OTG_AP_CMD_SET_PERIPHERAL      4
                                       
#define OTG_AP_CMD_MDELAY_100          10
#define OTG_AP_CMD_TEST_MDELAY         11
#define OTG_AP_CMD_TEST_DUMP_REG       12
#define OTG_AP_CMD_TEST_SET_REG_ADD    13
#define OTG_AP_CMD_TEST_REG_WRITE      14


#define OTG_AP_CMD_TMP_FORCE_FULL      20
#define OTG_AP_CMD_TMP_FORCE_HIGH      21
#define OTG_AP_CMD_TMP_FORCE_CLEAN     22


static int fd=0;

enum usb_otg_state {
	OTG_STATE_UNDEFINED = 0,

	/* single-role peripheral, and dual-role default-b */
	OTG_STATE_B_IDLE,
	OTG_STATE_B_SRP_INIT,
	OTG_STATE_B_PERIPHERAL,

	/* extra dual-role default-b states */
	OTG_STATE_B_WAIT_ACON,
	OTG_STATE_B_HOST,

	/* dual-role default-a */
	OTG_STATE_A_IDLE,
	OTG_STATE_A_WAIT_VRISE,
	OTG_STATE_A_WAIT_BCON,
	OTG_STATE_A_HOST,
	OTG_STATE_A_SUSPEND,
	OTG_STATE_A_PERIPHERAL,
	OTG_STATE_A_WAIT_VFALL,
	OTG_STATE_A_VBUS_ERR,
};
//******************************************************************
// Name:otg_ap_sec_delay
// Description:main function
// Input:int argc , char *argv[]
// Output:int
//*****************************************************************
int otg_ap_sec_delay(int isecDelay)
{
  int  j1,j2,isec;
  
  j1=time((time_t *)NULL);
  isec=0;
  do {
  	j2=time((time_t *)NULL);
  	
  	if (difftime(j2,j1)>=1)
  	   {isec++;
  	    j1=j2;
  	   }
  	
  	}while(isec<isecDelay);
 return(0);

}	
//******************************************************************
// Name:otg_ap_state_string
// Description:state name reference
// Input:enum usb_otg_state state
// Output:char *
//        
//*****************************************************************
static const char *otg_ap_state_string(enum usb_otg_state state)
{
	switch (state) {
	case OTG_STATE_A_IDLE:		return "a_idle";
	case OTG_STATE_A_WAIT_VRISE:	return "a_wait_vrise";
	case OTG_STATE_A_WAIT_BCON:	return "a_wait_bcon";
	case OTG_STATE_A_HOST:		return "a_host";
	case OTG_STATE_A_SUSPEND:	return "a_suspend";
	case OTG_STATE_A_PERIPHERAL:	return "a_peripheral";
	case OTG_STATE_A_WAIT_VFALL:	return "a_wait_vfall";
	case OTG_STATE_A_VBUS_ERR:	return "a_vbus_err";
	case OTG_STATE_B_IDLE:		return "b_idle";
	case OTG_STATE_B_SRP_INIT:	return "b_srp_init";
	case OTG_STATE_B_PERIPHERAL:	return "b_peripheral";
	case OTG_STATE_B_WAIT_ACON:	return "b_wait_acon";
	case OTG_STATE_B_HOST:		return "b_host";
	default:			return "UNDEFINED";
	}
}


//******************************************************************
// Name:otg_ap_open_device
// Description:open the device 
// Input:none
// Output:others=>Fail
//        0=>ok
//*****************************************************************
int otg_ap_open_device(void)
{
  
 
  fd=open("/dev/FOTG2XX",O_RDWR);
  
  if (fd==-1)
     {
     printf("??? Open Device Fail...\n");	
     return -1;
    }


  return 0;
}




//******************************************************************
// Name:otg_ap_TEST_B_Auto_RoleChange
// Description:Auto Role Change for Device-A
// Input:void
// Output:others=>Fail
//        0=>ok
//   1   |----->3<X<30--------->  10    x<30       3               1
// idle -----> Peripheral -----> Host -----> Peripheralst -----> Idle
//*****************************************************************
int otg_ap_TEST_B_Auto_RoleChange(void)
{
 int iTimercounter,iTestCounter;
 int iInput; 
 
 iTestCounter=0;
 do {
     //Step1:Set to idle
       ioctl(fd,OTG_AP_CMD_SET_IDLE);
      
       
     //Step2:Waiting user issue the SRP
       printf(">>> AP:Device-B issue SRP...\n");
       otg_ap_sec_delay(1);
       ioctl(fd,OTG_AP_CMD_SET_HOST);

            
     //Step3:Waiting for State changing to Host mode
       printf(">>> AP:UUT-B delay 3 sec to Waite for State change...\n");
       otg_ap_sec_delay(3);
       iTimercounter=0;
       do {
     	  iTimercounter++;
       	  otg_ap_sec_delay(1);
       	   if (iTimercounter>30) //waiting 30 sec time out
       	       {
       	       	printf("??? AP:time out to enter OTG_STATE_B_HOST(30 sec.)\n");
       	       	return(1);
       	       	}
       	}while(ioctl(fd,OTG_AP_CMD_GET_STATE)!=OTG_STATE_B_HOST);
     
     //Step4:delay 3 sec to exit the host mode 
       printf(">>> AP:Device-B Wait 10 sec to exit Host mode\n");
       otg_ap_sec_delay(10);    
       ioctl(fd,OTG_AP_CMD_SET_IDLE);
       
     //Step5:Waiting for changing to peripheral mode           
       iTimercounter=0;
       do {
     	   iTimercounter++;
          otg_ap_sec_delay(1);
       	   if (iTimercounter>30) //waiting 30 sec time out
       	       {
       	       	printf("??? AP:time out to enter OTG_STATE_B_PERIPHERAL(30 sec.)\n");
       	       	return(1);
       	       	}

       	}while(ioctl(fd,OTG_AP_CMD_GET_STATE)!=OTG_STATE_B_PERIPHERAL);
            
     //Step6:Delay 5 sec to idle the Device
     
       otg_ap_sec_delay(5);
      
     iTestCounter++;
     printf("=======================>>> AP:Auto RoleChange PASS = %d...\n",iTestCounter);
       
     }while(iTestCounter<10);
     
     
  return(0);

}

//******************************************************************
// Name:otg_ap_TEST_A_Auto_RoleChange
// Description:Auto Role Change for Device-A
// Input:void
// Output:others=>Fail
//        0=>ok
//             X<10    5             8       x<30    1      
//        idle -----> Host -----> Peripheral -----> Host -----> Idle
//*****************************************************************
int otg_ap_TEST_A_Auto_RoleChange(void)
{
 int iTimercounter,iTestCounter;
 
 iTestCounter=0;
 do {

     //Step1.Set to idle
       ioctl(fd,OTG_AP_CMD_SET_IDLE);
     
     //Step2.Waiting for changing to Host Mode(SRP)
       printf(">>> AP:Waiting for change to Host State...\n");
       iTimercounter=0;
       do {
       	   
          otg_ap_sec_delay(1);
       	   iTimercounter++;
       	   if (iTimercounter>30) //waiting 30 sec for OPT to request role change
       	       {
       	       	printf("??? AP:Waiting for Role Change time out (30 sec.)\n");
       	       	return(1);
       	       	}
       	}while(ioctl(fd,OTG_AP_CMD_GET_STATE)==OTG_STATE_A_IDLE);
     
     //Step3.delay 5sec ,and set to idle (Change to Peripheral)(HNP-1)
       printf(">>> AP:delay 10 sec to enter idle mode(Issue HNP-1)...\n");
       otg_ap_sec_delay(10);
       ioctl(fd,OTG_AP_CMD_SET_IDLE);
       
       iTimercounter=0;
       do {

       	}while(ioctl(fd,OTG_AP_CMD_GET_STATE)!=OTG_STATE_A_PERIPHERAL);       
    
     //Step4.Waiting for Changing to Host mode again (HNP-2)
       printf(">>> AP:delay 5 sec to Wait for change to Host State(HNP-2)...\n");
       otg_ap_sec_delay(5);  
       iTimercounter=0;
       do {
     	   
           otg_ap_sec_delay(1);
       	   iTimercounter++;
       	   if (iTimercounter>30) //waiting 30 sec time out
       	       {
       	       	printf("??? AP:time out to enter OTG_STATE_A_HOST(30 sec.)\n");
       	       	return(1);
       	       	}
       	}while(ioctl(fd,OTG_AP_CMD_GET_STATE)!=OTG_STATE_A_HOST);
        
     //Step5.Delay 2 sec to start another test
       otg_ap_sec_delay(2);   
       
       iTestCounter++;
     printf("=========================>>> AP:Auto RoleChange PASS = %d...\n",iTestCounter);
  
  
   }while(iTestCounter<10);
     


  ioctl(fd,OTG_AP_CMD_SET_IDLE);
  
  return(0);

}


//******************************************************************
// Name:otg_ap_process_command
// Description:proocess the command input
// Input:char *pCmd,char *pCmd2
// Output:1=>Fail
//        0=>ok
//*****************************************************************
int otg_ap_process_command(char *pCmd)
{
  int iReturn;
  unsigned long u32Address,u32Data;
  switch(*pCmd)
  {
//--- For Operation ---
  	case 'i':
  	         iReturn=ioctl(fd,OTG_AP_CMD_SET_IDLE);
  	         if (iReturn==0)
  	             printf(">>> Set To idle Finish...\n");
  	         else
  	             printf("??? Set To idle Fail...\n");
  	 	
  	break;
  	case 'h':
  	         iReturn=ioctl(fd,OTG_AP_CMD_SET_HOST);
  	         if (iReturn==0)
  	             printf(">>> Set To Host Finish...\n");
  	         else
  	             printf("??? Set To Host Fail...\n");
  	
  	break;    	
  	case 's'://Get the status...
  	         iReturn=ioctl(fd,OTG_AP_CMD_GET_STATE);
                 if (iReturn==-1)
                    {
                    printf("??? ioctl Fail...\n");	
                    return -1;      	
                    }
                 else
                    printf(">>> OTG State = %s \n",otg_ap_state_string(iReturn));
  	            
  	         return(0);
  	break;  

  	case 'p':
  	         iReturn=ioctl(fd,OTG_AP_CMD_SET_PERIPHERAL);
  	         if (iReturn==0)
  	             printf(">>> Set To Peripheral Finish...\n");
  	         else
  	             printf("??? Set To Peripheral Fail...\n");
  	 	
  	break;



//--- For Certification ---
  	case 'A'://For Auto Role Change Test
                 if (ioctl(fd,OTG_AP_CMD_GET_STATE)==OTG_STATE_A_IDLE)
                    otg_ap_TEST_A_Auto_RoleChange();
                 else if (ioctl(fd,OTG_AP_CMD_GET_STATE)==OTG_STATE_B_IDLE)
                          otg_ap_TEST_B_Auto_RoleChange();
                      else printf("??? Error...Please enter the idle state first...\n");   
                      
                          

  	       return (0);	
  	break;    



//--- For DBG ---	
  	case 'm':
  	         iReturn=ioctl(fd,OTG_AP_CMD_TEST_MDELAY);
  	         if (iReturn==0)
  	             printf(">>> mdelay test Finish...\n");
  	         else
  	             printf("??? mdelay test Fail...\n");
  	       return (0);	
  	break;    

  	case 'd':
  	         iReturn=ioctl(fd,OTG_AP_CMD_TEST_DUMP_REG);
  	         if (iReturn==0)
  	             printf(">>> Dump Memory Finish...\n");
  	         else
  	             printf("??? Dump Memory Fail...\n");
  	       return (0);	
  	break;    
  	case 'W':
                 printf(">>> Please Input Address(0~0x300)=0x"); 
                 scanf("%x",&u32Address);
                 printf(">>> Please Input data(32 bit)=0x"); 
                 scanf("%x",&u32Data);

  	         iReturn=ioctl(fd,OTG_AP_CMD_TEST_SET_REG_ADD,u32Address);
  	         iReturn=ioctl(fd,OTG_AP_CMD_TEST_REG_WRITE,u32Data);

  	         if (iReturn==0)
  	             printf(">>> Write Memory Finish...\n");
  	         else
  	             printf("??? Write Memory Fail...\n");
  	       return (0);	
  	break;    




//--- For TEMP ---
  	case 'F':
  	         iReturn=ioctl(fd,OTG_AP_CMD_TMP_FORCE_FULL);
  	         if (iReturn==0)
  	             printf(">>> Force Full Speed Finish...\n");
  	         else
  	             printf("??? Force Full Speed Fail...\n");
  	       return (0);	
  	break;
  	case 'H':
  	         iReturn=ioctl(fd,OTG_AP_CMD_TMP_FORCE_HIGH);
  	         if (iReturn==0)
  	             printf(">>> Force High Speed Finish...\n");
  	         else
  	             printf("??? Force High Speed Fail...\n");
  	       return (0);	
  	break;  	  		

  	case 'C':
  	         iReturn=ioctl(fd,OTG_AP_CMD_TMP_FORCE_CLEAN);
  	         if (iReturn==0)
  	             printf(">>> Clean the 'Force Speed'...\n");
  	         else
  	             printf("??? Clean the 'Force Speed'...\n");
  	       return (0);	
  	break;  

  	case '?':
  	       printf("*** Faraday Linux OTG AP Help ***\n");
  	       printf("--- For Operation ---\n");
  	       printf("    <1>.'i' => Set to Idle (For A/B-Device)\n");  	
  	       printf("    <2>.'h' => Set to Host (For A/B-Device)\n");  	
  	       printf("    <3>.'s' => Get the State (For A/B-Device)\n");
  	       printf("    <4>.'p' => Set to Peripheral (For B-Device) \n");  
  	       printf("    <6>.'?' => Get the Help (For A/B-Device)\n");  
  	       printf("--- For Certification ---\n");
  	       printf("    <10>.'A' => TEST=>For AutoRoleChange 10 times(For A/B-Device)\n");  
  	       printf("--- For DBG ---\n");
  	       printf("    <20>.'m' => DBG=>Get the mdelay test (For A/B-Device)\n");    	       
  	       printf("    <21>.'d' => DBG=>Dump memory (For A/B-Device)\n");  	       
  	       printf("    <22>.'W' => DBG=>Write memory (For A/B-Device)\n");  	       
 	       
  	       printf("--- For TEMP ---\n");
  	       printf("    <30>.'F' => TMP=>Force to Full Speed(For A/B-Device)\n");  
  	       printf("    <31>.'H' => TMP=>Force to High Speed(For A/B-Device)\n");    	         	       
  	       printf("    <32>.'C' => TMP=>Clean the 'Force Speed'(For A/B-Device)\n");  
  	       return (0);	
  	break;    	

  	default:
  	       printf("??? Input Command not support...('p' => For Help...)\n");
               return(-1);
  	break;
  	
  	
  	}

  return(0);

}
//******************************************************************
// Name:main
// Description:main function
// Input:int argc , char *argv[]
// Output:int
//*****************************************************************
int  otg_ap_parsing(int *ptr,int *Result)
{
   int jj;
   int u32Counter=0;
   int  u8Temp;
  //Checking for 0x...
  if ((*ptr!=0x30)|(*(ptr+1)!=0x78))
      {
      printf("??? Input error\n");	
      return 0;
      }
  ptr=ptr+2;

  //Parsing the value input 

       	while(*(ptr)!=0)
       	   {
            u8Temp=*(ptr)-30;
            u32Counter=u32Counter*16+u8Temp;
       	   ptr++;
           };
       
       printf("Parsing Result = %d  \n",u32Counter);




}

//******************************************************************
// Name:main
// Description:main function
// Input:int argc , char *argv[]
// Output:int
//*****************************************************************
int main(int argc , char *argv[])
{


   if (argc!=2)
       {printf("??? Input Format Error...\n");
        printf(">>> Format = ./ap x (x => ? for help)\n");
       return (-1);
       }


 
  if (otg_ap_open_device())
     return -1;
  
  
  
   if (otg_ap_process_command(argv[1]))
       return(-1);
 



  return 0;
 	

}





