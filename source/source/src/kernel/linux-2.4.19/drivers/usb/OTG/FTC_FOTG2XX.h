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

//*************************************************************************
//****************************** 1.Normal Define ****************************
//*************************************************************************

#define TRUE			1
#define FALSE			0
#define MASK_F0			0xF0
	
#define BIT0			0x00000001
#define BIT1			0x00000002
#define BIT2			0x00000004
#define BIT3			0x00000008
#define BIT4			0x00000010
#define BIT5			0x00000020
#define BIT6			0x00000040
#define BIT7			0x00000080

#define BIT8			0x00000100
#define BIT9			0x00000200
#define BIT10			0x00000400
#define BIT11			0x00000800
#define BIT12			0x00001000
#define BIT13			0x00002000
#define BIT14			0x00004000
#define BIT15			0x00008000	
	
#define BIT16			0x00010000
#define BIT17			0x00020000
#define BIT18			0x00040000
#define BIT19			0x00080000
#define BIT20			0x00100000
#define BIT21			0x00200000
#define BIT22			0x00400000
#define BIT23			0x00800000	
	
#define BIT24			0x01000000
#define BIT25			0x02000000
#define BIT26			0x04000000
#define BIT27			0x08000000
#define BIT28			0x10000000
#define BIT29			0x20000000
#define BIT30			0x40000000
#define BIT31			0x80000000	
	
#define UINT8                   u8
#define UINT16                  u16
#define UINT32                  u32
	                        
#define mLowByte(u16)	        ((u8)(u16	 ))
#define mHighByte(u16)	        ((u8)(u16 >> 8))


//*************************************************************************
//****************************** 2.Struct Define************************
//*************************************************************************

    typedef struct FTC_OTGC_STS
	{

        //Standard  
	struct otg_transceiver	otg;
	spinlock_t		lock;
	int			got_irq;

	//For FOTG2XX
	int                     A_HNP_to_Peripheral; //1=>When A-device change to Peripheral
	int                     iTMP_Force_Speed;
        u32                     wCurrentInterruptMask;

	} FTC_OTGC_STS;



/*-------------------------------------------------------------------------*/
//*************************************************************************
//****************************** 3.Debug Info Define************************
//*************************************************************************
#define DEBUG
#define VERBOSE
//#define OPEN_CRITICAL_MESSAGE
//Internal Option
#define DBG_OTG_OFF 	0x00
#define DBG_OTG_FUNC	0x01
#define DBG_OTG_ERROR	0x02
#define DBG_OTG_TRACE	0x04
#define DBG_OTG_INFO	0x08
#define USB_DBG_OTG_CONDITION 	(DBG_OTG_INFO)


#define xprintk(dev,level,fmt,args...) printk(level "%s : " fmt , driver_name , ## args)
#define wprintk(level,fmt,args...) printk(level "%s : " fmt , driver_name , ## args)

#ifdef DEBUG
#define DBG(dev,fmt,args...) xprintk(dev , KERN_DEBUG , fmt , ## args)
#else
#define DBG(dev,fmt,args...) do { } while (0)
#endif /* DEBUG */

#ifdef VERBOSE
#define VDBG DBG
#else
#define VDBG(dev,fmt,args...) do { } while (0)
#endif	/* VERBOSE */


#define ERROR(dev,fmt,args...) xprintk(dev , KERN_ERR , fmt , ## args)
#define WARN(dev,fmt,args...) xprintk(dev , KERN_WARNING , fmt , ## args)
#define INFO(dev,fmt,args...) xprintk(dev , KERN_INFO , fmt , ## args)


//For DBG_OTG_FUNC
#if (USB_DBG_OTG_CONDITION & DBG_OTG_FUNC)  
#define DBG_OTG_FUNCC(fmt,args...) wprintk(KERN_INFO , fmt , ## args)
#else
#define DBG_OTG_FUNCC(fmt,args...)
#endif
//For DBG_OTG_TRACE
#if (USB_DBG_OTG_CONDITION & DBG_OTG_TRACE)  
#define DBG_OTG_TRACEC(fmt,args...) wprintk(KERN_INFO , fmt , ## args)
#else
#define DBG_OTG_TRACEC(fmt,args...)
#endif
//For DBG_OTG_INFO
#if (USB_DBG_OTG_CONDITION & DBG_OTG_INFO)  
#define DBG_OTG_INFOC(fmt,args...) wprintk(KERN_INFO , fmt , ## args)
#else
#define DBG_OTG_TRACEC(fmt,args...)
#endif


