INSTALL_DIR	= $(PROJ_INSTALL)/usr/local/www/camera_api
INSTALL_MANUAL	= yes
IO_DIR		= /usr/local/www/io
PT_DIR		= /usr/local/www/pt
include $(PROJ_CGI2)/makefiles/makefile_exe
install:
	install -D $(BIN) $(INSTALL_DIR)/$(BIN)
	cd $(INSTALL_DIR);\
	ln -sf $(BIN) Open;\
	ln -sf $(BIN) Close;\
	ln -sf $(BIN) CreateEventServer;\
	ln -sf $(BIN) CreateMotionEvent;\
	ln -sf $(BIN) CreateIOEvent;\
	ln -sf $(BIN) CreateMessageAction;\
	ln -sf $(BIN) DeleteEventServer;\
	ln -sf $(BIN) DeleteEvent;\
	ln -sf $(BIN) DeleteEventAction;\
	ln -sf ../adm/get_group.cgi GetGroup;\
	mkdir -p io;\
	ln -sf $(IO_DIR)/ioctrl.cgi io/ioctrl.cgi;\
	ln -sf $(IO_DIR)/query_io.cgi io/query_io.cgi;\
	ln -sf $(IO_DIR)/filterctrl.cgi io/filterctrl.cgi;\
	ln -sf $(IO_DIR)/query_filter.cgi io/query_filter.cgi;\
	mkdir -p pt;\
	ln -sf $(PT_DIR)/ptctrl.cgi pt/ptctrl.cgi;
	
	
uninstall:
	rm -f $(INSTALL_DIR)/$(BIN)
