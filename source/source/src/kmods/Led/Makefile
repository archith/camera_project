##================================================================
##   -Makefile ---------------------------------------------------
##================================================================
CC	:= arm-linux-gcc
LD	:= arm-linux-ld
STRIP   := arm-linux-strip

SRC	= leds.c
OBJ	= leds.o
obj-m	+= $(OBJ)

LINUX_SRC = $(PROJ_ROOT)/src/kernel/linux

CFLAGS := -D__KERNEL__ -Wall -Wstrict-prototypes -I$(LINUX_SRC)/include \
        -O2 -fomit-frame-pointer -fno-strict-aliasing -fno-common -pipe -DMODULE 
#CFLAGS += -D_EVM_BOARD_ -D_DEVFS_
CFLAGS += -D_DEVFS_
all:modules install

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

modules: $(OBJ)
#	$(CC) -c $(CFLAGS) -o $(OBJ) $(SRC)

install:
	install -D $(OBJ) $(PROJ_KMODULE)/$(OBJ)

clean: uninstall
	rm -rf *~
	rm -rf $(OBJ)

gpl-clean:
	rm -rf *.c  *.h

uninstall:
	rm -f $(PROJ_KMODULE)/$(OBJ)
