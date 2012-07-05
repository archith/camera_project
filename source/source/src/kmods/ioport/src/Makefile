##================================================================
##   -Makefile ---------------------------------------------------
##================================================================
CC		= arm-linux-gcc
LD		= arm-linux-ld
STRIP 		= arm-linux-strip

SRC	= ioport.c 
OBJ	= ioport.o
obj-m	+= $(OBJ)

LINUX_SRC	= $(PROJ_ROOT)/src/kernel/linux

CFLAGS := -D__KERNEL__ -Wall -Wstrict-prototypes -I$(LINUX_SRC)/include \
        -O2 -fomit-frame-pointer -fno-strict-aliasing -fno-common -pipe \
	-DMODULE -I$(PROJ_ROOT)/src/kmods/ioport/src

#CFLAGS	+= -DDEBUG
CFLAGS	+= -DEXPORT_SYMTAB
CFLAGS	+= -D_DEVFS_
#CFLAGS += -DIOPORT_ADDOUT


all:	modules install 

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

modules: $(OBJ)
#	ln -sf ../src/ioport.h ../inc/ioport.h
#	ln -sf ../src/ioport_driver.h ../inc/ioport_driver.h
#	ln -sf ../src/ioport_global.h ../inc/ioport_global.h
#	$(LD) -r $^ -o ioport.o 
#	make -C $(PROJ_ROOT)/src/kernel/linux SUBDIRS=${shell pwd} modules
#	$(STRIP) --strip-debug --remove-section=note --discard-all --discard-locals ioport.ko

install:
	install -D $(OBJ) $(PROJ_KMODULE)/$(OBJ)

clean: uninstall
	rm -rf *.o

gpl-clean:
	@echo "GPL-CLEAR" $(PWD)
	@rm -rf *.c *.h

uninstall:
	@rm -rf $(PROJ_KMODULE)/$(OBJ)
