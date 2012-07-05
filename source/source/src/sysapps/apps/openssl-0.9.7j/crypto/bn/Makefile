#
# OpenSSL/crypto/bn/Makefile
#

DIR=	bn
TOP=	../..
CC=	cc
CPP=    $(CC) -E
INCLUDES= -I.. -I$(TOP) -I../../include
CFLAG=-g
INSTALL_PREFIX=
OPENSSLDIR=     /usr/local/ssl
INSTALLTOP=/usr/local/ssl
MAKEDEPPROG=	makedepend
MAKEDEPEND=	$(TOP)/util/domd $(TOP) -MD $(MAKEDEPPROG)
MAKEFILE=	Makefile
AR=		ar r

BN_ASM=		bn_asm.o
# or use
#BN_ASM=	bn86-elf.o

CFLAGS= $(INCLUDES) $(CFLAG)
ASFLAGS= $(INCLUDES) $(ASFLAG)

GENERAL=Makefile
TEST=bntest.c exptest.c
APPS=

LIB=$(TOP)/libcrypto.a
LIBSRC=	bn_add.c bn_div.c bn_exp.c bn_lib.c bn_ctx.c bn_mul.c bn_mod.c \
	bn_print.c bn_rand.c bn_shift.c bn_word.c bn_blind.c \
	bn_kron.c bn_sqrt.c bn_gcd.c bn_prime.c bn_err.c bn_sqr.c bn_asm.c \
	bn_recp.c bn_mont.c bn_mpi.c bn_exp2.c bn_x931p.c

LIBOBJ=	bn_add.o bn_div.o bn_exp.o bn_lib.o bn_ctx.o bn_mul.o bn_mod.o \
	bn_print.o bn_rand.o bn_shift.o bn_word.o bn_blind.o \
	bn_kron.o bn_sqrt.o bn_gcd.o bn_prime.o bn_err.o bn_sqr.o $(BN_ASM) \
	bn_recp.o bn_mont.o bn_mpi.o bn_exp2.o bn_x931p.o

SRC= $(LIBSRC)

EXHEADER= bn.h
HEADER=	bn_lcl.h bn_prime.h $(EXHEADER)

ALL=    $(GENERAL) $(SRC) $(HEADER)

top:
	(cd ../..; $(MAKE) DIRS=crypto SDIRS=$(DIR) sub_all)

all:	lib

bn_prime.h: bn_prime.pl
	$(PERL) bn_prime.pl >bn_prime.h

divtest: divtest.c ../../libcrypto.a
	cc -I../../include divtest.c -o divtest ../../libcrypto.a

bnbug: bnbug.c ../../libcrypto.a top
	cc -g -I../../include bnbug.c -o bnbug ../../libcrypto.a

lib:	$(LIBOBJ)
	$(AR) $(LIB) $(LIBOBJ)
	$(RANLIB) $(LIB) || echo Never mind.
	@touch lib

# elf
asm/bn86-elf.s:	asm/bn-586.pl ../perlasm/x86asm.pl
	(cd asm; $(PERL) bn-586.pl elf $(CFLAGS) > bn86-elf.s)

asm/co86-elf.s:	asm/co-586.pl ../perlasm/x86asm.pl
	(cd asm; $(PERL) co-586.pl elf $(CFLAGS) > co86-elf.s)

# a.out
asm/bn86-out.o: asm/bn86unix.cpp
	$(CPP) -DOUT asm/bn86unix.cpp | as -o asm/bn86-out.o

asm/co86-out.o: asm/co86unix.cpp
	$(CPP) -DOUT asm/co86unix.cpp | as -o asm/co86-out.o

# bsdi
asm/bn86bsdi.o: asm/bn86unix.cpp
	$(CPP) -DBSDI asm/bn86unix.cpp | sed 's/ :/:/' | as -o asm/bn86bsdi.o

asm/co86bsdi.o: asm/co86unix.cpp
	$(CPP) -DBSDI asm/co86unix.cpp | sed 's/ :/:/' | as -o asm/co86bsdi.o

asm/bn86unix.cpp: asm/bn-586.pl ../perlasm/x86asm.pl
	(cd asm; $(PERL) bn-586.pl cpp >bn86unix.cpp )

