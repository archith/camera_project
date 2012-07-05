#
# linux/drivers/nand/Makefile
#
# $Id: Makefile,v 1.1.1.1 2006/03/13 10:29:36 jedy Exp $

O_TARGET	:= nandlink.o

export-objs	:= nand.o nand_ecc.o

nandobjs-y			:= nand.o
nandobjs-$(CONFIG_MTD_NAND_ECC) += nand_ecc.o

obj-$(CONFIG_MTD_NAND)		+= $(nandobjs-y)
obj-$(CONFIG_MTD_NAND_SPIA)	+= spia.o

include $(TOPDIR)/Rules.make
