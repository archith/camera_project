INSTALL_DIR	= $(PROJ_INSTALL)/usr/local/bin
INSTALL_MANUAL	= yes
GPL_LICENSE	= yes
include $(PROJ_CGI2)/makefiles/makefile_exe
install:
	install -D --mode=0755 $(BIN) $(INSTALL_DIR)/$(BIN)
	cd $(PROJ_INSTALL)/usr/local/bin/;\
	ln -sf stunnel jabber_stunnel

uninstall:
	rm -f $(INSTALL_DIR)/$(BIN);
	cd $(PROJ_INSTALL)/usr/local/bin/;\
	rm -f jabber_stunnel
