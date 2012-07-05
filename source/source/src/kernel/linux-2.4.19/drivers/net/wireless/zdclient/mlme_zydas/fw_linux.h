/*
 * os-glue/ads/fw_ads.h
 *
 * Copyright (C) 2004  Ivan Wang <ivan@faraday.com.tw>
 */ 

#ifndef _FW_ADS_H_
#define _FW_ADS_H_

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/reboot.h>
#include <asm/io.h>
#include <asm/unaligned.h>
#include <asm/processor.h>
#include <linux/ethtool.h>
#include <linux/inetdevice.h>
#include <linux/bitops.h>
#include <linux/if.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/ip.h>
#include <linux/wireless.h>
#include <linux/if_arp.h>

/* data type definition */
typedef unsigned int    UINT32;
typedef unsigned short  UINT16;
typedef unsigned char   UINT8;

#define fw_memcpy memcpy
#define fw_memcmp memcmp
#define fw_memset memset
#define fw_strcmp strcmp
#define fw_strcpy strcpy

extern unsigned short mlme_dbg_level;
#define fw_printf(level,string,args...) printk(string,## args)


#define fw_showstr(level,string,args...) { printk(string,## args);printk("\n");}
unsigned char fw_rand_bytes(void);
void fw_add_timer(void *timer);
void fw_init_timer(void *timer);
void fw_del_timer(void *timer);
void fw_setup_timer(void *timer,unsigned int period,void *data,void *func);
#endif

