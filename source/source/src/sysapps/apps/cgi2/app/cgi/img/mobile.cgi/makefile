INSTALL_MANUAL	= yes
INSTALL_DIR	= $(PROJ_INSTALL)/usr/local/www/img
include $(PROJ_CGI2)/makefiles/makefile_exe
install:
	cp mobile.sh mobile.cgi
	install -D $(BIN) $(INSTALL_DIR)/$(BIN)
	
uninstall:
	rm -f $(INSTALL_DIR)/$(BIN)