/*-------------------------------------------------------------------------*/
//*************************************************************************
//****************************** 4.Others Define************************
//*************************************************************************

#ifndef container_of
#define	container_of	list_entry
#endif

#ifndef likely
#define likely(x)	(x)
#define unlikely(x)	(x)
#endif


//*************************************************************************
//****************************** 5.HW Macro Define************************
//*************************************************************************
//### For AHB #########################
#define AHB_BASE_ADDRESS	                   (IO_ADDRESS(CPE_AHB_BASE))
#define mdwAHBPort(bOffset)	                   *((volatile UINT32 *) ( AHB_BASE_ADDRESS))

//### For OTG #########################
#define OTG_BASE_ADDRESS	                   (IO_ADDRESS(CPE_FOTG200_BASE))
#define mbFOTGPort(bOffset)	                   *((volatile UINT8 *) ( OTG_BASE_ADDRESS |  (u32)bOffset))
#define mwFOTGPort(bOffset)	                   *((volatile UINT16 *) ( OTG_BASE_ADDRESS |  (u32)bOffset))
#define mdwFOTGPort(bOffset)	                   *((volatile UINT32 *) ( OTG_BASE_ADDRESS |  (u32)bOffset))


  //Offset:0x080(OTG Control/Status Register) => Suppose Word-Read & Word-Write
    //~B Function
#define mdwOTGC_Control_B_BUS_REQ_Rd()		  (mdwFOTGPort(0x80)& BIT0)	
#define mdwOTGC_Control_B_BUS_REQ_Set()  	  (mdwFOTGPort(0x80) |=  BIT0)  
#define mdwOTGC_Control_B_BUS_REQ_Clr()  	  (mdwFOTGPort(0x80) &=  (~BIT0))  

#define mdwOTGC_Control_B_HNP_EN_Rd()		  (mdwFOTGPort(0x80)& BIT1)	
#define mdwOTGC_Control_B_HNP_EN_Set()		  (mdwFOTGPort(0x80) |=  BIT1)   
#define mdwOTGC_Control_B_HNP_EN_Clr()  	  (mdwFOTGPort(0x80) &=  (~BIT1))  

#define mdwOTGC_Control_B_DSCHG_VBUS_Rd()	  (mdwFOTGPort(0x80)& BIT2)	
#define mdwOTGC_Control_B_DSCHG_VBUS_Set()	  (mdwFOTGPort(0x80) |=  BIT2)    
#define mdwOTGC_Control_B_DSCHG_VBUS_Clr() 	  (mdwFOTGPort(0x80) &=  (~BIT2))  

    //~A Function 
#define mdwOTGC_Control_A_BUS_REQ_Rd()	          (mdwFOTGPort(0x80)& BIT4)	
#define mdwOTGC_Control_A_BUS_REQ_Set()	          (mdwFOTGPort(0x80) |=  BIT4)    
#define mdwOTGC_Control_A_BUS_REQ_Clr()	          (mdwFOTGPort(0x80) &=  (~BIT4))        
    
#define mdwOTGC_Control_A_BUS_DROP_Rd()	          (mdwFOTGPort(0x80)& BIT5)	
#define mdwOTGC_Control_A_BUS_DROP_Set()	  (mdwFOTGPort(0x80) |=  BIT5)      
#define mdwOTGC_Control_A_BUS_DROP_Clr()	  (mdwFOTGPort(0x80) &=  (~BIT5))      
    
#define mdwOTGC_Control_A_SET_B_HNP_EN_Rd()	  (mdwFOTGPort(0x80)& BIT6)	
#define mdwOTGC_Control_A_SET_B_HNP_EN_Set()	  (mdwFOTGPort(0x80) |=  BIT6)    
#define mdwOTGC_Control_A_SET_B_HNP_EN_Clr()	  (mdwFOTGPort(0x80) &=  (~BIT6))    
    
