CC = gcc
CFLAGS = -W -Wall -O2
CPPFLAGS = -I.
LDLIBS = -lz
PROGS = mkcramfs cramfsck

all: $(PROGS)

install: all
	@[ -d ../../bin ] || mkdir ../../bin
	cp -f mkcramfs ../../bin
	cp -f cramfsck ../../bin

distclean clean:
	rm -f $(PROGS)

.PHONY: all clean
