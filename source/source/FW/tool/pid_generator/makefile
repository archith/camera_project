PWD	= ${shell pwd}
BASE    = ${shell basename $(PWD)}
INSTALL_DIR = $(PROJ_ROOT)/FW
CC	= gcc
CFLAGS 	= -I$(PROJ_CGI2)/../model_conf
SRCS	= $(wildcard *.c)
OBJS	= $(SRCS:.c=.o)
BIN	= $(BASE)

%.o: %.c	
	$(CC) $(CFLAGS) -c $< -o $@

all: .depend $(BIN) install
	
$(BIN):	$(OBJS)
	$(CC) -o $@ $(OBJS)

gpl-clean:
	rm -f $(SRCS) .depend 

clean:
	rm -f $(OBJS) $(BIN) .depend PID $(INSTALL_DIR)/PID

install: $(INSTALL_DIR)/PID

$(INSTALL_DIR)/PID:
	export PATH=$(PWD):$(PATH)
	./$(BIN)
	cp -f PID $(INSTALL_DIR)/PID
	
uninstall:

.depend:
ifneq ($(SRCS),)
ifeq ($(wildcard .depend),)
	$(CC) -M $(CFLAGS) $(SRCS) > .depend
endif
endif

ifneq ($(wildcard .depend),)
include .depend
endif

