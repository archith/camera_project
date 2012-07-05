SUBDIRS += model_conf
SUBDIRS += ftp
SUBDIRS += install_pool
SUBDIRS += samba_3.06
SUBDIRS += cgi2
SUBDIRS += auth
SUBDIRS += hydra
SUBDIRS += openssl-0.9.7j
SUBDIRS += WPA_Supplicant
SUBDIRS += wireless_tools.26
SUBDIRS += LedAP
SUBDIRS += button_ap
SUBDIRS += dhcpcd-1.3.22-pl1
SUBDIRS += rc.sethost
SUBDIRS += Qset
SUBDIRS += cron
SUBDIRS += NTP
#SUBDIRS += ntpsetcron
SUBDIRS += dsp_dae
SUBDIRS += webd
SUBDIRS += ddns
SUBDIRS += ledoff
SUBDIRS += sc_dst
SUBDIRS += stunnel
SUBDIRS += iptables-1.3.4
SUBDIRS += ipfilter
SUBDIRS += io_init
SUBDIRS += ccdc_daemon
SUBDIRS += reboot
SUBDIRS += lighttpd-1.4.13
SUBDIRS += wavplay
SUBDIRS += simple
SUBDIRS += rc_start
#SUBDIRS += smart_grab
#SUBDIRS += send_file

ifeq ($(PROJ_INSTALL),)
all install:
	@echo "YOU SHALL NOT PASS!!!"
	@echo "Please set environment first."
	@exit
else
all install gpl-clean:
	@for i in `echo $(SUBDIRS)`; do \
		$(MAKE) -C $$i $@ || exit 1; \
	done
endif
clean mrproper:
	@for i in `echo $(SUBDIRS)`; do \
		$(MAKE) -C $$i $@; \
	done

