######################################################################
#	Module Name: Makefile
#	Abstract: Makefile for rt61 linux driver on kernel series 2.4
######################################################################


CC := arm-linux-gcc
LD := arm-linux-ld

include ./config.mk

WFLAGS := -DAGGREGATION_SUPPORT -DWMM_SUPPORT -DWPA_SUPPLICANT_SUPPORT -Wall -Wstrict-prototypes -Wno-trigraphs
#CFLAGS := -D__KERNEL__ -I$(LINUX_SRC)/include -O2 -fomit-frame-pointer -fno-strict-aliasing -fno-common -pipe -mpreferred-stack-boundary=2 -march=i686 -DMODULE -DMODVERSIONS -include $(LINUX_SRC)/include/linux/modversions.h
CFLAGS := -D__KERNEL__ -I$(LINUX_SRC)/include -O2 -fomit-frame-pointer -fno-strict-aliasing -fno-common -pipe -march=armv4 -DMODULE -Wno-trigraphs

## Comment/uncomment the following line to enable/disable debugging
#ifeq ($(FCC_TEST_FW), yes)
CFLAGS += -DDBG
#endif

# Support WSC function
WLAN_HAS_WSC=n

ifeq ($(WLAN_HAS_WSC),yes)
WFLAGS += -DWSC_SUPPORT
endif

CFLAGS += $(WFLAGS) -D_LED_ -D_FLASHMAC_ -D_TIWLAN_PROC_
#CFLAGS += -I$(PROJ_ROOT)/src/sysapps/share/sc_config 
BIN = rt2661.bin rt2561.bin

OBJ = rt61.o

rt61-obj = rtmp_main.o rtmp_task.o mlme.o connect.o sync.o assoc.o auth.o auth_rsp.o rtmp_data.o rtmp_init.o sanity.o rtmp_wep.o rtmp_info.o eeprom.o rtmp_tkip.o wpa.o md5.o 
ifeq ($(WLAN_HAS_WSC),yes)
rt61-obj +=	wsc.o	\
	sha2.o	\
	hmac.o	\
	evp_enc.o	\
	dh_key.o
endif

rt61.o:   $(rt61-obj)
	$(LD) -r $^ -o $@


## NDDIR/ifcfg-ra0
ND_CONF=/etc/sysconfig/network-scripts/ifcfg-ra0

ifdef NDDIR
	NDDIR := $(NDDIR)
else
 	NDDIR := /etc/sysconfig/network-scripts
endif

ifdef MOD_CONF
	MOD_CONF := $(MOD_CONF)	
else
	MOD_CONF := /etc/modules.conf 
endif

ifdef MODDIR
 	MOD_ROOT := $(MODDIR)
else
 	MOD_ROOT := /lib/modules/$(shell uname -r)/extra
endif

all: $(OBJ) install
	@touch config.mk
	
clean: uninstall
	rm -rf *.o *~ core .*.cmd *.ko *.mod.c .tmp_versions built-in.o

gpl-clean: clean

config:
	@touch config.mk
	@./Configure

install:
	install $(OBJ) $(PROJ_KMODULE)
	install $(BIN) $(PROJ_ROOT)/src/rootfs/etc
#	install rt61sta.dat $(PROJ_ROOT)/src/rootfs/etc
	
uninstall:
	rm -rf $(PROJ_ROOT)/src/rootfs/etc/rt2561.bin
	rm -rf $(PROJ_ROOT)/src/rootfs/etc/rt2661.bin
#	rm -rf $(PROJ_ROOT)/src/rootfs/etc/rt61sta.dat
