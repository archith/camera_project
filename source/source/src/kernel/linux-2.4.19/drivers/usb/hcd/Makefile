#
# Makefile for USB Host Controller Driver
# framework and drivers
#

O_TARGET	:=

#Bruce add below
export-objs :=			ehci-hcd.o ehci-hcd_FUSBH200.o ehci-hcd_FOTG2XX.o


obj-$(CONFIG_USB_EHCI_HCD)			+= ehci-hcd.o
obj-$(CONFIG_FARADAY_FUSBH200)			+= ehci-hcd_FUSBH200.o
obj-$(CONFIG_FARADAY_FOTG200)			+= ehci-hcd_FOTG2XX.o

# Extract lists of the multi-part drivers.
# The 'int-*' lists are the intermediate files used to build the multi's.
multi-y		:= $(filter $(list-multi), $(obj-y))
multi-m		:= $(filter $(list-multi), $(obj-m))
int-y		:= $(sort $(foreach m, $(multi-y), $($(basename $(m))-objs)))
int-m		:= $(sort $(foreach m, $(multi-m), $($(basename $(m))-objs)))

# Take multi-part drivers out of obj-y and put components in.
obj-y		:= $(filter-out $(list-multi), $(obj-y)) $(int-y)

# Translate to Rules.make lists.
OX_OBJS		:= $(obj-y)
MX_OBJS		:= $(obj-m)
MIX_OBJS	:= $(int-m)

include $(TOPDIR)/Rules.make
CFLAGS_ehci-hcd.o = -gdwarf-2

