CROSS = arm-linux-
CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip
STRIPCMD = $(STRIP) --remove-section=.note --remove-section=.comment

CFLAGS			+= -I../cgi2/inc
CFLAGS			+= -I../
LDFLAGS			= -s

BIN			= ledoff

SRCS			= $(BIN).c

OBJS			= $(BIN).o


LIBS 			+= -L../cgi2/bin -lpond -lpthread

STRIPCMD		= $(STRIP) --remove-section=.note --remove-section=.comment


all: $(BIN) install

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN): $(OBJS) 
	$(CC) -o $@ $^ $(LIBS)

clean: 
	@rm -f $(BIN) *.o 

gpl-clean:
	rm -rf $(SRCS) *.h

mrproper:clean 

install:
	@install -D $(BIN) $(PROJ_INSTALL)/usr/local/bin/$(BIN)

