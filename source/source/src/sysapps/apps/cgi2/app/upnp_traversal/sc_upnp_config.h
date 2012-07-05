#ifndef __SC_UPNP_CONFIG_H__
#define __SC_UPNP_CONFIG_H__


#define SCUPNP_PREV_SETTING_FILE	"/tmp/upnp_traversal_setting"
#define SCUPNP_REMOTEHOST_MAX_LEN  	20
#define SCUPNP_IP_MAX_LEN			20
#define SCUPNP_DESC_MAX_LNE 		256
#define SCUPNP_PROTOCOL_LEN			4
#define SCUPNP_REQUEST_COUNT		23	//(http+https+rtsp+20rtp)
typedef struct
{
	int enable;
	char RemoteHost[SCUPNP_REMOTEHOST_MAX_LEN];
	char client_ip[SCUPNP_IP_MAX_LEN];
	char PortMappingDesc[SCUPNP_DESC_MAX_LNE];
	unsigned short ExternalPort[SCUPNP_REQUEST_COUNT];
	unsigned short InternalPort[SCUPNP_REQUEST_COUNT];
	char Protocol[SCUPNP_REQUEST_COUNT][SCUPNP_PROTOCOL_LEN];
}ScUPnPIgdInfo;

int GetSettings(ScUPnPIgdInfo *current, ScUPnPIgdInfo *prev);
#endif

