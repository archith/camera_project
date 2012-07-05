/*
 * file_storage.c -- File-backed USB Storage Gadget, for USB development
 *
 * Copyright (C) 2003 Alan Stern
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the above-listed copyright holders may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * The File-backed Storage Gadget acts as a USB Mass Storage device,
 * appearing to the host as a disk drive.  In addition to providing an
 * example of a genuinely useful gadget driver for a USB device, it also
 * illustrates a technique of double-buffering for increased throughput.
 * Last but not least, it gives an easy way to probe the behavior of the
 * Mass Storage drivers in a USB host.
 *
 * Backing storage is provided by a regular file or a block device, specified
 * by the "file" module parameter.  Access can be limited to read-only by
 * setting the optional "ro" module parameter.
 *
 * The gadget supports the Control-Bulk (CB), Control-Bulk-Interrupt (CBI),
 * and Bulk-Only (also known as Bulk-Bulk-Bulk or BBB) transports, selected
 * by the optional "transport" module parameter.  It also supports the
 * following protocols: RBC (0x01), ATAPI or SFF-8020i (0x02), QIC-157 (0c03),
 * UFI (0x04), SFF-8070i (0x05), and transparent SCSI (0x06), selected by
 * the optional "protocol" module parameter.  For testing purposes the
 * gadget will indicate that it has removable media if the optional
 * "removable" module parameter is set.  In addition, the default Vendor ID,
 * Product ID, and release number can be overridden.
 *
 * There is support for multiple logical units (LUNs), each of which has
 * its own backing file.  The number of LUNs can be set using the optional
 * "luns" module parameter (anywhere from 1 to 8), and the corresponding
 * files are specified using comma-separated lists for "file" and "ro".
 * The default number of LUNs is taken from the number of "file" elements;
 * it is 1 if "file" is not given.  If "removable" is not set then a backing
 * file must be specified for each LUN.  If it is set, then an unspecified
 * or empty backing filename means the LUN's medium is not loaded.
 *
 * Requirements are modest; only a bulk-in and a bulk-out endpoint are
 * needed (an interrupt-out endpoint is also needed for CBI).  The memory
 * requirement amounts to two 16K buffers, size configurable by a parameter.
 * Support is included for both full-speed and high-speed operation.
 *
 * Module options:
 *
 *	file=filename[,filename...]
 *				Required if "removable" is not set, names of
 *					the files or block devices used for
 *					backing storage
 *	ro=b[,b...]		Default false, booleans for read-only access
 *	luns=N			Default N = number of filenames, number of
 *					LUNs to support
 *	transport=XXX		Default BBB, transport name (CB, CBI, or BBB)
 *	protocol=YYY		Default SCSI, protocol name (RBC, 8020 or
 *					ATAPI, QIC, UFI, 8070, or SCSI;
 *					also 1 - 6)
 *	removable		Default false, boolean for removable media
 *	vendor=0xVVVV		Default 0x0525 (NetChip), USB Vendor ID
 *	product=0xPPPP		Default 0xa4a5 (FSG), USB Product ID
 *	release=0xRRRR		Override the USB release number (bcdDevice)
 *	buflen=N		Default N=16384, buffer size used (will be
 *					rounded down to a multiple of
 *					PAGE_CACHE_SIZE)
 *	stall			Default determined according to the type of
 *					USB device controller (usually true),
 *					boolean to permit the driver to halt
 *					bulk endpoints
 *
 * If CONFIG_USB_FILE_STORAGE_TEST is not set, only the "file" and "ro"
 * options are available; default values are used for everything else.
 *
 * This gadget driver is heavily based on "Gadget Zero" by David Brownell.
 */


/*
 *				Driver Design
 *
 * The FSG driver is fairly straightforward.  There is a main kernel
 * thread that handles most of the work.  Interrupt routines field
 * callbacks from the controller driver: bulk- and interrupt-request
 * completion notifications, endpoint-0 events, and disconnect events.
 * Completion events are passed to the main thread by wakeup calls.  Many
 * ep0 requests are handled at interrupt time, but SetInterface,
 * SetConfiguration, and device reset requests are forwarded to the
 * thread in the form of "exceptions" using SIGUSR1 signals (since they
 * should interrupt any ongoing file I/O operations).
 *
 * The thread's main routine implements the standard command/data/status
 * parts of a SCSI interaction.  It and its subroutines are full of tests
 * for pending signals/exceptions -- all this polling is necessary since
 * the kernel has no setjmp/longjmp equivalents.  (Maybe this is an
 * indication that the driver really wants to be running in userspace.)
 * An important point is that so long as the thread is alive it keeps an
 * open reference to the backing file.  This will prevent unmounting
 * the backing file's underlying filesystem and could cause problems
 * during system shutdown, for example.  To prevent such problems, the
 * thread catches INT, TERM, and KILL signals and converts them into
 * an EXIT exception.
 *
 * In normal operation the main thread is started during the gadget's
 * fsg_bind() callback and stopped during fsg_unbind().  But it can also
 * exit when it receives a signal, and there's no point leaving the
 * gadget running when the thread is dead.  So just before the thread
 * exits, it deregisters the gadget driver.  This makes things a little
 * tricky: The driver is deregistered at two places, and the exiting
 * thread can indirectly call fsg_unbind() which in turn can tell the
 * thread to exit.  The first problem is resolved through the use of the
 * REGISTERED atomic bitflag; the driver will only be deregistered once.
 * The second problem is resolved by having fsg_unbind() check
 * fsg->state; it won't try to stop the thread if the state is already
 * FSG_STATE_TERMINATED.
 *
 * To provide maximum throughput, the driver uses a circular pipeline of
 * buffer heads (struct fsg_buffhd).  In principle the pipeline can be
 * arbitrarily long; in practice the benefits don't justify having more
 * than 2 stages (i.e., double buffering).  But it helps to think of the
 * pipeline as being a long one.  Each buffer head contains a bulk-in and
 * a bulk-out request pointer (since the buffer can be used for both
 * output and input -- directions always are given from the host's
 * point of view) as well as a pointer to the buffer and various state
 * variables.
 *
 * Use of the pipeline follows a simple protocol.  There is a variable
 * (fsg->next_buffhd_to_fill) that points to the next buffer head to use.
 * At any time that buffer head may still be in use from an earlier
 * request, so each buffer head has a state variable indicating whether
 * it is EMPTY, FULL, or BUSY.  Typical use involves waiting for the
 * buffer head to be EMPTY, filling the buffer either by file I/O or by
 * USB I/O (during which the buffer head is BUSY), and marking the buffer
 * head FULL when the I/O is complete.  Then the buffer will be emptied
 * (again possibly by USB I/O, during which it is marked BUSY) and
 * finally marked EMPTY again (possibly by a completion routine).
 *
 * A module parameter tells the driver to avoid stalling the bulk
 * endpoints wherever the transport specification allows.  This is
 * necessary for some UDCs like the SuperH, which cannot reliably clear a
 * halt on a bulk endpoint.  However, under certain circumstances the
 * Bulk-only specification requires a stall.  In such cases the driver
 * will halt the endpoint and set a flag indicating that it should clear
 * the halt in software during the next device reset.  Hopefully this
 * will permit everything to work correctly.
 *
 * One subtle point concerns sending status-stage responses for ep0
 * requests.  Some of these requests, such as device reset, can involve
 * interrupting an ongoing file I/O operation, which might take an
 * arbitrarily long time.  During that delay the host might give up on
 * the original ep0 request and issue a new one.  When that happens the
 * driver should not notify the host about completion of the original
 * request, as the host will no longer be waiting for it.  So the driver
 * assigns to each ep0 request a unique tag, and it keeps track of the
 * tag value of the request associated with a long-running exception
 * (device-reset, interface-change, or configuration-change).  When the
 * exception handler is finished, the status-stage response is submitted
 * only if the current ep0 request tag is equal to the exception request
 * tag.  Thus only the most recently received ep0 request will get a
 * status-stage response.
 *
 * Warning: This driver source file is too long.  It ought to be split up
 * into a header file plus about 3 separate .c files, to handle the details
 * of the Gadget, USB Mass Storage, and SCSI protocols.
 */



#include <linux/config.h>

#include <asm/system.h>
#include <asm/uaccess.h>

#include <linux/bitops.h>
#include <linux/blkdev.h>
#include <linux/compiler.h>
#include <linux/completion.h>
#include <linux/dcache.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/limits.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/rwsem.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/uts.h>
#include <linux/version.h>
#include <linux/wait.h>

#include <linux/usb_ch9.h>
#include <linux/usb_gadget.h>

#define FOTG2XX_AVOID_REDEFINE //For OTG
#include <linux/usb.h>

/*-------------------------------------------------------------------------*/


#undef DEBUG
#undef VERBOSE
#undef DUMP_MSGS

//#define DEBUG
//#define VERBOSE
//#define DUMP_MSGS

#define DRIVER_DESC		"File-backed Storage Gadget"
#define DRIVER_NAME		"g_file_storage"
#define DRIVER_VERSION		"14 January 2004"


static const char  sysname[] = UTS_SYSNAME;//For OTG200 CV1-2


static const char longname[] = DRIVER_DESC;
static const char shortname[] = DRIVER_NAME;


MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("Alan Stern");
MODULE_LICENSE("Dual BSD/GPL");

/* Thanks to NetChip Technologies for donating this product ID.
 *
 * DO NOT REUSE THESE IDs with any other driver!!  Ever!!
 * Instead:  allocate your own, using normal USB-IF procedures. */
//#define DRIVER_VENDOR_ID	0x0525	// NetChip
//#define DRIVER_PRODUCT_ID	0xa4a5	// Linux-USB File-backed Storage Gadget

#define DRIVER_VENDOR_ID	0x2310	//Faraday
#define DRIVER_PRODUCT_ID	0x6688	// Linux-USB File-backed Storage Gadget

/*-------------------------------------------------------------------------*/

/*
 * Hardware-specific configuration, controlled by which device
 * controller driver was configured.
 *
 * CHIP ... hardware identifier
 * DRIVER_VERSION_NUM ... alerts the host side driver to differences
 * EP_*_NAME ... which endpoints do we use for which purpose?
 * EP_*_NUM ... numbers for them (often limited by hardware)
 * FS_BULK_IN_MAXPACKET ... maxpacket value for full-speed bulk-in ep
 * FS_BULK_OUT_MAXPACKET ... maxpacket value for full-speed bulk-out ep
 * HIGHSPEED ... define if ep0 and descriptors need high speed support
 * MAX_USB_POWER ... define if we use other than 100 mA bus current
 * SELFPOWER ... if we can run on bus power, zero
 * NO_BULK_STALL ... bulk endpoint halts don't work well so avoid them
 */


/*
 * NetChip 2280, PCI based.
 *
 * This has half a dozen configurable endpoints, four with dedicated
 * DMA channels to manage their FIFOs.  It supports high speed.
 * Those endpoints can be arranged in any desired configuration.
 */
#ifdef	CONFIG_USB_GADGET_NET2280
#define CHIP			"net2280"
#define DRIVER_VERSION_NUM	0x0211
static const char EP_BULK_IN_NAME[] = "ep-a";
#define EP_BULK_IN_NUM		1
#define FS_BULK_IN_MAXPACKET	64
static const char EP_BULK_OUT_NAME[] = "ep-b";
#define EP_BULK_OUT_NUM		2
#define FS_BULK_OUT_MAXPACKET	64
static const char EP_INTR_IN_NAME[] = "ep-e";
#define EP_INTR_IN_NUM		5
#define HIGHSPEED
#endif


/*
 * Dummy_hcd, software-based loopback controller.
 *
 * This imitates the abilities of the NetChip 2280, so we will use
 * the same configuration.
 */
#ifdef	CONFIG_USB_GADGET_DUMMY
#define CHIP			"dummy"
#define DRIVER_VERSION_NUM	0x0212
static const char EP_BULK_IN_NAME[] = "ep-a";
#define EP_BULK_IN_NUM		1
#define FS_BULK_IN_MAXPACKET	64
static const char EP_BULK_OUT_NAME[] = "ep-b";
#define EP_BULK_OUT_NUM		2
#define FS_BULK_OUT_MAXPACKET	64
static const char EP_INTR_IN_NAME[] = "ep-e";
#define EP_INTR_IN_NUM		5
#define HIGHSPEED
#endif


/*
 * PXA-2xx UDC:  widely used in second gen Linux-capable PDAs.
 *
 * This has fifteen fixed-function full speed endpoints, and it
 * can support all USB transfer types.
 *
 * These supports three or four configurations, with fixed numbers.
 * The hardware interprets SET_INTERFACE, net effect is that you
 * can't use altsettings or reset the interfaces independently.
 * So stick to a single interface.
 */
#ifdef	CONFIG_USB_GADGET_PXA2XX
#define CHIP			"pxa2xx"
#define DRIVER_VERSION_NUM	0x0213
static const char EP_BULK_IN_NAME[] = "ep1in-bulk";
#define EP_BULK_IN_NUM		1
#define FS_BULK_IN_MAXPACKET	64
static const char EP_BULK_OUT_NAME[] = "ep2out-bulk";
#define EP_BULK_OUT_NUM		2
#define FS_BULK_OUT_MAXPACKET	64
static const char EP_INTR_IN_NAME[] = "ep6in-bulk";
#define EP_INTR_IN_NUM		6
#endif


/*
 * SuperH UDC:  UDC built-in to some Renesas SH processors.
 *
 * This has three fixed-function full speed bulk/interrupt endpoints.
 *
 * Only one configuration and interface is supported (SET_CONFIGURATION
 * and SET_INTERFACE are handled completely by the hardware).
 */
#ifdef	CONFIG_USB_GADGET_SUPERH
#define CHIP			"superh"
#define DRIVER_VERSION_NUM	0x0215
static const char EP_BULK_IN_NAME[] = "ep2in-bulk";
#define EP_BULK_IN_NUM		2
#define FS_BULK_IN_MAXPACKET	64
static const char EP_BULK_OUT_NAME[] = "ep1out-bulk";
#define EP_BULK_OUT_NUM		1
#define FS_BULK_OUT_MAXPACKET	64
static const char EP_INTR_IN_NAME[] = "ep3in-bulk";
#define EP_INTR_IN_NUM		3
#define NO_BULK_STALL
#endif


/*
 * Toshiba TC86C001 ("Goku-S") UDC
 *
 * This has three semi-configurable full speed bulk/interrupt endpoints.
 */
#ifdef	CONFIG_USB_GADGET_GOKU
#define CHIP			"goku"
#define DRIVER_VERSION_NUM	0x0216
static const char EP_BULK_OUT_NAME [] = "ep1-bulk";
#define EP_BULK_OUT_NUM		1
#define FS_BULK_IN_MAXPACKET	64
static const char EP_BULK_IN_NAME [] = "ep2-bulk";
#define EP_BULK_IN_NUM		2
#define FS_BULK_OUT_MAXPACKET	64
static const char EP_INTR_IN_NAME [] = "ep3-bulk";
#define EP_INTR_IN_NUM		3
#endif


//FUSB220;;12152004;;Start;;Bruce;;Modify
#ifdef	CONFIG_USB_GADGET_FTC220
#define CHIP			"FTC220"
#define DRIVER_VERSION_NUM	0x0217
static const char EP_BULK_IN_NAME[] = "ep1-bulkin";
#define EP_BULK_IN_NUM		1
#define FS_BULK_IN_MAXPACKET	64
static const char EP_BULK_OUT_NAME[] = "ep2-bulkout";
#define EP_BULK_OUT_NUM		2
#define FS_BULK_OUT_MAXPACKET	64
static const char EP_INTR_IN_NAME[] = "ep3-intin";
#define EP_INTR_IN_NUM		5
#define HIGHSPEED
static const char driver_name [] = "FTC_udc";
#define NO_BULK_STALL 
#endif
//FUSB220;;12152004;;End;;Bruce;;Modify

//FOTG200;;02212005;;Start;;Bruce;;Modify
#ifdef	CONFIG_USB_GADGET_FOTG2XX
#define CHIP			"FOTG200-Peripheral"
#define DRIVER_VERSION_NUM	0x0218
static const char EP_BULK_IN_NAME[] = "ep1-bulkin";
#define EP_BULK_IN_NUM		1
#define FS_BULK_IN_MAXPACKET	64
static const char EP_BULK_OUT_NAME[] = "ep2-bulkout";
#define EP_BULK_OUT_NUM		2
#define FS_BULK_OUT_MAXPACKET	64
static const char EP_INTR_IN_NAME[] = "ep3-intin";
#define EP_INTR_IN_NUM		5
#define HIGHSPEED
static const char driver_name [] = "FTC_udc";
#define NO_BULK_STALL 
#endif
//FOTG200;;02212005;;End;;Bruce;;Modify

/*-------------------------------------------------------------------------*/

#ifndef CHIP
#	error Configure some USB peripheral controller driver!
#endif

/* Power usage is config specific.
 * Hardware that supports remote wakeup defaults to disabling it.
 */
#ifndef	SELFPOWER
/* default: say we're self-powered */
#define SELFPOWER USB_CONFIG_ATT_SELFPOWER
/* else:
 * - SELFPOWER value must be zero
 * - MAX_USB_POWER may be nonzero.
 */
#endif

#ifndef	MAX_USB_POWER
/* Any hub supports this steady state bus power consumption */
#define MAX_USB_POWER	100	/* mA */
#endif

/* We don't support remote wake-up */

#ifdef NO_BULK_STALL
#define CAN_STALL	0
#else
#define CAN_STALL	1
#endif


/*-------------------------------------------------------------------------*/

