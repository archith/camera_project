#ifndef OAMP_PATTERN_H
#define OAMP_PATTERN_H

#define OAMP_XML_TYPE 0
#define OAMP_OCTECT_TYPE 1
enum
{
	OAMP_OK=0,
	OAMP_POLL=2,
	OAMP_NOK=4,
	OAMP_INSANE=5,
	OAMP_LATER=6,
	OAMP_INPROGRESS=9,
	OAMP_INVALID_PRAM=10,
	OAMP_SESSION=11,
	OAMP_MISSING_TARGET=12
};

int pattern_buildall(int response_code, ...);
int pattern_http_header(char* fmt, ...);
int pattern_xml_header(void);
int pattern_xml_footer(void);
int pattern_xml_actionStatus(int response_code, ...);
int pattern_xml_DeviceConfiguration_DeviceBasicInfo(char* version);
int pattern_xml_DeviceConfiguration_DeviceNetworkInfo(char* hostName, char* domainName);
int pattern_xml_DeviceConfiguration_AddressingSetting_InterfaceList(char* interfaceName, int addressingType, char* MACAddress, char* statusMode, char* IPAddress, char* subnetMask);
int pattern_xml_DeviceConfiguration_AddressingSetting_DNSServerList(char* dnsServerIPAddress);
int pattern_xml_DeviceConfiguration_AddressingSetting_defaultGatewayAddress(char* RountingDevice, char* defaultGatewayAddress);
int pattern_xml_DeviceConfiguration_AddressingSetting_DNSServerList(char* dnsServerIPAddress);
int pattern_xml_DeviceConfiguration_OperationalSettings(int OAMPMode, int GUIMode, int BasicConfigurationDoneMode);
int pattern_xml_DeviceConfiguration_FirmwareUpgradeSetting(char* acceptPushMode, char* updateProtocol, char* acceptPullMode);
int pattern_xml_DeviceConfiguration_BonjourSetting(char* serviceNmae, int stateMode, int BonjourMode);
int pattern_xml_DeviceConfiguration_TimeSetting(char* POSIXTimeZoneID, char* timeZoneName, char* IPAddress, int portNo);
int pattern_xml_DeviceConfiguration_LogSettings(char* IPAddress, int portNo);
int pattern_xml_SlaveDevice(void);
int pattern_xml_WirelessCapabilities(char* RegulatoryDomain, int sc, int ec);
int pattern_xml_WirelessClientParameters(char* protection, char* ssid, char* authKeyType, char* authKey);


#endif
