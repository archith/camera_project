SUBDIRS += tool
all install:
	@for i in `echo $(SUBDIRS)`; do \
		$(MAKE) -s --no-print-director -C $$i $@ || exit 1; \
	done
	@ln -sf $(PROJ_ROOT)/img/norboot.bin        $(PROJ_ROOT)/FW/loader
	@ln -sf $(PROJ_ROOT)/img/vmlinuz            $(PROJ_ROOT)/FW/kernel
	@ln -sf $(PROJ_ROOT)/img/fs.img             $(PROJ_ROOT)/FW/filesystem 
	@./FW.sh

gpl-clean clean uninstall:
	@for i in `echo $(SUBDIRS)`; do \
		$(MAKE) -s --no-print-director -C $$i $@ || exit 1; \
	done
	@rm -f loader kernel filesystem $(MODEL).bin
	
