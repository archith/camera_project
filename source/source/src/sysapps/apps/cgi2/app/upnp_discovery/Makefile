INSTALL_DIR	= $(PROJ_INSTALL)/usr/local/bin
INSTALL_MANUAL	= yes
include $(PROJ_CGI2)/makefiles/makefile_exe
install:
	install -D --mode=0755 $(BIN) $(INSTALL_DIR)/$(BIN)
	install -D --mode=0755 dummy.txt $(PROJ_INSTALL)/usr/local/www/upnp/dummy.txt
	cd $(PROJ_INSTALL)/usr/local/www/upnp;\
	ln -sf /tmp/device.xml .;\
	ln -sf /tmp/scpd_basic.xml .

uninstall:
	rm -f $(INSTALL_DIR)/$(BIN);
	rm -rf $(PROJ_INSTALL)/usr/local/www/upnp