#define mdwOTGC_Control_A_SRP_DET_EN_Rd()	  (mdwFOTGPort(0x80)& BIT7)	
#define mdwOTGC_Control_A_SRP_DET_EN_Set()	  (mdwFOTGPort(0x80) |=  BIT7)       
#define mdwOTGC_Control_A_SRP_DET_EN_Clr()	  (mdwFOTGPort(0x80) &=  (~BIT7))    
    
#define mdwOTGC_Control_A_SRP_RESP_TYPE_Rd()	  (mdwFOTGPort(0x80)& BIT8)	
#define mdwOTGC_Control_A_SRP_RESP_TYPE_Set(b)	  (mdwFOTGPort(0x80) |=  b)      
#define mdwOTGC_Control_A_SRP_RESP_TYPE_Clr()	  (mdwFOTGPort(0x80) &=  (~BIT8))      

#define mdwOTGC_Control_VBUS_FLT_SEL_Rd()	  (mdwFOTGPort(0x80)& BIT10)	
#define mdwOTGC_Control_VBUS_FLT_SEL_Set()	  (mdwFOTGPort(0x80) |=  BIT10)     
#define mdwOTGC_Control_VBUS_FLT_SEL_Clr()	  (mdwFOTGPort(0x80) &=  (~BIT10))  


#define mdwOTGC_Control_B_SESS_END_Rd()	          (mdwFOTGPort(0x80)& BIT16)	  
#define mdwOTGC_Control_B_SESS_VLD_Rd()	          (mdwFOTGPort(0x80)& BIT17)	  
#define mdwOTGC_Control_A_SESS_VLD_Rd()	          (mdwFOTGPort(0x80)& BIT18)	  
#define mdwOTGC_Control_A_VBUS_VLD_Rd()	          (mdwFOTGPort(0x80)& BIT19)  
#define mdwOTGC_Control_CROLE_Rd()	          (mdwFOTGPort(0x80)& BIT20) //0:Host 1:Peripheral
#define mdwOTGC_Control_ID_Rd()	                  (mdwFOTGPort(0x80)& BIT21) //0:A-Device 1:B-Device
#define mdwOTGC_Control_Rd()	                  (mdwFOTGPort(0x80))    
#define mdwOTGC_Control_Speed_Rd()	          ((mdwFOTGPort(0x80)& 0x00C00000)>>22)
  
#define A_SRP_RESP_TYPE_VBUS	                  0x00	  
#define A_SRP_RESP_TYPE_DATA_LINE                 0x100
  

#define mwHost20_Control_ForceFullSpeed_Rd()      (mdwFOTGPort(0x80) &=  BIT12) 	
#define mwHost20_Control_ForceFullSpeed_Set()     (mdwFOTGPort(0x80) |=  BIT12)  	
#define mwHost20_Control_ForceFullSpeed_Clr()	  (mdwFOTGPort(0x80) &=  ~BIT12)    

#define mwHost20_Control_ForceHighSpeed_Rd()      (mdwFOTGPort(0x80) &=  BIT14) 	
#define mwHost20_Control_ForceHighSpeed_Set()     (mdwFOTGPort(0x80) |=  BIT14)  	
#define mwHost20_Control_ForceHighSpeed_Clr()	  (mdwFOTGPort(0x80) &=  ~BIT14)    

  
#define mdwOTG20_Control_Phy_Reset_Set()	  (mdwFOTGPort(0x80)|=BIT15)
#define mdwOTG20_Control_Phy_Reset_Clr()	  (mdwFOTGPort(0x80) &=  ~BIT15) 
#define mdwOTG20_Control_OTG_Reset_Set()	  (mdwFOTGPort(0x80)|=BIT24)
#define mdwOTG20_Control_OTG_Reset_Clr()	  (mdwFOTGPort(0x80) &=  ~BIT24)   
  
  
  
  
   //Offset:0x84(OTG Interrupt Status Register) 
