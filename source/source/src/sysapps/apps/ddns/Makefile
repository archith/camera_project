MAKE=$(shell which make)

SUBDIR =
SUBDIR +=ddnsinit
SUBDIR +=ez-ipupdate-3.0.11b7

$(SUBDIR)::
	@for i in $(SUBDIR) ; do $(MAKE) -C $$i || exit 1; done

all: $(SUBDIR)

install:
	@for i in $(SUBDIR) ; do $(MAKE) -C $$i install || exit 1; done

clean:
	@for i in $(SUBDIR) ; do $(MAKE) -C $$i clean || exit 1; done

gpl-clean:
	@for i in $(SUBDIR) ; do $(MAKE) -C $$i gpl-clean || exit 1; done
