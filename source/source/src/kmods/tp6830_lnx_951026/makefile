CC      := arm-linux-gcc
LD      := arm-linux-ld
STRIP   := arm-linux-strip

LINUX_DIR = $(PROJ_ROOT)/src/kernel/linux
INSTALL   = /usr/bin/install

RELEASE = $(shell uname -r)
MODULE_INC=-I$(LINUX_DIR)/drivers/usb -I$(LINUX_DIR)/include -include $(LINUX_DIR)/include/linux/config.h
MODULE_DEFS:=-DMODULE -D__KERNEL__ $(shell [ -f $(LINUX_DIR)/include/linux/modversions.h ] && echo -DEXPORT_SYMTAB -DMODVERSIONS -include $(LINUX_DIR)/include/linux/modversions.h )
MODULE_OPT = -O3
MODULE_OPT_WARN =  -Wall -Wstrict-prototypes -fomit-frame-pointer  -pipe
MODULE_CFLAGS = $(MODULE_INC) $(DEBUG) $(MODULE_OPT) $(MODULE_OPT_WARN) $(MODULE_DEFS)

all: topro.o install

clean:
	rm *.o -f

gpl-clean: clean


topro.o: pwc-ctrl.o pwc-if.o pwc-misc.o pwc-uncompress.o
	$(LD) -r -o topro.o pwc-ctrl.o pwc-if.o pwc-misc.o pwc-uncompress.o
.c.o:
	$(CC) $(MODULE_CFLAGS) -c $<

install: 
	install -D topro.o $(PROJ_KMODULE)/topro.o

init:
	insmod videodev.o.gz
	insmod mod_test.o
#	lsmod
test:
	rmmod mod_test
	insmod mod_test.o
#	lsmod
