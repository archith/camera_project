
.PHONY: clean headers_install install

SUBDIRS := src


all: $(SUBDIRS)

$(SUBDIRS)::
	@$(MAKE) --no-print-directory -C $@

headers_install install clean:
	@for i in `echo $(SUBDIRS)`; do \
		$(MAKE) --no-print-directory -C $$i $@; \
	done

gpl-clean:
	@for i in `echo $(SUBDIRS) lib`; do \
		$(MAKE) --no-print-directory -C $$i $@; \
	done
	@rm -f inc/*
