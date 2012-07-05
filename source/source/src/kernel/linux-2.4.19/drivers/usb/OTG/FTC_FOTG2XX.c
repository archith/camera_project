/*
 * Copyright (c) 2005 by Faraday
 *  
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/devfs_fs_kernel.h>

//--For USB include
#include <linux/usb_ch9.h>
#include <linux/usb_gadget.h>
#include <linux/usb_otg.h>
//--For asm include
#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h> 
#include <asm/pci.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <asm/arch/cpe_int.h>

//--For OTG include
#include "FTC_FOTG2XX.h"
#include "../gadget/FTC_FOTG200_udc.h"
//--For usb include change between Linux2.4.18-USB and Linux2.6.10-USB
#define FOTG2XX_AVOID_REDEFINE 
#include <linux/usb.h>

//--Function pre define
static int OTGH_host_quick_Reset(void);
void OTGP_Close(void);
void OTGH_Close(void);
void OTGP_Open(void);
void OTGH_Open(void);
void OTGC_A_PHY_Reset(void);
int OTGC_Waiting_VBUS_On(u32 bmsec);
int OTGC_Waiting_VBUS_Off(u32 bmsec);
static void OTGC_enable_vbus_draw(u8 btype);
void OTG_RoleChange(void);
void OTGC_Init(void);
static int OTGC_AP_ioctl(struct inode * inode, struct file * file,unsigned int cmd,unsigned long arg);
int Host_Disconnect_for_OTG(struct usb_bus *bus, unsigned port_num);
static int FOTG2XX_start_hnp(struct otg_transceiver *dev);


//--Driver & Module Info
#define	DRIVER_DESC		"FOTG2XX USB Controller"
#define	DRIVER_VERSION	        "05-March 2005"
static const char driver_name [] = "FTC_FOTG2XX";
static const char driver_desc [] = DRIVER_DESC;
MODULE_AUTHOR("bruce@faraday-tech.com");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
static struct file_operations otg_fops = {
	owner:		THIS_MODULE,
	ioctl:		OTGC_AP_ioctl,
	
};

#define OTG_MAJOR     42

//--Global Variable Definition
static struct FTC_OTGC_STS      *pFTC_OTG=0;
static struct otg_transceiver *xceiv;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//---------------------- Group-1:Basic Function -------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static const char *OTGC_state_string(enum usb_otg_state state)
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


//*********************************************************
// Name: OTGH_host_quick_Reset 
// Description://For HNP critical timing 
//           OTG need the quickly-bus-reset
// Input:void
// Output:0:OK    
//********************************************************* 
static int OTGH_host_quick_Reset(void)
{
 
  //<1>.Issue Port Reset
        mwHost20_PORTSC_PortReset_Set();
  
 
  //<2>.Delay 55 ms
        mdelay(55);

  //<3>.Stop 
         mwHost20_PORTSC_PortReset_Clr();

  //<4>.Waiting for Reset complete
        while(mwHost20_PORTSC_PortReset_Rd()==1);
  
  //<5>.Issue SOF
        mbHost20_USBCMD_RunStop_Set();
        
  DBG_OTG_TRACEC("-OTGH_host_quick_Reset()(0x30=0x%x)\n",mdwFOTGPort(0x30));

  
  return (0);

}



//*********************************************************
// Name: OTGP_Close
// Description:
//         Disable Interrupt for Peripheral
// Input:void
// Output:void    
//********************************************************* 
void OTGP_Close(void)
{
	u32 wTemp;

    DBG_OTG_FUNCC("+OTGP_Close()\n");
	
   //<1>.Clear All the Interrupt
	mUsbGlobIntDis();
	mdwOTGC_GINT_MASK_PERIPHERAL_Set();

   //<2>.Clear all the Interrupt Status
	wTemp=mUsbIntGroupRegRd();
	mUsbIntGroupRegSet(0);

	//Interrupt source group 0(0x144)
	wTemp=mUsbIntSrc0Rd();
	mUsbIntSrc0Set(0);

	//Interrupt source group 1(0x148)
	wTemp=mUsbIntSrc1Rd();
	mUsbIntSrc1Set(0);

	//Interrupt source group 2(0x14C)
	wTemp=mUsbIntSrc2Rd();
	mUsbIntSrc2Set(0);

   //<3>.Turn off D+
   if (mdwOTGC_Control_CROLE_Rd()>0)//For Current Role = Peripheral
         mUsbUnPLGSet();


   //<4>.Clear the variable
	pFTC_OTG->otg.gadget->b_hnp_enable = 0;
	pFTC_OTG->otg.gadget->a_hnp_support = 0;
	pFTC_OTG->otg.gadget->a_alt_hnp_support = 0;
        
 
    

}
//*********************************************************
// Name: OTGH_Close
// Description:
//         Disable Interrupt for Host
// Input:void
// Output:void    
//********************************************************* 
void OTGH_Close(void)
{
	u32 wTemp;

    DBG_OTG_FUNCC("+OTGH_Close()(0x30=0x%x)\n",mdwFOTGPort(0x30));	

  //<1>.Enable Interrupt Mask
    mdwOTGC_GINT_MASK_HOST_Set();
  
  //<2>.Clear the Interrupt status
    wTemp=mdwHost20_USBINTR_Rd();
    wTemp=wTemp&0x0000003F;
    mdwHost20_USBSTS_Set(wTemp);
   
   

}
//*********************************************************
// Name: OTGP_Open
// Description:
//          open Peripheral Interrupt
// Input:void
// Output:void    
//********************************************************* 
void OTGP_Open(void)
{
    DBG_OTG_FUNCC("+OTGP_Open()\n");	
  
  //<1>.Turn On the Interrupt for Peripheral
     mUsbGlobIntEnSet();
     mdwOTGC_GINT_MASK_PERIPHERAL_Clr();     
     mUsbUnPLGClr();

}
//*********************************************************
// Name: OTGH_Open
// Description:
//          open Host Interrupt
// Input:void
// Output:void    
//********************************************************* 
void OTGH_Open(void)
{

    DBG_OTG_FUNCC("+OTGH_Open()(0x30=0x%x)\n",mdwFOTGPort(0x30));	
  //<1>.Turn On the Interrupt
    mdwOTGC_Control_A_SRP_DET_EN_Clr();
    mdwOTGC_GINT_MASK_HOST_Clr();


}

#define OPEN_PHY_RESET

#ifdef OPEN_PHY_RESET
//*********************************************************
// Name: OTGC_A_PHY_Reset
// Description:
//          Phy Reset
// Input:void
// Output:void    
//********************************************************* 
void OTGC_A_PHY_Reset(void)
{
   DBG_OTG_FUNCC("+OTGC_A_PHY_Reset()\n");	
   printk("+OTGC_A_PHY_Reset()\n");	

   do{
     	mdelay(10);
   
   }while(mdwOTGC_Control_B_SESS_END_Rd()==0);
   mdwOTG20_Control_Phy_Reset_Set();
   mdelay(5);
   mdwOTG20_Control_Phy_Reset_Clr();
   printk("OTG Controller Reset...()\n");   
   mdwOTG20_Control_OTG_Reset_Set();
   mdelay(5);
   mdwOTG20_Control_OTG_Reset_Clr();
}
#endif

//*********************************************************
// Name: OTGC_Waiting_VBUS_On
// Description:
//           Waiting for VBUS On
// Input:bmsec => wait bmsec time
// Output: 0 => ok      
//********************************************************* 
int OTGC_Waiting_VBUS_On(u32 bmsec)
{
  u8 bExitFlag=0;
  u32 wTimer_ms=0;

   DBG_OTG_FUNCC("+OTGC_Waiting_VBUS_On()\n");	

  do{
      if (mdwOTGC_Control_A_VBUS_VLD_Rd()>0)   
          bExitFlag=1;
    
    	 mdelay(1);//delay 1ms
    	 wTimer_ms++;
    	 if (wTimer_ms>bmsec)   
    	    {                                         
    	     bExitFlag=2; 
    	     ERROR(pFTC_OTG,"??? Time out for waiting for VBUS On...\n");
    	     return(1);//Time out
    	    }         
  
  }while(bExitFlag==0);   

 return(0);

}
//*********************************************************
// Name: OTGC_Waiting_VBUS_Off
// Description:
//           Waiting for VBUS OFF
// Input:bmsec => wait bmsec time
// Output: 0 => ok      
//********************************************************* 
int OTGC_Waiting_VBUS_Off(u32 bmsec)
{
  u8 bExitFlag=0;
  u32 wTimer_ms=0;
   DBG_OTG_FUNCC("+OTGC_Waiting_VBUS_Off()\n");	

  do{
      if (mdwOTGC_Control_A_VBUS_VLD_Rd()==0)   
          bExitFlag=1;
    
    	 mdelay(1);//delay 1ms
    	 wTimer_ms++;
    	 if (wTimer_ms>bmsec)   
    	    {                                         
    	     bExitFlag=2; 
    	     ERROR(pFTC_OTG,"??? Time out for waiting for VBUS Off...\n");
    	     return(1);//Time out
    	    }         
  
  }while(bExitFlag==0);   

 return(0);

}
//*********************************************************
// Name: OTGC_enable_vbus_draw
// Description:
//            Drive/Drop VBUS      
// Input:0=>Drop VBUS
//       1=>Drive VBUS  
// Output: 0 => ok      
//********************************************************* 
static void OTGC_enable_vbus_draw(u8 btype)
{
   DBG_OTG_FUNCC("+(OTGC_enable_vbus_draw(btype=%d))\n",btype);	


  if (btype>0)
     {//Drive VBUS
      if (mdwOTGC_Control_A_VBUS_VLD_Rd()>0)
          return ;
      mdwOTGC_Control_A_BUS_DROP_Clr(); //Exit when Current Role = Host
      mdwOTGC_Control_A_BUS_REQ_Set();
      OTGC_Waiting_VBUS_On(5000);
      INFO(pFTC_OTG,">>> Drive VBUS ok...\n");
     }
  else
     {//Drop VBUS
      if (mdwOTGC_Control_A_VBUS_VLD_Rd()==0)
          return ;
      mdwOTGC_Control_A_BUS_REQ_Clr();
      mdwOTGC_Control_A_BUS_DROP_Set();  
      OTGC_Waiting_VBUS_Off(5000);    
      INFO(pFTC_OTG,">>> Drop VBUS ok...\n");
     }
 
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//---------------------- Group-2:AP Function ----------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


//*********************************************************
// Name: OTGC_AP_Get_State
// Description: AP will get the OTG State from OTG Driver
// Input:void
// Output:otg.state
//********************************************************* 
static int OTGC_AP_Get_State(void)
{
  DBG_OTG_FUNCC("+OTGC_AP_Get_State() (pFTC_OTG->otg.state=%d)\n",pFTC_OTG->otg.state);		

  return((int)(pFTC_OTG->otg.state));

}

//*********************************************************
// Name: OTGC_AP_Set_to_Idle
// Description: AP will call this function call to set device to idle
// Input:void
// Output:0:OK
//        others:Fail    
//********************************************************* 
static u8 OTGC_AP_Set_to_Idle(void)
{
  DBG_OTG_FUNCC("+OTGC_AP_Set_to_Idle()\n");		


  pFTC_OTG->otg.host->A_Disable_Set_Feature_HNP=1;

  if (mdwOTGC_Control_ID_Rd()==0)
     {
     	//For A Device
       if (pFTC_OTG->otg.state==OTG_STATE_A_HOST)
     	    {

	       if (pFTC_OTG->otg.host->b_hnp_enable)
 	            {//For => A-Host & HNP Enable ==>Then Role change to Peripheral Mode
 	            INFO(pFTC_OTG,">>> call otg_start_hnp\n"); 
 	            FOTG2XX_start_hnp(xceiv);
 	            return(0); 
 	            }
     	       else{//For Normale A-Host-Idle
     	            Host_Disconnect_for_OTG(pFTC_OTG->otg.host,0);
     	            mdwOTGC_Control_A_BUS_REQ_Clr();//Turn On VBUS
     	            mdwOTGC_Control_A_BUS_DROP_Set();
     	            mdwOTGC_Control_A_SRP_DET_EN_Set();
                    while(mdwOTGC_Control_A_VBUS_VLD_Rd()>0);
                    pFTC_OTG->otg.state=OTG_STATE_A_IDLE;
                    return(0); 
                    }
     	    }
       if (pFTC_OTG->otg.state==OTG_STATE_A_IDLE)
     	    {
     	    INFO(pFTC_OTG,">>> Device-A is already in Idle mode\n");
     	    return(0);
     	    }
     	 else{
     	 INFO(pFTC_OTG,">>> Set to Idle Fail...(Current State = %s)\n",OTGC_state_string(pFTC_OTG->otg.state));
     	  return(-1);
     	 }
     	 return(0);

     }else
     {

     	//For B Device 
     	 if (pFTC_OTG->otg.state==OTG_STATE_B_HOST)
     	    {//Change to peripheral==>Host disconnect 
     	    //printk("TEMP==> B exit the host mode...\n");
     	    Host_Disconnect_for_OTG(pFTC_OTG->otg.host,0);

            // pFTC_OTG->otg.state=OTG_STATE_B_IDLE;     	    
     	    return(0);     	    	
     	    }
     	if (mdwOTGC_Control_A_VBUS_VLD_Rd()>0)
     	   {
     	   ERROR(pFTC_OTG,"??? Can not set Device-B to idle state(When VBUS>0 )...\n");	
     	   return(-1);
     	   }
     	     
     	if (pFTC_OTG->otg.state==OTG_STATE_B_IDLE)
     	   {INFO(pFTC_OTG,">>> Device-B is already in Idle State... \n");
     	    return(0);
     	   }
     	if (pFTC_OTG->otg.state==OTG_STATE_B_PERIPHERAL)
           {
            OTGP_Close();
            mdwOTGC_Control_B_BUS_REQ_Clr();
            mdwOTGC_Control_B_HNP_EN_Clr();
            mdwOTGC_Control_B_DSCHG_VBUS_Clr();
            pFTC_OTG->otg.state=OTG_STATE_B_IDLE; 
     	    return(0);           	
           	
           }
        else{
     	     INFO(pFTC_OTG,">>>  Set to Idle Fail...(Current State = %s)\n",OTGC_state_string(pFTC_OTG->otg.state));
    	    
     	    return(-1);

               }

     }
  return(0);

}

//*********************************************************
// Name: OTGC_AP_Set_to_Host
// Description: AP will call this function call to set device to Host
// Input:void
// Output:0:OK
//        others:Fail    
//********************************************************* 
static u8 OTGC_AP_Set_to_Host(void)
{
  DBG_OTG_FUNCC("+OTGC_AP_Set_to_Host()\n");		
  pFTC_OTG->otg.host->A_Disable_Set_Feature_HNP=1;
  if (mdwOTGC_Control_ID_Rd()==0)
     {

     //For A Device
     if (pFTC_OTG->otg.state==OTG_STATE_A_IDLE)	
     	{ 
     	   mdwOTGC_Control_A_BUS_DROP_Clr(); 
     	   mdwOTGC_Control_A_BUS_REQ_Set();//Turn On VBUS
     	   mdwOTGC_Control_A_SRP_DET_EN_Clr();
           while(mdwOTGC_Control_A_VBUS_VLD_Rd()==0);     	   
           pFTC_OTG->otg.state=OTG_STATE_A_HOST;     	   
     	   INFO(pFTC_OTG,">>> Set to Host Finish...\n");
     	   OTGH_Open();
        return(0);     	   
     	}
     if (pFTC_OTG->otg.state==OTG_STATE_A_HOST)	
     	{ 
     	 INFO(pFTC_OTG,">>> Device-A is already in the Host Mode...\n");
        return(0);
     	}     	
     if (pFTC_OTG->otg.state==OTG_STATE_A_PERIPHERAL)	
     	{ 
     	 INFO(pFTC_OTG,">>> Set to Host Fail...Device-A is PERIPHERAL Mode(Bus control by B-Device)...\n");
         return(-1);
     	}     	
     else
        {
     	 INFO(pFTC_OTG,">>> Set to Host Fail...(Current State = %s)\n",OTGC_state_string(pFTC_OTG->otg.state));
         return(-1);
        }



     	 
     }else
     {
     	//For B Device 
      if (pFTC_OTG->otg.state==OTG_STATE_B_IDLE)
     	 {
          //Step1:
     	  pFTC_OTG->otg.gadget->ops->wakeup((pFTC_OTG->otg.gadget)); //Issue SRP/HNP

     	  return(0);
     	 }
      if (pFTC_OTG->otg.state==OTG_STATE_B_HOST)
     	 {
     	 INFO(pFTC_OTG,">>> Device-B is already in the Host Mode...\n");
     	  return(0);
     	 }
      if (pFTC_OTG->otg.state==OTG_STATE_B_PERIPHERAL)
     	 {
     	 
     	 if (OTGC_AP_Set_to_Idle()==0)
  	     pFTC_OTG->otg.gadget->ops->wakeup((pFTC_OTG->otg.gadget)); //Issue SRP/HNP
         else
     	    INFO(pFTC_OTG,">>> Set to Host Fail...Device-B is PERIPHERAL Mode(Bus control by A-Device)...\n");
     	  return(0);
     	 }
      else 	 
     	{ 
     	 ERROR(pFTC_OTG,"??? Set to Host Fail...(Current State = %s)\n",OTGC_state_string(pFTC_OTG->otg.state));
     	 return(1);
     	}
     
     	
     }



  return(0);

}

//*********************************************************
// Name: OTGC_AP_Set_to_Peripheral
// Description: AP will call this function call to set device to Peripheral
// Input:void
// Output:0:OK
//        others:Fail    
//********************************************************* 
static u8 OTGC_AP_Set_to_Peripheral(void)
{
  DBG_OTG_FUNCC("+OTGC_AP_Set_to_Peripheral()\n");		

  if (mdwOTGC_Control_ID_Rd()==0)
     {
     //For A Device
   
     	   ERROR(pFTC_OTG,"??? Cannot Set Device-A to Peripheral mode...\n");
        return(0);     	   


    	 
     }else
     {
     	//For B Device 
      if (pFTC_OTG->otg.state==OTG_STATE_B_IDLE)
     	 {
     	  OTGP_Open();
    	  return(0);
     	 }
     else      	
     	{ 
     	 ERROR(pFTC_OTG,"??? Cat not set to Peripheral(When B Device is not in idle)(State=%d)...\n",pFTC_OTG->otg.state);
     	 return(-1);
     	}
     }



  return(0);

}


//*********************************************************
// Name: OTGC_AP_CMD_mdelay_test_Fun
// Description: To test the mdelay function                                
// Input: void
// Output: int     
//********************************************************* 
static int OTGC_AP_CMD_mdelay_test_Fun(void)
{
    u32 wTemp;

            printk("*** mdelay Test (unit=1ms) ***\n");

    	    wTemp=0;
    	    do{
    	    mdelay(1);
    	    wTemp++;
    	    if ((wTemp%1000)==0)
    	       printk(">>> mdelay test => %d sec\n",(wTemp/1000));
    	    
    	    }while(wTemp<10*1000);
    	    
            printk("*** mdelay Test (unit=10ms) ***\n");

    	    wTemp=0;
    	    do{
    	    mdelay(10);
    	    wTemp++;
    	    if ((wTemp%100)==0)
    	       printk(">>> mdelay test => %d sec\n",(wTemp/100));
    	    
    	    }while(wTemp<10*100);    	    
    	    
            printk("*** mdelay Test (unit=100ms) ***\n");

    	    wTemp=0;
    	    do{
    	    mdelay(100);
    	    wTemp++;
    	    if ((wTemp%10)==0)
    	       printk(">>> mdelay test => %d sec\n",(wTemp/10));
    	    
    	    }while(wTemp<10*10);    	        	    
    	    
            printk("*** mdelay Test (unit=1000ms) ***\n");

    	    wTemp=0;
    	    do{
    	    mdelay(1000);
    	    wTemp++;

    	       printk(">>> mdelay test => %d sec\n",(wTemp));
    	    
    	    }while(wTemp<10);      	    
    	    
    	    
    	    
  return(0);
}
//*********************************************************
// Name: OTGC_AP_Dump_Reg_Fun
// Description: To dump the memory                                
// Input: void 
// Output:int      
//********************************************************* 
static int OTGC_AP_Dump_Reg_Fun(void)
{

  u32 regadd=0x00;


  printk(" ******** Dump the OTG200 Register ********\n");
  do {
      printk ("%3x =>>>   %8x  %8x   %8x   %8x\n",regadd, mdwFOTGPort(regadd)
       ,(mdwFOTGPort((regadd+4))) ,(mdwFOTGPort((regadd+8))) ,(mdwFOTGPort((regadd+12))));
      
      regadd+=0x10; 
      
     }while(regadd<0x200);

  return(0);
}
//*********************************************************
// Name: OTGC_AP_ioctl
// Description: Request from user mode AP                                      
// Input:struct inode * inode, struct file * file
//      ,unsigned int cmd,unsigned long arg 
// Output: int    
//********************************************************* 
static unsigned long ulAP_REG_Address=0;
static int OTGC_AP_ioctl(struct inode * inode, struct file * file
                             ,unsigned int cmd,unsigned long arg)
{

    DBG_OTG_FUNCC("+OTGC_AP_ioctl()\n");

    switch(cmd){
    	case OTG_AP_CMD_GET_STATE:
    	
    	     return(OTGC_AP_Get_State());
    	
    	
    	break;
    	case OTG_AP_CMD_SET_HOST:
    	     return(OTGC_AP_Set_to_Host());
    	     
    	break;
    	case OTG_AP_CMD_SET_IDLE:
    	
    	     return(OTGC_AP_Set_to_Idle());
    	
    	break;

    	case OTG_AP_CMD_SET_PERIPHERAL:	   	
    	    
    	    return(OTGC_AP_Set_to_Peripheral());	

    	break;    	   	

     	case OTG_AP_CMD_MDELAY_100:	   	
       
             mdelay(100);
    	    return(0);	

    	break;       	   	

	   	
     	case OTG_AP_CMD_TEST_MDELAY:   	
       

    	    return(OTGC_AP_CMD_mdelay_test_Fun());	

    	break;       	   	
    	case OTG_AP_CMD_TEST_DUMP_REG: 	   	
    	    
    	    return(OTGC_AP_Dump_Reg_Fun());	

    	break;        	   	

    	case OTG_AP_CMD_TEST_SET_REG_ADD: 
    	     
    	     ulAP_REG_Address=arg;  	
    	     printk(">>> OTG Driver set address=0x%x\n",arg);
    	     
    	    return(0);	

    	break;       

    	case OTG_AP_CMD_TEST_REG_WRITE: 	   	
    	     mdwFOTGPort(ulAP_REG_Address)=arg;
    	     printk(">>> OTG Driver write (data=0x%x) to (address=0x%x)\n",arg,ulAP_REG_Address);
    	    return(0);	

    	break;       


    	case OTG_AP_CMD_TMP_FORCE_FULL:    	   	
    	     
    	     pFTC_OTG->iTMP_Force_Speed=1;
    	    return(0);	

    	break;        	   	
    	case OTG_AP_CMD_TMP_FORCE_HIGH:	   	
    	    
    	    pFTC_OTG->iTMP_Force_Speed=2;
    	    return(0);	

    	break;        	   	
    	case OTG_AP_CMD_TMP_FORCE_CLEAN:   	
    	    
    	    pFTC_OTG->iTMP_Force_Speed=0;
    	    return(0);	

    	break;      
    	   	
    	}


	return 0;
}




//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//--------Group-3:Interface between Host/Peripheral/OTG Function --------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//*********************************************************
// Name: FOTG2XX_get_otg_transceiver
// Description: Host/Peripheral Driver will call this function to get 
//              the (single) OTG transceiver driver
// Input:void
// Output:the structure pointer
//********************************************************* 
struct otg_transceiver *FOTG2XX_get_otg_transceiver(void)
{
       INFO(pFTC_OTG,">>> +FOTG2XX_get_otg_transceiver\n");
       if (xceiv)
	  return ((struct otg_transceiver *)xceiv);
       else{
       
       ERROR(pFTC_OTG,"??? Error for FOTG2XX_get_otg_transceiver...\n");
	  return ((struct otg_transceiver *)xceiv);       
       }
}
EXPORT_SYMBOL(FOTG2XX_get_otg_transceiver);



//*********************************************************
// Name: FOTG2XX_set_host
// Description:
//            1.Call from Host-mode
//            2.bind/unbind the host control
//            3.Let OTG know how to call the Host Function Call
// Input:1.*otg
//       2.*host = 0 => Unbing host
//         *host = 1 => Bing host
// Output: 0 => ok      
//********************************************************* 
static int
FOTG2XX_set_host(struct otg_transceiver *potg, struct usb_bus *host)
{
   struct FTC_OTGC_STS	*fotg = container_of(potg, struct FTC_OTGC_STS, otg);
   
   DBG_OTG_FUNCC("+(FOTG2XX_set_host)\n");	
 //<1>.Checking input
	if (!potg || fotg != pFTC_OTG)
		return -ENODEV;

 //<2>.Unbind host processing
	if (!host) {
		OTGH_Close();//Disable Interrupt
		fotg->otg.host = 0;
		return 0;
	}

  //<3>.Bind the host control
	fotg->otg.host = host;
	ERROR(&fotg->client.dev, "registered host\n");

	if (fotg->otg.gadget)
		{
		  OTGC_Init();

		  return 0;
		}
	return 0;



}
//*********************************************************
// Name: FOTG2XX_set_peripheral
// Description:
//            1.Call from Peripheral-mode
//            2.bind/unbind the Peripheral control
//            3.Let OTG know how to call the Peripheral Function Call
// Input: 1.*otg
//        2.*gadget = 0 => Unbing host
//          *gadget = 1 => Bing host
// Output:void      
//********************************************************* 
static int
FOTG2XX_set_peripheral(struct otg_transceiver *potg, struct usb_gadget *gadget)
{
   struct FTC_OTGC_STS	*fotg = container_of(potg, struct FTC_OTGC_STS, otg);
   
   DBG_OTG_FUNCC("+(FOTG2XX_set_peripheral)\n");	
   
   if (!potg || fotg != pFTC_OTG)
		return -ENODEV;

  //<1>.Handle the event of unbind peripheral driver
	if (!gadget) {
		OTGP_Close();//Disable Interrupt
		if (!fotg->otg.default_a)//For A => After unbind driver => srop the VBUS
			OTGC_enable_vbus_draw(1);
		fotg->otg.gadget = 0;

		return 0;
	}

  //<2>.Handle the event of binding peripheral driver
	fotg->otg.gadget = gadget;
	ERROR(&fotg->client.dev, "registered gadget\n");
	/* gadget driver may be suspended until vbus_connect () */
	if (fotg->otg.host)
		{
		  OTGC_Init();

		  return 0;
		}
	return 0;


}