#define fakedev_printk(level, dev, format, args...) \
	printk(level "%s %s: " format , DRIVER_NAME , (dev)->name , ## args)

#define xprintk(f,level,fmt,args...) \
	fakedev_printk(level , (f)->gadget , fmt , ## args)
#define yprintk(l,level,fmt,args...) \
	fakedev_printk(level , &(l)->dev , fmt , ## args)




#define DBG_TEMP2(fmt,args...) \
    printk(KERN_INFO "%s : " fmt , "fsg" , ## args)//Bruce



 
#ifdef DEBUG


#define DBG_FUNCC(fmt,args...) \
    printk(KERN_INFO "%s : " fmt , "fsg" , ## args)//Bruce
#define DBG_TEMP(fmt,args...) \
    printk(KERN_INFO "%s : " fmt , "fsg" , ## args)//Bruce
//KERN_DEBUG   
#define DBG(fsg,fmt,args...) \
	xprintk(fsg , KERN_INFO , fmt , ## args)
#define LDBG(lun,fmt,args...) \
	yprintk(lun , KERN_INFO , fmt , ## args)
#define MDBG(fmt,args...) \
	printk(KERN_INFO DRIVER_NAME ": " fmt , ## args)
	
	
#else

#define DBG_FUNCC(fmt,args...) \
	do { } while (0)//Bruce
#define DBG_TEMP(fmt,args...) \
	do { } while (0)//Bruce
	
#define DBG(fsg,fmt,args...) \
	do { } while (0)
#define LDBG(lun,fmt,args...) \
	do { } while (0)
#define MDBG(fmt,args...) \
	do { } while (0)
#undef VERBOSE
#undef DUMP_MSGS


#endif /* DEBUG */

#ifdef VERBOSE

#define VDBG	DBG
#define VLDBG	LDBG



#else

#define VDBG(fsg,fmt,args...) \
	do { } while (0)
#define VLDBG(lun,fmt,args...) \
	do { } while (0)
#endif /* VERBOSE */




#define ERROR(fsg,fmt,args...) \
	xprintk(fsg , KERN_ERR , fmt , ## args)
#define LERROR(lun,fmt,args...) \
	yprintk(lun , KERN_ERR , fmt , ## args)

#define WARN(fsg,fmt,args...) \
	xprintk(fsg , KERN_WARNING , fmt , ## args)
#define LWARN(lun,fmt,args...) \
	yprintk(lun , KERN_WARNING , fmt , ## args)

#ifdef INFOMSG//For FOTG200-V1-2

#define INFO(fsg,fmt,args...) \
	xprintk(fsg , KERN_INFO , fmt , ## args)

#define LINFO(lun,fmt,args...) \
	yprintk(lun , KERN_INFO , fmt , ## args)

#define MINFO(fmt,args...) \
	printk(KERN_INFO DRIVER_NAME ": " fmt , ## args)


#else//INFOMSG

#define INFO(fsg,fmt,args...) \
		do { } while (0)

#define LINFO(lun,fmt,args...) \
		do { } while (0)

#define MINFO(fmt,args...) \
		do { } while (0)



#endif//INFOMSG


/*-------------------------------------------------------------------------*/

/* Encapsulate the module parameter settings */

#define MAX_LUNS	8

//static char		*file[MAX_LUNS] = {NULL, };
static char		*file[MAX_LUNS] = {"/ram.img",NULL, };//Bruce;;Modify
//static int		ro[MAX_LUNS] = {0, };
static int		ro[MAX_LUNS] = {0,0, };//Bruce;;Modify
//static unsigned int	luns = 0;
static unsigned int	luns = 1;//Bruce;;Modify
static char 		*transport = "BBB";
static char 		*protocol = "SCSI";
static int 		     removable = 1;//Bruce;;Modify
static unsigned short	vendor = DRIVER_VENDOR_ID;
static unsigned short	product = DRIVER_PRODUCT_ID;
static unsigned short	release = DRIVER_VERSION_NUM;
static unsigned int	buflen = 16384;
static int		stall = CAN_STALL;

static struct {
	unsigned int	nluns;

	char		*transport_parm;
	char		*protocol_parm;
	int		removable;
	unsigned short	vendor;
	unsigned short	product;
	unsigned short	release;
	unsigned int	buflen;
	int		can_stall;

	int		transport_type;
	char		*transport_name;
	int		protocol_type;
	char		*protocol_name;

} mod_data;


MODULE_PARM(file, "1-8s");
MODULE_PARM_DESC(file, "names of backing files or devices");

MODULE_PARM(ro, "1-8b");
MODULE_PARM_DESC(ro, "true to force read-only");


/* In the non-TEST version, only the file and ro module parameters
 * are available. */
#ifdef CONFIG_USB_FILE_STORAGE_TEST

MODULE_PARM(luns, "i");
MODULE_PARM_DESC(luns, "number of LUNs");

MODULE_PARM(transport, "s");
MODULE_PARM_DESC(transport, "type of transport (BBB, CBI, or CB)");

MODULE_PARM(protocol, "s");
MODULE_PARM_DESC(protocol, "type of protocol (RBC, 8020, QIC, UFI, "
		"8070, or SCSI)");

MODULE_PARM(removable, "b");
MODULE_PARM_DESC(removable, "true to simulate removable media");

MODULE_PARM(vendor, "h");
MODULE_PARM_DESC(vendor, "USB Vendor ID");

MODULE_PARM(product, "h");
MODULE_PARM_DESC(product, "USB Product ID");

MODULE_PARM(release, "h");
MODULE_PARM_DESC(release, "USB release number");

MODULE_PARM(buflen, "i");
MODULE_PARM_DESC(buflen, "I/O buffer size");

MODULE_PARM(stall, "i");
MODULE_PARM_DESC(stall, "false to prevent bulk stalls");

#endif /* CONFIG_USB_FILE_STORAGE_TEST */


/*-------------------------------------------------------------------------*/

/* USB protocol value = the transport method */
#define USB_PR_CBI	0x00		// Control/Bulk/Interrupt
#define USB_PR_CB	0x01		// Control/Bulk w/o interrupt
#define USB_PR_BULK	0x50		// Bulk-only

/* USB subclass value = the protocol encapsulation */
#define USB_SC_RBC	0x01		// Reduced Block Commands (flash)
#define USB_SC_8020	0x02		// SFF-8020i, MMC-2, ATAPI (CD-ROM)
#define USB_SC_QIC	0x03		// QIC-157 (tape)
#define USB_SC_UFI	0x04		// UFI (floppy)
#define USB_SC_8070	0x05		// SFF-8070i (removable)
#define USB_SC_SCSI	0x06		// Transparent SCSI

/* Bulk-only data structures */

/* Command Block Wrapper */
struct bulk_cb_wrap {
	u32	Signature;		// Contains 'USBC'
	u32	Tag;			// Unique per command id
	u32	DataTransferLength;	// Size of the data
	u8	Flags;			// Direction in bit 7
	u8	Lun;			// LUN (normally 0)
	u8	Length;			// Of the CDB, <= MAX_COMMAND_SIZE
	u8	CDB[16];		// Command Data Block
};

#define USB_BULK_CB_WRAP_LEN	31
#define USB_BULK_CB_SIG		0x43425355	// Spells out USBC
#define USB_BULK_IN_FLAG	0x80

/* Command Status Wrapper */
struct bulk_cs_wrap {
	u32	Signature;		// Should = 'USBS'
	u32	Tag;			// Same as original command
	u32	Residue;		// Amount not transferred
	u8	Status;			// See below
};

#define USB_BULK_CS_WRAP_LEN	13
#define USB_BULK_CS_SIG		0x53425355	// Spells out 'USBS'
#define USB_STATUS_PASS		0
#define USB_STATUS_FAIL		1
#define USB_STATUS_PHASE_ERROR	2

/* Bulk-only class specific requests */
#define USB_BULK_RESET_REQUEST		0xff
#define USB_BULK_GET_MAX_LUN_REQUEST	0xfe


/* CBI Interrupt data structure */
struct interrupt_data {
	u8	bType;
	u8	bValue;
};

#define CBI_INTERRUPT_DATA_LEN		2

/* CBI Accept Device-Specific Command request */
#define USB_CBI_ADSC_REQUEST		0x00


#define MAX_COMMAND_SIZE	16	// Length of a SCSI Command Data Block

/* SCSI commands that we recognize */
#define SC_FORMAT_UNIT			0x04
#define SC_INQUIRY			0x12
#define SC_MODE_SELECT_6		0x15
#define SC_MODE_SELECT_10		0x55
#define SC_MODE_SENSE_6			0x1a
#define SC_MODE_SENSE_10		0x5a
#define SC_PREVENT_ALLOW_MEDIUM_REMOVAL	0x1e
#define SC_READ_6			0x08
#define SC_READ_10			0x28
#define SC_READ_12			0xa8
#define SC_READ_CAPACITY		0x25
#define SC_READ_FORMAT_CAPACITIES	0x23
#define SC_RELEASE			0x17
#define SC_REQUEST_SENSE		0x03
#define SC_RESERVE			0x16
#define SC_SEND_DIAGNOSTIC		0x1d
#define SC_START_STOP_UNIT		0x1b
#define SC_SYNCHRONIZE_CACHE		0x35
#define SC_TEST_UNIT_READY		0x00
#define SC_VERIFY			0x2f
#define SC_WRITE_6			0x0a
#define SC_WRITE_10			0x2a
#define SC_WRITE_12			0xaa

/* SCSI Sense Key/Additional Sense Code/ASC Qualifier values */
#define SS_NO_SENSE				0
#define SS_COMMUNICATION_FAILURE		0x040800
#define SS_INVALID_COMMAND			0x052000
#define SS_INVALID_FIELD_IN_CDB			0x052400
#define SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE	0x052100
#define SS_LOGICAL_UNIT_NOT_SUPPORTED		0x052500
#define SS_MEDIUM_NOT_PRESENT			0x023a00
#define SS_MEDIUM_REMOVAL_PREVENTED		0x055302
#define SS_NOT_READY_TO_READY_TRANSITION	0x062800
#define SS_RESET_OCCURRED			0x062900
#define SS_SAVING_PARAMETERS_NOT_SUPPORTED	0x053900
#define SS_UNRECOVERED_READ_ERROR		0x031100
#define SS_WRITE_ERROR				0x030c02
#define SS_WRITE_PROTECTED			0x072700

#define SK(x)		((u8) ((x) >> 16))	// Sense Key byte, etc.
#define ASC(x)		((u8) ((x) >> 8))
#define ASCQ(x)		((u8) (x))


/*-------------------------------------------------------------------------*/

/*
 * These definitions will permit the compiler to avoid generating code for
 * parts of the driver that aren't used in the non-TEST version.  Even gcc
 * can recognize when a test of a constant expression yields a dead code
 * path.
 *
 * Also, in the non-TEST version, open_backing_file() is only used during
 * initialization and the sysfs attribute store_xxx routines aren't used
 * at all.  We will define NORMALLY_INIT to mark them as __init so they
 * don't occupy kernel code space unnecessarily.
 */

#ifdef CONFIG_USB_FILE_STORAGE_TEST

#define transport_is_bbb()	(mod_data.transport_type == USB_PR_BULK)
#define transport_is_cbi()	(mod_data.transport_type == USB_PR_CBI)
#define protocol_is_scsi()	(mod_data.protocol_type == USB_SC_SCSI)
#define backing_file_is_open(curlun)	((curlun)->filp != NULL)
#define NORMALLY_INIT

#else

#define transport_is_bbb()	1
#define transport_is_cbi()	0
#define protocol_is_scsi()	1
#define backing_file_is_open(curlun)	1
#define NORMALLY_INIT		__init

#endif /* CONFIG_USB_FILE_STORAGE_TEST */


struct lun {
	struct file	*filp;
	loff_t		file_length;
	loff_t		num_sectors;

	unsigned int	ro : 1;
	unsigned int	prevent_medium_removal : 1;
	unsigned int	registered : 1;

	u32		sense_data;
	u32		sense_data_info;
	u32		unit_attention_data;

#define BUS_ID_SIZE	20
	struct __lun_device {
		char	name[BUS_ID_SIZE];
		void	*driver_data;
	} dev;
};


/* Big enough to hold our biggest descriptor */
#define EP0_BUFSIZE	256
#define DELAYED_STATUS	(EP0_BUFSIZE + 999)	// An impossibly large value

/* Number of buffers we will use.  2 is enough for double-buffering */
#define NUM_BUFFERS	2

enum fsg_buffer_state {
	BUF_STATE_EMPTY = 0,
	BUF_STATE_FULL,
	BUF_STATE_BUSY
};

struct fsg_buffhd {
	void				*buf;
	dma_addr_t			dma;
	volatile enum fsg_buffer_state	state;
	struct fsg_buffhd		*next;

	/* The NetChip 2280 is faster, and handles some protocol faults
	 * better, if we don't submit any short bulk-out read requests.
	 * So we will record the intended request length here. */
	unsigned int			bulk_out_intended_length;

	struct usb_request		*inreq;
	volatile int			inreq_busy;
	struct usb_request		*outreq;
	volatile int			outreq_busy;
};

enum fsg_state {
	FSG_STATE_COMMAND_PHASE = -10,		// This one isn't used anywhere
	FSG_STATE_DATA_PHASE,
	FSG_STATE_STATUS_PHASE,

	FSG_STATE_IDLE = 0,
	FSG_STATE_ABORT_BULK_OUT,
	FSG_STATE_RESET,
	FSG_STATE_INTERFACE_CHANGE,
	FSG_STATE_CONFIG_CHANGE,
	FSG_STATE_DISCONNECT,
	FSG_STATE_EXIT,
	FSG_STATE_TERMINATED
};

enum data_direction {
	DATA_DIR_UNKNOWN = 0,
	DATA_DIR_FROM_HOST,
	DATA_DIR_TO_HOST,
	DATA_DIR_NONE
};

struct fsg_dev {
	/* lock protects: state, all the req_busy's, and cbbuf_cmnd */
	spinlock_t		lock;
	struct usb_gadget	*gadget;

	/* filesem protects: backing files in use */
	struct rw_semaphore	filesem;

	struct usb_ep		*ep0;		// Handy copy of gadget->ep0
	struct usb_request	*ep0req;	// For control responses
	volatile unsigned int	ep0_req_tag;
	const char		*ep0req_name;

	struct usb_request	*intreq;	// For interrupt responses
	volatile int		intreq_busy;
	struct fsg_buffhd	*intr_buffhd;

 	unsigned int		bulk_out_maxpacket;
	enum fsg_state		state;		// For exception handling
	unsigned int		exception_req_tag;

	u8			config, new_config;

	unsigned int		running : 1;
	unsigned int		bulk_in_enabled : 1;
	unsigned int		bulk_out_enabled : 1;
	unsigned int		intr_in_enabled : 1;
	unsigned int		phase_error : 1;
	unsigned int		short_packet_received : 1;
	unsigned int		bad_lun_okay : 1;

	unsigned long		atomic_bitflags;
#define REGISTERED		0
#define CLEAR_BULK_HALTS	1

	struct usb_ep		*bulk_in;
	struct usb_ep		*bulk_out;
	struct usb_ep		*intr_in;

	struct fsg_buffhd	*next_buffhd_to_fill;
	struct fsg_buffhd	*next_buffhd_to_drain;
	struct fsg_buffhd	buffhds[NUM_BUFFERS];

	wait_queue_head_t	thread_wqh;
	int			thread_wakeup_needed;
	struct completion	thread_notifier;
	int			thread_pid;
	struct task_struct	*thread_task;
	sigset_t		thread_signal_mask;

	int			cmnd_size;
	u8			cmnd[MAX_COMMAND_SIZE];
	enum data_direction	data_dir;
	u32			data_size;
	u32			data_size_from_cmnd;
	u32			tag;
	unsigned int		lun;
	u32			residue;
	u32			usb_amount_left;

	/* The CB protocol offers no way for a host to know when a command
	 * has completed.  As a result the next command may arrive early,
	 * and we will still have to handle it.  For that reason we need
	 * a buffer to store new commands when using CB (or CBI, which
	 * does not oblige a host to wait for command completion either). */
	int			cbbuf_cmnd_size;
	u8			cbbuf_cmnd[MAX_COMMAND_SIZE];

	unsigned int		nluns;
	struct lun		*luns;
	struct lun		*curlun;
};

typedef void (*fsg_routine_t)(struct fsg_dev *);

static int inline exception_in_progress(struct fsg_dev *fsg)
{
	return (fsg->state > FSG_STATE_IDLE);
}

/* Make bulk-out requests be divisible by the maxpacket size */
static void inline set_bulk_out_req_length(struct fsg_dev *fsg,
		struct fsg_buffhd *bh, unsigned int length)
{
	unsigned int	rem;
    
    DBG_FUNCC("+ (FileStorage)set_bulk_out_req_length()\n");
    
	bh->bulk_out_intended_length = length;
	rem = length % fsg->bulk_out_maxpacket;
	if (rem > 0)
		length += fsg->bulk_out_maxpacket - rem;
	bh->outreq->length = length;
}

static struct fsg_dev			*the_fsg;
static struct usb_gadget_driver		fsg_driver;

static void	close_backing_file(struct lun *curlun);
static void	close_all_backing_files(struct fsg_dev *fsg);


/*-------------------------------------------------------------------------*/

#ifdef DUMP_MSGS

static void dump_msg(struct fsg_dev *fsg, const char *label,
		const u8 *buf, unsigned int length)
{
	unsigned int	start, num, i;
	char		line[52], *p;

    DBG_FUNCC("+ (FileStorage)dump_msg()\n"); 
     
	if (length >= 512)
		return;
	DBG(fsg, "%s, length %u:\n", label, length);

	start = 0;
	while (length > 0) {
		num = min(length, 16u);
		p = line;
		for (i = 0; i < num; ++i) {
			if (i == 8)
				*p++ = ' ';
			sprintf(p, " %02x", buf[i]);
			p += 3;
		}
		*p = 0;
		printk(KERN_DEBUG "%6x: %s\n", start, line);
		buf += num;
		start += num;
		length -= num;
	}
}

static void inline dump_cdb(struct fsg_dev *fsg)
{}

#else

static void inline dump_msg(struct fsg_dev *fsg, const char *label,
		const u8 *buf, unsigned int length)
{}

static void inline dump_cdb(struct fsg_dev *fsg)
{
	int	i;
	char	cmdbuf[3*MAX_COMMAND_SIZE + 1];

    DBG_FUNCC("+ (FileStorage)dump_cdb()\n");
    
	for (i = 0; i < fsg->cmnd_size; ++i)
		sprintf(cmdbuf + i*3, " %02x", fsg->cmnd[i]);
	VDBG(fsg, "SCSI CDB: %s\n", cmdbuf);

}

#endif /* DUMP_MSGS */


static int fsg_set_halt(struct fsg_dev *fsg, struct usb_ep *ep)
{
	const char	*name;

	if (ep == fsg->bulk_in)
		name = "bulk-in";
	else if (ep == fsg->bulk_out)
		name = "bulk-out";
	else
		name = ep->name;
	DBG(fsg, "%s set halt\n", name);
	return usb_ep_set_halt(ep);
}


/*-------------------------------------------------------------------------*/

/* Routines for unaligned data access */

static u16 inline get_be16(u8 *buf)
{
	return ((u16) buf[0] << 8) | ((u16) buf[1]);
}

static u32 inline get_be32(u8 *buf)
{
	return ((u32) buf[0] << 24) | ((u32) buf[1] << 16) |
			((u32) buf[2] << 8) | ((u32) buf[3]);
}

static void inline put_be16(u8 *buf, u16 val)
{
	buf[0] = val >> 8;
	buf[1] = val;
}

static void inline put_be32(u8 *buf, u32 val)
{
	buf[0] = val >> 24;
	buf[1] = val >> 16;
	buf[2] = val >> 8;
	buf[3] = val;
}


/*-------------------------------------------------------------------------*/

/*
 * DESCRIPTORS ... most are static, but strings and (full) configuration
 * descriptors are built on demand.  Also the (static) config and interface
 * descriptors are adjusted during fsg_bind().
 */
#define STRING_MANUFACTURER	1
#define STRING_PRODUCT		2
#define STRING_SERIAL		3

/* There is only one configuration. */
#define	CONFIG_VALUE		1

static struct usb_device_descriptor
device_desc = {
	.bLength =		sizeof device_desc,
	.bDescriptorType =	USB_DT_DEVICE,

	.bcdUSB =		__constant_cpu_to_le16(0x0200),
	.bDeviceClass =		USB_CLASS_PER_INTERFACE,

	/* The next three values can be overridden by module parameters */
	.idVendor =		__constant_cpu_to_le16(DRIVER_VENDOR_ID),
	.idProduct =		__constant_cpu_to_le16(DRIVER_PRODUCT_ID),
	.bcdDevice =		__constant_cpu_to_le16(DRIVER_VERSION_NUM),

	.iManufacturer =	STRING_MANUFACTURER,
	.iProduct =		STRING_PRODUCT,
	.iSerialNumber =	STRING_SERIAL,
	.bNumConfigurations =	1,
};

static struct usb_config_descriptor
config_desc = {
	.bLength =		sizeof config_desc,
	.bDescriptorType =	USB_DT_CONFIG,

	/* wTotalLength adjusted during bind() */
	.bNumInterfaces =	1,
	.bConfigurationValue =	CONFIG_VALUE,
	.bmAttributes =		USB_CONFIG_ATT_ONE | SELFPOWER,
	.bMaxPower =		(MAX_USB_POWER + 1) / 2,
};

//For OTG ;;Start
static struct usb_otg_descriptor otg_desc = {
	.bLength =		sizeof(otg_desc),
	.bDescriptorType =	USB_DT_OTG,

	.bmAttributes =		USB_OTG_SRP,
};
//For OTG;;End


/* There is only one interface. */

static struct usb_interface_descriptor
intf_desc = {
	.bLength =		sizeof intf_desc,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bNumEndpoints =	2,		// Adjusted during bind()
	.bInterfaceClass =	USB_CLASS_MASS_STORAGE,
	.bInterfaceSubClass =	USB_SC_SCSI,	// Adjusted during bind()
	.bInterfaceProtocol =	USB_PR_BULK,	// Adjusted during bind()
};

/* Three full-speed endpoint descriptors: bulk-in, bulk-out,
 * and interrupt-in. */

static const struct usb_endpoint_descriptor
fs_bulk_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	EP_BULK_IN_NUM | USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(FS_BULK_IN_MAXPACKET),
};

static const struct usb_endpoint_descriptor
fs_bulk_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	EP_BULK_OUT_NUM,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(FS_BULK_OUT_MAXPACKET),
};

static const struct usb_endpoint_descriptor
fs_intr_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	EP_INTR_IN_NUM | USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	__constant_cpu_to_le16(2),
	.bInterval =		32,	// frames -> 32 ms
};

#ifdef	HIGHSPEED

/*
 * USB 2.0 devices need to expose both high speed and full speed
 * descriptors, unless they only run at full speed.
 *
 * That means alternate endpoint descriptors (bigger packets)
 * and a "device qualifier" ... plus more construction options
 * for the config descriptor.
 */
static const struct usb_endpoint_descriptor
hs_bulk_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	EP_BULK_IN_NUM | USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(512),
};

static const struct usb_endpoint_descriptor
hs_bulk_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	EP_BULK_OUT_NUM,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(512),
	.bInterval =		1,	// NAK every 1 uframe
};

static const struct usb_endpoint_descriptor
hs_intr_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	EP_INTR_IN_NUM | USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	__constant_cpu_to_le16(2),
	.bInterval =		9,	// 2**(9-1) = 256 uframes -> 32 ms
};

static struct usb_qualifier_descriptor
dev_qualifier = {
	.bLength =		sizeof dev_qualifier,
	.bDescriptorType =	USB_DT_DEVICE_QUALIFIER,

	.bcdUSB =		__constant_cpu_to_le16(0x0200),
	.bDeviceClass =		USB_CLASS_PER_INTERFACE,

	.bNumConfigurations =	1,
};

/* Maxpacket and other transfer characteristics vary by speed. */
#define ep_desc(g,fs,hs)	(((g)->speed==USB_SPEED_HIGH) ? (hs) : (fs))

#else

/* If there's no high speed support, maxpacket doesn't change. */
#define ep_desc(g,fs,hs)	fs

#endif	/* !HIGHSPEED */


/* The CBI specification limits the serial string to 12 uppercase hexadecimal
 * characters. */
static char				serial[13];

/* Static strings, in ISO 8859/1 */
static struct usb_string		strings[] = {
	//{ STRING_MANUFACTURER, UTS_SYSNAME " " UTS_RELEASE " with " CHIP, },	//For FOTG200-CV1-2
    { STRING_MANUFACTURER, sysname , },	//For FOTG200-CV1-2
	{ STRING_PRODUCT, longname, },
	{ STRING_SERIAL, serial, },
	{ }			// end of list
};

static struct usb_gadget_strings	stringtab = {
	.language	= 0x0409,		// en-us
	.strings	= strings,
};


/*
 * Config descriptors are handcrafted.  They must agree with the code
 * that sets configurations and with code managing interfaces and their
 * altsettings.  They must also handle different speeds and other-speed
 * requests.
 */
static int populate_config_buf(enum usb_device_speed speed,
		u8 *buf0, u8 type, unsigned index)
{
	u8	*buf = buf0;
#ifdef HIGHSPEED
	int	hs;
#endif

    DBG_FUNCC("+ (FileStorage)populate_config_buf()\n");

	if (index > 0)
		return -EINVAL;
	if (config_desc.wTotalLength  > EP0_BUFSIZE)
		return -EDOM;

	/* Config (or other speed config) */
	memcpy(buf, &config_desc, USB_DT_CONFIG_SIZE);
	buf[1] = type;
	buf += USB_DT_CONFIG_SIZE;

	/* Interface */
	memcpy(buf, &intf_desc, USB_DT_INTERFACE_SIZE);
	buf += USB_DT_INTERFACE_SIZE;

	/* The endpoints in the interface (at that speed) */
#ifdef HIGHSPEED
	hs = (speed == USB_SPEED_HIGH);
	if (type == USB_DT_OTHER_SPEED_CONFIG)
		hs = !hs;
	if (hs) {
		memcpy(buf, &hs_bulk_in_desc, USB_DT_ENDPOINT_SIZE);
		buf += USB_DT_ENDPOINT_SIZE;
		memcpy(buf, &hs_bulk_out_desc, USB_DT_ENDPOINT_SIZE);
		buf += USB_DT_ENDPOINT_SIZE;
		if (transport_is_cbi()) {
			memcpy(buf, &hs_intr_in_desc, USB_DT_ENDPOINT_SIZE);
			buf += USB_DT_ENDPOINT_SIZE;
		}
	} else
#endif
	{
		memcpy(buf, &fs_bulk_in_desc, USB_DT_ENDPOINT_SIZE);
		buf += USB_DT_ENDPOINT_SIZE;
		memcpy(buf, &fs_bulk_out_desc, USB_DT_ENDPOINT_SIZE);
		buf += USB_DT_ENDPOINT_SIZE;
		if (transport_is_cbi()) {
			memcpy(buf, &fs_intr_in_desc, USB_DT_ENDPOINT_SIZE);
			buf += USB_DT_ENDPOINT_SIZE;
		}
	}

//For OTG;;Start
	memcpy(buf, &otg_desc, USB_DT_CONFIG_SIZE);
	buf += 3;
//For OTG;;end

	return buf - buf0;
}


/*-------------------------------------------------------------------------*/

/* These routines may be called in process context or in_irq */

static void wakeup_thread(struct fsg_dev *fsg)
{
	
	DBG_FUNCC("+ (FileStorage)wakeup_thread()\n");

	/* Tell the main thread that something has happened */

    udelay(100);

	fsg->thread_wakeup_needed = 1;
	wake_up_all(&fsg->thread_wqh);
}


static void raise_exception(struct fsg_dev *fsg, enum fsg_state new_state)
{
	unsigned long		flags;
	struct task_struct	*thread_task;
    
    DBG_FUNCC("+ (FileStorage)raise_exception()\n");
	
	/* Do nothing if a higher-priority exception is already in progress.
	 * If a lower-or-equal priority exception is in progress, preempt it
	 * and notify the main thread by sending it a signal. */
	spin_lock_irqsave(&fsg->lock, flags);
	if (fsg->state <= new_state) {
		fsg->exception_req_tag = fsg->ep0_req_tag;
		fsg->state = new_state;
		thread_task = fsg->thread_task;
		if (thread_task)
			send_sig_info(SIGUSR1, (void *) 1L, thread_task);
	}
	spin_unlock_irqrestore(&fsg->lock, flags);
}


/*-------------------------------------------------------------------------*/

/* The disconnect callback and ep0 routines.  These always run in_irq,
 * except that ep0_queue() is called in the main thread to acknowledge
 * completion of various requests: set config, set interface, and
 * Bulk-only device reset. */

static void fsg_disconnect(struct usb_gadget *gadget)
{
	struct fsg_dev		*fsg = get_gadget_data(gadget);
    
    DBG_FUNCC("+ (FileStorage)fsg_disconnect()\n");
    
	DBG(fsg, "disconnect or port reset\n");
	raise_exception(fsg, FSG_STATE_DISCONNECT);
}


static int ep0_queue(struct fsg_dev *fsg)
{
	int	rc;
    
    DBG_FUNCC("+ (FileStorage)ep0_queue()\n");
    
	rc = usb_ep_queue(fsg->ep0, fsg->ep0req, GFP_ATOMIC);
	if (rc != 0 && rc != -ESHUTDOWN) {

		/* We can't do much more than wait for a reset */
		WARN(fsg, "error in submission: %s --> %d\n",
				fsg->ep0->name, rc);
	}
	return rc;
}

static void ep0_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct fsg_dev		*fsg = (struct fsg_dev *) ep->driver_data;

    DBG_FUNCC("+ (FileStorage)ep0_complete()\n");

	if (req->actual > 0)
		dump_msg(fsg, fsg->ep0req_name, req->buf, req->actual);
	if (req->status || req->actual != req->length)
		DBG(fsg, "%s --> %d, %u/%u\n", __FUNCTION__,
				req->status, req->actual, req->length);
	if (req->status == -ECONNRESET)		// Request was cancelled
		usb_ep_fifo_flush(ep);

	if (req->status == 0 && req->context)
		((fsg_routine_t) (req->context))(fsg);
}


/*-------------------------------------------------------------------------*/

/* Bulk and interrupt endpoint completion handlers.
 * These always run in_irq. */

static void bulk_in_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct fsg_dev		*fsg = (struct fsg_dev *) ep->driver_data;
	struct fsg_buffhd	*bh = (struct fsg_buffhd *) req->context;

    DBG_FUNCC("+ (FileStorage)bulk_in_complete()\n");





	if (req->status || req->actual != req->length)
		DBG(fsg, "%s --> %d, %u/%u\n", __FUNCTION__,
				req->status, req->actual, req->length);
	if (req->status == -ECONNRESET)		// Request was cancelled
		usb_ep_fifo_flush(ep);

	/* Hold the lock while we update the request and buffer states */
	spin_lock(&fsg->lock);
	bh->inreq_busy = 0;
	bh->state = BUF_STATE_EMPTY;
	spin_unlock(&fsg->lock);
	wakeup_thread(fsg);
}

static void bulk_out_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct fsg_dev		*fsg = (struct fsg_dev *) ep->driver_data;
	struct fsg_buffhd	*bh = (struct fsg_buffhd *) req->context;

    DBG_FUNCC("+ (FileStorage)bulk_out_complete()\n");

	dump_msg(fsg, "bulk-out", req->buf, req->actual);
	if (req->status || req->actual != bh->bulk_out_intended_length)
		DBG(fsg, "%s --> %d, %u/%u\n", __FUNCTION__,
				req->status, req->actual,
				bh->bulk_out_intended_length);
	if (req->status == -ECONNRESET)		// Request was cancelled
		usb_ep_fifo_flush(ep);

	/* Hold the lock while we update the request and buffer states */
	spin_lock(&fsg->lock);
	bh->outreq_busy = 0;
	bh->state = BUF_STATE_FULL;
	spin_unlock(&fsg->lock);
	wakeup_thread(fsg);
}

static void intr_in_complete(struct usb_ep *ep, struct usb_request *req)
{

  DBG_FUNCC("+ (FileStorage)intr_in_complete()\n");

#ifdef CONFIG_USB_FILE_STORAGE_TEST
	struct fsg_dev		*fsg = (struct fsg_dev *) ep->driver_data;
	struct fsg_buffhd	*bh = (struct fsg_buffhd *) req->context;

	if (req->status || req->actual != req->length)
		DBG(fsg, "%s --> %d, %u/%u\n", __FUNCTION__,
				req->status, req->actual, req->length);
	if (req->status == -ECONNRESET)		// Request was cancelled
		usb_ep_fifo_flush(ep);

	/* Hold the lock while we update the request and buffer states */
	spin_lock(&fsg->lock);
	fsg->intreq_busy = 0;
	bh->state = BUF_STATE_EMPTY;
	spin_unlock(&fsg->lock);
	wakeup_thread(fsg);
#endif /* CONFIG_USB_FILE_STORAGE_TEST */
}


/*-------------------------------------------------------------------------*/

/* Ep0 class-specific handlers.  These always run in_irq. */

static void received_cbi_adsc(struct fsg_dev *fsg, struct fsg_buffhd *bh)
{

  DBG_FUNCC("+ (FileStorage)received_cbi_adsc()\n");

#ifdef CONFIG_USB_FILE_STORAGE_TEST
	struct usb_request	*req = fsg->ep0req;
	static u8		cbi_reset_cmnd[6] = {
			SC_SEND_DIAGNOSTIC, 4, 0xff, 0xff, 0xff, 0xff};

	/* Error in command transfer? */
	if (req->status || req->length != req->actual ||
			req->actual < 6 || req->actual > MAX_COMMAND_SIZE) {

		/* Not all controllers allow a protocol stall after
		 * receiving control-out data, but we'll try anyway. */
		fsg_set_halt(fsg, fsg->ep0);
		return;			// Wait for reset
	}

	/* Is it the special reset command? */
	if (req->actual >= sizeof cbi_reset_cmnd &&
			memcmp(req->buf, cbi_reset_cmnd,
				sizeof cbi_reset_cmnd) == 0) {

		/* Raise an exception to stop the current operation
		 * and reinitialize our state. */
		DBG(fsg, "cbi reset request\n");
		raise_exception(fsg, FSG_STATE_RESET);
		return;
	}

	VDBG(fsg, "CB[I] accept device-specific command\n");
	spin_lock(&fsg->lock);

	/* Save the command for later */
	if (fsg->cbbuf_cmnd_size)
		WARN(fsg, "CB[I] overwriting previous command\n");
	fsg->cbbuf_cmnd_size = req->actual;
	memcpy(fsg->cbbuf_cmnd, req->buf, fsg->cbbuf_cmnd_size);

	spin_unlock(&fsg->lock);
	wakeup_thread(fsg);
#endif /* CONFIG_USB_FILE_STORAGE_TEST */
}


static int class_setup_req(struct fsg_dev *fsg,
		const struct usb_ctrlrequest *ctrl)
{
	struct usb_request	*req = fsg->ep0req;
	int			value = -EOPNOTSUPP;

    DBG_FUNCC("+ (FileStorage)class_setup_req()\n");

	if (!fsg->config)
		{
         DBG_TEMP("FileStorage => (!fsg->config) & return \n");//Bruce
		
		return value;
		}

	/* Handle Bulk-only class-specific requests */
	if (transport_is_bbb()) {

    DBG_TEMP("FileStorage => Bulk-only class \n");//Bruce
		
		switch (ctrl->bRequest) {

		case USB_BULK_RESET_REQUEST:
            DBG_TEMP("FileStorage => Bulk-only class(USB_BULK_RESET_REQUEST) \n");//Bruce

			if (ctrl->bRequestType != (USB_DIR_OUT |
					USB_TYPE_CLASS | USB_RECIP_INTERFACE))
				break;
			if (ctrl->wIndex != 0) {
				value = -EDOM;
				break;
			}

			/* Raise an exception to stop the current operation
			 * and reinitialize our state. */
			DBG(fsg, "bulk reset request\n");
			raise_exception(fsg, FSG_STATE_RESET);
			value = DELAYED_STATUS;
			break;

		case USB_BULK_GET_MAX_LUN_REQUEST:
            
            DBG_TEMP("FileStorage => Bulk-only class(USB_BULK_GET_MAX_LUN_REQUEST) \n");//Bruce

			if (ctrl->bRequestType != (USB_DIR_IN |
					USB_TYPE_CLASS | USB_RECIP_INTERFACE))
				break;
			if (ctrl->wIndex != 0) {
				value = -EDOM;
				break;
			}
			VDBG(fsg, "get max LUN\n");
			*(u8 *) req->buf = fsg->nluns - 1;
			value = min(ctrl->wLength, (u16) 1);
			break;
		}
	}

	/* Handle CBI class-specific requests */
	else {
        
        DBG_TEMP("FileStorage => CBI-only class \n");//Bruce

		switch (ctrl->bRequest) {

		case USB_CBI_ADSC_REQUEST:
			if (ctrl->bRequestType != (USB_DIR_OUT |
					USB_TYPE_CLASS | USB_RECIP_INTERFACE))
				break;
			if (ctrl->wIndex != 0) {
				value = -EDOM;
				break;
			}
			if (ctrl->wLength > MAX_COMMAND_SIZE) {
				value = -EOVERFLOW;
				break;
			}
			value = ctrl->wLength;
			fsg->ep0req->context = received_cbi_adsc;
			break;
		}
	}

	if (value == -EOPNOTSUPP)
		VDBG(fsg,
			"unknown class-specific control req "
			"%02x.%02x v%04x i%04x l%u\n",
			ctrl->bRequestType, ctrl->bRequest,
			ctrl->wValue, ctrl->wIndex, ctrl->wLength);
			
    DBG_TEMP("- (FileStorage)class_setup_req()=>Return (value=%d) \n",value);//Bruce			
	return value;
}


/*-------------------------------------------------------------------------*/

/* Ep0 standard request handlers.  These always run in_irq. */

static int standard_setup_req(struct fsg_dev *fsg,
		const struct usb_ctrlrequest *ctrl)
{
	struct usb_request	*req = fsg->ep0req;
	int			value = -EOPNOTSUPP;

    DBG_FUNCC("+ (FileStorage)standard_setup_req()  (ctrl->bRequest = %d)\n",ctrl->bRequest);


	/* Usually this just stores reply data in the pre-allocated ep0 buffer,
	 * but config change events will also reconfigure hardware. */
 
	 
	switch (ctrl->bRequest) {

	case USB_REQ_GET_DESCRIPTOR:
		if (ctrl->bRequestType != (USB_DIR_IN | USB_TYPE_STANDARD |
				USB_RECIP_DEVICE))
			break;
		switch (ctrl->wValue >> 8) {

		case USB_DT_DEVICE:
			VDBG(fsg, "get device descriptor\n");
			value = min(ctrl->wLength, (u16) sizeof device_desc);
			memcpy(req->buf, &device_desc, value);
			break;
#ifdef HIGHSPEED
		case USB_DT_DEVICE_QUALIFIER:
			VDBG(fsg, "get device qualifier\n");
			value = min(ctrl->wLength, (u16) sizeof dev_qualifier);
			memcpy(req->buf, &dev_qualifier, value);
			break;

		case USB_DT_OTHER_SPEED_CONFIG:
			VDBG(fsg, "get other-speed config descriptor\n");
			goto get_config;
#endif /* HIGHSPEED */
		case USB_DT_CONFIG:
			VDBG(fsg, "get configuration descriptor\n");
#ifdef HIGHSPEED
		get_config:
#endif /* HIGHSPEED */
			value = populate_config_buf(fsg->gadget->speed,
					req->buf,
					ctrl->wValue >> 8,
					ctrl->wValue & 0xff);
			if (value >= 0)
				value = min(ctrl->wLength, (u16) value);
			break;

		case USB_DT_STRING:
			VDBG(fsg, "get string descriptor\n");

			/* wIndex == language code */
			value = usb_gadget_get_string(&stringtab,
					ctrl->wValue & 0xff, req->buf);
			if (value >= 0)
				value = min(ctrl->wLength, (u16) value);
			break;
		}
		break;

	/* One config, two speeds */
	case USB_REQ_SET_CONFIGURATION:
		if (ctrl->bRequestType != (USB_DIR_OUT | USB_TYPE_STANDARD |
				USB_RECIP_DEVICE))
			break;
		VDBG(fsg, "set configuration\n");
		if (ctrl->wValue == CONFIG_VALUE || ctrl->wValue == 0) {
			fsg->new_config = ctrl->wValue;

			/* Raise an exception to wipe out previous transaction
			 * state (queued bufs, etc) and set the new config. */
			raise_exception(fsg, FSG_STATE_CONFIG_CHANGE);
			value = DELAYED_STATUS;
		}
		break;
	case USB_REQ_GET_CONFIGURATION:
		if (ctrl->bRequestType != (USB_DIR_IN | USB_TYPE_STANDARD |
				USB_RECIP_DEVICE))
			break;
		VDBG(fsg, "get configuration\n");
		*(u8 *) req->buf = fsg->config;
		value = min(ctrl->wLength, (u16) 1);
		break;

	case USB_REQ_SET_INTERFACE:
		if (ctrl->bRequestType != (USB_DIR_OUT| USB_TYPE_STANDARD |
				USB_RECIP_INTERFACE))
			break;
		if (fsg->config && ctrl->wIndex == 0) {

			/* Raise an exception to wipe out previous transaction
			 * state (queued bufs, etc) and install the new
			 * interface altsetting. */
			raise_exception(fsg, FSG_STATE_INTERFACE_CHANGE);
			value = DELAYED_STATUS;
		}
		break;
	case USB_REQ_GET_INTERFACE:
		if (ctrl->bRequestType != (USB_DIR_IN | USB_TYPE_STANDARD |
				USB_RECIP_INTERFACE))
			break;
		if (!fsg->config)
			break;
		if (ctrl->wIndex != 0) {
			value = -EDOM;
			break;
		}
		VDBG(fsg, "get interface\n");
		*(u8 *) req->buf = 0;
		value = min(ctrl->wLength, (u16) 1);
		break;

	default:
		VDBG(fsg,
			"unknown control req %02x.%02x v%04x i%04x l%u\n",
			ctrl->bRequestType, ctrl->bRequest,
			ctrl->wValue, ctrl->wIndex, ctrl->wLength);
	}

	return value;
}


static int fsg_setup(struct usb_gadget *gadget,
		const struct usb_ctrlrequest *ctrl)
{
	struct fsg_dev		*fsg = get_gadget_data(gadget);
	int			rc;

    DBG_FUNCC("+ (FileStorage)fsg_setup()\n");

	++fsg->ep0_req_tag;		// Record arrival of a new request
	fsg->ep0req->context = NULL;
	fsg->ep0req->length = 0;
	dump_msg(fsg, "ep0-setup", (u8 *) ctrl, sizeof(*ctrl));

	if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_CLASS)
		rc = class_setup_req(fsg, ctrl);
	else
		rc = standard_setup_req(fsg, ctrl);

	/* Respond with data/status or defer until later? */
	if (rc >= 0 && rc != DELAYED_STATUS) {
		fsg->ep0req->length = rc;
		fsg->ep0req_name = (ctrl->bRequestType & USB_DIR_IN ?
				"ep0-in" : "ep0-out");
		rc = ep0_queue(fsg);
	}

	/* Device either stalls (rc < 0) or reports success */
	return rc;
}


/*-------------------------------------------------------------------------*/

/* All the following routines run in process context */


/* Use this for bulk or interrupt transfers, not ep0 */
static void start_transfer(struct fsg_dev *fsg, struct usb_ep *ep,
		struct usb_request *req, volatile int *pbusy,
		volatile enum fsg_buffer_state *state)
{
	int	rc;

    DBG_FUNCC("+ (FileStorage)start_transfer()\n");

    
    
    
	if (ep == fsg->bulk_in)
		dump_msg(fsg, "bulk-in", req->buf, req->length);
	else if (ep == fsg->intr_in)
		dump_msg(fsg, "intr-in", req->buf, req->length);
	*pbusy = 1;
	*state = BUF_STATE_BUSY;
	rc = usb_ep_queue(ep, req, GFP_KERNEL);
	if (rc != 0) {
		*pbusy = 0;
		*state = BUF_STATE_EMPTY;

		/* We can't do much more than wait for a reset */

		/* Note: currently the net2280 driver fails zero-length
		 * submissions if DMA is enabled. */
		if (rc != -ESHUTDOWN && !(rc == -EOPNOTSUPP &&
						req->length == 0))
			WARN(fsg, "error in submission: %s --> %d\n",
					ep->name, rc);
	}
}


static int sleep_thread(struct fsg_dev *fsg)
{
	int	rc;

   DBG_FUNCC("+ (FileStorage)sleep_thread()\n");

	/* Wait until a signal arrives or we are woken up */
	rc = wait_event_interruptible(fsg->thread_wqh,
			fsg->thread_wakeup_needed);
	fsg->thread_wakeup_needed = 0;
	return (rc ? -EINTR : 0);
}


/*-------------------------------------------------------------------------*/

static int do_read(struct fsg_dev *fsg)
{
	struct lun		*curlun = fsg->curlun;
	u32			lba;
	struct fsg_buffhd	*bh;
	int			rc;
	u32			amount_left;
	loff_t			file_offset, file_offset_tmp;
	unsigned int		amount;
	unsigned int		partial_page;
	ssize_t			nread;

    DBG_FUNCC("+ (FileStorage)do_read()\n");

	/* Get the starting Logical Block Address and check that it's
	 * not too big */
	if (fsg->cmnd[0] == SC_READ_6)
		lba = (fsg->cmnd[1] << 16) | get_be16(&fsg->cmnd[2]);
	else {
		lba = get_be32(&fsg->cmnd[2]);

		/* We allow DPO (Disable Page Out = don't save data in the
		 * cache) and FUA (Force Unit Access = don't read from the
		 * cache), but we don't implement them. */
		if ((fsg->cmnd[1] & ~0x18) != 0) {
			curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
			return -EINVAL;
		}
	}
	if (lba >= curlun->num_sectors) {
		curlun->sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		return -EINVAL;
	}
	file_offset = ((loff_t) lba) << 9;

	/* Carry out the file reads */
	amount_left = fsg->data_size_from_cmnd;
	if (unlikely(amount_left == 0))
		return -EIO;		// No default reply

	for (;;) {

		/* Figure out how much we need to read:
		 * Try to read the remaining amount.
		 * But don't read more than the buffer size.
		 * And don't try to read past the end of the file.
		 * Finally, if we're not at a page boundary, don't read past
		 *	the next page.
		 * If this means reading 0 then we were asked to read past
		 *	the end of file. */
		amount = min((unsigned int) amount_left, mod_data.buflen);
		amount = min((loff_t) amount,
				curlun->file_length - file_offset);
		partial_page = file_offset & (PAGE_CACHE_SIZE - 1);
		if (partial_page > 0)
			amount = min(amount, (unsigned int) PAGE_CACHE_SIZE -
					partial_page);

		/* Wait for the next buffer to become available */
		bh = fsg->next_buffhd_to_fill;
		while (bh->state != BUF_STATE_EMPTY) {
			if ((rc = sleep_thread(fsg)) != 0)
				return rc;
		}

		/* If we were asked to read past the end of file,
		 * end with an empty buffer. */
		if (amount == 0) {
			curlun->sense_data =
					SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
			curlun->sense_data_info = file_offset >> 9;
			bh->inreq->length = 0;
			bh->state = BUF_STATE_FULL;
			break;
		}

		/* Perform the read */
		file_offset_tmp = file_offset;
		nread = curlun->filp->f_op->read(curlun->filp,
				(char *) bh->buf,
				amount, &file_offset_tmp);
		VLDBG(curlun, "file read %u @ %llu -> %d\n", amount,
				(unsigned long long) file_offset,
				(int) nread);
		if (signal_pending(current))
			return -EINTR;

		if (nread < 0) {
			LDBG(curlun, "error in file read: %d\n",
					(int) nread);
			nread = 0;
		} else if (nread < amount) {
			LDBG(curlun, "partial file read: %d/%u\n",
					(int) nread, amount);
			nread -= (nread & 511);	// Round down to a block
		}
		file_offset  += nread;
		amount_left  -= nread;
		fsg->residue -= nread;
		bh->inreq->length = nread;
		bh->state = BUF_STATE_FULL;

		/* If an error occurred, report it and its position */
		if (nread < amount) {
			curlun->sense_data = SS_UNRECOVERED_READ_ERROR;
			curlun->sense_data_info = file_offset >> 9;
			break;
		}

		if (amount_left == 0)
			break;		// No more left to read

		/* Send this buffer and go read some more */
		bh->inreq->zero = 0;
		start_transfer(fsg, fsg->bulk_in, bh->inreq,
				&bh->inreq_busy, &bh->state);
		fsg->next_buffhd_to_fill = bh->next;
	}

	return -EIO;		// No default reply
}


/*-------------------------------------------------------------------------*/

static int do_write(struct fsg_dev *fsg)
{
	struct lun		*curlun = fsg->curlun;
	u32			lba;
	struct fsg_buffhd	*bh;
	int			get_some_more;
	u32			amount_left_to_req, amount_left_to_write;
	loff_t			usb_offset, file_offset, file_offset_tmp;
	unsigned int		amount;
	unsigned int		partial_page;
	ssize_t			nwritten;
	int			rc;

    DBG_FUNCC("+ (FileStorage)do_write()\n");
      
	if (curlun->ro) {
		curlun->sense_data = SS_WRITE_PROTECTED;
		return -EINVAL;
	}
	curlun->filp->f_flags &= ~O_SYNC;	// Default is not to wait

	/* Get the starting Logical Block Address and check that it's
	 * not too big */
	if (fsg->cmnd[0] == SC_WRITE_6)
		lba = (fsg->cmnd[1] << 16) | get_be16(&fsg->cmnd[2]);
	else {
		lba = get_be32(&fsg->cmnd[2]);

		/* We allow DPO (Disable Page Out = don't save data in the
		 * cache) and FUA (Force Unit Access = write directly to the
		 * medium).  We don't implement DPO; we implement FUA by
		 * performing synchronous output. */
		if ((fsg->cmnd[1] & ~0x18) != 0) {
			curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
			return -EINVAL;
		}
		if (fsg->cmnd[1] & 0x08)	// FUA
			curlun->filp->f_flags |= O_SYNC;
	}

	if (lba >= curlun->num_sectors) {
		curlun->sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		
		return -EINVAL;
	}




	/* Carry out the file writes */
	get_some_more = 1;
	file_offset = usb_offset = ((loff_t) lba) << 9;
	amount_left_to_req = amount_left_to_write = fsg->data_size_from_cmnd;

	while (amount_left_to_write > 0) {

		/* Queue a request for more data from the host */
		bh = fsg->next_buffhd_to_fill;
		if (bh->state == BUF_STATE_EMPTY && get_some_more) {

			/* Figure out how much we want to get:
			 * Try to get the remaining amount.
			 * But don't get more than the buffer size.
			 * And don't try to go past the end of the file.
			 * If we're not at a page boundary,
			 *	don't go past the next page.
			 * If this means getting 0, then we were asked
			 *	to write past the end of file.
			 * Finally, round down to a block boundary. */
			amount = min(amount_left_to_req, mod_data.buflen);
			amount = min((loff_t) amount, curlun->file_length -
					usb_offset);
			partial_page = usb_offset & (PAGE_CACHE_SIZE - 1);
			if (partial_page > 0)
				amount = min(amount,
	(unsigned int) PAGE_CACHE_SIZE - partial_page);

			if (amount == 0) {
				get_some_more = 0;
				curlun->sense_data =
					SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;

			
		
				curlun->sense_data_info = usb_offset >> 9;
				continue;
			}
			amount -= (amount & 511);
			if (amount == 0) {

				/* Why were we were asked to transfer a
				 * partial block? */
				get_some_more = 0;
				continue;
			}

			/* Get the next buffer */
			usb_offset += amount;
			fsg->usb_amount_left -= amount;
			amount_left_to_req -= amount;
			if (amount_left_to_req == 0)
				get_some_more = 0;

			/* amount is always divisible by 512, hence by
			 * the bulk-out maxpacket size */
			bh->outreq->length = bh->bulk_out_intended_length =
					amount;
			start_transfer(fsg, fsg->bulk_out, bh->outreq,
					&bh->outreq_busy, &bh->state);
			fsg->next_buffhd_to_fill = bh->next;
			continue;
		}

		/* Write the received data to the backing file */
		bh = fsg->next_buffhd_to_drain;
		if (bh->state == BUF_STATE_EMPTY && !get_some_more)
			break;			// We stopped early
		if (bh->state == BUF_STATE_FULL) {
			fsg->next_buffhd_to_drain = bh->next;
			bh->state = BUF_STATE_EMPTY;

			/* Did something go wrong with the transfer? */
			if (bh->outreq->status != 0) {
				curlun->sense_data = SS_COMMUNICATION_FAILURE;
				curlun->sense_data_info = file_offset >> 9;
				break;
			}

			amount = bh->outreq->actual;
			if (curlun->file_length - file_offset < amount) {
				LERROR(curlun,
	"write %u @ %llu beyond end %llu\n",
	amount, (unsigned long long) file_offset,
	(unsigned long long) curlun->file_length);
				amount = curlun->file_length - file_offset;
			}

			/* Perform the write */
			file_offset_tmp = file_offset;
			nwritten = curlun->filp->f_op->write(curlun->filp,
					(char *) bh->buf,
					amount, &file_offset_tmp);
			VLDBG(curlun, "file write %u @ %llu -> %d\n", amount,
					(unsigned long long) file_offset,
					(int) nwritten);
			





			if (signal_pending(current))
				return -EINTR;		// Interrupted!

			if (nwritten < 0) {
				LDBG(curlun, "error in file write: %d\n",
						(int) nwritten);
				nwritten = 0;
			} else if (nwritten < amount) {
				LDBG(curlun, "partial file write: %d/%u\n",
						(int) nwritten, amount);
				nwritten -= (nwritten & 511);
						// Round down to a block
			}
			file_offset += nwritten;
			amount_left_to_write -= nwritten;
			fsg->residue -= nwritten;

			/* If an error occurred, report it and its position */
			if (nwritten < amount) {
				curlun->sense_data = SS_WRITE_ERROR;
				curlun->sense_data_info = file_offset >> 9;
				break;
			}

			/* Did the host decide to stop early? */
			if (bh->outreq->actual != bh->outreq->length) {
				fsg->short_packet_received = 1;
				break;
			}
			continue;
		}

		/* Wait for something to happen */
		if ((rc = sleep_thread(fsg)) != 0)
			return rc;

	}

	return -EIO;		// No default reply
}


/*-------------------------------------------------------------------------*/

/* Sync the file data, don't bother with the metadata.
 * This code was copied from fs/buffer.c:sys_fdatasync(). */
static int fsync_sub(struct lun *curlun)
{
	struct file	*filp = curlun->filp;
	struct inode	*inode;
	int		rc, err;

    DBG_FUNCC("+ (FileStorage)fsync_sub()\n");

	if (curlun->ro || !filp)
		return 0;
	if (!filp->f_op->fsync)
		return -EINVAL;

	inode = filp->f_dentry->d_inode;
	down(&inode->i_sem);
	rc = filemap_fdatasync(inode->i_mapping);
	err = filp->f_op->fsync(filp, filp->f_dentry, 1);
	if (!rc)
		rc = err;
	err = filemap_fdatawait(inode->i_mapping);
	if (!rc)
		rc = err;
	up(&inode->i_sem);
	VLDBG(curlun, "fdatasync -> %d\n", rc);
    DBG_FUNCC("- (FileStorage)fsync_sub()\n");	
	return rc;
}

static void fsync_all(struct fsg_dev *fsg)
{
	int	i;

    DBG_FUNCC("+ (FileStorage)fsync_all()\n");
    
	for (i = 0; i < fsg->nluns; ++i)
		fsync_sub(&fsg->luns[i]);
}

static int do_synchronize_cache(struct fsg_dev *fsg)
{
	struct lun	*curlun = fsg->curlun;
	int		rc;

	/* We ignore the requested LBA and write out all file's
	 * dirty data buffers. */
	rc = fsync_sub(curlun);
	if (rc)
		curlun->sense_data = SS_WRITE_ERROR;
	return 0;
}


/*-------------------------------------------------------------------------*/

static void invalidate_sub(struct lun *curlun)
{
	struct file	*filp = curlun->filp;
	struct inode	*inode = filp->f_dentry->d_inode;

    DBG_FUNCC("+ (FileStorage)invalidate_sub()\n");

	invalidate_inode_pages(inode);
}

static int do_verify(struct fsg_dev *fsg)
{
	struct lun		*curlun = fsg->curlun;
	u32			lba;
	u32			verification_length;
	struct fsg_buffhd	*bh = fsg->next_buffhd_to_fill;
	loff_t			file_offset, file_offset_tmp;
	u32			amount_left;
	unsigned int		amount;
	ssize_t			nread;

    DBG_FUNCC("+ (FileStorage)do_verify()\n");

	/* Get the starting Logical Block Address and check that it's
	 * not too big */
	lba = get_be32(&fsg->cmnd[2]);
	if (lba >= curlun->num_sectors) {
		curlun->sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		return -EINVAL;
	}

	/* We allow DPO (Disable Page Out = don't save data in the
	 * cache) but we don't implement it. */
	if ((fsg->cmnd[1] & ~0x10) != 0) {
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}

	verification_length = get_be16(&fsg->cmnd[7]);
	if (unlikely(verification_length == 0))
		return -EIO;		// No default reply

	/* Prepare to carry out the file verify */
	amount_left = verification_length << 9;
	file_offset = ((loff_t) lba) << 9;

	/* Write out all the dirty buffers before invalidating them */
	fsync_sub(curlun);
	if (signal_pending(current))
		return -EINTR;

	invalidate_sub(curlun);
	if (signal_pending(current))
		return -EINTR;

	/* Just try to read the requested blocks */
	while (amount_left > 0) {

		/* Figure out how much we need to read:
		 * Try to read the remaining amount, but not more than
		 * the buffer size.
		 * And don't try to read past the end of the file.
		 * If this means reading 0 then we were asked to read
		 * past the end of file. */
		amount = min((unsigned int) amount_left, mod_data.buflen);
		amount = min((loff_t) amount,
				curlun->file_length - file_offset);
		if (amount == 0) {
			curlun->sense_data =
					SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
			curlun->sense_data_info = file_offset >> 9;
			break;
		}

		/* Perform the read */
		file_offset_tmp = file_offset;
		nread = curlun->filp->f_op->read(curlun->filp,
				(char *) bh->buf,
				amount, &file_offset_tmp);
		VLDBG(curlun, "file read %u @ %llu -> %d\n", amount,
				(unsigned long long) file_offset,
				(int) nread);
		if (signal_pending(current))
			return -EINTR;

		if (nread < 0) {
			LDBG(curlun, "error in file verify: %d\n",
					(int) nread);
			nread = 0;
		} else if (nread < amount) {
			LDBG(curlun, "partial file verify: %d/%u\n",
					(int) nread, amount);
			nread -= (nread & 511);	// Round down to a sector
		}
		if (nread == 0) {
			curlun->sense_data = SS_UNRECOVERED_READ_ERROR;
			curlun->sense_data_info = file_offset >> 9;
			break;
		}
		file_offset += nread;
		amount_left -= nread;
	}
	return 0;
}


/*-------------------------------------------------------------------------*/

static int do_inquiry(struct fsg_dev *fsg, struct fsg_buffhd *bh)
{
	u8	*buf = (u8 *) bh->buf;

	static char vendor_id[] = "Linux   ";
	static char product_id[] = "File-Stor Gadget";

    DBG_FUNCC("+ (FileStorage)do_inquiry()\n");

	if (!fsg->curlun) {		// Unsupported LUNs are okay
		fsg->bad_lun_okay = 1;
		memset(buf, 0, 36);
		buf[0] = 0x7f;		// Unsupported, no device-type
		return 36;
	}

	memset(buf, 0, 8);	// Non-removable, direct-access device
	if (mod_data.removable)
		buf[1] = 0x80;
	buf[2] = 2;		// ANSI SCSI level 2
	buf[3] = 2;		// SCSI-2 INQUIRY data format
	buf[4] = 31;		// Additional length
				// No special options
	sprintf(buf + 8, "%-8s%-16s%04x", vendor_id, product_id,
			DRIVER_VERSION_NUM);
	return 36;
}


static int do_request_sense(struct fsg_dev *fsg, struct fsg_buffhd *bh)
{
	struct lun	*curlun = fsg->curlun;
	u8		*buf = (u8 *) bh->buf;
	u32		sd, sdinfo;

    DBG_FUNCC("+ (FileStorage)do_request_sense()\n");

	/*
	 * From the SCSI-2 spec., section 7.9 (Unit attention condition):
	 *
	 * If a REQUEST SENSE command is received from an initiator
	 * with a pending unit attention condition (before the target
	 * generates the contingent allegiance condition), then the
	 * target shall either:
	 *   a) report any pending sense data and preserve the unit
	 *	attention condition on the logical unit, or,
	 *   b) report the unit attention condition, may discard any
	 *	pending sense data, and clear the unit attention
	 *	condition on the logical unit for that initiator.
	 *
	 * FSG normally uses option a); enable this code to use option b).
	 */
#if 0
	if (curlun && curlun->unit_attention_data != SS_NO_SENSE) {
		curlun->sense_data = curlun->unit_attention_data;
		curlun->unit_attention_data = SS_NO_SENSE;
	}
#endif

	if (!curlun) {		// Unsupported LUNs are okay
		fsg->bad_lun_okay = 1;
		sd = SS_LOGICAL_UNIT_NOT_SUPPORTED;
		sdinfo = 0;
	} else {
		sd = curlun->sense_data;
		sdinfo = curlun->sense_data_info;
		curlun->sense_data = SS_NO_SENSE;
		curlun->sense_data_info = 0;
	}

	memset(buf, 0, 18);
	buf[0] = 0x80 | 0x70;			// Valid, current error
	buf[2] = SK(sd);
	put_be32(&buf[3], sdinfo);		// Sense information
	buf[7] = 18 - 7;			// Additional sense length
	buf[12] = ASC(sd);
	buf[13] = ASCQ(sd);
	return 18;
}


static int do_read_capacity(struct fsg_dev *fsg, struct fsg_buffhd *bh)
{
	struct lun	*curlun = fsg->curlun;
	u32		lba = get_be32(&fsg->cmnd[2]);
	int		pmi = fsg->cmnd[8];
	u8		*buf = (u8 *) bh->buf;

    DBG_FUNCC("+ (FileStorage)do_read_capacity()\n");

	/* Check the PMI and LBA fields */
	if (pmi > 1 || (pmi == 0 && lba != 0)) {
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}

	put_be32(&buf[0], curlun->num_sectors - 1);	// Max logical block
	put_be32(&buf[4], 512);				// Block length
	return 8;
}


static int do_mode_sense(struct fsg_dev *fsg, struct fsg_buffhd *bh)
{
	struct lun	*curlun = fsg->curlun;
	int		mscmnd = fsg->cmnd[0];
	u8		*buf = (u8 *) bh->buf;
	u8		*buf0 = buf;
	int		pc, page_code;
	int		changeable_values, all_pages;
	int		valid_page = 0;
	int		len, limit;

    DBG_FUNCC("+ (FileStorage)do_mode_sense()\n");


    DBG_TEMP("FileStorage =>do_mode_sense() => Check  Mask away DBD\n");//Bruce

	if ((fsg->cmnd[1] & ~0x08) != 0) {		// Mask away DBD
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;

        DBG_TEMP("FileStorage =>do_mode_sense() => (fsg->cmnd[1] & ~0x08) != 0) & return \n");//Bruce
		
		return -EINVAL;
	}

    DBG_TEMP("FileStorage =>do_mode_sense() => Check  SS_SAVING_PARAMETERS_NOT_SUPPORTED\n");//Bruce

	pc = fsg->cmnd[2] >> 6;
	page_code = fsg->cmnd[2] & 0x3f;
	if (pc == 3) {
		curlun->sense_data = SS_SAVING_PARAMETERS_NOT_SUPPORTED;
		return -EINVAL;
	}
	changeable_values = (pc == 1);
	all_pages = (page_code == 0x3f);


    DBG_TEMP("FileStorage =>do_mode_sense() => Check  Write the mode parameter header...\n");//Bruce

	/* Write the mode parameter header.  Fixed values are: default
	 * medium type, no cache control (DPOFUA), and no block descriptors.
	 * The only variable value is the WriteProtect bit.  We will fill in
	 * the mode data length later. */
	memset(buf, 0, 8);
	if (mscmnd == SC_MODE_SENSE_6) {
		buf[2] = (curlun->ro ? 0x80 : 0x00);		// WP, DPOFUA
		buf += 4;
		limit = 255;
	} else {			// SC_MODE_SENSE_10
		buf[3] = (curlun->ro ? 0x80 : 0x00);		// WP, DPOFUA
		buf += 8;
		limit = 65535;		// Should really be mod_data.buflen
	}

	/* No block descriptors */

    DBG_TEMP("FileStorage =>do_mode_sense() =>The mode pages, in numerical order. \n");//Bruce

	/* The mode pages, in numerical order.  The only page we support
	 * is the Caching page. */
	if (page_code == 0x08 || all_pages) {
		valid_page = 1;
		buf[0] = 0x08;		// Page code
		buf[1] = 10;		// Page length
		memset(buf+2, 0, 10);	// None of the fields are changeable

		if (!changeable_values) {
			buf[2] = 0x04;	// Write cache enable,
					// Read cache not disabled
					// No cache retention priorities
			put_be16(&buf[4], 0xffff);  // Don't disable prefetch
					// Minimum prefetch = 0
			put_be16(&buf[8], 0xffff);  // Maximum prefetch
			put_be16(&buf[10], 0xffff); // Maximum prefetch ceiling
		}
		buf += 12;
	}


    DBG_TEMP("FileStorage =>do_mode_sense() => Check that a valid page...\n");//Bruce

	/* Check that a valid page was requested and the mode data length
	 * isn't too long. */
	len = buf - buf0;
	if (!valid_page || len > limit) {
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}


    DBG_TEMP("FileStorage =>do_mode_sense() =>  Store the mode data length...\n");//Bruce

	/*  Store the mode data length */
	if (mscmnd == SC_MODE_SENSE_6)
		buf0[0] = len - 1;
	else
		put_be16(buf0, len - 2);
	return len;
}


static int do_start_stop(struct fsg_dev *fsg)
{
	struct lun	*curlun = fsg->curlun;
	int		loej, start;

    DBG_FUNCC("+ (FileStorage)do_start_stop()\n");

	if (!mod_data.removable) {
		curlun->sense_data = SS_INVALID_COMMAND;
		return -EINVAL;
	}

	// int immed = fsg->cmnd[1] & 0x01;
	loej = fsg->cmnd[4] & 0x02;
	start = fsg->cmnd[4] & 0x01;

#ifdef CONFIG_USB_FILE_STORAGE_TEST
	if ((fsg->cmnd[1] & ~0x01) != 0 ||		// Mask away Immed
			(fsg->cmnd[4] & ~0x03) != 0) {	// Mask LoEj, Start
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}

	if (!start) {

		/* Are we allowed to unload the media? */
		if (curlun->prevent_medium_removal) {
			LDBG(curlun, "unload attempt prevented\n");
			curlun->sense_data = SS_MEDIUM_REMOVAL_PREVENTED;
			return -EINVAL;
		}
		if (loej) {		// Simulate an unload/eject
			up_read(&fsg->filesem);
			down_write(&fsg->filesem);
			close_backing_file(curlun);
			up_write(&fsg->filesem);
			down_read(&fsg->filesem);
		}
	} else {

		/* Our emulation doesn't support mounting; the medium is
		 * available for use as soon as it is loaded. */
		if (!backing_file_is_open(curlun)) {
			curlun->sense_data = SS_MEDIUM_NOT_PRESENT;
			return -EINVAL;
		}
	}
#endif
	return 0;
}


static int do_prevent_allow(struct fsg_dev *fsg)
{
	struct lun	*curlun = fsg->curlun;
	int		prevent;

    DBG_FUNCC("+ (FileStorage)do_prevent_allow()\n");

	if (!mod_data.removable) {
		curlun->sense_data = SS_INVALID_COMMAND;
		return -EINVAL;
	}

	prevent = fsg->cmnd[4] & 0x01;
	if ((fsg->cmnd[4] & ~0x01) != 0) {		// Mask away Prevent
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}

	if (curlun->prevent_medium_removal && !prevent)
		fsync_sub(curlun);
	curlun->prevent_medium_removal = prevent;
	return 0;
}


static int do_read_format_capacities(struct fsg_dev *fsg,
			struct fsg_buffhd *bh)
{
	struct lun	*curlun = fsg->curlun;
	u8		*buf = (u8 *) bh->buf;

    DBG_FUNCC("+ (FileStorage)do_read_format_capacities()\n");

	buf[0] = buf[1] = buf[2] = 0;
	buf[3] = 8;		// Only the Current/Maximum Capacity Descriptor
	buf += 4;

	put_be32(&buf[0], curlun->num_sectors);		// Number of blocks
	put_be32(&buf[4], 512);				// Block length
	buf[4] = 0x02;					// Current capacity
	return 12;
}


static int do_mode_select(struct fsg_dev *fsg, struct fsg_buffhd *bh)
{
	struct lun	*curlun = fsg->curlun;

    DBG_FUNCC("+ (FileStorage)do_mode_select()===> Do not Support ...\n");

	/* We don't support MODE SELECT */
	curlun->sense_data = SS_INVALID_COMMAND;
	return -EINVAL;
}


/*-------------------------------------------------------------------------*/

static int halt_bulk_in_endpoint(struct fsg_dev *fsg)
{
	int	rc;

    DBG_FUNCC("+ (FileStorage)halt_bulk_in_endpoint()\n");

	rc = fsg_set_halt(fsg, fsg->bulk_in);
	if (rc == -EAGAIN)
		VDBG(fsg, "delayed bulk-in endpoint halt\n");
	while (rc != 0) {
		if (rc != -EAGAIN) {
			WARN(fsg, "usb_ep_set_halt -> %d\n", rc);
			rc = 0;
			break;
		}

		/* Wait for a short time and then try again */
		set_current_state(TASK_INTERRUPTIBLE);
		if (schedule_timeout(HZ / 10) != 0)
			return -EINTR;
		rc = usb_ep_set_halt(fsg->bulk_in);
	}
	return rc;
}

static int pad_with_zeros(struct fsg_dev *fsg)
{
	struct fsg_buffhd	*bh = fsg->next_buffhd_to_fill;
	u32			nkeep = bh->inreq->length;
	u32			nsend;
	int			rc;

    DBG_FUNCC("+ (FileStorage)pad_with_zeros()\n");

	bh->state = BUF_STATE_EMPTY;		// For the first iteration
	fsg->usb_amount_left = nkeep + fsg->residue;
	while (fsg->usb_amount_left > 0) {

		/* Wait for the next buffer to be free */
		while (bh->state != BUF_STATE_EMPTY) {
			if ((rc = sleep_thread(fsg)) != 0)
				return rc;
		}

		nsend = min(fsg->usb_amount_left, (u32) mod_data.buflen);
		memset(bh->buf + nkeep, 0, nsend - nkeep);
		bh->inreq->length = nsend;
		bh->inreq->zero = 0;
		start_transfer(fsg, fsg->bulk_in, bh->inreq,
				&bh->inreq_busy, &bh->state);
		bh = fsg->next_buffhd_to_fill = bh->next;
		fsg->usb_amount_left -= nsend;
		nkeep = 0;
	}
	return 0;
}

static int throw_away_data(struct fsg_dev *fsg)
{
	struct fsg_buffhd	*bh;
	u32			amount;
	int			rc;
   
     DBG_FUNCC("+ (FileStorage)throw_away_data()\n");

	while ((bh = fsg->next_buffhd_to_drain)->state != BUF_STATE_EMPTY ||
			fsg->usb_amount_left > 0) {

		/* Throw away the data in a filled buffer */
		if (bh->state == BUF_STATE_FULL) {
			bh->state = BUF_STATE_EMPTY;
			fsg->next_buffhd_to_drain = bh->next;

			/* A short packet or an error ends everything */
			if (bh->outreq->actual != bh->outreq->length ||
					bh->outreq->status != 0) {
				raise_exception(fsg, FSG_STATE_ABORT_BULK_OUT);
				return -EINTR;
			}
			continue;
		}

		/* Try to submit another request if we need one */
		bh = fsg->next_buffhd_to_fill;
		if (bh->state == BUF_STATE_EMPTY && fsg->usb_amount_left > 0) {
			amount = min(fsg->usb_amount_left,
					(u32) mod_data.buflen);

			/* amount is always divisible by 512, hence by
			 * the bulk-out maxpacket size */
			bh->outreq->length = bh->bulk_out_intended_length =
					amount;
			start_transfer(fsg, fsg->bulk_out, bh->outreq,
					&bh->outreq_busy, &bh->state);
			fsg->next_buffhd_to_fill = bh->next;
			fsg->usb_amount_left -= amount;
			continue;
		}

		/* Otherwise wait for something to happen */
		if ((rc = sleep_thread(fsg)) != 0)
			return rc;
	}
	return 0;
}


static int finish_reply(struct fsg_dev *fsg)
{
	struct fsg_buffhd	*bh = fsg->next_buffhd_to_fill;
	int			rc = 0;

    DBG_FUNCC("+ (FileStorage)finish_reply()\n");


	switch (fsg->data_dir) {
	case DATA_DIR_NONE:
		break;			// Nothing to send

	/* If we don't know whether the host wants to read or write,
	 * this must be CB or CBI with an unknown command.  We mustn't
	 * try to send or receive any data.  So stall both bulk pipes
	 * if we can and wait for a reset. */
	case DATA_DIR_UNKNOWN:
		if (mod_data.can_stall) {
			fsg_set_halt(fsg, fsg->bulk_out);
			rc = halt_bulk_in_endpoint(fsg);
		}
		break;

	/* All but the last buffer of data must have already been sent */
	case DATA_DIR_TO_HOST:
		if (fsg->data_size == 0)
			;		// Nothing to send

		/* If there's no residue, simply send the last buffer */
		else if (fsg->residue == 0) {
			bh->inreq->zero = 0;
			start_transfer(fsg, fsg->bulk_in, bh->inreq,
					&bh->inreq_busy, &bh->state);
			fsg->next_buffhd_to_fill = bh->next;
		}

		/* There is a residue.  For CB and CBI, simply mark the end
		 * of the data with a short packet.  However, if we are
		 * allowed to stall, there was no data at all (residue ==
		 * data_size), and the command failed (invalid LUN or
		 * sense data is set), then halt the bulk-in endpoint
		 * instead. */
		else if (!transport_is_bbb()) {
			if (mod_data.can_stall &&
					fsg->residue == fsg->data_size &&
	(!fsg->curlun || fsg->curlun->sense_data != SS_NO_SENSE)) {
				bh->state = BUF_STATE_EMPTY;
				rc = halt_bulk_in_endpoint(fsg);
			} else {
				bh->inreq->zero = 1;
				start_transfer(fsg, fsg->bulk_in, bh->inreq,
						&bh->inreq_busy, &bh->state);
				fsg->next_buffhd_to_fill = bh->next;
			}
		}

		/* For Bulk-only, if we're allowed to stall then send the
		 * short packet and halt the bulk-in endpoint.  If we can't
		 * stall, pad out the remaining data with 0's. */
		else {
			if (mod_data.can_stall) {
				bh->inreq->zero = 1;
				start_transfer(fsg, fsg->bulk_in, bh->inreq,
						&bh->inreq_busy, &bh->state);
				fsg->next_buffhd_to_fill = bh->next;
				rc = halt_bulk_in_endpoint(fsg);
			} else
				rc = pad_with_zeros(fsg);
		}
		break;

	/* We have processed all we want from the data the host has sent.
	 * There may still be outstanding bulk-out requests. */
	case DATA_DIR_FROM_HOST:
		if (fsg->residue == 0)
			;		// Nothing to receive

		/* Did the host stop sending unexpectedly early? */
		else if (fsg->short_packet_received) {
			raise_exception(fsg, FSG_STATE_ABORT_BULK_OUT);
			rc = -EINTR;
		}

		/* We haven't processed all the incoming data.  If we are
		 * allowed to stall, halt the bulk-out endpoint and cancel
		 * any outstanding requests. */
		else if (mod_data.can_stall) {
			fsg_set_halt(fsg, fsg->bulk_out);
			raise_exception(fsg, FSG_STATE_ABORT_BULK_OUT);
			rc = -EINTR;
		}

		/* We can't stall.  Read in the excess data and throw it
		 * all away. */
		else
			rc = throw_away_data(fsg);
		break;
	}
	return rc;
}


static int send_status(struct fsg_dev *fsg)
{
	struct lun		*curlun = fsg->curlun;
	struct fsg_buffhd	*bh;
	int			rc;
	u8			status = USB_STATUS_PASS;
	u32			sd, sdinfo = 0;

    DBG_FUNCC("+ (FileStorage)send_status()\n");

	/* Wait for the next buffer to become available */
	bh = fsg->next_buffhd_to_fill;
	while (bh->state != BUF_STATE_EMPTY) {
		if ((rc = sleep_thread(fsg)) != 0)
			return rc;
	}

	if (curlun) {
		sd = curlun->sense_data;
		sdinfo = curlun->sense_data_info;
	} else if (fsg->bad_lun_okay)
		sd = SS_NO_SENSE;
	else
		sd = SS_LOGICAL_UNIT_NOT_SUPPORTED;

	if (fsg->phase_error) {
		DBG(fsg, "sending phase-error status\n");
		
		status = USB_STATUS_PHASE_ERROR;
		sd = SS_INVALID_COMMAND;
	} else if (sd != SS_NO_SENSE) {
		DBG(fsg, "sending command-failure status\n");
		
		status = USB_STATUS_FAIL;
		VDBG(fsg, "  sense data: SK x%02x, ASC x%02x, ASCQ x%02x;"
				"  info x%x\n",
				SK(sd), ASC(sd), ASCQ(sd), sdinfo);
	}

	if (transport_is_bbb()) {
		struct bulk_cs_wrap	*csw = (struct bulk_cs_wrap *) bh->buf;

		/* Store and send the Bulk-only CSW */
		csw->Signature = __constant_cpu_to_le32(USB_BULK_CS_SIG);
		csw->Tag = fsg->tag;
		csw->Residue = fsg->residue;
		csw->Status = status;
		bh->inreq->length = USB_BULK_CS_WRAP_LEN;
		bh->inreq->zero = 0;
		start_transfer(fsg, fsg->bulk_in, bh->inreq,
				&bh->inreq_busy, &bh->state);

	} else if (mod_data.transport_type == USB_PR_CB) {

		/* Control-Bulk transport has no status stage! */
		return 0;

	} else {			// USB_PR_CBI
		struct interrupt_data	*buf = (struct interrupt_data *)
						bh->buf;

		/* Store and send the Interrupt data.  UFI sends the ASC
		 * and ASCQ bytes.  Everything else sends a Type (which
		 * is always 0) and the status Value. */
		if (mod_data.protocol_type == USB_SC_UFI) {
			buf->bType = ASC(sd);
			buf->bValue = ASCQ(sd);
		} else {
			buf->bType = 0;
			buf->bValue = status;
		}
		fsg->intreq->length = CBI_INTERRUPT_DATA_LEN;

		fsg->intr_buffhd = bh;		// Point to the right buffhd
		fsg->intreq->buf = bh->inreq->buf;
		fsg->intreq->dma = bh->inreq->dma;
		fsg->intreq->context = bh;
		start_transfer(fsg, fsg->intr_in, fsg->intreq,
				&fsg->intreq_busy, &bh->state);
	}

	fsg->next_buffhd_to_fill = bh->next;
	return 0;
}


/*-------------------------------------------------------------------------*/

/* Check whether the command is properly formed and whether its data size
 * and direction agree with the values we already have. */
static int check_command(struct fsg_dev *fsg, int cmnd_size,
		enum data_direction data_dir, unsigned int mask,
		int needs_medium, const char *name)
{
	int			i;
	int			lun = fsg->cmnd[1] >> 5;
	static const char	dirletter[4] = {'u', 'o', 'i', 'n'};
	char			hdlen[20];
	struct lun		*curlun;

    DBG_FUNCC("+ (FileStorage)check_command()\n");

	/* Adjust the expected cmnd_size for protocol encapsulation padding.
	 * Transparent SCSI doesn't pad. */
	if (protocol_is_scsi())
		;

	/* There's some disagreement as to whether RBC pads commands or not.
	 * We'll play it safe and accept either form. */
	else if (mod_data.protocol_type == USB_SC_RBC) {
		if (fsg->cmnd_size == 12)
			cmnd_size = 12;

	/* All the other protocols pad to 12 bytes */
	} else
		cmnd_size = 12;

	hdlen[0] = 0;
	if (fsg->data_dir != DATA_DIR_UNKNOWN)
		sprintf(hdlen, ", H%c=%u", dirletter[(int) fsg->data_dir],
				fsg->data_size);
	VDBG(fsg, "SCSI command: %s;  Dc=%d, D%c=%u;  Hc=%d%s\n",
			name, cmnd_size, dirletter[(int) data_dir],
			fsg->data_size_from_cmnd, fsg->cmnd_size, hdlen);

    DBG_TEMP("- (FileStorage)=>check_command=>Check Dir \n");//Bruce			

	/* We can't reply at all until we know the correct data direction
	 * and size. */
	if (fsg->data_size_from_cmnd == 0)
		data_dir = DATA_DIR_NONE;
	if (fsg->data_dir == DATA_DIR_UNKNOWN) {	// CB or CBI
		fsg->data_dir = data_dir;
		fsg->data_size = fsg->data_size_from_cmnd;

	} else {					// Bulk-only
		if (fsg->data_size < fsg->data_size_from_cmnd) {

			/* Host data size < Device data size is a phase error.
			 * Carry out the command, but only transfer as much
			 * as we are allowed. */
            DBG_TEMP("- (FileStorage)=>check_command=>fsg->data_size < fsg->data_size_from_cmnd \n");//Bruce			
			
			fsg->data_size_from_cmnd = fsg->data_size;
			fsg->phase_error = 1;
			
		}
	}
	fsg->residue = fsg->usb_amount_left = fsg->data_size;

	/* Conflicting data directions is a phase error */
	if (fsg->data_dir != data_dir && fsg->data_size_from_cmnd > 0)
		goto phase_error;

	/* Verify the length of the command itself */
	if (cmnd_size != fsg->cmnd_size) {

		/* Special case workaround: MS-Windows issues REQUEST SENSE
		 * with cbw->Length == 12 (it should be 6). */
		if (fsg->cmnd[0] == SC_REQUEST_SENSE && fsg->cmnd_size == 12)
			cmnd_size = fsg->cmnd_size;
		else
			goto phase_error;
	}

    DBG_TEMP("- (FileStorage)=>check_command=>Check LUN values \n");//Bruce			

	/* Check that the LUN values are oonsistent */
	if (transport_is_bbb()) {
		if (fsg->lun != lun)
			DBG(fsg, "using LUN %d from CBW, "
					"not LUN %d from CDB\n",
					fsg->lun, lun);
	} else
		fsg->lun = lun;		// Use LUN from the command

	/* Check the LUN */
	if (fsg->lun >= 0 && fsg->lun < fsg->nluns) {
		fsg->curlun = curlun = &fsg->luns[fsg->lun];
		if (fsg->cmnd[0] != SC_REQUEST_SENSE) {
            
            DBG_TEMP("- (FileStorage)=>check_command=>fsg->cmnd[0] != SC_REQUEST_SENSE \n");//Bruce			
			
			curlun->sense_data = SS_NO_SENSE;
			curlun->sense_data_info = 0;
		}
	} else {
		fsg->curlun = curlun = NULL;
		fsg->bad_lun_okay = 0;

		/* INQUIRY and REQUEST SENSE commands are explicitly allowed
		 * to use unsupported LUNs; all others may not. */
		if (fsg->cmnd[0] != SC_INQUIRY &&
				fsg->cmnd[0] != SC_REQUEST_SENSE) {
			DBG(fsg, "unsupported LUN %d\n", fsg->lun);
			return -EINVAL;
		}
	}

    DBG_TEMP("- (FileStorage)=>check_command=>Check unit attention condition \n");//Bruce			

	/* If a unit attention condition exists, only INQUIRY and
	 * REQUEST SENSE commands are allowed; anything else must fail. */
	if (curlun && curlun->unit_attention_data != SS_NO_SENSE &&
			fsg->cmnd[0] != SC_INQUIRY &&
			fsg->cmnd[0] != SC_REQUEST_SENSE) {
		curlun->sense_data = curlun->unit_attention_data;
		curlun->unit_attention_data = SS_NO_SENSE;

       DBG_TEMP("- (FileStorage)=>check_command=>return -EINVAL(curlun && curlun->unit_attention_data != SS_NO_SENSE...) \n");//Bruce			
		
		return -EINVAL;
	}

	/* Check that only command bytes listed in the mask are non-zero */
	fsg->cmnd[1] &= 0x1f;			// Mask away the LUN
	for (i = 1; i < cmnd_size; ++i) {
		if (fsg->cmnd[i] && !(mask & (1 << i))) {
			if (curlun)
				curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
            
            DBG_TEMP("- (FileStorage)=>check_command=>return -EINVAL(fsg->cmnd[i] && !(mask & (1 << i))...) \n");//Bruce			
			return -EINVAL;

		}
	}

	/* If the medium isn't mounted and the command needs to access
	 * it, return an error. */
	if (curlun && !backing_file_is_open(curlun) && needs_medium) {
		curlun->sense_data = SS_MEDIUM_NOT_PRESENT;
        
        
        DBG_TEMP("- (FileStorage)=>check_command=>return -EINVAL(curlun && !backing_file_is_open(curlun)...) \n");//Bruce			
		return -EINVAL;
	}

	return 0;

phase_error:
    DBG_TEMP("- (FileStorage)=>check_command=>phase_error \n");//Bruce			

	fsg->phase_error = 1;
	return -EINVAL;
}


static int do_scsi_command(struct fsg_dev *fsg)
{
	struct fsg_buffhd	*bh;
	int			rc;
	int			reply = -EINVAL;
	int			i;
	static char		unknown[16];

    DBG_FUNCC("+ (check_command)do_scsi_command()\n");

	dump_cdb(fsg);

	/* Wait for the next buffer to become available for data or status */
	bh = fsg->next_buffhd_to_drain = fsg->next_buffhd_to_fill;
	while (bh->state != BUF_STATE_EMPTY) {
		if ((rc = sleep_thread(fsg)) != 0)
			return rc;
		}
	fsg->phase_error = 0;
	fsg->short_packet_received = 0;

	down_read(&fsg->filesem);	// We're using the backing file
	switch (fsg->cmnd[0]) {

	case SC_INQUIRY:
		fsg->data_size_from_cmnd = fsg->cmnd[4];
		if ((reply = check_command(fsg, 6, DATA_DIR_TO_HOST,
				(1<<4), 0,
				"INQUIRY")) == 0)
			reply = do_inquiry(fsg, bh);
		break;

	case SC_MODE_SELECT_6:
		fsg->data_size_from_cmnd = fsg->cmnd[4];
		if ((reply = check_command(fsg, 6, DATA_DIR_FROM_HOST,
				(1<<1) | (1<<4), 0,
				"MODE SELECT(6)")) == 0)
			reply = do_mode_select(fsg, bh);
		break;

	case SC_MODE_SELECT_10:
		fsg->data_size_from_cmnd = get_be16(&fsg->cmnd[7]);
		if ((reply = check_command(fsg, 10, DATA_DIR_FROM_HOST,
				(1<<1) | (3<<7), 0,
				"MODE SELECT(10)")) == 0)
			reply = do_mode_select(fsg, bh);
		break;

	case SC_MODE_SENSE_6:
		fsg->data_size_from_cmnd = fsg->cmnd[4];
		if ((reply = check_command(fsg, 6, DATA_DIR_TO_HOST,
				(1<<1) | (1<<2) | (1<<4), 0,
				"MODE SENSE(6)")) == 0)
			reply = do_mode_sense(fsg, bh);
		break;

	case SC_MODE_SENSE_10:
		fsg->data_size_from_cmnd = get_be16(&fsg->cmnd[7]);
		if ((reply = check_command(fsg, 10, DATA_DIR_TO_HOST,
				(1<<1) | (1<<2) | (3<<7), 0,
				"MODE SENSE(10)")) == 0)
			reply = do_mode_sense(fsg, bh);
		break;

	case SC_PREVENT_ALLOW_MEDIUM_REMOVAL:
		fsg->data_size_from_cmnd = 0;
		if ((reply = check_command(fsg, 6, DATA_DIR_NONE,
				(1<<4), 0,
				"PREVENT-ALLOW MEDIUM REMOVAL")) == 0)
			reply = do_prevent_allow(fsg);
		break;

	case SC_READ_6:
		i = fsg->cmnd[4];
		fsg->data_size_from_cmnd = (i == 0 ? 256 : i) << 9;
		if ((reply = check_command(fsg, 6, DATA_DIR_TO_HOST,
				(7<<1) | (1<<4), 1,
				"READ(6)")) == 0)
			reply = do_read(fsg);
		break;

	case SC_READ_10:
		fsg->data_size_from_cmnd = get_be16(&fsg->cmnd[7]) << 9;
		if ((reply = check_command(fsg, 10, DATA_DIR_TO_HOST,
				(1<<1) | (0xf<<2) | (3<<7), 1,
				"READ(10)")) == 0)
			reply = do_read(fsg);
		break;

	case SC_READ_12:
		fsg->data_size_from_cmnd = get_be32(&fsg->cmnd[6]) << 9;
		if ((reply = check_command(fsg, 12, DATA_DIR_TO_HOST,
				(1<<1) | (0xf<<2) | (0xf<<6), 1,
				"READ(12)")) == 0)
			reply = do_read(fsg);
		break;

	case SC_READ_CAPACITY:
		fsg->data_size_from_cmnd = 8;
		if ((reply = check_command(fsg, 10, DATA_DIR_TO_HOST,
				(0xf<<2) | (1<<8), 1,
				"READ CAPACITY")) == 0)
			reply = do_read_capacity(fsg, bh);
		break;

	case SC_READ_FORMAT_CAPACITIES:
		fsg->data_size_from_cmnd = get_be16(&fsg->cmnd[7]);
		if ((reply = check_command(fsg, 10, DATA_DIR_TO_HOST,
				(3<<7), 1,
				"READ FORMAT CAPACITIES")) == 0)
			reply = do_read_format_capacities(fsg, bh);
		break;

	case SC_REQUEST_SENSE:
		fsg->data_size_from_cmnd = fsg->cmnd[4];
		if ((reply = check_command(fsg, 6, DATA_DIR_TO_HOST,
				(1<<4), 0,
				"REQUEST SENSE")) == 0)
			reply = do_request_sense(fsg, bh);
		break;

	case SC_START_STOP_UNIT:
		fsg->data_size_from_cmnd = 0;
		if ((reply = check_command(fsg, 6, DATA_DIR_NONE,
				(1<<1) | (1<<4), 0,
				"START-STOP UNIT")) == 0)
			reply = do_start_stop(fsg);
		break;

	case SC_SYNCHRONIZE_CACHE:
		fsg->data_size_from_cmnd = 0;
		if ((reply = check_command(fsg, 10, DATA_DIR_NONE,
				(0xf<<2) | (3<<7), 1,
				"SYNCHRONIZE CACHE")) == 0)
			reply = do_synchronize_cache(fsg);
		break;

	case SC_TEST_UNIT_READY:
		fsg->data_size_from_cmnd = 0;
		reply = check_command(fsg, 6, DATA_DIR_NONE,
				0, 1,
				"TEST UNIT READY");
		break;

	/* Although optional, this command is used by MS-Windows.  We
	 * support a minimal version: BytChk must be 0. */
	case SC_VERIFY:
		fsg->data_size_from_cmnd = 0;
		if ((reply = check_command(fsg, 10, DATA_DIR_NONE,
				(1<<1) | (0xf<<2) | (3<<7), 1,
				"VERIFY")) == 0)
			reply = do_verify(fsg);
		break;

	case SC_WRITE_6:
		i = fsg->cmnd[4];
		fsg->data_size_from_cmnd = (i == 0 ? 256 : i) << 9;
		if ((reply = check_command(fsg, 6, DATA_DIR_FROM_HOST,
				(7<<1) | (1<<4), 1,
				"WRITE(6)")) == 0)
			reply = do_write(fsg);
		break;

	case SC_WRITE_10:
		fsg->data_size_from_cmnd = get_be16(&fsg->cmnd[7]) << 9;
		if ((reply = check_command(fsg, 10, DATA_DIR_FROM_HOST,
				(1<<1) | (0xf<<2) | (3<<7), 1,
				"WRITE(10)")) == 0)
			reply = do_write(fsg);
		break;

	case SC_WRITE_12:
		fsg->data_size_from_cmnd = get_be32(&fsg->cmnd[6]) << 9;
		if ((reply = check_command(fsg, 12, DATA_DIR_FROM_HOST,
				(1<<1) | (0xf<<2) | (0xf<<6), 1,
				"WRITE(12)")) == 0)
			reply = do_write(fsg);
		break;

	/* Some mandatory commands that we recognize but don't implement.
	 * They don't mean much in this setting.  It's left as an exercise
	 * for anyone interested to implement RESERVE and RELEASE in terms
	 * of Posix locks. */
	case SC_FORMAT_UNIT:
	case SC_RELEASE:
	case SC_RESERVE:
	case SC_SEND_DIAGNOSTIC:
		// Fall through

	default:
		fsg->data_size_from_cmnd = 0;
		sprintf(unknown, "Unknown x%02x", fsg->cmnd[0]);
		if ((reply = check_command(fsg, fsg->cmnd_size,
				DATA_DIR_UNKNOWN, 0xff, 0, unknown)) == 0) {
			fsg->curlun->sense_data = SS_INVALID_COMMAND;
			reply = -EINVAL;
		}
		break;
	}
	up_read(&fsg->filesem);

	if (reply == -EINTR || signal_pending(current))
		return -EINTR;

	/* Set up the single reply buffer for finish_reply() */
	if (reply == -EINVAL)
		reply = 0;		// Error reply length
	if (reply >= 0 && fsg->data_dir == DATA_DIR_TO_HOST) {
		reply = min((u32) reply, fsg->data_size_from_cmnd);
		bh->inreq->length = reply;
		bh->state = BUF_STATE_FULL;
		fsg->residue -= reply;
	}				// Otherwise it's already set

	return 0;
}


/*-------------------------------------------------------------------------*/

static int received_cbw(struct fsg_dev *fsg, struct fsg_buffhd *bh)
{
	struct usb_request	*req = bh->outreq;
	struct bulk_cb_wrap	*cbw = (struct bulk_cb_wrap *) req->buf;

    DBG_FUNCC("+ (FileStorage)received_cbw()\n");


	/* Was this a real packet? */
	if (req->status)
		return -EINVAL;

	/* Is the CBW valid? */
	if (req->actual != USB_BULK_CB_WRAP_LEN ||
			cbw->Signature != __constant_cpu_to_le32(
				USB_BULK_CB_SIG)) {
		DBG(fsg, "invalid CBW: len %u sig 0x%x\n",
				req->actual,
				le32_to_cpu(cbw->Signature));

		/* The Bulk-only spec says we MUST stall the bulk pipes!
		 * If we want to avoid stalls, set a flag so that we will
		 * clear the endpoint halts at the next reset. */
		if (!mod_data.can_stall)
			set_bit(CLEAR_BULK_HALTS, &fsg->atomic_bitflags);
		fsg_set_halt(fsg, fsg->bulk_out);
		halt_bulk_in_endpoint(fsg);
		return -EINVAL;
	}

	/* Is the CBW meaningful? */
	if (cbw->Lun >= MAX_LUNS || cbw->Flags & ~USB_BULK_IN_FLAG ||
			cbw->Length < 6 || cbw->Length > MAX_COMMAND_SIZE) {
		DBG(fsg, "non-meaningful CBW: lun = %u, flags = 0x%x, "
				"cmdlen %u\n",
				cbw->Lun, cbw->Flags, cbw->Length);

		/* We can do anything we want here, so let's stall the
		 * bulk pipes if we are allowed to. */
		if (mod_data.can_stall) {
			fsg_set_halt(fsg, fsg->bulk_out);
			halt_bulk_in_endpoint(fsg);
		}
		return -EINVAL;
	}

	/* Save the command for later */
	fsg->cmnd_size = cbw->Length;
	memcpy(fsg->cmnd, cbw->CDB, fsg->cmnd_size);
	if (cbw->Flags & USB_BULK_IN_FLAG)
		fsg->data_dir = DATA_DIR_TO_HOST;
	else
		fsg->data_dir = DATA_DIR_FROM_HOST;
	fsg->data_size = cbw->DataTransferLength;
	if (fsg->data_size == 0)
		fsg->data_dir = DATA_DIR_NONE;
	fsg->lun = cbw->Lun;
	fsg->tag = cbw->Tag;
	return 0;
}


static int get_next_command(struct fsg_dev *fsg)
{
	struct fsg_buffhd	*bh;
	int			rc = 0;

    DBG_FUNCC("+ (FileStorage)get_next_command()\n");

	if (transport_is_bbb()) {

		/* Wait for the next buffer to become available */
		bh = fsg->next_buffhd_to_fill;
		while (bh->state != BUF_STATE_EMPTY) {
			if ((rc = sleep_thread(fsg)) != 0)
				return rc;
			}

		/* Queue a request to read a Bulk-only CBW */
		set_bulk_out_req_length(fsg, bh, USB_BULK_CB_WRAP_LEN);
		start_transfer(fsg, fsg->bulk_out, bh->outreq,
				&bh->outreq_busy, &bh->state);

		/* We will drain the buffer in software, which means we
		 * can reuse it for the next filling.  No need to advance
		 * next_buffhd_to_fill. */

		/* Wait for the CBW to arrive */
		while (bh->state != BUF_STATE_FULL) {
			if ((rc = sleep_thread(fsg)) != 0)
				return rc;
			}
		rc = received_cbw(fsg, bh);
		bh->state = BUF_STATE_EMPTY;

	} else {		// USB_PR_CB or USB_PR_CBI

		/* Wait for the next command to arrive */
		while (fsg->cbbuf_cmnd_size == 0) {
			if ((rc = sleep_thread(fsg)) != 0)
				return rc;
			}

		/* Is the previous status interrupt request still busy?
		 * The host is allowed to skip reading the status,
		 * so we must cancel it. */
		if (fsg->intreq_busy)
			usb_ep_dequeue(fsg->intr_in, fsg->intreq);

		/* Copy the command and mark the buffer empty */
		fsg->data_dir = DATA_DIR_UNKNOWN;
		spin_lock_irq(&fsg->lock);
		fsg->cmnd_size = fsg->cbbuf_cmnd_size;
		memcpy(fsg->cmnd, fsg->cbbuf_cmnd, fsg->cmnd_size);
		fsg->cbbuf_cmnd_size = 0;
		spin_unlock_irq(&fsg->lock);
	}
	return rc;
}


/*-------------------------------------------------------------------------*/

static int enable_endpoint(struct fsg_dev *fsg, struct usb_ep *ep,
		const struct usb_endpoint_descriptor *d)
{
	int	rc;

    DBG_FUNCC("+ (FileStorage)enable_endpoint()\n");

	ep->driver_data = fsg;
	rc = usb_ep_enable(ep, d);
	if (rc)
		ERROR(fsg, "can't enable %s, result %d\n", ep->name, rc);
	return rc;
}

static int alloc_request(struct fsg_dev *fsg, struct usb_ep *ep,
		struct usb_request **preq)
{
	
	DBG_FUNCC("+ (FileStorage)alloc_request()\n");
	
	*preq = usb_ep_alloc_request(ep, GFP_ATOMIC);
	if (*preq)
		return 0;
	ERROR(fsg, "can't allocate request for %s\n", ep->name);
	return -ENOMEM;
}

/*
 * Reset interface setting and re-init endpoint state (toggle etc).
 * Call with altsetting < 0 to disable the interface.  The only other
 * available altsetting is 0, which enables the interface.
 */
static int do_set_interface(struct fsg_dev *fsg, int altsetting)
{
	int	rc = 0;
	int	i;
	const struct usb_endpoint_descriptor	*d;

    DBG_FUNCC("+ (FileStorage)do_set_interface()\n");

	if (fsg->running)
		DBG(fsg, "reset interface\n");
		
reset:

    DBG_TEMP("FileStorage =>do_set_interface()=> Deallocate the requests \n");//Bruce

	/* Deallocate the requests */
	for (i = 0; i < NUM_BUFFERS; ++i) {
		struct fsg_buffhd *bh = &fsg->buffhds[i];

		if (bh->inreq) {
			usb_ep_free_request(fsg->bulk_in, bh->inreq);
			bh->inreq = NULL;
		}
		if (bh->outreq) {
			usb_ep_free_request(fsg->bulk_out, bh->outreq);
			bh->outreq = NULL;
		}
	}
	if (fsg->intreq) {
		usb_ep_free_request(fsg->intr_in, fsg->intreq);
		fsg->intreq = NULL;
	}

    DBG_TEMP("FileStorage =>do_set_interface()=> Disable the endpoints \n");//Bruce

	/* Disable the endpoints */
	if (fsg->bulk_in_enabled) {
		usb_ep_disable(fsg->bulk_in);
		fsg->bulk_in_enabled = 0;
	}
	if (fsg->bulk_out_enabled) {
		usb_ep_disable(fsg->bulk_out);
		fsg->bulk_out_enabled = 0;
	}
	if (fsg->intr_in_enabled) {
		usb_ep_disable(fsg->intr_in);
		fsg->intr_in_enabled = 0;
	}

	fsg->running = 0;
	if (altsetting < 0 || rc != 0)
		return rc;

	DBG(fsg, "set interface %d\n", altsetting);

    DBG_TEMP("FileStorage =>do_set_interface()=> Enable the endpoints \n");//Bruce

	/* Enable the endpoints */
	d = ep_desc(fsg->gadget, &fs_bulk_in_desc, &hs_bulk_in_desc);
	if ((rc = enable_endpoint(fsg, fsg->bulk_in, d)) != 0)
		goto reset;
	fsg->bulk_in_enabled = 1;

	d = ep_desc(fsg->gadget, &fs_bulk_out_desc, &hs_bulk_out_desc);
	if ((rc = enable_endpoint(fsg, fsg->bulk_out, d)) != 0)
		goto reset;
	fsg->bulk_out_enabled = 1;

	if (transport_is_cbi()) {
		d = ep_desc(fsg->gadget, &fs_intr_in_desc, &hs_intr_in_desc);
		if ((rc = enable_endpoint(fsg, fsg->intr_in, d)) != 0)
			goto reset;
		fsg->intr_in_enabled = 1;
	}
    
    DBG_TEMP("FileStorage =>do_set_interface()=> Allocate the requests \n");//Bruce

	/* Allocate the requests */
	for (i = 0; i < NUM_BUFFERS; ++i) {
		struct fsg_buffhd	*bh = &fsg->buffhds[i];

		if ((rc = alloc_request(fsg, fsg->bulk_in, &bh->inreq)) != 0)
			goto reset;
		if ((rc = alloc_request(fsg, fsg->bulk_out, &bh->outreq)) != 0)
			goto reset;
		bh->inreq->buf = bh->outreq->buf = bh->buf;
		bh->inreq->dma = bh->outreq->dma = bh->dma;
		bh->inreq->context = bh->outreq->context = bh;
		bh->inreq->complete = bulk_in_complete;
		bh->outreq->complete = bulk_out_complete;
	}
	if (transport_is_cbi()) {
		if ((rc = alloc_request(fsg, fsg->intr_in, &fsg->intreq)) != 0)
			goto reset;
		fsg->intreq->complete = intr_in_complete;
	}

	fsg->running = 1;
	for (i = 0; i < fsg->nluns; ++i)
		fsg->luns[i].unit_attention_data = SS_RESET_OCCURRED;
	return rc;
}


/*
 * Change our operational configuration.  This code must agree with the code
 * that returns config descriptors, and with interface altsetting code.
 *
 * It's also responsible for power management interactions.  Some
 * configurations might not work with our current power sources.
 * For now we just assume the gadget is always self-powered.
 */
static int do_set_config(struct fsg_dev *fsg, u8 new_config)
{
	int	rc = 0;


    DBG_FUNCC("+ (FileStorage)do_set_config()\n");

	/* Disable the single interface */
	if (fsg->config != 0) {
		DBG(fsg, "reset config\n");
		fsg->config = 0;
		rc = do_set_interface(fsg, -1);
	}

	/* Enable the interface */
	if (new_config != 0) {
		fsg->config = new_config;
		if ((rc = do_set_interface(fsg, 0)) != 0)
			fsg->config = 0;	// Reset on errors
		else {
			char *speed;

			switch (fsg->gadget->speed) {
			case USB_SPEED_LOW:	speed = "low";	break;
			case USB_SPEED_FULL:	speed = "full";	break;
			case USB_SPEED_HIGH:	speed = "high";	break;
			default: 		speed = "?";	break;
			}

			INFO(fsg, "%s speed config #%d\n", speed, fsg->config);

		}
	}
    DBG_FUNCC("- (FileStorage)do_set_config()\n");
	return rc;
}


/*-------------------------------------------------------------------------*/

static void handle_exception(struct fsg_dev *fsg)
{
	siginfo_t		info;
	int			sig;
	int			i;
	int			num_active;
	struct fsg_buffhd	*bh;
	enum fsg_state		old_state;
	u8			new_config;
	struct lun		*curlun;
	unsigned int		exception_req_tag;
	int			rc;


    DBG_FUNCC("+ (FileStorage)handle_exception()\n");

	/* Clear the existing signals.  Anything but SIGUSR1 is converted
	 * into a high-priority EXIT exception. */
	for (;;) {
		spin_lock_irq(&current->sigmask_lock);
		sig = dequeue_signal(&fsg->thread_signal_mask, &info);
		spin_unlock_irq(&current->sigmask_lock);
		if (!sig)
			break;
		if (sig != SIGUSR1) {
			if (fsg->state < FSG_STATE_EXIT)
				DBG(fsg, "Main thread exiting on signal\n");
			raise_exception(fsg, FSG_STATE_EXIT);
		}
	}

	/* Cancel all the pending transfers */
	if (fsg->intreq_busy)
		usb_ep_dequeue(fsg->intr_in, fsg->intreq);
	for (i = 0; i < NUM_BUFFERS; ++i) {
		bh = &fsg->buffhds[i];
		if (bh->inreq_busy)
			usb_ep_dequeue(fsg->bulk_in, bh->inreq);
		if (bh->outreq_busy)
			usb_ep_dequeue(fsg->bulk_out, bh->outreq);
	}

	/* Wait until everything is idle */
	for (;;) {
		num_active = fsg->intreq_busy;
		for (i = 0; i < NUM_BUFFERS; ++i) {
			bh = &fsg->buffhds[i];
			num_active += bh->inreq_busy + bh->outreq_busy;
		}
		//Andrew fix bug;;if (num_active == 0)
	if ((num_active == 0)||                   // YPING fix bug
       (fsg->state == FSG_STATE_EXIT) ||      // YPING fix bug
       (fsg->state == FSG_STATE_TERMINATED))  // YPING fix bug
			break;
		if (sleep_thread(fsg))
			return;
	}

	/* Clear out the controller's fifos */
	if (fsg->bulk_in_enabled)
		usb_ep_fifo_flush(fsg->bulk_in);
	if (fsg->bulk_out_enabled)
		usb_ep_fifo_flush(fsg->bulk_out);
	if (fsg->intr_in_enabled)
		usb_ep_fifo_flush(fsg->intr_in);

	/* Reset the I/O buffer states and pointers, the SCSI
	 * state, and the exception.  Then invoke the handler. */
	spin_lock_irq(&fsg->lock);

	for (i = 0; i < NUM_BUFFERS; ++i) {
		bh = &fsg->buffhds[i];
		bh->state = BUF_STATE_EMPTY;
	}
	fsg->next_buffhd_to_fill = fsg->next_buffhd_to_drain =
			&fsg->buffhds[0];

	exception_req_tag = fsg->exception_req_tag;
	new_config = fsg->new_config;
	old_state = fsg->state;

	if (old_state == FSG_STATE_ABORT_BULK_OUT)
		fsg->state = FSG_STATE_STATUS_PHASE;
	else {
		for (i = 0; i < fsg->nluns; ++i) {
			curlun = &fsg->luns[i];
			curlun->prevent_medium_removal = 0;
			curlun->sense_data = curlun->unit_attention_data =
					SS_NO_SENSE;
			curlun->sense_data_info = 0;
		}
		fsg->state = FSG_STATE_IDLE;
	}
	spin_unlock_irq(&fsg->lock);

	/* Carry out any extra actions required for the exception */
	switch (old_state) {
	default:
		break;

	case FSG_STATE_ABORT_BULK_OUT:
		send_status(fsg);
		spin_lock_irq(&fsg->lock);
		if (fsg->state == FSG_STATE_STATUS_PHASE)
			fsg->state = FSG_STATE_IDLE;
		spin_unlock_irq(&fsg->lock);
		break;

	case FSG_STATE_RESET:
		/* In case we were forced against our will to halt a
		 * bulk endpoint, clear the halt now.  (The SuperH UDC
		 * requires this.) */
		if (test_and_clear_bit(CLEAR_BULK_HALTS,
				&fsg->atomic_bitflags)) {
			usb_ep_clear_halt(fsg->bulk_in);
			usb_ep_clear_halt(fsg->bulk_out);
		}

		if (transport_is_bbb()) {
			if (fsg->ep0_req_tag == exception_req_tag)
				ep0_queue(fsg);	// Complete the status stage

		} else if (transport_is_cbi())
			send_status(fsg);	// Status by interrupt pipe

		/* Technically this should go here, but it would only be
		 * a waste of time.  Ditto for the INTERFACE_CHANGE and
		 * CONFIG_CHANGE cases. */
		// for (i = 0; i < fsg->nluns; ++i)
		//	fsg->luns[i].unit_attention_data = SS_RESET_OCCURRED;
		break;

	case FSG_STATE_INTERFACE_CHANGE:
		rc = do_set_interface(fsg, 0);
		if (fsg->ep0_req_tag != exception_req_tag)
			break;
		if (rc != 0)			// STALL on errors
			fsg_set_halt(fsg, fsg->ep0);
		else				// Complete the status stage
			ep0_queue(fsg);
		break;

	case FSG_STATE_CONFIG_CHANGE:
		rc = do_set_config(fsg, new_config);
		if (fsg->ep0_req_tag != exception_req_tag)
			break;
		if (rc != 0)			// STALL on errors
			fsg_set_halt(fsg, fsg->ep0);
		else				// Complete the status stage
			ep0_queue(fsg);
		break;

	case FSG_STATE_DISCONNECT:
		fsync_all(fsg);
		do_set_config(fsg, 0);		// Unconfigured state
		break;

	case FSG_STATE_EXIT:
	case FSG_STATE_TERMINATED:
		do_set_config(fsg, 0);			// Free resources
		spin_lock_irq(&fsg->lock);
		fsg->state = FSG_STATE_TERMINATED;	// Stop the thread
		spin_unlock_irq(&fsg->lock);
		break;
	}
}


/*-------------------------------------------------------------------------*/

static int fsg_main_thread(void *fsg_)
{
	struct fsg_dev		*fsg = (struct fsg_dev *) fsg_;

    DBG_FUNCC("+ (FileStorage)fsg_main_thread()\n");


	fsg->thread_task = current;

	/* Release all our userspace resources */
	daemonize();
	reparent_to_init();
	strncpy(current->comm, "file-storage-gadget",
			sizeof(current->comm) - 1);

	/* Allow the thread to be killed by a signal, but set the signal mask
	 * to block everything but INT, TERM, KILL, and USR1. */
	siginitsetinv(&fsg->thread_signal_mask, sigmask(SIGINT) |
			sigmask(SIGTERM) | sigmask(SIGKILL) |
			sigmask(SIGUSR1));
	spin_lock_irq(&current->sigmask_lock);
	flush_signals(current);
	current->blocked = fsg->thread_signal_mask;
	recalc_sigpending(current);
	spin_unlock_irq(&current->sigmask_lock);

	/* Arrange for userspace references to be interpreted as kernel
	 * pointers.  That way we can pass a kernel pointer to a routine
	 * that expects a __user pointer and it will work okay. */
	set_fs(get_ds());

	/* Wait for the gadget registration to finish up */
	wait_for_completion(&fsg->thread_notifier);

	/* The main loop */
	while (fsg->state != FSG_STATE_TERMINATED) {
		if (exception_in_progress(fsg) || signal_pending(current)) {
			handle_exception(fsg);
			continue;
		}

		if (!fsg->running) {
			sleep_thread(fsg);
			continue;
		}

		if (get_next_command(fsg))
			continue;

		spin_lock_irq(&fsg->lock);
		if (!exception_in_progress(fsg))
			fsg->state = FSG_STATE_DATA_PHASE;
		spin_unlock_irq(&fsg->lock);

		if (do_scsi_command(fsg) || finish_reply(fsg))
			continue;

		spin_lock_irq(&fsg->lock);
		if (!exception_in_progress(fsg))
			fsg->state = FSG_STATE_STATUS_PHASE;
		spin_unlock_irq(&fsg->lock);

		if (send_status(fsg))
			continue;

		spin_lock_irq(&fsg->lock);
		if (!exception_in_progress(fsg))
			fsg->state = FSG_STATE_IDLE;
		spin_unlock_irq(&fsg->lock);
		}

	fsg->thread_task = NULL;
	flush_signals(current);

	/* In case we are exiting because of a signal, unregister the
	 * gadget driver and close the backing file. */
	if (test_and_clear_bit(REGISTERED, &fsg->atomic_bitflags)) {
		usb_gadget_unregister_driver(&fsg_driver);
		close_all_backing_files(fsg);
	}

	/* Let the unbind and cleanup routines know the thread has exited */
	complete_and_exit(&fsg->thread_notifier, 0);
}


/*-------------------------------------------------------------------------*/

/* If the next two routines are called while the gadget is registered,
 * the caller must own fsg->filesem for writing. */

static int NORMALLY_INIT open_backing_file(struct lun *curlun,
		const char *filename)
{
	int				ro;
	struct file			*filp = NULL;
	int				rc = -EINVAL;
	struct inode			*inode = NULL;
	loff_t				size;
	loff_t				num_sectors;

    DBG_FUNCC("+ (FileStorage)open_backing_file()\n");
    

    DBG_TEMP("Open backing file: %s\n", filename);//Bruce
    
	/* R/W if we can, R/O if we must */
	ro = curlun->ro;
	if (!ro) {
		filp = filp_open(filename, O_RDWR | O_LARGEFILE, 0);
		if (-EROFS == PTR_ERR(filp))
			ro = 1;
	}
	if (ro)
		filp = filp_open(filename, O_RDONLY | O_LARGEFILE, 0);
	if (IS_ERR(filp)) {
		LINFO(curlun, "unable to open backing file: %s\n", filename);
		return PTR_ERR(filp);
	}
   
    DBG_TEMP( "Open backing file Finish...: %s\n", filename);//Bruce

	if (!(filp->f_mode & FMODE_WRITE))
		ro = 1;

	if (filp->f_dentry)
		inode = filp->f_dentry->d_inode;
	if (inode && S_ISBLK(inode->i_mode)) {
		kdev_t		dev = inode->i_rdev;
        
    DBG_TEMP( "inode=S_ISBLK(inode->i_mode)\n");//Bruce

		if (blk_size[MAJOR(dev)])
			size = (loff_t) blk_size[MAJOR(dev)][MINOR(dev)] <<
					BLOCK_SIZE_BITS;
		else {
			LINFO(curlun, "unable to find file size: %s\n",
					filename);
			goto out;
		}
	} else if (inode && S_ISREG(inode->i_mode))
		size = inode->i_size;
	else {
		LINFO(curlun, "invalid file type: %s\n", filename);
		goto out;
	}


    DBG_TEMP("File Type & Size Check finish...Size=%d\n",(int)size);//Bruce

	/* If we can't read the file, it's no good.
	 * If we can't write the file, use it read-only. */
	if (!filp->f_op || !filp->f_op->read) {
		LINFO(curlun, "file not readable: %s\n", filename);
		goto out;
	}
	if (IS_RDONLY(inode) || !filp->f_op->write)
		ro = 1;

	num_sectors = size >> 9;	// File size in 512-byte sectors
	if (num_sectors == 0) {
		LINFO(curlun, "file too small: %s\n", filename);
		rc = -ETOOSMALL;
		goto out;
	}

    LINFO(curlun, "get_file...\n");
	get_file(filp);
    LINFO(curlun, "get_file Finish...\n");

	curlun->ro = ro;
	curlun->filp = filp;
	curlun->file_length = size;
	curlun->num_sectors = num_sectors;
	LDBG(curlun, "open backing file: %s\n", filename);
	rc = 0;

out:
	filp_close(filp, current->files);
	return rc;
}


static void close_backing_file(struct lun *curlun)
{
	
	DBG_FUNCC("+ (FileStorage)close_backing_file()\n");
	
	if (curlun->filp) {
		LDBG(curlun, "close backing file\n");
		fput(curlun->filp);
		curlun->filp = NULL;
	}
}

static void close_all_backing_files(struct fsg_dev *fsg)
{
	int	i;

    DBG_FUNCC("+ (FileStorage)close_all_backing_files()\n");

	for (i = 0; i < fsg->nluns; ++i)
		close_backing_file(&fsg->luns[i]);
}


/*-------------------------------------------------------------------------*/

static void fsg_unbind(struct usb_gadget *gadget)
{
	struct fsg_dev		*fsg = get_gadget_data(gadget);
	int			i;
	struct usb_request	*req = fsg->ep0req;

    DBG_FUNCC("+ (FileStorage)fsg_unbind()\n");

	DBG(fsg, "unbind\n");
	clear_bit(REGISTERED, &fsg->atomic_bitflags);

	/* If the thread isn't already dead, tell it to exit now */
	if (fsg->state != FSG_STATE_TERMINATED) {
		raise_exception(fsg, FSG_STATE_EXIT);
		wait_for_completion(&fsg->thread_notifier);

		/* The cleanup routine waits for this completion also */
		complete(&fsg->thread_notifier);
	}

	/* Free the data buffers */
	for (i = 0; i < NUM_BUFFERS; ++i) {
		struct fsg_buffhd	*bh = &fsg->buffhds[i];

		if (bh->buf)
			usb_ep_free_buffer(fsg->bulk_in, bh->buf, bh->dma,
					mod_data.buflen);
	}

	/* Free the request and buffer for endpoint 0 */
	if (req) {
		if (req->buf)
			usb_ep_free_buffer(fsg->ep0, req->buf,
					req->dma, EP0_BUFSIZE);
		usb_ep_free_request(fsg->ep0, req);
	}

	set_gadget_data(gadget, 0);
}


static int __init check_parameters(struct fsg_dev *fsg)
{
	int	prot;

    DBG_FUNCC("+ (FileStorage)check_parameters()\n");


	/* Store the default values */
	mod_data.transport_type = USB_PR_BULK;
	mod_data.transport_name = "Bulk-only";
	mod_data.protocol_type = USB_SC_SCSI;
	mod_data.protocol_name = "Transparent SCSI";

	prot = simple_strtol(mod_data.protocol_parm, NULL, 0);

#ifdef CONFIG_USB_FILE_STORAGE_TEST
	if (strnicmp(mod_data.transport_parm, "BBB", 10) == 0) {
		;		// Use default setting
	} else if (strnicmp(mod_data.transport_parm, "CB", 10) == 0) {
		mod_data.transport_type = USB_PR_CB;
		mod_data.transport_name = "Control-Bulk";
	} else if (strnicmp(mod_data.transport_parm, "CBI", 10) == 0) {
		mod_data.transport_type = USB_PR_CBI;
		mod_data.transport_name = "Control-Bulk-Interrupt";
	} else {
		INFO(fsg, "invalid transport: %s\n", mod_data.transport_parm);
		return -EINVAL;
	}

	if (strnicmp(mod_data.protocol_parm, "SCSI", 10) == 0 ||
			prot == USB_SC_SCSI) {
		;		// Use default setting
	} else if (strnicmp(mod_data.protocol_parm, "RBC", 10) == 0 ||
			prot == USB_SC_RBC) {
		mod_data.protocol_type = USB_SC_RBC;
		mod_data.protocol_name = "RBC";
	} else if (strnicmp(mod_data.protocol_parm, "8020", 4) == 0 ||
			strnicmp(mod_data.protocol_parm, "ATAPI", 10) == 0 ||
			prot == USB_SC_8020) {
		mod_data.protocol_type = USB_SC_8020;
		mod_data.protocol_name = "8020i (ATAPI)";
	} else if (strnicmp(mod_data.protocol_parm, "QIC", 3) == 0 ||
			prot == USB_SC_QIC) {
		mod_data.protocol_type = USB_SC_QIC;
		mod_data.protocol_name = "QIC-157";
	} else if (strnicmp(mod_data.protocol_parm, "UFI", 10) == 0 ||
			prot == USB_SC_UFI) {
		mod_data.protocol_type = USB_SC_UFI;
		mod_data.protocol_name = "UFI";
	} else if (strnicmp(mod_data.protocol_parm, "8070", 4) == 0 ||
			prot == USB_SC_8070) {
		mod_data.protocol_type = USB_SC_8070;
		mod_data.protocol_name = "8070i";
	} else {
		INFO(fsg, "invalid protocol: %s\n", mod_data.protocol_parm);
		return -EINVAL;
	}

	mod_data.buflen &= PAGE_CACHE_MASK;
	if (mod_data.buflen <= 0) {
		INFO(fsg, "invalid buflen\n");
		return -ETOOSMALL;
	}
#endif /* CONFIG_USB_FILE_STORAGE_TEST */

	return 0;
}


static int __init fsg_bind(struct usb_gadget *gadget)
{
	struct fsg_dev		*fsg = the_fsg;
	int			rc;
	int			i;
	struct lun		*curlun;
	struct usb_ep		*ep;
	struct usb_request	*req;
	char			*pathbuf, *p;

    DBG_FUNCC("+ (FileStorage)fsg_bind()\n");

	fsg->gadget = gadget;
	set_gadget_data(gadget, fsg);
	fsg->ep0 = gadget->ep0;
	fsg->ep0->driver_data = fsg;

	if ((rc = check_parameters(fsg)) != 0)
		goto out;

	/* Find out how many LUNs there should be */
	i = mod_data.nluns;
	if (i == 0) {
		for (i = MAX_LUNS; i > 1; --i) {
			if (file[i - 1])
				break;
		}
	}
	if (i > MAX_LUNS) {
		INFO(fsg, "invalid number of LUNs: %d\n", i);
		rc = -EINVAL;
		goto out;
	}

	/* Create the LUNs and open their backing files.  We can't register
	 * the LUN devices until the gadget itself is registered, which
	 * doesn't happen until after fsg_bind() returns. */
	fsg->luns = kmalloc(i * sizeof(struct lun), GFP_KERNEL);
	if (!fsg->luns) {
		rc = -ENOMEM;
		goto out;
	}
	memset(fsg->luns, 0, i * sizeof(struct lun));
	fsg->nluns = i;

	for (i = 0; i < fsg->nluns; ++i) {
		curlun = &fsg->luns[i];
		curlun->ro = ro[i];
		curlun->dev.driver_data = fsg;
		snprintf(curlun->dev.name, BUS_ID_SIZE,
				"%s-lun%d", gadget->name, i);

		if (file[i] && *file[i]) {
			if ((rc = open_backing_file(curlun, file[i])) != 0)
				goto out;
		} else if (!mod_data.removable) {
			INFO(fsg, "no file given for LUN%d\n", i);
			rc = -EINVAL;
			goto out;
		}
	}
    DBG_TEMP( "fsg_bind=> Fix up the descriptors \n");//Bruce

	/* Fix up the descriptors */
	device_desc.bMaxPacketSize0 = fsg->ep0->maxpacket;
#ifdef HIGHSPEED
	dev_qualifier.bMaxPacketSize0 = fsg->ep0->maxpacket;		// ???
#endif
	device_desc.idVendor = cpu_to_le16(mod_data.vendor);
	device_desc.idProduct = cpu_to_le16(mod_data.product);
	device_desc.bcdDevice = cpu_to_le16(mod_data.release);

	i = (transport_is_cbi() ? 3 : 2);	// Number of endpoints
	config_desc.wTotalLength = USB_DT_CONFIG_SIZE + USB_DT_INTERFACE_SIZE
			+ USB_DT_ENDPOINT_SIZE * i;
	intf_desc.bNumEndpoints = i;
	intf_desc.bInterfaceSubClass = mod_data.protocol_type;
	intf_desc.bInterfaceProtocol = mod_data.transport_type;

//For OTG
    
    config_desc.wTotalLength +=3;

//For OTG

    DBG_TEMP( "fsg_bind=>Find all the endpoints we will use \n");//Bruce

	/* Find all the endpoints we will use */
	i=0;
	gadget_for_each_ep(ep, gadget) {
    DBG_TEMP( "fsg_bind=>gadget_for_each_ep (ep->name=%s) \n",ep->name);//Bruce
    i++;
		if (strcmp(ep->name, EP_BULK_IN_NAME) == 0)
			{fsg->bulk_in = ep;
             DBG_TEMP( "fsg_bind=> Found BULK_IN ED \n");//Bruce
 
			}
		else if (strcmp(ep->name, EP_BULK_OUT_NAME) == 0)
			{fsg->bulk_out = ep;
             DBG_TEMP("fsg_bind=> Found BULK_OUT ED \n");//Bruce
			}
		else if (strcmp(ep->name, EP_INTR_IN_NAME) == 0)
			{fsg->intr_in = ep;
             DBG_TEMP( "fsg_bind=> Found INTR_IN ED \n");//Bruce
			}
	if (i>5)  //Bruce;;Modify
	   break; //Bruce;;Modify	 
	}
	
   DBG_TEMP("fsg_bind=>Check the ed type \n");//Bruce

	if (!fsg->bulk_in || !fsg->bulk_out ||
			(transport_is_cbi() && !fsg->intr_in)) {
		DBG(fsg, "unable to find all endpoints\n");
		rc = -ENOTSUPP;
		goto out;
	}
	fsg->bulk_out_maxpacket = (gadget->speed == USB_SPEED_HIGH ? 512 :
			FS_BULK_OUT_MAXPACKET);

	rc = -ENOMEM;
   DBG_TEMP( "Allocate the request and buffer for endpoint 0  \n");//Bruce

//For OTG;;Start

	if (gadget->is_otg) {
		otg_desc.bmAttributes |= USB_OTG_HNP,
		config_desc.bmAttributes |= USB_CONFIG_ATT_WAKEUP;
	}

//For ORG;;End    

	/* Allocate the request and buffer for endpoint 0 */
	fsg->ep0req = req = usb_ep_alloc_request(fsg->ep0, GFP_KERNEL);
	if (!req)
		goto out;
	req->buf = usb_ep_alloc_buffer(fsg->ep0, EP0_BUFSIZE,
			&req->dma, GFP_KERNEL);
	if (!req->buf)
		goto out;
	req->complete = ep0_complete;

   DBG_TEMP("Allocate the data buffers\n");//Bruce

	/* Allocate the data buffers */
	for (i = 0; i < NUM_BUFFERS; ++i) {
		struct fsg_buffhd	*bh = &fsg->buffhds[i];

		bh->buf = usb_ep_alloc_buffer(fsg->bulk_in, mod_data.buflen,
				&bh->dma, GFP_KERNEL);
		if (!bh->buf)
			goto out;
		bh->next = bh + 1;
	}
	fsg->buffhds[NUM_BUFFERS - 1].next = &fsg->buffhds[0];
   DBG_TEMP("This should reflect the actual gadget power source\n");//Bruce

	/* This should reflect the actual gadget power source */
	usb_gadget_set_selfpowered(gadget);
   DBG_TEMP( "encode serial[] from the driver version string\n");//Bruce

	/* On a real device, serial[] would be loaded from permanent
	 * storage.  We just encode it from the driver version string. */
	for (i = 0; i < sizeof(serial) - 2; i += 2) {
		unsigned char		c = DRIVER_VERSION[i / 2];

		if (!c)
			break;
		sprintf(&serial[i], "%02X", c);
	}
   DBG_TEMP("kernel_thread(fsg_main_thread...\n");//Bruce
	if ((rc = kernel_thread(fsg_main_thread, fsg, (CLONE_VM | CLONE_FS |
			CLONE_FILES))) < 0)
		goto out;
	fsg->thread_pid = rc;

	INFO(fsg, DRIVER_DESC ", version: " DRIVER_VERSION "\n");
	INFO(fsg, "Number of LUNs=%d\n", fsg->nluns);

	pathbuf = kmalloc(PATH_MAX, GFP_KERNEL);
	for (i = 0; i < fsg->nluns; ++i) {
		curlun = &fsg->luns[i];
		if (backing_file_is_open(curlun)) {
			p = NULL;
			if (pathbuf) {
				p = d_path(curlun->filp->f_dentry,
					curlun->filp->f_vfsmnt,
					pathbuf, PATH_MAX);
				if (IS_ERR(p))
					p = NULL;
			}
			LINFO(curlun, "ro=%d, file: %s\n",
					curlun->ro, (p ? p : "(error)"));
		}
	}
	kfree(pathbuf);

	DBG(fsg, "transport=%s (x%02x)\n",
			mod_data.transport_name, mod_data.transport_type);
	DBG(fsg, "protocol=%s (x%02x)\n",
			mod_data.protocol_name, mod_data.protocol_type);
	DBG(fsg, "VendorID=x%04x, ProductID=x%04x, Release=x%04x\n",
			mod_data.vendor, mod_data.product, mod_data.release);
	DBG(fsg, "removable=%d, stall=%d, buflen=%u\n",
			mod_data.removable, mod_data.can_stall,
			mod_data.buflen);
	DBG(fsg, "I/O thread pid: %d\n", fsg->thread_pid);
    
   DBG_TEMP( "fsg_bind OK...\n");//Bruce

	return 0;

out:
       DBG_TEMP(  "fsg_bind Fail(out)...\n");//Bruce

	fsg->state = FSG_STATE_TERMINATED;	// The thread is dead
	fsg_unbind(gadget);
	close_all_backing_files(fsg);
	return rc;
}


/*-------------------------------------------------------------------------*/

static struct usb_gadget_driver		fsg_driver = {
#ifdef HIGHSPEED
	.speed		= USB_SPEED_HIGH,
#else
	.speed		= USB_SPEED_FULL,
#endif
	.function	= (char *) longname,
	.bind		= fsg_bind,
	.unbind		= fsg_unbind,
	.disconnect	= fsg_disconnect,
	.setup		= fsg_setup,

	.driver		= {
		.name		= (char *) shortname,
		// .release = ...
		// .suspend = ...
		// .resume = ...
	},
};


static int __init fsg_alloc(void)
{
	struct fsg_dev		*fsg;

    DBG_FUNCC("+ (FileStorage)fsg_alloc()\n");

	fsg = kmalloc(sizeof *fsg, GFP_KERNEL);
	if (!fsg)
		return -ENOMEM;
	memset(fsg, 0, sizeof *fsg);
	spin_lock_init(&fsg->lock);
	init_rwsem(&fsg->filesem);
	init_waitqueue_head(&fsg->thread_wqh);
	init_completion(&fsg->thread_notifier);

	the_fsg = fsg;
	return 0;
}


static void fsg_free(struct fsg_dev *fsg)
{
	DBG_FUNCC("+ (FileStorage)fsg_free()\n");
	
	kfree(fsg->luns);
	kfree(fsg);
}


static int __init fsg_init(void)
{
	int		rc;
	struct fsg_dev	*fsg;

    DBG_FUNCC("+ (FileStorage)fsg_init()\n");

	/* Put the module parameters where they belong -- arghh! */
	mod_data.nluns = luns;
	mod_data.transport_parm = transport;
	mod_data.protocol_parm = protocol;
	mod_data.removable = removable;
	mod_data.vendor = vendor;
	mod_data.product = product;
	mod_data.release = release;
	mod_data.buflen = buflen;
	mod_data.can_stall = stall;

	if ((rc = fsg_alloc()) != 0)
		return rc;
	fsg = the_fsg;
	if ((rc = usb_gadget_register_driver(&fsg_driver)) != 0) {
		fsg_free(fsg);
		return rc;
	}
	set_bit(REGISTERED, &fsg->atomic_bitflags);

	/* Tell the thread to start working */
	complete(&fsg->thread_notifier);
	return 0;
}
module_init(fsg_init);


static void __exit fsg_cleanup(void)
{
	struct fsg_dev	*fsg = the_fsg;

   DBG_FUNCC("+ (FileStorage)fsg_cleanup()\n");

	/* Unregister the driver iff the thread hasn't already done so */
	if (test_and_clear_bit(REGISTERED, &fsg->atomic_bitflags))
		usb_gadget_unregister_driver(&fsg_driver);

	/* Wait for the thread to finish up */
	wait_for_completion(&fsg->thread_notifier);

	close_all_backing_files(fsg);
	fsg_free(fsg);
}
module_exit(fsg_cleanup);
