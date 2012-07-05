#ifndef _ZD_DEBUG_
#define _ZD_DEBUG_

#include <linux/string.h>
#include <stdarg.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include "zd1205.h"

//#define ZD_DEBUG

#ifdef ZD_DEBUG
//#define ZDEBUG(args...) printk(KERN_DEBUG "%s (%s) " __FUNCTION__ "/%d: ", "zd1205", dev->name, __LINE__); printk(args)
#define ZENTER() printk(KERN_DEBUG "%s: (enter) %s, %s line %i\n", "zd1205", __FUNCTION__,__FILE__,__LINE__)
#define ZEXIT() printk(KERN_DEBUG "%s: (exit) %s, %s line %i\n", "zd1205", __FUNCTION__,__FILE__,__LINE__)
#else
//#define ZDDEBUG(args...) //do {} while (0)
#define ZENTER() //do {} while (0)
#define ZEXIT() //do {} while (0)
#endif

int zd1205_zd_dbg_ioctl(struct zd1205_private *macp, struct zdap_ioctl *zdreq);
void zd1205_lb_mode(struct zd1205_private *macp);
void zd1205_set_sniffer_mode(struct zd1205_private *macp);
void zd1205_dump_regs(struct zd1205_private *macp);
void zd1205_dump_cnters(struct zd1205_private *macp);
void zd1205_cam_clear(struct zd1205_private *macp);
void zd1205_cam_dump(struct zd1205_private *macp);
void zd1205_get_ap(struct zd1205_private *macp, u32 n);
void zd1205_get_mic_error_count(struct zd1205_private *macp);
void zd1205_set_ap(struct zd1205_private *macp, u32 n);
void zd1205_set_vap(struct zd1205_private *macp, u32 n);
void zd1205_get_vap(struct zd1205_private *macp, u32 n);
void zd1205_set_apc(struct zd1205_private *macp, u32 n);
void zd1205_get_apc(struct zd1205_private *macp, u32 n);
void zd1205_set_op_mode(struct zd1205_private *macp, u32 mode);
void zd1205_wep_on_off(struct zd1205_private *macp, u32 value);
void zd1205_do_cam(struct zd1205_private *macp, u32 value);
void zd1205_do_vap(struct zd1205_private *macp, u32 value);
void zd1205_do_apc(struct zd1205_private *macp, u32 value);
void zd1205_dump_rfds(struct zd1205_private *macp);
#endif