//*********************************************************
// Name: FOTG2XX_set_power
// Description:
//            1.Effective for B Device
//            2.B device can set the VBUS power after receiving 
//              the SET_CONFIGURATION
// PS:Faraday do not have this feature   
// Input: void
// Output:void      
//********************************************************* 
static int FOTG2XX_set_power(struct otg_transceiver *dev, unsigned mA)
{

	
	return 0;
}

//*********************************************************
// Name: FOTG2XX_start_srp
// Description:
//            1.Call from B-Peripheral mode 
//            2.Issue the SRP
// Input: struct otg_transceiver *dev
// Output:int      
//********************************************************* 
static int FOTG2XX_start_srp(struct otg_transceiver *dev)//Only For B-Device
{
	
    u32 wTemp;

    struct FTC_OTGC_STS	*fotg = container_of(dev, struct FTC_OTGC_STS, otg);
	
	
    DBG_OTG_FUNCC("+(FOTG2XX_start_srp)\n");	


    //Reset phy
   //   OTGC_A_PHY_Reset();
	               

  //<1>.Checking condition
	if (!dev || fotg != pFTC_OTG
			|| fotg->otg.state != OTG_STATE_B_IDLE)
		return -ENODEV;
  
  //<2>.Checking OTG_BSESSEND
	if (!mdwOTGC_Control_B_SESS_END_Rd())
		return -EINVAL;

  //<3>.Issue the SRP 
    mdwOTGC_INT_STS_Clr(OTGC_INT_BSRPDN);//Clear the interrupt status 
    fotg->otg.state = OTG_STATE_B_SRP_INIT;
    mUsbUnPLGClr();//Pull high the D+
    mdwOTGC_Control_B_HNP_EN_Clr();       
    mdwOTGC_Control_B_BUS_REQ_Set();//Issue the bus requst
  
  //<4>.Waiting for the SRP complete  
        //Here we must use the polling to wait the SRP complete
        //
      mdelay (11);  
       wTemp=0;
       do{
          if (mdwOTGC_INT_STS_Rd()&OTGC_INT_BSRPDN)
             {

            INFO(pFTC_OTG,">>> OTG-B:SRP Detected OK...\n");	            	
            goto Detected_SRP_OK;             	
            }
         mdelay (1);           
         wTemp++;

         if (wTemp>(10*1000)) //Waiting 10 sec
            {
            ERROR(pFTC_OTG,"??? OTG-B:B can't issue SRP...(wTemp=%d)\n",wTemp);	
            goto Detected_SRP_INIT_FAIL;
            }

       }
    while(1); // <A>.The done interrupt
	
  //<4>.Waiting for the VBUS-Turn-on
Detected_SRP_OK: 
         if (OTGC_Waiting_VBUS_On(10000)==0)//Waiting Host turn on VBUS 10 sec
            {
           //Turn On VBUS ok
          fotg->otg.state = OTG_STATE_B_PERIPHERAL;
           OTGP_Open();
           return (0);
            }
         else
           {
            //Time Out=>Type-A do not turn on the bus
            ERROR(pFTC_OTG,"??? OTG-B:Device-A Not Responding(Waiting Drive VBUS Fail)...\n");	
            goto Detected_SRP_INIT_FAIL;
           }


Detected_SRP_INIT_FAIL: //Recover to the original state
            mUsbUnPLGSet();//Pull low the D+    
            mdwOTGC_Control_B_BUS_REQ_Clr(); 
            fotg->otg.state = OTG_STATE_B_IDLE;
            return(1) ;	
}

