

KINCLUDES=/home/prolific/pl1091/src/kernel/linux-2.4.x/include
DRVINCLUDES=/home/prolific/pl1091/src/kernel/linux-2.4.x/drivers

MODULE=		cachetsUI

CC=		/usr/local/arm/bin/arm-linux-gcc

CPPFLAGS=	-D__KERNEL__ -I$(KINCLUDES) -I$(DRVINCLUDES)
MODFLAGS=	-DMODULE
KERNFLAGS=      $(CPPFLAGS) $(CPUFLAGS) $(SMPFLAGS) \
		-Wall -Wstrict-prototypes -Wno-trigraphs -O2 -fomit-frame-pointer \
		-fno-strict-aliasing -fno-common -Wno-unused

CFLAGS=		$(KERNFLAGS) $(DBGCFLAGS) $(MODFLAGS)

OBJ = cachetsUI.o

all: cachetsUI

clean:
	rm -f *.o core cachetsUI

cachetsUI: $(OBJ)
	$(CC) $(CFLAGS) -o cachetsUI $(OBJ)
