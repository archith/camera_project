# Default Configure

CC                      = arm-linux-gcc
LD                      = arm-linux-ld

STRIP = arm-linux-strip
STRIPCMD = $(STRIP) --remove-section=.note --remove-section=.comment

CFLAGS                  =
LDFLAGS                 = -s
WD                      = ${shell pwd}
TOPDIR                  = $(WD)/../..

LIB_DIR = $(PROJ_ROOT)/src/sysapps/apps/cgi2
INSTALL_DIR     = $(PROJ_ROOT)/img/fs/shadow_root/usr/local/bin

CFLAGS  = -I$(LIB_DIR)/inc
LDFLAGS += -L$(LIB_DIR)/bin -lpond -lpthread -L$(OLD_LIB)/bin

 
BIN  = ddnsinit

OBJS = ddnsinit.o 


CFLAGS += -I../../cgi/libcgicomm/inc


all: $(BIN) install

%.o: %.c
	@$(CC) -c $(CFLAGS) $< -o $@

$(BIN): $(OBJS) 
	@$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(STRIPCMD) $@
 

install: 
	install -D $(BIN) $(PROJ_INSTALL)/usr/local/bin/$(BIN)

uninstall:
	rm -rf  $(PROJ_INSTALL)/usr/local/bin/$(BIN)

clean: uninstall
	rm -f $(BIN) $(OBJS) *~

gpl-clean:
	@echo "GPL-CLEAR" $(PWD)
	@rm -f *.c *.h 

