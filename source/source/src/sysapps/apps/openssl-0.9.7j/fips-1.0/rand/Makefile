#
# OpenSSL/fips-1.0/rand/Makefile
#

DIR=	rand
TOP=	../..
CC=	cc
INCLUDES=
CFLAG=-g
INSTALL_PREFIX=
OPENSSLDIR=     /usr/local/ssl
INSTALLTOP=/usr/local/ssl
MAKEDEPPROG=	makedepend
MAKEDEPEND=	$(TOP)/util/domd $(TOP) -MD $(MAKEDEPPROG)
MAKEFILE=	Makefile
AR=		ar r

CFLAGS= $(INCLUDES) $(CFLAG)

GENERAL=Makefile
TEST= fips_randtest.c fips_rngvs.c
APPS=

LIB=$(TOP)/libcrypto.a
LIBSRC=fips_rand.c fips_rand_selftest.c
LIBOBJ=fips_rand.o fips_rand_selftest.o

SRC= $(LIBSRC)

EXHEADER= fips_rand.h
HEADER=	$(EXHEADER)

ALL=    $(GENERAL) $(SRC) $(HEADER)

top:
	(cd $(TOP); $(MAKE) DIRS=fips SDIRS=$(DIR) sub_all)

all:	lib

lib:	$(LIBOBJ)
	@echo $(LIBOBJ) > lib

files:
	$(PERL) $(TOP)/util/files.pl Makefile >> $(TOP)/MINFO

links:
	@$(PERL) $(TOP)/util/mklink.pl $(TOP)/include/openssl $(EXHEADER)
	@$(PERL) $(TOP)/util/mklink.pl $(TOP)/test $(TEST)
	@$(PERL) $(TOP)/util/mklink.pl $(TOP)/apps $(APPS)

install:
	@headerlist="$(EXHEADER)"; for i in $$headerlist; \
	do \
	  (cp $$i $(INSTALL_PREFIX)$(INSTALLTOP)/include/openssl/$$i; \
	  chmod 644 $(INSTALL_PREFIX)$(INSTALLTOP)/include/openssl/$$i ); \
	done

tags:
	ctags $(SRC)

tests:

Q=../testvectors/rng/req
A=../testvectors/rng/rsp

fips_test:
	-rm -rf $(A)
	mkdir $(A)
	if [ -f $(Q)/ANSI931_TDES2MCT.req ]; then $(TOP)/util/shlib_wrap.sh $(TOP)/test/fips_rngvs mct < $(Q)/ANSI931_TDES2MCT.req > $(A)/ANSI931_TDES2MCT.rsp; fi
	if [ -f $(Q)/ANSI931_TDES2VST.req ]; then $(TOP)/util/shlib_wrap.sh $(TOP)/test/fips_rngvs vst < $(Q)/ANSI931_TDES2VST.req > $(A)/ANSI931_TDES2VST.rsp; fi

lint:
	lint -DLINT $(INCLUDES) $(SRC)>fluff

depend:
	$(MAKEDEPEND) -- $(CFLAG) $(INCLUDES) $(DEPFLAG) -- $(SRC) $(TEST)

dclean:
	$(PERL) -pe 'if (/^# DO NOT DELETE THIS LINE/) {print; exit(0);}' $(MAKEFILE) >Makefile.new
	mv -f Makefile.new $(MAKEFILE)

clean:
	rm -f *.o *.obj lib tags core .pure .nfs* *.old *.bak fluff

# DO NOT DELETE THIS LINE -- make depend depends on it.
