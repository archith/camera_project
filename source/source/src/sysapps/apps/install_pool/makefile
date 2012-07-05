SUBDIR += html web_msg ocx
all clean h info distclean install uninstall gpl-clean:
	@for i in $(SUBDIR); do \
		$(MAKE) -C $$i $@  || exit 1 ; \
	done
	