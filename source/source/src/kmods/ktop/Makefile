##================================================================
##   -Makefile ---------------------------------------------------
##================================================================
CC	:= arm-linux-gcc
LD	:= arm-linux-ld
STRIP	:= arm-linux-strip

SRC	= ktop.c
OBJ	= ktop.o
obj-m	+= $(OBJ)

LINUX_SRC = $(PROJ_ROOT)/src/kernel/linux-2.4.19

CFLAGS := -D__KERNEL__ -I$(LINUX_SRC)/include -Wall -Wstrict-prototypes -Wno-trigraphs -Os -fno-strict-aliasing -fno-common -Uarm -fno-common -pipe -mapcs-32 -D__LINUX_ARM_ARCH__=4 -march=armv4  -mshort-load-bytes -msoft-float -DMODULE  -nostdinc -iwithprefix include -DKBUILD_BASENAME=ktop 

all: $(OBJ) install

$(OBJ): 
#	$(STRIP) --strip-debug --discard-all --discard-locals $(OBJ)
	$(CC) -c $(CFLAGS) -o $(OBJ) $(SRC)

install:
	install -D $(OBJ) $(PROJ_KMODULE)/$(OBJ)

clean: uninstall
	rm -f *~
	rm -rf $(OBJ)

gpl-clean:
	rm -rf $(SRC)
	
uninstall:
	rm -f $(PROJ_KMODULE)/$(OBJ)
