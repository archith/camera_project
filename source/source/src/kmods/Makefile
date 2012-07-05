SUBDIRS += ktop
SUBDIRS += DM9102
SUBDIRS += ioport
SUBDIRS += Led
SUBDIRS += Switch
SUBDIRS += RT61_Linux_STA_Drv1.1.2.1
#SUBDIRS += plsensor
SUBDIRS += plsensor-1.2.11
SUBDIRS += tp6830_lnx_951026
SUBDIRS += reset
SUBDIRS += watchdog
#SUBDIRS += tw9910

ifeq ($(PROJ_KMODULE),)
all install:
	@echo "YOU SHALL NOT PASS!!!"
	@echo "Please set environment first."
	@exit
else
all:
	echo "Build Kernel Modules..."
	@for i in `echo $(SUBDIRS)`; do \
		$(MAKE) -C $$i $@ || exit 1; \
	done

install:
	@for i in `echo $(SUBDIRS)`; do \
		$(MAKE) -C $$i $@ || exit 1; \
	done
endif
clean mrproper gpl-clean:
	@for i in `echo $(SUBDIRS)`; do \
		$(MAKE) -C $$i $@ || exit 1; \
	done