//*********************************************************
// Name: FOTG2XX_start_hnp
// Description:Call from Host(A) to start HNP
// Input: void
// Output:void      
//********************************************************* 
static int FOTG2XX_start_hnp(struct otg_transceiver *dev)
{

    int wTemp;
    struct FTC_OTGC_STS	*fotg;
   
    fotg = pFTC_OTG;
    
    DBG_OTG_FUNCC("+(FOTG2XX_start_hnp)\n");	

  //<1>.Close the Host
         OTGH_Close();

  //<2>.Open the Peripheral
         OTGP_Open();
   

  //<3>.Enable Register
         //Set HW
         DBG_OTG_TRACEC(">>>FOTG2XX_start_hnp()-->mdwOTGC_Control_A_SET_B_HNP_EN_Set\n");
         mdwOTGC_Control_A_SET_B_HNP_EN_Set();
         mUsbUnPLGClr();
  
  //<4>.A-Host should disconnect the device issue the HNP 
     	 Host_Disconnect_for_OTG(pFTC_OTG->otg.host,0);

  //<5>.Waiting for Role Change ==> if fail clear the all the variable 
         wTemp=0;
         while(pFTC_OTG->otg.state == OTG_STATE_A_HOST)//Waiting 500ms
         {
         mdelay(10);
         wTemp++;
         if (wTemp>50)
            {//Time Out for waiting for  Rolechange
                ERROR(pFTC_OTG,"???Waiting for Role Change Fail...\n");

            	mdwOTGC_Control_A_SET_B_HNP_EN_Clr();
            	pFTC_OTG->otg.host->b_hnp_enable=0;
            	return(1);
            }
         };

	return 0;

} 

