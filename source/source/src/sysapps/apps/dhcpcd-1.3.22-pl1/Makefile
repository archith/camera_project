CROSS = arm-linux-
CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip
STRIPCMD = $(STRIP) --remove-section=.note --remove-section=.comment

CFLAGS =-DGOT_FORK
#CFLAGS +=-DDEBUG
#CFLAGS += -D_LCDON_
CFLAGS += -D_LOG_SPEC_		# Need LOG #
CFLAGS += -I../cgi2/inc
#CFLAGS += -D_SUPPORT_AUTOIP_ -I./AutoIP

LDFLAGS = -s
LIBS += -L../cgi2/bin -ldhcp_pond

BIN = dhcpcd 

SRCS = arp.c buildmsg.c client.c dhcpcd.c peekfd.c signals.c udpipgen.c 
OBJS = $(SRCS:.c=.o)

#AUTOIPOBJ = ./AutoIP/autoip.o

all: $(BIN) install

$(BIN): $(AUTOIPOBJ) $(OBJS)
	@$(CC) $(LDFLAGS) -o $@ $^  $(LIBS) -lpthread
	@$(STRIPCMD) $@

clean: uninstall
	rm -f $(BIN) *.elf $(OBJS) $(AUTOIPOBJ)

gpl-clean: clean
	@echo "GPL-CLEAR" $(PWD)

install:
	@install -D $(BIN) $(PROJ_INSTALL)/usr/local/bin/$(BIN)

uninstall:
	@rm -f $(PROJ_INSTALL)/usr/local/bin/$(BIN)
