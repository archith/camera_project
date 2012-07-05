CROSS	= arm-linux-
CC	= $(CROSS)gcc
LD	= $(CROSS)ld
STRIP	= $(CROSS)strip
STRIPCMD = $(STRIP) --remove-section=.note --remove-section=.comment

CFLAGS	+= -I../cgi2/inc
LDFLAGS = -s
LIBS	+= -L../cgi2/bin
LIBS	+= -lpond -lpthread
LDFLAGS += $(LIBS)

BIN = webd
SRCS = httpd_config.c ssl_config.c port_control.c webd.c
OBJS = $(SRCS:.c=.o)
HDRS = config.h webd.h


all: $(BIN) install

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@
	$(STRIPCMD) $(BIN)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install:
	install -D --mode=755 $(BIN) $(PROJ_INSTALL)/usr/local/bin/$(BIN)

clean: uninstall
	rm -f $(BIN) *.o

uninstall:
	@rm -f $(PROJ_INSTALL)/usr/local/bin/$(BIN)

gpl-clean:
	@rm -f $(SRCS) $(HDRS) $(BIN) README.neutral

