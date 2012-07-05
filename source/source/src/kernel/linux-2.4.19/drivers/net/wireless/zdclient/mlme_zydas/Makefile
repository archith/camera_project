#
# Makefile for the Ethernet Bonding driver
#

O_TARGET := mlme_zydas.o

obj-y	 := mlme.o \
            auth.o  \
            assoc.o \
	        fw_linux.o    \
	        hw_ctrl.o \
	        sync.o

obj-m	 := $(O_TARGET)

include $(TOPDIR)/Rules.make

#CFLAGS_assoc.o = -gdwarf-2
#CFLAGS_auth.o = -gdwarf-2
#CFLAGS_mlme.o = -gdwarf-2
#CFLAGS_sync.o = -gdwarf-2
