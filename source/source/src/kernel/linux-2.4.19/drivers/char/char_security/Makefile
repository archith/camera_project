#
# drivers/net/security/Makefile
#
# Makefile for the Linux Wireless network device drivers.
#

O_TARGET := es.o

obj-y		:=
obj-m		:=
obj-n		:=
obj-		:=

# Things that need to export symbols

obj-$(CONFIG_SECURITY)+= security.o 

include $(TOPDIR)/Rules.make

clean:
	rm -f core *.o *.a *.s
	
CFLAGS_security.o = -gdwarf-2

