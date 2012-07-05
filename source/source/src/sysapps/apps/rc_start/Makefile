CROSS = arm-linux-
CC = $(CROSS)gcc
CFLAGS += -Os -Wall -s 
CFLAGS += -I ./include
CFLAGS += -I ../
CFLAGS += -I ../cgi2/inc/ -L ../cgi2/bin/ -lpond -lpthread

INSTALL_DIR = ../../../../img/fs/shadow_root/usr/local/bin
#INSTALL_DIR = /nfs

BIN = rc

ifndef CROSS
CFLAGS += -DTEST
endif

LIBS  = rc.o
#LIBS += ./snmp/snmp_cfg.o
#LIBS += ./snmp/chl_util.o
LIBS += ./cdp/cdp_cfg.o
#LIBS += ./ssh/ssh_cfg.o

all: $(BIN) install 
	
$(BIN):$(LIBS)
	$(CC) $(CFLAGS) -o $@ $^
	cp $(BIN) $(INSTALL_DIR)

install:
	cp $(BIN) $(INSTALL_DIR)
clean:
	rm -f $(BIN) $(LIBS) 
gpl-clean:
	rm -f $(BIN) *.c */*.c */*.h
	rm -rf include/ 

