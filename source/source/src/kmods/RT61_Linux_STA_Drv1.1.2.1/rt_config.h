/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2006, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    rt_config.h

    Abstract:
    Central header file to maintain all include files for all driver routines.

    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------
    Rory Chen   02-10-2005    Porting from RT2500

*/

#ifndef __RT_CONFIG_H__
#define __RT_CONFIG_H__

//#define PROFILE_PATH                "/etc/rt61sta.dat"
#define PROFILE_PATH                "/mnt/ramdisk/rt61sta.dat"
#define NIC_DEVICE_NAME             "RT61STA"
// Super mode, RT2561S: super(high throughput, aggregiation, piggy back), RT2561T: Package type(TQFP)
#define RT2561_IMAGE_FILE_NAME      "/etc/rt2561.bin"
#define RT2561S_IMAGE_FILE_NAME     "/etc/rt2561s.bin"
#define RT2661_IMAGE_FILE_NAME      "/etc/rt2661.bin"
#define RALINK_PASSPHRASE           "Ralink"
#define DRIVER_VERSION				"1.1.2.1"

// Query from UI
#define DRV_MAJORVERSION		1
#define DRV_MINORVERSION		1
#define DRV_SUBVERSION	        1	
#define DRV_TESTVERSION	        0	
#define DRV_YEAR		        2007
#define DRV_MONTH		        5 
#define DRV_DAY		            22

/* Operational parameters that are set at compile time. */
#if !defined(__OPTIMIZE__)  ||  !defined(__KERNEL__)
#warning  You must compile this file with the correct options!
#warning  See the last lines of the source file.
#error  You must compile this driver with "-O".
#endif

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>

#include <linux/init.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/ethtool.h>
#include <linux/wireless.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/if_arp.h>
#include <linux/ctype.h>
#include <linux/sockios.h>
#include <linux/threads.h>

#if LINUX_VERSION_CODE >= 0x20407
#include <linux/mii.h>
#endif

// load firmware
#define __KERNEL_SYSCALLS__
#include <linux/unistd.h>
#include <asm/uaccess.h>

#define NONCOPY_RX
#define THREAD_ISR  /* IRQ tasklet */


#ifndef ULONG
#define CHAR            char
#define INT             int
#define SHORT           int
#define UINT            u32
#define ULONG           u32
#define USHORT          u16
#define UCHAR           u8

#define uint32			u32
#define uint8			u8


#define BOOLEAN         u8
//#define LARGE_INTEGER s64
#define VOID            void
#define LONG            int
#define LONGLONG        s64
#define ULONGLONG       u64
typedef VOID            *PVOID;
typedef CHAR            *PCHAR;
typedef UCHAR           *PUCHAR;
typedef USHORT          *PUSHORT;
typedef LONG            *PLONG;
typedef ULONG           *PULONG;

typedef union _LARGE_INTEGER {
    struct {
        ULONG LowPart;
        LONG HighPart;
    }vv;
    struct {
        ULONG LowPart;
        LONG HighPart;
    } u;
    s64 QuadPart;
} LARGE_INTEGER;

#endif

#define ETH_LENGTH_OF_ADDRESS	6	//Add by Zero:Jul.05.2007

#define IN
#define OUT

#define TRUE        1
#define FALSE       0

#define NDIS_STATUS                             INT
#define NDIS_STATUS_SUCCESS                     0x00
#define NDIS_STATUS_FAILURE                     0x01
#define NDIS_STATUS_RESOURCES                   0x03


// ** Wireless Extensions **
// 1. wireless events support        : v14 or newer
// 2. requirements of wpa-supplicant : v15 or newer
#if defined(RALINK_WPA_SUPPLICANT_SUPPORT) && defined(NATIVE_WPA_SUPPLICANT_SUPPORT)
#error "The compile flag RALINK_WPA_SUPPLICANT_SUPPORT and NATIVE_WPA_SUPPLICANT_SUPPORT must exclusively exists"
#endif // NATIVE_WPA_SUPPLICANT_SUPPORT //

#if WIRELESS_EXT < 15
#if defined(RALINK_WPA_SUPPLICANT_SUPPORT) || defined(NATIVE_WPA_SUPPLICANT_SUPPORT)
#error "You should upgrade your wireless_extension to 15 or upper to support wpa_supplicant"
#endif // NATIVE_WPA_SUPPLICANT_SUPPORT //
#endif


#include    "rtmp_type.h"
#include    "rtmp_def.h"
#include    "rt2661.h"
#include    "rtmp.h"
#include    "mlme.h"
#include    "oid.h"
#include    "wpa.h"
#include    "md5.h"


//---> Add by Zero:Jul.04.2007
#ifdef MAT_SUPPORT
#include "mat.h"
#endif // MAT_SUPPORT //
//<---

#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
#include	"rtmp_wext.h"

#define		MAX_WPA_IE_LEN 64 // Used for SIOCSIWGENIE/SIOCGIWGENIE. Sync to kernelsource/include/net/ieee80211.h
#endif // NATIVE_WPA_SUPPLICANT_SUPPORT //

#define	NIC2661_PCI_DEVICE_ID	    0x0401
#define NIC2561Turbo_PCI_DEVICE_ID  0x0301
#define NIC2561_PCI_DEVICE_ID	    0x0302
#define	NIC_PCI_VENDOR_ID		    0x1814


/* Check CONFIG_SMP defined*/
#ifndef NR_CPUS
#define NR_CPUS 1
#endif
#if NR_CPUS > 1
#define MEM_ALLOC_FLAG      GFP_ATOMIC
#else
#define MEM_ALLOC_FLAG      (GFP_DMA | GFP_ATOMIC)
#endif

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) ((a)*65536+(b)*256+(c))
#endif

#if 1
#define RT_READ_PROFILE     // Driver reads RaConfig profile parameters from rt61sta.dat
#endif
                          
            
#endif  // __RT_CONFIG_H__
