#
# drivers/net/zdclient/Makefile
#
# Makefile for the Linux Wireless network device drivers.
#

O_TARGET := zdclient.o

obj-y		:=
obj-m		:=
obj-n		:=
obj-		:=

# Things that need to export symbols
# export-objs	:= airo.o orinoco.o hermes.o

obj-$(CONFIG_ZYDAS_CLIENT)+= zd1205.o zdasocsvc.o zdauthreq.o zdauthrsp.o \
			zdmmrx.o zdshared.o zdhci.o zdglobal.o zdencrypt.o \
			zdpmfilter.o zdpsmon.o zdsynch.o zdbuf.o zd1205_proc.o \
			zdhw.o zddebug.o zdtkipseed.o zdmic.o mlme_zydas/mlme_zydas.o

subdir-$(CONFIG_ZYDAS_CLIENT) += mlme_zydas

include $(TOPDIR)/Rules.make  

#CFLAGS_zd1205.o += -gdwarf-2
#CFLAGS_zd1205_proc.o += -gdwarf-2
#CFLAGS_zdasocsvc.o += -gdwarf-2
#CFLAGS_zdbuf.o += -gdwarf-2
#CFLAGS_zdglobal.o += -gdwarf-2
#CFLAGS_zdhw.o += -gdwarf-2
#CFLAGS_zdhci.o += -gdwarf-2
#CFLAGS_zdpmfilter.o += -gdwarf-2
#CFLAGS_zdutils.o += -gdwarf-2