#define mdwOTGC_INT_STS_Rd()                      (mdwFOTGPort(0x84))
#define mdwOTGC_INT_STS_Clr(wValue)               (mdwFOTGPort(0x84) |= wValue)    

#define OTGC_INT_BSRPDN                           BIT0  
#define OTGC_INT_ASRPDET                          BIT4
#define OTGC_INT_AVBUSERR                         BIT5
#define OTGC_INT_RLCHG                            BIT8
#define OTGC_INT_IDCHG                            BIT9
#define OTGC_INT_OVC                              BIT10
#define OTGC_INT_BPLGRMV                          BIT11
#define OTGC_INT_APLGRMV                          BIT12

#define OTGC_INT_A_TYPE                           (OTGC_INT_ASRPDET|OTGC_INT_AVBUSERR|OTGC_INT_OVC|OTGC_INT_RLCHG|OTGC_INT_IDCHG|OTGC_INT_APLGRMV)
#define OTGC_INT_B_TYPE                           (OTGC_INT_AVBUSERR|OTGC_INT_OVC|OTGC_INT_RLCHG|OTGC_INT_IDCHG)



   //Offset:0x088(OTG Interrupt Enable Register) 
#define mdwOTGC_INT_Enable_Rd()                   (mdwFOTGPort(0x88))
#define mdwOTGC_INT_Enable_Set(wValue)            (mdwFOTGPort(0x88)|= wValue)
#define mdwOTGC_INT_Enable_Clr(wValue)            (mdwFOTGPort(0x88)&= (~wValue))

#define mdwOTGC_INT_Enable_BSRPDN_Set()           (mdwFOTGPort(0x88) |=  BIT0)   
#define mdwOTGC_INT_Enable_ASRPDET_Set()          (mdwFOTGPort(0x88) |=  BIT4)   
#define mdwOTGC_INT_Enable_AVBUSERR_Set()         (mdwFOTGPort(0x88) |=  BIT5)   
#define mdwOTGC_INT_Enable_RLCHG_Set()            (mdwFOTGPort(0x88) |=  BIT8)   
#define mdwOTGC_INT_Enable_IDCHG_Set()            (mdwFOTGPort(0x88) |=  BIT9)   
#define mdwOTGC_INT_Enable_OVC_Set()              (mdwFOTGPort(0x88) |=  BIT10)   
#define mdwOTGC_INT_Enable_BPLGRMV_Set()          (mdwFOTGPort(0x88) |=  BIT11)   
#define mdwOTGC_INT_Enable_APLGRMV_Set()          (mdwFOTGPort(0x88) |=  BIT12)   

#define mdwOTGC_INT_Enable_BSRPDN_Clr()           (mdwFOTGPort(0x88) &= ~BIT0)        
#define mdwOTGC_INT_Enable_ASRPDET_Clr()          (mdwFOTGPort(0x88) &= ~BIT4)        
#define mdwOTGC_INT_Enable_AVBUSERR_Clr()         (mdwFOTGPort(0x88) &= ~BIT5)        
#define mdwOTGC_INT_Enable_RLCHG_Clr()            (mdwFOTGPort(0x88) &= ~BIT8)        
#define mdwOTGC_INT_Enable_IDCHG_Clr()            (mdwFOTGPort(0x88) &= ~BIT9)        
#define mdwOTGC_INT_Enable_OVC_Clr()              (mdwFOTGPort(0x88) &= ~BIT10)        
#define mdwOTGC_INT_Enable_BPLGRMV_Clr()          (mdwFOTGPort(0x88) &= ~BIT11)       
#define mdwOTGC_INT_Enable_APLGRMV_Clr()          (mdwFOTGPort(0x88) &= ~BIT12)       

   //Offset:0x0C0();;Interrupt Status bit 
