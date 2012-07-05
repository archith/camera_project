CROSS = arm-linux-
CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip
STRIPCMD = $(STRIP) --remove-section=.note --remove-section=.comment
CFLAGS	+= -I../cgi2/inc -I../

LDFLAGS = -s

BIN =  crond

OBJS = cron.o cron-parent.o
LIBS += -L../cgi2/bin -lpond -lpthread

all: $(BIN) install

$(BIN): $(OBJS)
	@$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@$(STRIPCMD) $@

clean: 
	rm -f $(BIN) *.elf $(OBJS)

gpl-clean:
	@echo "GPL-CLEAR" $(PWD)

install:
	@install -D $(BIN) $(PROJ_INSTALL)/usr/sbin/$(BIN)
