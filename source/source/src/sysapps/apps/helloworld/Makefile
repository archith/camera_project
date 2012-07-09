CROSS = arm-linux-
CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip

CFLAGS =-DEMBED

LDFLAGS = -s

BIN =  helloworld

OBJS =  main.o

all: $(BIN) install

$(BIN): $(OBJS)
	@$(CC) $(LDFLAGS) -o $@ $^ 

gpl-clean clean: 
	rm -f $(BIN) *.elf $(OBJS)

install:
	@install -D $(BIN) $(PROJ_INSTALL)/usr/local/bin/$(BIN)

uninstall:
	@rm -f $(PROJ_INSTALL)/usr/local/bin/$(BIN)
