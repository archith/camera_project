INSTALL_DIR	= $(PROJ_INSTALL)/usr/local/www/adm
INSTALL_MANUAL	= yes

include $(PROJ_CGI2)/makefiles/makefile_exe
install:
	install -D --mode=0755 reset_to_default.sh $(INSTALL_DIR)/$(BIN)

uninstall:
	rm -f $(INSTALL_DIR)/$(BIN)
