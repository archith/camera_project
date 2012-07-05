CROSS = arm-linux-
CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip
STRIPCMD = $(STRIP) --remove-section=.note --remove-section=.comment

CFLAGS			+= -I../cgi2/inc
LDFLAGS			= -s

BIN			= ntp_setcron

SRCS			= $(BIN).c
OBJS			= $(SRCS:.c=.o)

LIBS += -L../cgi2/bin -lpond


all: $(BIN) install

%.o: %.c
	@$(CC) -c $(CFLAGS) $< -o $@

$(BIN): $(OBJS)
	@$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@$(STRIPCMD) $@

clean: uninstall
	@rm -f $(BIN) *.o 

gpl-clean:
	rm -rf $(SRCS) *.h

install:
	@install -D $(BIN) $(PROJ_INSTALL)/usr/local/bin/$(BIN)

uninstall:
	@rm -f $(PROJ_INSTALL)/usr/local/bin/$(BIN)
