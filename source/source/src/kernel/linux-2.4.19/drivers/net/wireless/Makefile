#
# drivers/net/wireless/Makefile
#
# Makefile for the Linux Wireless network device drivers.
#

O_TARGET := wireless_net.o

obj-y		:=
obj-m		:=
obj-n		:=
obj-		:=

mod-subdirs     := zdap
subdir-$(CONFIG_ZYDAS_AP) += zdap

mod-subdirs     := zdclient
subdir-$(CONFIG_ZYDAS_CLIENT) += zdclient


# Things that need to export symbols
export-objs	:= airo.o orinoco.o hermes.o

obj-$(CONFIG_HERMES)		+= orinoco.o hermes.o
obj-$(CONFIG_PCMCIA_HERMES)	+= orinoco_cs.o
obj-$(CONFIG_APPLE_AIRPORT)	+= airport.o
obj-$(CONFIG_PLX_HERMES)	+= orinoco_plx.o
obj-$(CONFIG_PCI_HERMES)	+= orinoco_pci.o
obj-$(CONFIG_AIRO)		+= airo.o
obj-$(CONFIG_AIRO_CS)		+= airo_cs.o airo.o


ifeq ($(CONFIG_ZYDAS_CLIENT),y)
mod-subdirs     += zdclient
subdir-y        += zdclient
obj-y           += zdclient/zdclient.o
endif

ifeq ($(CONFIG_ZYDAS_AP),y)
mod-subdirs     += zdap
subdir-y        += zdap
obj-y           += zdap/zdap.o
endif

include $(TOPDIR)/Rules.make
