CFLAGS += -DMODEL_$(MODEL)
INSTALL_DIR	= $(PROJ_INSTALL)/usr/local/www/adm
INSTALL_MANUAL	= yes
CGI_DIR =  $(PROJ_INSTALL)/usr/local/www/oamp
include $(PROJ_CGI2)/makefiles/makefile_exe
install:
	install -D $(BIN) $(CGI_DIR)/$(BIN)
	cd $(CGI_DIR);\
	ln -sf $(BIN) System.xml;\
	ln -sf $(BIN) DeviceBasicInfo.xml;\
	ln -sf $(BIN) DeviceNetworkInfo.xml;\
	ln -sf $(BIN) AddressingSetting.xml;\
	ln -sf $(BIN) OperationSetting.xml;\
	ln -sf $(BIN) FirmwareUpgradeSetting.xml;\
	ln -sf $(BIN) BonjourSetting.xml;\
	ln -sf $(BIN) TimeSetting.xml;\
	ln -sf $(BIN) LogSetting.xml;\
	ln -sf $(BIN) WirelessCapabilities.xml;\
	ln -sf $(BIN) WirelessClientParameters.xml;\
	cd $(PROJ_INSTALL)/usr/local/bin;\
	ln -sf stunnel oamp_stunnel
	
uninstall:
	rm -f $(PROJ_INSTALL)/usr/local/bin/$(BIN)