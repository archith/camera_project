#ifndef _ALL_GROUP_WEBULK_OPS_H_
#define _ALL_GROUP_WEBULK_OPS_H_

#include <webulk_ops.h>

extern struct webulk_ops im_group_webulk_ops;
extern struct webulk_ops log_group_webulk_ops;
extern struct webulk_ops ddns_group_webulk_ops;
extern struct webulk_ops audio_group_webulk_ops;
extern struct webulk_ops image_group_webulk_ops;
extern struct webulk_ops web_group_webulk_ops;
extern struct webulk_ops bonj_group_webulk_ops;
extern struct webulk_ops upnp_group_webulk_ops;
extern struct webulk_ops ftp_group_webulk_ops;
extern struct webulk_ops dnswitch_group_webulk_ops;
extern struct webulk_ops qos_group_webulk_ops;
extern struct webulk_ops smtp_group_webulk_ops;
extern struct webulk_ops event_group_webulk_ops;
extern struct webulk_ops user_group_webulk_ops;
extern struct webulk_ops rtsp_group_webulk_ops;
extern struct webulk_ops ipfilter_group_webulk_ops;
extern struct webulk_ops serial_group_webulk_ops;
extern struct webulk_ops ptz_group_webulk_ops;
extern struct webulk_ops system_group_webulk_ops;
extern struct webulk_ops wlan_group_webulk_ops;
extern struct webulk_ops schedule_group_webulk_ops;
extern struct webulk_ops mot_group_webulk_ops;
extern struct webulk_ops io_group_webulk_ops;
extern struct webulk_ops net_group_webulk_ops;
#define ALL_GROUP_WEBULK_OPS_LIST {&im_group_webulk_ops, &log_group_webulk_ops, \
	&ddns_group_webulk_ops,	&audio_group_webulk_ops, &image_group_webulk_ops, \
	&web_group_webulk_ops, &bonj_group_webulk_ops, &upnp_group_webulk_ops, \
	&ftp_group_webulk_ops, &dnswitch_group_webulk_ops, &qos_group_webulk_ops, \
	&smtp_group_webulk_ops, &event_group_webulk_ops, &user_group_webulk_ops, \
	&rtsp_group_webulk_ops, &ipfilter_group_webulk_ops,&serial_group_webulk_ops,\
	&ptz_group_webulk_ops, &system_group_webulk_ops, &wlan_group_webulk_ops, \
	&schedule_group_webulk_ops, &mot_group_webulk_ops, &io_group_webulk_ops, \
	&net_group_webulk_ops, NULL}
 #endif

