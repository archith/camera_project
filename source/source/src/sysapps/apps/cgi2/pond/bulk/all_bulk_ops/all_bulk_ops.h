#ifndef _ALL_BULK_OPS_H_
#define _ALL_BULK_OPS_H_

#include <bulk_ops.h>
#include <conf_sec.h>
#include <stdio.h>
#include <wlan_bulk_ops_def.h>
#include <im_bulk_ops_def.h>
#include <log_bulk_ops_def.h>
#include <mot_bulk_ops_def.h>
#include <ddns_bulk_ops_def.h>
#include <audio_bulk_ops_def.h>
#include <image_bulk_ops_def.h>
#include <web_bulk_ops_def.h>
#include <ftp_bulk_ops_def.h>
#include <dnswitch_bulk_ops_def.h>
#include <qos_bulk_ops_def.h>
#include <smtp_bulk_ops_def.h>
#include <message_bulk_ops_def.h>
#include <event_bulk_ops_def.h>
#include <user_bulk_ops_def.h>
#include <wlan_bulk_ops_def.h>
#include <rtsp_bulk_ops_def.h>
#include <ipfilter_bulk_ops_def.h>
#include <serial_bulk_ops_def.h>
#include <ptz_bulk_ops_def.h>
#include <system_bulk_ops_def.h>
#include <wlan_bulk_ops_def.h>
#include <schedule_bulk_ops_def.h>
#include <io_bulk_ops_def.h>
#include <network_bulk_ops_def.h>
#include <misc_bulk_ops_def.h>
#include <bonj_bulk_ops_def.h>
#include <upnp_bulk_ops_def.h>
#include <smb_bulk_ops_def.h>
#include <cdp_bulk_ops_def.h>

#define ALL_BULK_OPS_LIST {&im_bulk_ops, &log_bulk_ops, &mot_bulk_ops, \
	&ddns_bulk_ops, &audio_bulk_ops, &image_bulk_ops,  \
	&ftp_bulk_ops, &dnswitch_bulk_ops, \
	&qos_bulk_ops, &smtp_bulk_ops, &message_bulk_ops, &event_bulk_ops, \
	&user_bulk_ops, &RTSP_bulk_ops, &IPFILTER_bulk_ops, &com_bulk_ops, \
	&ptz_bulk_ops, &sys_bulk_ops, &wlan_bulk_ops, &schedule_bulk_ops, \
	&io_bulk_ops, &net_bulk_ops, &misc_bulk_ops, &bonj_bulk_ops,\
	&UPnP_bulk_ops, &web_bulk_ops, &smb_bulk_ops, &cdp_bulk_ops, NULL}

#endif
