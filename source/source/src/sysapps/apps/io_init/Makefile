CROSS		= arm-linux-
CC		= $(CROSS)gcc
STRIP		= $(CROSS)strip
STRIPCMD	= $(STRIP) --remove-section=.note --remove-section=.comment

CFLAGS			+= -I../cgi2/inc
#CFLAGS			+= -D_DEBUG_
LDFLAGS			= -s
LIBS			+= -L../cgi2/bin -lpond -lpthread
LDFLAGS			+= $(LIBS)

BIN			= io_init
SRCS			= $(BIN).c
OBJS			= $(SRCS:.c=.o)


all: $(BIN) install

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJS)
	@$(CC) $< $(LDFLAGS) -o $@
	@$(STRIPCMD) $@

clean: uninstall
	@rm -f $(BIN) *.o 

install:
	@install -D $(BIN) $(PROJ_INSTALL)/usr/local/bin/$(BIN)

uninstall:
	@rm -f $(PROJ_INSTALL)/usr/local/bin/$(BIN)

gpl-clean:
	@rm -f $(SRCS) $(BIN)

