#
# Makefile for the PL1061-specific device drivers.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now inherited from the
# parent makes..
#

# The target object and module list name.

O_TARGET	:= prolific.o

# Objects that export symbols.

export-objs	:= hotkey.o

# Object file lists.

# obj-y	:= rtc.o
obj-m	:=
obj-n	:=

# obj-$(CONFIG_PL1061_PS)		+= ps.o
# obj-$(CONFIG_PL1061_HOTKEY)	+= hotkey.o
# obj-$(CONFIG_FALARM)		+= falarm.o
# obj-$(CONFIG_PL1061_UART)	+= uart_regs.o uart_utils.o pl_uart.o

# Each configuration option enables a list of files.
obj-$(CONFIG_PROLIFIC_PS)   		+= ps.o
obj-$(CONFIG_PROLIFIC_RTC)  		+= rtc.o
obj-$(CONFIG_PROLIFIC_KEYPAD) 		+= keymap.o keypad61.o
obj-$(CONFIG_PROLIFIC_GPIO) 		+= gpio.o
obj-$(CONFIG_PROLIFIC_IDE_HD)   	+= ide_hd.o
obj-$(CONFIG_PROLIFIC_UART) 		+= uart_regs.o uart_utils.o pl_uart.o
obj-$(CONFIG_FALARM)      		    += falarm.o
obj-$(CONFIG_PROLIFIC_IRDA) 		+= uart_regs.o uart_utils.o pl_irda.o
obj-$(CONFIG_PROLIFIC_I2C_KEYPAD) 	+= i2ckey61.o
obj-$(CONFIG_PROLIFIC_REMOTE_CONTROLLER)  += cir61.o circode61.o

# gary-lin
obj-$(CONFIG_PL1029_NOR) += pl1029_nor.o
obj-$(CONFIG_PL1029_NOR) += pl1029_nor_ppi_amd.o
obj-$(CONFIG_PL1029_NOR_UPGRADE) += pl1029_nor_upgrade.o

# The global Rules.make.

ifndef CONFIG_VT
    obj-$(CONFIG_PROLIFIC_HID) += keyboard.o
endif


include $(TOPDIR)/Rules.make

