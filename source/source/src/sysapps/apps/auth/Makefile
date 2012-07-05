CROSS	= arm-linux-
CC	= $(CROSS)gcc
STRIP	= $(CROSS)strip

LIB_DIR	= $(PROJ_ROOT)/src/sysapps/apps/cgi2
INSTALL_DIR	= $(PROJ_ROOT)/img/fs/shadow_root/usr/local/bin
HTML_DIR	= $(PROJ_ROOT)/img/fs/shadow_root/usr/local/www

CFLAGS	= -I$(LIB_DIR)/inc

LDFLAGS	= -L$(LIB_DIR)/bin -lpond -lpthread -L$(OLD_LIB)/bin

BIN	= auth
HEADER	= $(BIN).h
SRC	= $(BIN).c
OBJ	= $(BIN).o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

all:	$(BIN) install

$(BIN):	$(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@
	$(STRIP) $@

install:
	@install -D --mode=0755 $(BIN) $(INSTALL_DIR)/$(BIN)
	ln -sf /mnt/ramdisk/.imgpw $(HTML_DIR)/.htpasswd
	ln -sf /mnt/ramdisk/.imgpw $(HTML_DIR)/img/.htpasswd
	ln -sf /mnt/ramdisk/.admpw $(HTML_DIR)/adm/.htpasswd
	ln -sf /mnt/ramdisk/.iopw $(HTML_DIR)/io/.htpasswd
	ln -sf /mnt/ramdisk/.ptpw $(HTML_DIR)/pt/.htpasswd
	ln -sf /mnt/ramdisk/.campw $(HTML_DIR)/camera_api/.htpasswd
	ln -sf /mnt/ramdisk/.campw $(HTML_DIR)/camera_api/io/.htpasswd
	ln -sf /mnt/ramdisk/.campw $(HTML_DIR)/camera_api/pt/.htpasswd

clean:	
	rm -f $(INSTALL_DIR)/$(BIN)
	rm -f $(BIN)
	rm -f $(OBJ)

gpl-clean:
	rm -f $(SRC) $(HEADER) $(BIN)
