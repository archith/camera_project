CROSS = arm-linux-uclibc-
CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip
STRIPCMD = $(STRIP) --remove-section=.note --remove-section=.comment

CFLAGS                  = -I../cgi2/inc

LDFLAGS                 += -L../cgi2/bin


BIN =  ipfilter

SRCS			= $(BIN).c
OBJS			= $(SRCS:.c=.o)

all: $(BIN) install

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -lpond -lpthread
	@$(STRIPCMD) $@

clean: 
	rm -f $(BIN) *.elf $(OBJS)

install:
	@install -D $(BIN) $(PROJ_INSTALL)/usr/sbin/$(BIN)

gpl-clean:
	rm -f $(SRCS) 