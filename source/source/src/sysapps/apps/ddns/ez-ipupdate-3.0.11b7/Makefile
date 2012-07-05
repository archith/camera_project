# Default Configure

CC                      = arm-linux-gcc
LD                      = arm-linux-ld

STRIP = arm-linux-strip
STRIPCMD = $(STRIP) --remove-section=.note --remove-section=.comment

CFLAGS                  =
LDFLAGS                 = -s
WD                      = ${shell pwd}
TOPDIR                  = $(WD)/../..
#CFLAGS 		+= -D_ALWAYS_USE_DYNDNS_CHKIP_

LIB_DIR = $(PROJ_ROOT)/src/sysapps/apps/cgi2
INSTALL_DIR     = $(PROJ_ROOT)/img/fs/shadow_root/usr/local/bin

CFLAGS  += -I$(LIB_DIR)/inc
LDFLAGS += -L$(LIB_DIR)/bin -lpond -lpthread -L$(OLD_LIB)/bin

BIN  = ez-ipupdate

OBJS=ez-ipupdate.o 


#CFLAGS += -DDEBUG 
#ifeq ($(OEM),Neutral)
CFLAGS                 += -D_GNUDIP_DDNS_
OBJS += md5.o
#endif


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

gpl-clean: clean
