# Makefile for CCITT subroutines and sample programs

# Modify the CC definition to invoke your ANSI-C compiler
#
CC	=  gcc
CC      := arm-linux-gcc
LD      := arm-linux-ld
STRIP   := arm-linux-strip

CFLAGS	+= -I../../../kmods/tp6830_lnx_951026/
CFLAGS	+= -I../cgi2/inc


# Rules to build sample programs do not depend on a UNIX library program
#

ALL = ccdc ccdc_daemon

all: $(ALL) install

OBJECTS = ccdc.o ./shm/shm.o
BIN	= ccdc

OBJECTS2= ccdc_daemon.o topro.o ./shm/shm.o
BIN2	= ccdc_daemon


$(BIN): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) 

$(BIN2):$(OBJECTS2)
	$(CC) -o $@ $(OBJECTS2) -L../cgi2/bin -lpond -lpthread

clean: uninstall
	rm -f $(OBJECTS) $(OBJECTS2) $(BIN) $(BIN2)

gpl-clean:
	rm -rf $(BIN) $(BIN2) ccdc.c ./shm/shm.c ccdc_daemon.c topro.c *.h

install:
	install -D $(BIN) $(PROJ_INSTALL)/usr/local/bin/$(BIN)
	install -D $(BIN2) $(PROJ_INSTALL)/usr/local/bin/$(BIN2)

uninstall:
	rm -rf $(PROJ_INSTALL)/usr/local/bin/$(BIN)
	rm -rf $(PROJ_INSTALL)/usr/local/bin/$(BIN2)

