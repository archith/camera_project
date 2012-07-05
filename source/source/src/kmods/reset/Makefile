##================================================================
##   -Makefile ---------------------------------------------------
##================================================================
CC	:= arm-linux-gcc
LD	:= arm-linux-ld
STRIP   := arm-linux-strip

SRC	= reset.c
OBJ	= reset.o
obj-m	+= $(OBJ)

LINUX_SRC = $(PROJ_ROOT)/src/kernel/linux

CFLAGS := -D__KERNEL__ -I$(LINUX_SRC)/include  -Wall -Wstrict-prototypes -Wno-trigraphs -Os -fno-strict-aliasing -fno-common -Uarm -fno-common -pipe -mapcs-32 -D__LINUX_ARM_ARCH__=4 -march=armv4  -mshort-load-bytes -msoft-float -DMODULE  -nostdinc -iwithprefix include -DKBUILD_BASENAME=reset

all:modules install

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

modules: 
#	$(CC) -c $(CFLAGS) -o $(OBJ) $(SRC)

install: $(OBJ)
	install -D $(OBJ) $(PROJ_KMODULE)/$(OBJ)

clean: uninstall
	rm -rf *~
	rm -rf $(OBJ)

gpl-clean:
	rm -rf *.c  *.h

uninstall:
	rm -f $(PROJ_KMODULE)/$(OBJ)