//*********************************************************
// Name: FOTG2XX_Force_Speed
// Description:Force speed
// Input: void
// Output:0 => ok
//        1 => Fail
//********************************************************* 
int FOTG2XX_Force_Speed(void)
{
   switch (pFTC_OTG->iTMP_Force_Speed)
   {
      case 0://Clear All
      	mwHost20_Control_ForceFullSpeed_Clr();
      	mwHost20_Control_ForceHighSpeed_Clr();
         break;
      case 1://Force Full Speed
      	mwHost20_Control_ForceFullSpeed_Set();
      	mwHost20_Control_ForceHighSpeed_Clr();
         break;
      default://Force High Speed
         mwHost20_Control_ForceFullSpeed_Clr();
         mwHost20_Control_ForceHighSpeed_Set();
         break;

   }
        
   INFO(pFTC_OTG,">>> Force Speed Finish...(Speed=%d)\n",pFTC_OTG->iTMP_Force_Speed);
   return 0;

}
EXPORT_SYMBOL(FOTG2XX_Force_Speed);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//------------------------- Group-4:OTG Function ------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//*********************************************************
// Name: OTG_RoleChange
// Description:This function will take care the event about role change.
//             It will close/init some function.
// Input: void
// Output:void      
//********************************************************* 
void OTG_RoleChange(void)
{

  int wTempCounter;
  u32 wRole=mdwOTGC_Control_CROLE_Rd();

#ifdef OPEN_CRITICAL_MESSAGE 
    DBG_OTG_FUNCC("+(OTG_RoleChange)wRole=%d\n",wRole);	
#endif

  if (wRole==0)
     {//Change to Host
       if (mdwOTGC_Control_ID_Rd()==0)
          {//Device-A: change to Host
            mdwOTGC_Control_A_SET_B_HNP_EN_Clr();
           OTGC_enable_vbus_draw(1);
   	   OTGP_Close();       
           OTGH_Open();           
           pFTC_OTG->otg.state = OTG_STATE_A_HOST; 
           pFTC_OTG->otg.host->is_b_host=0;
           pFTC_OTG->otg.host->b_hnp_enable=0;
           if (pFTC_OTG->A_HNP_to_Peripheral>0)//Device A from Peripheral to Host
               {
               	pFTC_OTG->otg.host->A_Disable_Set_Feature_HNP=1;//Tell to host that 'Do not set feature B_HNP_Enable'
               	mdelay(10);
                //<1>.Waiting for OPT-B Connect after HNP-2 (200ms)
                      wTempCounter=0;
                      while(mwHost20_PORTSC_ConnectStatus_Rd()==0)
                           {
                            mdelay(1);
                            wTempCounter++;
                            //if (wTempCounter>200)//Waiting for 200 ms
                            if (wTempCounter>300)//Waiting for 200 ms
                               {

                                INFO(pFTC_OTG,">>> OTG-B do not connect under 200 ms...\n");                               	
                        	break;
                                }
                           }
                //<2>.If connect => Issue quick reset
                            if (mwHost20_PORTSC_ConnectStatus_Rd()>0)                       
                                {
                                 //mdelay(200);//For OPT-A Test
                                 mdelay(300);//For OPT-A Test
                                 OTGH_host_quick_Reset();

                                }
                //OTGC_AP_Set_to_Idle();  
               }
               pFTC_OTG->A_HNP_to_Peripheral=0;
     
               
               
          }
       else{//Device-B: Change to Host
           pFTC_OTG->otg.host->is_b_host=1;
   	   OTGP_Close();     
           OTGH_Open();        
           mdwOTGC_Control_B_HNP_EN_Clr();
           mdwOTGC_Control_B_DSCHG_VBUS_Clr();   
           pFTC_OTG->otg.state = OTG_STATE_B_HOST;     
           mdwOTGC_Control_B_BUS_REQ_Clr();        
	
           }   

     }
  else{//Change to Peripheral
      
       if (mdwOTGC_Control_ID_Rd()==0)
          {//Device-A: change to Peripheral        	
       	   mdwOTGC_Control_A_SET_B_HNP_EN_Clr();       	
           pFTC_OTG->otg.state = OTG_STATE_A_PERIPHERAL;
           pFTC_OTG->A_HNP_to_Peripheral=1;
         }
       	else{//Device-B: Change to Peripheral
       	   mdwOTGC_Control_B_BUS_REQ_Clr();
           pFTC_OTG->otg.state = OTG_STATE_B_PERIPHERAL;

       	 }
       OTGH_Close();       
       OTGP_Open();

      }

}	

