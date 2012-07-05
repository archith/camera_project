TOPDIR	= $(shell pwd)
VERSION=1.0.15

export TOPDIR VERSION

CONFIG_CACHE_ON=1
CONFIG_ARCH_ARM=1

CROSS	= arm-linux-
OFORMAT	= elf32-littlearm
CC	= $(CROSS)gcc
LD	= $(CROSS)ld
NM	= $(CROSS)nm
OBJCOPY	= $(CROSS)objcopy
OBJDUMP	= $(CROSS)objdump
STRIP	= $(CROSS)strip
export OFORMAT CC LD NM OBJCOPY OBJDUMP

CFLAGS	+= -I$(TOPDIR)/include -Wall -Werror -Os -fomit-frame-pointer -fstrict-aliasing -fno-common -fno-strength-reduce -msoft-float -mlittle-endian -Wstrict-prototypes -fno-builtin -fno-pic
CFLAGS  += -DCONFIG_CACHE_ON=$(CONFIG_CACHE_ON) -DCONFIG_ARCH_ARM=1 -march=armv4
LFLAGS	= -static --oformat $(OFORMAT)

# Can define the two MACROs at the same time
#CFLAGS += -D_LAN_DRV_IP100A_ 
CFLAGS += -D_LAN_DRV_DM9102_ #-Dzero_debug
ifeq	($(MODEL), WVC2300)
	CFLAGS += -D_WVC2300_
else
	CFLAGS += -D_PVC2300_
endif

export CFLAGS LFLAGS

SRCS	= $(wildcard *.S) $(wildcard *.c)
SUBDIRS	= libfloat drivers lib net shell arm sercomm httpdownload 
MODULES	= $(foreach DIR, $(SUBDIRS), $(DIR)/$(DIR).o)
OBJS	= diagnostic/march.o
INSTALLDIRS 	= sercomm 
INSTALLSUBDIRS	= libfloat lib arm drivers
INSTALLMODULES	= $(foreach DIR, $(INSTALLSUBDIRS), $(DIR)/$(DIR).o)
#INSTALLMODULES	+= drivers/dma.o drivers/led.o drivers/nor.o drivers/nor_ppi_amd.o drivers/serial.o #drivers/nor_ppi_intel.o drivers/nor_spi.o 

all:  dep include/version.h module norboot.bin install #norinstall norinstall.bin
	
.PHONY: force
diagnostic/march.o: force
	make -C diagnostic

#norinstall.bin: norinstall/norinstall.lds norinstall/install.o norboot.bin
#	@echo -e "\n== $@ =="
#	$(LD) -r -o norbootpkg.o -b binary norboot.bin
#	$(LD) $(LFLAGS) -T norinstall/norinstall.lds norinstall/install.o norbootpkg.o $(INSTALLMODULES) -o norinstall.o
#	rm norbootpkg.o
#	$(NM) norinstall.o | sort > System.map
#	@echo -e "\n== $@ =="
#	$(OBJCOPY) -O binary -j .text -j .data norinstall.o norinstall.bin
#	@echo "--- make dis $(basename $@).dis ---"; \
#	$(OBJDUMP) -x -h -S -l --show-raw-insn $(basename $@).o > $(basename $@).dis; \

norboot.bin:  norboot.lds bootsect.o init.o script.o http_page.o http_progress.o http_page2.o $(OBJS) $(MODULES)
#	@echo -e "\n== $@ =="
	$(LD) $(LFLAGS) -T norboot.lds bootsect.o init.o script.o $(OBJS) $(MODULES) -o norboot.o
	$(NM) norboot.o | sort > System.map
#	@echo -e "\n== $@ =="
	$(OBJCOPY) -O binary -j .boot norboot.o norboot.bin
	$(OBJCOPY) -O binary -j .boot norboot.o norboot_boot.bin
	$(OBJCOPY) -O binary -j .text -j .data norboot.o norboot_text.bin
	cat norboot_text.bin >> norboot.bin
#	@echo "--- make dis $(basename $@).dis ---"; \
	$(OBJDUMP) -x -h -S -l --show-raw-insn $(basename $@).o > $(basename $@).dis; \

script.o: default.sh
#	@echo -e "\n== $@ =="
	## $(OBJCOPY) -I binary -O $(OFORMAT) default.sh script.o
	$(LD) -r -o $@ -b binary $^

http_page.o: httpdownload/upg_fw.htm
	$(LD) -r -o $@ -b binary $^

http_page2.o: httpdownload/upg_fw2.htm
	$(LD) -r -o $@ -b binary $^

http_progress.o: httpdownload/upg_pg.htm
	$(LD) -r -o $@ -b binary $^

install: 
#	@echo -e "\n== $@ =="
	install -d $(PROJ_ROOT)/img
	cp -f norboot.bin $(PROJ_ROOT)/img
#	cp -f norinstall.bin $(PROJ_ROOT)/img
#	cp -f norinstall.bin $(PROJ_ROOT)/img/norinstall-$(VERSION).bin

include/version.h: ./Makefile
	@echo \#define VERSION \"$(VERSION)\" > .ver
	@echo \#define PROBOOT_COMPILE_TIME \"`date "+%Y/%m/%d"`\" >> .ver
	@mv -f .ver $@

dep: include/version.h
ifeq ($(wildcard .depend),)
	@(for DIR in . $(SUBDIRS); do \
		$(MAKE) -C $${DIR} depend; \
	done)
endif

module:
	@(for DIR in $(SUBDIRS); do \
		$(MAKE) -C $${DIR} || exit 1; \
	done)

norinstall:
	@(for DIR in $(INSTALLDIRS); do \
		$(MAKE) -C $${DIR}; \
	done)
gpl-clean:
	find . -name "*.c" | xargs rm -f
	find . -name ".depend" | xargs rm -f
	find . -name "*.S" | xargs rm -f

clean:
	@echo -e "\n== $@ =="
	find . -name \*.o -exec rm -f {} \;
	find . -name \*.out -exec rm -f {} \;
	@rm -f System.map norboot.bin norboot.dis norboot_boot.bin norboot_text.bin norinstall.bin norinstall.dis
	@rm -f include/version.h

distclean: clean
	@echo -e "\n== $@ =="
	find . -name .depend -exec rm -f {} \;
	rm -f System.map vmproboot proboot.bin norboot.bin norboot_text.bin norboot_data.bin pImage.bin norboot.dis norinstall.dis norinstall.bin

include $(TOPDIR)/Rules.make
