CROSS = arm-linux-
CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip
STRIPCMD = $(STRIP) --remove-section=.note --remove-section=.comment
LDFLAGS			= -s

BIN			= do_leds

SRCS			= $(BIN).c
OBJS			= $(SRCS:.c=.o)

all: $(BIN) install

%.o: %.c
	@$(CC) -c $(CFLAGS) $< -o $@

$(BIN): $(OBJS)
	@$(CC) $(LDFLAGS) -o $@ $^ 
	@$(STRIPCMD) $@

clean: 
	@rm -f $(BIN) *.o 

gpl-clean:
	@echo "GPL-CLEAR" $(PWD)
	@rm -rf *.c *.h

mrproper:clean 

install:
	@install -D $(BIN) $(PROJ_INSTALL)/usr/local/bin/$(BIN)