//*********************************************************
// Name: OTGC_Init
// Description:
//    1.Init the OTG Structure Variable
//    2.Init the Interrupt register(OTG-Controller layer)
//    3.Call the OTG_RoleChange function to init the Host/Peripheral
// Input: void
// Output:void      
//********************************************************* 
void OTGC_Init(void)
{
   

  DBG_OTG_FUNCC("+(OTGC_Init)\n");	

  pFTC_OTG->A_HNP_to_Peripheral=0;
  pFTC_OTG->otg.host->A_Disable_Set_Feature_HNP=1;

   //<1>.Read the ID 
  if (mdwOTGC_Control_ID_Rd()>0)
          {//Change to B Type
          	   //<1.1>.Init Variable
                       pFTC_OTG->wCurrentInterruptMask=OTGC_INT_B_TYPE;  
                       pFTC_OTG->otg.state = OTG_STATE_B_IDLE;         
                       mdwOTGC_Control_A_SRP_DET_EN_Clr();
               //<1.2>.Init Interrupt
                       mdwOTGC_INT_Enable_Clr(OTGC_INT_A_TYPE);      
                       mdwOTGC_INT_Enable_Set(OTGC_INT_B_TYPE);     
                       mdwOTGC_GINT_MASK_OTG_Clr();    	   
         	   //<1.3>.Init the Host/Peripheral
                       OTG_RoleChange();
           }
        else{//Changfe to A Type
            //<2.1>.Init Variable
                    pFTC_OTG->wCurrentInterruptMask=OTGC_INT_A_TYPE;     
                    pFTC_OTG->otg.state = OTG_STATE_A_IDLE; 
                    pFTC_OTG->otg.default_a=1; 
                    //Enable the SRP detect
                    mdwOTGC_Control_A_SRP_RESP_TYPE_Clr();
                           
            //<2.2>. Init Interrupt
                    mdwOTGC_INT_Enable_Clr(OTGC_INT_B_TYPE);
                    mdwOTGC_INT_Enable_Set(OTGC_INT_A_TYPE);                  
                    mdwOTGC_GINT_MASK_OTG_Clr();  
      	    //<2.3>.Init the Host/Peripheral
                    OTG_RoleChange();

           }      

}