#define mdwOTGC_GINT_STS_HOST_Rd()                (mdwFOTGPort(0xC0) &=  BIT2)   
#define mdwOTGC_GINT_STS_HOST_WClr()              (mdwFOTGPort(0xC0) |=  BIT2)   
#define mdwOTGC_GINT_STS_OTG_Rd()                 (mdwFOTGPort(0xC0) &=  BIT1)   
#define mdwOTGC_GINT_STS_OTG_WClr()               (mdwFOTGPort(0xC0) |=  BIT1)   
#define mdwOTGC_GINT_STS_PERIPHERAL_Rd()          (mdwFOTGPort(0xC0) &=  BIT0)   
#define mdwOTGC_GINT_STS_PERIPHERAL_WClr()        (mdwFOTGPort(0xC0) |=  BIT0)

   //Offset:0x0C4();;Interrupt Mask bit 	
#define mdwOTGC_GINT_HI_ACTIVE_Set()              (mdwFOTGPort(0xC4) |=  BIT3)   
#define mdwOTGC_GINT_HI_ACTIVE_Clr()              (mdwFOTGPort(0xC4) &=  ~BIT3)   
#define mdwOTGC_GINT_MASK_HOST_Set()              (mdwFOTGPort(0xC4) |=  BIT2)   
#define mdwOTGC_GINT_MASK_HOST_Clr()              (mdwFOTGPort(0xC4) &=  ~BIT2)   
#define mdwOTGC_GINT_MASK_OTG_Set()               (mdwFOTGPort(0xC4) |=  BIT1)   
#define mdwOTGC_GINT_MASK_OTG_Clr()               (mdwFOTGPort(0xC4) &=  ~BIT1)   
#define mdwOTGC_GINT_MASK_PERIPHERAL_Set()        (mdwFOTGPort(0xC4) |=  BIT0)   
#define mdwOTGC_GINT_MASK_PERIPHERAL_Clr()        (mdwFOTGPort(0xC4) &=  ~BIT0)   
	


//### For Peripheral #########################
   //Offset:0x100()
#define mdwOTGC_ChipEnable_Set()                  (mdwFOTGPort(0x100)|=BIT5)
#define mdwOTGC_HALFSPEEDEnable_Set()             (mdwFOTGPort(0x100)|=BIT1))


//### For Host #########################           

#define mbHost20_USBCMD_RunStop_Rd()	          (mdwFOTGPort(0x10) &=  BIT0)      
#define mbHost20_USBCMD_RunStop_Set()	          (mdwFOTGPort(0x10) |=  BIT0)       
#define mbHost20_USBCMD_RunStop_Clr()	          (mdwFOTGPort(0x10) &=  ~BIT0)       

#define mbHost20_USBCMD_AsynchronousEnable_Rd()   (mdwFOTGPort(0x10) &=  BIT5) 
#define mbHost20_USBCMD_AsynchronousEnable_Set()  (mdwFOTGPort(0x10) |=  BIT5) 
#define mbHost20_USBCMD_AsynchronousEnable_Clr()  (mdwFOTGPort(0x10) &=  ~BIT5)

#define mbHost20_USBCMD_PeriodicEnable_Rd()       (mdwFOTGPort(0x10) &=  BIT4) 	
#define mbHost20_USBCMD_PeriodicEnable_Set()      (mdwFOTGPort(0x10) |=  BIT4)  	
#define mbHost20_USBCMD_PeriodicEnable_Clr()	  (mdwFOTGPort(0x10) &=  ~BIT4)  

   //For Host some interrupt
#define mdwHost20_USBINTR_Rd()                    (mdwFOTGPort(0x18))
#define mdwHost20_USBINTR_Set(dwValue)            (mdwFOTGPort(0x18)=dwValue)
#define mdwHost20_USBSTS_Rd()                     (mdwFOTGPort(0x14))
#define mdwHost20_USBSTS_Set(dwValue)             (mdwFOTGPort(0x14)=dwValue)

#define mwHost20_PORTSC_ConnectStatus_Rd()	  (mdwFOTGPort(0x30)|=BIT0) 

//For Host Port Reset
#define mwHost20_PORTSC_PortReset_Rd()		  (mdwFOTGPort(0x30) &=  BIT8) 	     	
#define mwHost20_PORTSC_PortReset_Set()		  (mdwFOTGPort(0x30) =  BIT8)      	
#define mwHost20_PORTSC_PortReset_Clr()		  (mdwFOTGPort(0x30) =  0)     	


//For AP Command Definition
	
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


