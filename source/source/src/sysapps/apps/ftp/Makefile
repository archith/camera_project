CROSS = arm-linux-
CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip

CFLAGS =-DEMBED

LDFLAGS = -s

BIN =  ftp

OBJS =  cmds.o cmdtab.o domacro.o ftp.o glob.o main.o ruserpass.o

all: $(BIN) install

$(BIN): $(OBJS)
	@$(CC) $(LDFLAGS) -o $@ $^ 

gpl-clean clean: 
	rm -f $(BIN) *.elf $(OBJS)

install:
	@install -D $(BIN) $(PROJ_INSTALL)/usr/local/bin/$(BIN)

uninstall:
	@rm -f $(PROJ_INSTALL)/usr/local/bin/$(BIN)