//*********************************************************
// Name: OTGC_INT_ISR
// Description:This interrupt service routine belongs to the OTG-Controller 
//            <1>.Check for ID_Change
//            <2>.Check for RL_Change
//            <3>.Error Detect
// Input: wINTStatus
// Output:void      
//********************************************************* 
void OTGC_INT_ISR(UINT32 wINTStatus)
{



  int wTempCounter;
#ifdef OPEN_CRITICAL_MESSAGE 
       DBG_OTG_FUNCC("+(OTGC_INT_ISR)(0x30=0x%x)\n",mdwFOTGPort(0x30));	
#endif

  //<1>.Check for ID_Change
  if (wINTStatus&OTGC_INT_IDCHG)
      {
       if (mdwOTGC_Control_ID_Rd()>0) 
          {//Change to B Type
             OTGC_Init();
          }
       else{//Changfe to A Type
             OTGC_Init();
           }   

      }else{//else of " if (wINTStatus&OTGC_INT_IDCHG) "    

  //<2>.Check for RL_Change
           if (wINTStatus&OTGC_INT_RLCHG)
               {

                     OTG_RoleChange();  
               }

 //<3>.Error Detect
     if (wINTStatus&OTGC_INT_AVBUSERR)
        {
         ERROR(pFTC_OTG,"??? Error:Interrupt OTGC_INT_AVBUSERR=1... \n");

        }         
     if (wINTStatus&OTGC_INT_OVC)
       {
        ERROR(pFTC_OTG,"??? Error:Interrupt OTGC_INT_OVC=1... \n");

       }          


           //<3>.Check for Type-A/Type-B Interrupt
           if (mdwOTGC_Control_ID_Rd()==0)  
              {//For Type-A Interrupt
             	if (wINTStatus&OTGC_INT_A_TYPE)
                  {
                    
                   if (wINTStatus&OTGC_INT_ASRPDET)
                      {
                       //<1>.SRP detected => then set global variable
                             INFO(pFTC_OTG,">>> OTG-A got the SRP from the DEvice-B ...\n");

                       //<2>.Turn on the V Bus 
                             pFTC_OTG->otg.state = OTG_STATE_A_WAIT_VRISE;
                             OTGC_enable_vbus_draw(1);
                             pFTC_OTG->otg.state = OTG_STATE_A_HOST;
                       //<3>.Should waiting for Device-Connect Wait 300ms 
                             INFO(pFTC_OTG,">>> OTG-A Waiting for OTG-B Connect,\n");
                             wTempCounter=0;
                             while(mwHost20_PORTSC_ConnectStatus_Rd()==0)
                                  {
                                   mdelay(1);
                                   wTempCounter++;
                                   if (wTempCounter>300)//Waiting for 300 ms
                                      {
                                       mdwOTGC_Control_A_SRP_DET_EN_Clr();
                                       INFO(pFTC_OTG,">>> OTG-B do not connect under 300 ms...\n");                               	
                               	       break;
                                      }
                                   }
                       //<4>.If Connect => issue quick Reset 
                            if (mwHost20_PORTSC_ConnectStatus_Rd()>0)                       
                                {mdelay(300);//For OPT-A Test
                                 OTGH_host_quick_Reset();
                                 OTGH_Open();
                                 pFTC_OTG->otg.host->A_Disable_Set_Feature_HNP=0;
                                }

                       }


                  }
              }else
              {//For Type-B Interrupt

              }
         }   //end of " if (wINTStatus&OTGC_INT_IDCHG) "

}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//---------------------- Group-5:Module Function-------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//*********************************************************
// Name: FOTG2XX_ISR
// Description:                                
//            Checking for interrupt source
//            <1>.OTG Control interrupt event
//            <2>.Host Mode interrupt event  
//            <3>.Peripheral interrupt event       
// Input: 
// Output:      
//********************************************************* 
static irqreturn_t FOTG2XX_ISR(int irq, void *_dev, struct pt_regs *r)
{
	u32 wINTStatus;
	struct FTC_OTGC_STS	*dev = _dev;
        int iTimerCounter;


#ifdef OPEN_CRITICAL_MESSAGE 
	DBG_OTG_FUNCC("+FOTG2XX_ISR(FOTG2XX_irq)\n");
#endif
	spin_lock(&dev->lock);

    wINTStatus = mdwOTGC_INT_STS_Rd(); 


    //<1>.Checking the OTG layer interrupt status
          if (wINTStatus>0)
             {
              mdwOTGC_INT_STS_Clr(wINTStatus); 
              if ((pFTC_OTG->wCurrentInterruptMask & wINTStatus)>0)
                  OTGC_INT_ISR(wINTStatus);
          
             mdwOTGC_INT_STS_Clr(wINTStatus); 
          
             }
	
	
    if (mdwOTGC_Control_CROLE_Rd()==0)
        {
   //<2>.For Host ISR  
    	 if (mdwOTGC_GINT_STS_HOST_Rd()>0)
    	    pFTC_OTG->otg.host->hcd_isr(pFTC_OTG->otg.host); 
        }else
        { 
   //<3>.For Peripheral ISR
    	 if (mdwOTGC_GINT_STS_PERIPHERAL_Rd()>0)         
            {

            //Here we must to check the HNP enable issue
           if (mUsbIntSrc2Rd()&BIT1)     
               {//Peripheral Suspend
               	if (pFTC_OTG->otg.gadget->b_hnp_enable==1)
                   {
	            //<2>.Waiting for Role Change interrupt status      
	                  iTimerCounter=0;
	                  while(iTimerCounter<7000)//Waiting 70ms
	                       {
	                       	if ((mdwOTGC_INT_STS_Rd()&OTGC_INT_RLCHG)>0)
	                       	    {//Role Change OK
	                       	     udelay(100);	
	                       	     OTGH_host_quick_Reset();	
	                       	     printk(">>> Device-B: Role Change to Host ok & Issue port reset...\n");
	                       	     pFTC_OTG->otg.gadget->b_hnp_enable=0;  
	                       	     OTGP_Close();
	                       	     goto Irq_Exit;
	                       	    }
	                       	udelay(10);
	                       	iTimerCounter++;

	                       	};
	              {printk("??? Device Not Responding...\n");

	              }
	              pFTC_OTG->otg.gadget->b_hnp_enable=0;                   	      
                   	
                   }//if (pFTC_OTG->otg.gadget->b_hnp_enable==1)
               }// if (mUsbIntSrc2Rd()&BIT1)  

            pFTC_OTG->otg.gadget->udc_isr();

            }//if (mdwOTGC_GINT_STS_PERIPHERAL_Rd()>0)   
        }

Irq_Exit:
	spin_unlock(&dev->lock);

	return IRQ_RETVAL(0);
	
	
}

  

