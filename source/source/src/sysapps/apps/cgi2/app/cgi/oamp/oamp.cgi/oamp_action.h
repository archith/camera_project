#ifndef OAMP_ACTION_H
#define OAMP_ACTION_H
#include <cgi-parse.h>
#define OAMP_ACTION_MAX_LEN 64
struct action_ds
{
	char name[128];
	int (*function)(LIST* qs_list);
};

int action_getVersions(LIST* qs_list);
int action_login(LIST* qs_list);
int action_logout(LIST* qs_list);
int action_changePassword(LIST* qs_list);
int action_reboot(LIST* qs_list);
int action_restore(LIST* qs_list);
int action_downloadConfigurationFile(LIST* qs_list);
int action_uploadConfigurationFile(LIST* qs_list);
int action_updateFirmware(LIST* qs_list);
int action_loadFirmware(LIST* qs_list);
int action_loadFirmwareStatus(LIST* qs_list);
int action_getSnapShot(LIST* qs_list);
int action_getDeviceHostname(LIST* qs_list);
int action_setDeviceDomain(LIST* qs_list);
int action_setDeviceHostname(LIST* qs_list);
int action_getDeviceNetworkInfo(LIST* qs_list);
int action_setDeviceNetworkInfo(LIST* qs_list);
int action_getDNSServers(LIST* qs_list);
int action_setDNSServer(LIST* qs_list);
int action_deleteDNSServer(LIST* qs_list);
int action_getGatewayInfo(LIST* qs_list);
int action_setGatewayInfo(LIST* qs_list);
int action_getOperationalSettings(LIST* qs_list);
int action_setOperationalSettings(LIST* qs_list);
int action_getFirmwareUpgradeSetting(LIST* qs_list);
int action_getBonjourSetting(LIST* qs_list);
int action_setBonjourSetting(LIST* qs_list);
int action_getTimeSetting(LIST* qs_list);
int action_setSNTPInfo(LIST* qs_list);
int action_deleteSNTPInfo(LIST* qs_list);
int action_setTimeZoneInfo(LIST* qs_list);
int action_getLogSettings(LIST* qs_list);
int action_setLogReceiver(LIST* qs_list);
int action_deleteLogReceiver(LIST* qs_list);
int action_ListSlaveDevices(LIST* qs_list);
int action_getWirelessCapabilities(LIST* qs_list);
int action_setWirelessClientParameters(LIST* qs_list);
int action_getWirelessClientParameters(LIST* qs_list);
int action_resetWirelessClientParameters(LIST* qs_list);
#endif

