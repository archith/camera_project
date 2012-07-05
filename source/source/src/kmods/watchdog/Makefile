##================================================================
##   -Makefile ---------------------------------------------------
##================================================================
CC	:= arm-linux-gcc
LD	:= arm-linux-ld
STRIP   := arm-linux-strip

SRC	= watchdog.c
OBJ	= watchdog.o
obj-m	+= $(OBJ)

LINUX_SRC = $(PROJ_ROOT)/src/kernel/linux

CFLAGS := -D__KERNEL__ -Wall -Wstrict-prototypes -I$(LINUX_SRC)/include \
        -O2 -fomit-frame-pointer -fno-strict-aliasing -fno-common -pipe -DMODULE 

all: $(OBJ) install

$(OBJ): 
	$(CC) -c $(CFLAGS) -o $(OBJ) $(SRC)

install:
	install -D $(OBJ) $(PROJ_KMODULE)/$(OBJ)

clean: uninstall
	rm -rf *~
	rm -rf $(OBJ)

gpl-clean:
	rm -rf $(SRC)

uninstall:
	rm -f $(PROJ_KMODULE)/$(OBJ)
