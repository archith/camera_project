#ifndef _ALL_WEBULK_OPS_H_
#define _ALL_WEBULK_OPS_H_

#include <webulk_ops.h>

extern struct webulk_ops im_webulk_ops;
extern struct webulk_ops log_webulk_ops;
extern struct webulk_ops ddns_webulk_ops;
extern struct webulk_ops audio_webulk_ops;
extern struct webulk_ops image_webulk_ops;
extern struct webulk_ops web_webulk_ops;
extern struct webulk_ops bonj_webulk_ops;
extern struct webulk_ops upnp_webulk_ops;
extern struct webulk_ops ftp_webulk_ops;
extern struct webulk_ops dnswitch_webulk_ops;
extern struct webulk_ops qos_webulk_ops;
extern struct webulk_ops smtp_webulk_ops;
extern struct webulk_ops message_webulk_ops;
extern struct webulk_ops event_webulk_ops;
extern struct webulk_ops user_webulk_ops;
extern struct webulk_ops rtsp_webulk_ops;
extern struct webulk_ops ipfilter_webulk_ops;
extern struct webulk_ops serial_webulk_ops;
extern struct webulk_ops ptz_webulk_ops;
extern struct webulk_ops system_webulk_ops;
extern struct webulk_ops wlan_webulk_ops;
extern struct webulk_ops schedule_webulk_ops;
extern struct webulk_ops mot_webulk_ops;
extern struct webulk_ops io_webulk_ops;
extern struct webulk_ops net_webulk_ops;
extern struct webulk_ops misc_webulk_ops;
extern struct webulk_ops smb_webulk_ops;
extern struct webulk_ops cdp_webulk_ops;
#define ALL_WEBULK_OPS_LIST {&im_webulk_ops, &log_webulk_ops, &ddns_webulk_ops,\
	&audio_webulk_ops, &image_webulk_ops, &web_webulk_ops, \
	&bonj_webulk_ops, &upnp_webulk_ops, &ftp_webulk_ops, &dnswitch_webulk_ops,\
	&qos_webulk_ops, &smtp_webulk_ops, &message_webulk_ops, &event_webulk_ops, \
	&user_webulk_ops, &rtsp_webulk_ops, &ipfilter_webulk_ops,&serial_webulk_ops,\
	&ptz_webulk_ops, &system_webulk_ops, &wlan_webulk_ops, &schedule_webulk_ops, \
	&mot_webulk_ops, &io_webulk_ops, &net_webulk_ops, &misc_webulk_ops, &smb_webulk_ops,\
	&cdp_webulk_ops, NULL}
 #endif

