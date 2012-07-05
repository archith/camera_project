#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).

USE_STANDARD_AS_RULE := true

O_TARGET		:= pl1029.o

# Object file lists.

#obj-y			:= arch.o irq.o dma.o serial.o addr.o
obj-y			:= arch.o irq.o dma.o pl_ser.o addr.o gio.o misc.o
obj-$(CONFIG_PCI)       += pl_pci.o
obj-$(CONFIG_PROC_HARDWARE) += pl_proc.o
obj-m			:=
obj-n			:=
obj-			:=
# obj-$(CONFIG_CACHETS)	+= utils/cachets/cachets.o
# obj-$(CONFIG_Rdebug)	+= utils/Rdebug/Rdebug.o

# export-objs		:= serial.o
export-objs		:= dma.o

include $(TOPDIR)/Rules.make
