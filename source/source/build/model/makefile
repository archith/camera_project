SUBDIRS += model_conf
SUBDIRS += install_pool
SUBDIRS += cgi2
SUBDIRS += rc.sethost
all:
	$(MAKE) -C $(PROJ_ROOT)/src/bootloader/proboot/ distclean all || exit 1; 
	@for i in `echo $(SUBDIRS)`; do \
		$(MAKE) -C $(PROJ_ROOT)/src/sysapps/apps/$$i clean all install || exit 1; \
	done
