
KINCLUDES=/home/prolific/pl1091/src/kernel/linux-2.4.x/include
DRVINCLUDES=/home/prolific/pl1091/src/kernel/linux-2.4.x/drivers

MODULE=		RdebugUI
CC=		/usr/local/arm/bin/arm-linux-gcc
CPPFLAGS=	-D__KERNEL__ -I$(KINCLUDES) -I$(DRVINCLUDES)
MODFLAGS=	-DMODULE
KERNFLAGS=      $(CPPFLAGS) $(CPUFLAGS) $(SMPFLAGS) \
		-Wall -Wstrict-prototypes -Wno-trigraphs -O2 -fomit-frame-pointer \
		-fno-strict-aliasing -fno-common -Wno-unused
CFLAGS=		$(KERNFLAGS) $(DBGCFLAGS) $(MODFLAGS)

OBJ = RdebugUI.o

all: RdebugUI

clean:
	rm -f *.o core RdebugUI

RdebugUI: $(OBJ)
	$(CC) $(CFLAGS) -o RdebugUI $(OBJ)
