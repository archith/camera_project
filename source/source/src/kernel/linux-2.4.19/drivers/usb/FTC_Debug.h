
//Debug Information Condition
#define COND_HOST_OFF 	         0x00
#define COND_HOST_HCD_FUN        0x01
#define COND_HOST_HUB_FUN        0x02
#define COND_HOST_USB_FUN        0x04
#define COND_HOST_EHCI_FUN       0x08
#define COND_HOST_TEMP	         0x80

#define USB_DBG 	(COND_HOST_OFF)//General Setting

#define xprintk(dev,level,fmt,args...) printk(level "%s : " fmt , driver_name , ## args)
#define wprintk(level,fmt,args...) printk(level "%s : " fmt , driver_name , ## args)

//DBG_HOST_HCD
#if (USB_DBG & COND_HOST_HCD_FUN)  
#define DBG_HOST_HCD(fmt,args...) printk(KERN_INFO "%s : " fmt,"HCD=>" ,  ## args)
#else
#define DBG_HOST_HCD(fmt,args...)
#endif
//DBG_HOST_HUB
#if (USB_DBG & COND_HOST_HUB_FUN)  
#define DBG_HOST_HUB(fmt,args...) printk(KERN_INFO "%s : " fmt,"HUB=> " ,  ## args)
#else
#define DBG_HOST_HUB(fmt,args...)
#endif
//DBG_HOST_USB
#if (USB_DBG & COND_HOST_USB_FUN)  
#define DBG_HOST_USB(fmt,args...) printk(KERN_INFO "%s : " fmt,"USB=> " ,  ## args)
#else
#define DBG_HOST_USB(fmt,args...)
#endif

//DBG_HOST_EHCI
#if (USB_DBG & COND_HOST_EHCI_FUN)  
#define DBG_HOST_EHCI(fmt,args...) printk(KERN_INFO "%s : " fmt,"EHCI=> " ,  ## args)
#else
#define DBG_HOST_EHCI(fmt,args...)
#endif
//DBG_HOST_TEMP
#if (USB_DBG & COND_HOST_TEMP)  
#define DBG_HOST_TEMP(fmt,args...) printk(KERN_INFO "%s : " fmt,"TEMP=> " ,  ## args)
#else
#define DBG_HOST_TEMP(fmt,args...)
#endif

//******************* Others 
//DBG(DEBUG)
#ifdef DEBUG
#define DBG(dev,fmt,args...) xprintk(dev , KERN_DEBUG , fmt , ## args)
#else
#define DBG(dev,fmt,args...) do { } while (0)
#endif /* DEBUG */

//VDBG(VERBOSE)
#define VERBOSE
 
#ifdef VERBOSE
#define VDBG DBG
#else
#define VDBG(dev,fmt,args...) do { } while (0)
#endif	/* VERBOSE */

//ERROR,WARN,INFO
#define ERROR(dev,fmt,args...) xprintk(dev , KERN_ERR , fmt , ## args)
#define WARN(dev,fmt,args...) xprintk(dev , KERN_WARNING , fmt , ## args)
#define INFO(dev,fmt,args...) xprintk(dev , KERN_INFO , fmt , ## args)



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
	
#define UINT8           u8
#define UINT16          u16
#define UINT32          u32



#define HOST_BASE_ADDRESS	                      (IO_ADDRESS(CPE_FOTG200_BASE))
#define mbHostPort(bOffset)	                       *((volatile u8 *) ( HOST_BASE_ADDRESS |  (u32)bOffset))
#define mwHostPort(bOffset)	                       *((volatile u16 *) ( HOST_BASE_ADDRESS |  (u32)bOffset))
#define mdwHostPort(bOffset)	                        *((volatile u32 *) ( HOST_BASE_ADDRESS |  (u32)bOffset))
