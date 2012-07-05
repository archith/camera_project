INSTALL_DIR	= $(PROJ_INSTALL)/usr/local/www/adm
INSTALL_MANUAL	= yes

include $(PROJ_CGI2)/makefiles/makefile_exe
CFLAGS += -D_FILE_OFFSET_BITS=64
install:
	install -D --mode=0755 $(BIN) $(INSTALL_DIR)/$(BIN)
	
uninstall:
	rm -f $(INSTALL_DIR)/$(BIN)	
