SUBDIR += inc
SUBDIR += pond
SUBDIR += dhcp_pond
SUBDIR += app
all h install : bin
	@for i in $(SUBDIR); do \
		$(MAKE) -C $$i h || exit 1; \
	done
	@for i in $(SUBDIR); do \
		$(MAKE) -C $$i $@ || exit 1; \
	done

clean uninstall gpl-clean:
	@for i in $(SUBDIR); do \
		$(MAKE) -C $$i $@ || exit 1; \
	done
	rm -rf bin

bin:
	mkdir -p $@
