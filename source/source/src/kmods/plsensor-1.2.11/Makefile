CC      := arm-linux-gcc
LD      := arm-linux-ld
STRIP   := arm-linux-strip


LINUX_DIR = /lib/modules/$(shell uname -r)/build
INSTALL   = /usr/bin/install

LINUX_DIR = $(PROJ_ROOT)/src/kernel/linux


RELEASE = $(shell uname -r)
MODULE_INC=-I$(LINUX_DIR)/drivers/usb -I$(LINUX_DIR)/include -include $(LINUX_DIR)/include/linux/config.h
MODULE_DEFS:=-DMODULE -D__KERNEL__ $(shell [ -f $(LINUX_DIR)/include/linux/modversions.h ] && echo -DEXPORT_SYMTAB -DMODVERSIONS -include $(LINUX_DIR)/include/linux/modversions.h )
MODULE_OPT = -O3
MODULE_OPT_WARN =  -Wall -Wstrict-prototypes -fomit-frame-pointer  -pipe
MODULE_CFLAGS = $(MODULE_INC) $(DEBUG) $(MODULE_OPT) $(MODULE_OPT_WARN) $(MODULE_DEFS)

all: plsensor-1.2.11.o install

clean: uninstall
	rm *.o -f

plsensor-1.2.11.o: sony098.o
	$(LD) -r -o plsensor-1.2.11.o sony098.o 

.c.o:
	$(CC) $(MODULE_CFLAGS) -c $<

install:
	install -D plsensor-1.2.11.o $(PROJ_KMODULE)/plsensor-1.2.11.o
	install -D plmedia/plmedia-1.2.11.1.o $(PROJ_KMODULE)/plmedia-1.2.11.1.o

uninstall:
	rm -rf $(PROJ_KMODULE)/plsensor-1.2.11.o

gpl-clean: 
	rm *.c

init:
	insmod videodev.o.gz
	insmod mod_test.o
#	lsmod
test:
	rmmod mod_test
	insmod mod_test.o
#	lsmod