asm/co86unix.cpp: asm/co-586.pl ../perlasm/x86asm.pl
	(cd asm; $(PERL) co-586.pl cpp >co86unix.cpp )

asm/sparcv8.o: asm/sparcv8.S

asm/sparcv8plus.o: asm/sparcv8plus.S

# Old GNU assembler doesn't understand V9 instructions, so we
# hire /usr/ccs/bin/as to do the job. Note that option is called
# *-gcc27, but even gcc 2>=8 users may experience similar problem
# if they didn't bother to upgrade GNU assembler. Such users should
# not choose this option, but be adviced to *remove* GNU assembler
# or upgrade it.
asm/sparcv8plus-gcc27.o: asm/sparcv8plus.S
	$(CC) $(ASFLAGS) -E asm/sparcv8plus.S | \
		/usr/ccs/bin/as -xarch=v8plus - -o asm/sparcv8plus-gcc27.o


asm/ia64.o:	asm/ia64.S

# Some compiler drivers (most notably HP-UX and Intel C++) don't
# understand .S extension:-( I wish I could pipe output from cc -E,
# but it's too compiler driver/ABI dependent to cover with a single
# rule...	<appro@fy.chalmers.se>
asm/ia64-cpp.o:	asm/ia64.S
	$(CC) $(ASFLAGS) -E asm/ia64.S > /tmp/ia64.$$$$.s &&	\
	$(CC) $(ASFLAGS) -c -o asm/ia64-cpp.o /tmp/ia64.$$$$.s;	\
	rm -f /tmp/ia64.$$$$.s

asm/x86_64-gcc.o: asm/x86_64-gcc.c
	$(CC) $(ASFLAGS) -c -o $@ $<

asm/pa-risc2W.o: asm/pa-risc2W.s
	/usr/ccs/bin/as -o asm/pa-risc2W.o asm/pa-risc2W.s

asm/linux_ppc32.s: asm/ppc.pl;	$(PERL) $< $@
asm/linux_ppc64.s: asm/ppc.pl;	$(PERL) $< $@
asm/aix_ppc32.s: asm/ppc.pl;	$(PERL) asm/ppc.pl $@
asm/aix_ppc64.s: asm/ppc.pl;	$(PERL) asm/ppc.pl $@
asm/osx_ppc32.s: asm/ppc.pl;	$(PERL) $< $@

files:
	$(PERL) $(TOP)/util/files.pl Makefile >> $(TOP)/MINFO

links:
	@$(PERL) $(TOP)/util/mklink.pl ../../include/openssl $(EXHEADER)
	@$(PERL) $(TOP)/util/mklink.pl ../../test $(TEST)
	@$(PERL) $(TOP)/util/mklink.pl ../../apps $(APPS)

install:
	@headerlist="$(EXHEADER)"; for i in $$headerlist ; \
	do  \
	(cp $$i $(INSTALL_PREFIX)$(INSTALLTOP)/include/openssl/$$i; \
	chmod 644 $(INSTALL_PREFIX)$(INSTALLTOP)/include/openssl/$$i ); \
	done;

exptest:
	rm -f exptest
	gcc -I../../include -g2 -ggdb -o exptest exptest.c ../../libcrypto.a

div:
	rm -f a.out
	gcc -I.. -g div.c ../../libcrypto.a

tags:
	ctags $(SRC)

tests:

lint:
	lint -DLINT $(INCLUDES) $(SRC)>fluff

depend:
	$(MAKEDEPEND) -- $(CFLAG) $(INCLUDES) $(DEPFLAG) -- $(PROGS) $(LIBSRC)

dclean:
	$(PERL) -pe 'if (/^# DO NOT DELETE THIS LINE/) {print; exit(0);}' $(MAKEFILE) >Makefile.new
	mv -f Makefile.new $(MAKEFILE)

clean:
	rm -f asm/co86unix.cpp asm/bn86unix.cpp asm/*-elf.* *.o */*.o *.obj lib tags core .pure .nfs* *.old *.bak fluff bn_asm.s

# DO NOT DELETE THIS LINE -- make depend depends on it.
