all:
	cp -f $(MODEL)_$(PROJ_DOMAIN)/model_conf.h $(PROJ_ROOT)/src/sysapps/apps/model_conf/
	cp -f $(MODEL)_$(PROJ_DOMAIN)/def_sys.conf $(PROJ_ROOT)/src/rootfs/etc
	ln -sf ${shell pwd}/$(MODEL)_$(PROJ_DOMAIN)/html $(PROJ_ROOT)/src/sysapps/apps/install_pool/html/
	cp -fb $(MODEL)_$(PROJ_DOMAIN)/web_msg $(PROJ_ROOT)/src/sysapps/apps/install_pool/web_msg
clean:
	rm -f $(PROJ_ROOT)/src/sysapps/apps/model_conf/model_conf.h
	rm -f $(PROJ_ROOT)/src/rootfs/etc/def_sys.conf
	rm -rf $(PROJ_ROOT)/src/sysapps/apps/install_pool/html
	rm -f $(PROJ_ROOT)/src/sysapps/apps/install_pool/web_msg/web_msg