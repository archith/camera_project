PWD	= ${shell pwd}
BASE    = ${shell basename $(PWD)}
INSTALL_DIR = $(PROJ_ROOT)/FW
CC	= gcc
CFLAGS 	= 
SRCS	= $(wildcard *.c)
OBJS	= $(SRCS:.c=.o)
BIN	= $(BASE)

%.o: %.c	
	$(CC) $(CFLAGS) -c $< -o $@

all: .depend $(BIN) install
	
$(BIN):	$(OBJS)
	$(CC) -o $@ $(OBJS)

clean gpl-clean:
	rm -f $(OBJS) $(BIN) .depend $(INSTALL_DIR)/$(BIN)

install: $(INSTALL_DIR)/$(BIN)

$(INSTALL_DIR)/$(BIN): $(BIN)
	install -D --mode=0755 $(BIN) $(INSTALL_DIR)/$(BIN)
	
uninstall:

.depend:
ifneq ($(SRCS),)
ifeq ($(wildcard .depend),)
	$(CC) -MM $(CFLAGS) $(SRCS) > .depend
endif
endif

ifneq ($(wildcard .depend),)
include .depend
endif

