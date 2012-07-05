SUBDIRS += checksum
SUBDIRS += FF
SUBDIRS += pid_generator
all install gpl-clean clean:
	@for i in `echo $(SUBDIRS)`; do \
		$(MAKE) -s --no-print-director -C $$i $@ || exit 1; \
	done