/*-------------------------------------------------------------------------*/
//*********************************************************
// Name: FOTG2XX_Remove
// Description:Remove the Driver                                     
// Input: 
// Output:      
//********************************************************* 
/* tear down the binding between this driver and the pci device */
static void FOTG2XX_Remove(void)
{
   DBG_OTG_FUNCC("+FOTG2XX_Remove()\n");

   pFTC_OTG->otg.state = OTG_STATE_UNDEFINED;
  
  
   if (pFTC_OTG->got_irq)  
         free_irq(IRQ_FOTG200, pFTC_OTG);//Andrew update


   kfree(pFTC_OTG); //Andrew update
   pFTC_OTG = 0;

   INFO(pFTC_OTG,"USB device unbind\n");
}



//*********************************************************
// Name: FOTG2XX_Probe
// Description:probe driver                                       
// Input: void
// Output: int          
//********************************************************* 
static int FOTG2XX_Probe(void)//FOTG200.ok
{
    int	retval=0;
    u8 wFound=0;
    
    struct otg_transceiver *the_OTG_controller;
	DBG_OTG_FUNCC("+FOTG2XX_Probe()\n");

  //<1>.Checking FOTG2XX
    if (mdwFOTGPort(0x00)==0x01000010)
        if (mdwFOTGPort(0x04)==0x00000001)
           if(mdwFOTGPort(0x08)==0x00000006) 
              wFound=1;

    DBG_OTG_FUNCC(">>> Checking FOTG2XX ...(0x00=0x%x)\n",wFOTGPeri_Port(0x00));              
    
    if (wFound==1)
       	INFO(pFTC_OTG,">>> Found FOTG2XX ...\n");
    else
       {
      	ERROR(pFTC_OTG,"??? Not Found FOTG2XX ...(0x00=0x%x)\n",wFOTGPeri_Port(0x00));
       	return(-EBUSY);
       	}
   
  //<2>.Register the Module
        DBG_OTG_TRACEC("FOTG2XX_Probe()--> devfs_register_chrdev(OTG_MAJOR=%d)\n",OTG_MAJOR);
	if (devfs_register_chrdev(OTG_MAJOR, "FOTG2XX", &otg_fops)) {
		err("unable to get major %d for otg devices", OTG_MAJOR);
		return -EBUSY;
	}


  //<3>.Init OTG Structure
	/* alloc, and start init */

	pFTC_OTG = kmalloc (sizeof *pFTC_OTG, SLAB_KERNEL);

	if (pFTC_OTG == NULL){
		ERROR(pFTC_OTG,"enomem FOTG2XX device\n");
		retval = -ENOMEM;
		goto done;
       }
    
       //Bruce;; otg_set_transceiver(&(pFTC_OTG->otg));
        xceiv=&(pFTC_OTG->otg);
    
        the_OTG_controller=&(pFTC_OTG->otg);
	//pending;;the_OTG_controller->dev = &(pFTC_OTG);
	the_OTG_controller->label = driver_name;
	the_OTG_controller->set_host = FOTG2XX_set_host,
	the_OTG_controller->set_peripheral = FOTG2XX_set_peripheral,
	the_OTG_controller->set_power = FOTG2XX_set_power,//Nobody will call this function in FOTG2XX
	the_OTG_controller->start_srp = FOTG2XX_start_srp,//Peripheral will call this function
	the_OTG_controller->start_hnp = FOTG2XX_start_hnp,//Peripheral and Host will not call 'start_hnp' 
	                                                  //OTGC_AP_Set_to_Idle will call 'FOTG2XX_start_hnp'
        the_OTG_controller->gadget=0;
        the_OTG_controller->host=0;        
	pFTC_OTG->otg.state = OTG_STATE_UNDEFINED;


  //<3>.Init FOTG2XX HW
        mdwOTGC_ChipEnable_Set();
        mdwOTGC_GINT_HI_ACTIVE_Set();//Fource FOTG200-Interrupt to High-Active
        //Disable all the interrupt 
        mdwOTGC_GINT_MASK_HOST_Set();
        mdwOTGC_GINT_MASK_OTG_Set();
        mdwOTGC_GINT_MASK_PERIPHERAL_Set();
        
        cpe_int_set_irq(IRQ_FOTG200, LEVEL, H_ACTIVE);

	    if (request_irq(IRQ_FOTG200, FOTG2XX_ISR, SA_INTERRUPT /*|SA_SAMPLE_RANDOM*/,
	    	            driver_name, pFTC_OTG) != 0) 
	    {
	    	DBG(dev, "request interrupt failed\n");
	    	retval = -EBUSY;
            pFTC_OTG->got_irq=0;	    	
	    	goto done;
	    }

    pFTC_OTG->got_irq=1;
    pFTC_OTG->iTMP_Force_Speed=1;//Default force speed to Full



	/* done */
	return 0;

done:
    if (pFTC_OTG)
		FOTG2XX_Remove();
		
	return retval;

}

//*********************************************************
// Name: cleanup
// Description: init driver                                         
// Input: void
// Output: int          
//********************************************************* 
static int __init init (void)
{
	INFO(pFTC_OTG,"Init FOTG2XX Driver\n");

	DBG_OTG_TRACEC("FOTG2XX_BASE_ADDRESS = 0x%x\n", OTG_BASE_ADDRESS);
	return FOTG2XX_Probe();
}
module_init (init);
//*********************************************************
// Name: init
// Description: init driver                                         
// Input:void
// Output: void           
//********************************************************* 
static void __exit cleanup (void) 
{
	INFO(pFTC_OTG,"Remove FOTG2XX Driver...\n");

	return FOTG2XX_Remove();
}
module_exit (cleanup);


