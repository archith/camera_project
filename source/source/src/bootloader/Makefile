SUBDIRS := proboot

all:
	echo "Build Boot Loader..."
	@for i in `echo $(SUBDIRS)`; do \
		$(MAKE) -C $$i $@; \
	done

install clean mrproper:
	@for i in `echo $(SUBDIRS)`; do \
		$(MAKE) -C $$i $@; \
	done

