##================================================================
##   -Makefile ---------------------------------------------------
##================================================================
CC	:= arm-linux-gcc
LD	:= arm-linux-ld
STRIP	:= arm-linux-strip

SRC	= dmfe.c
OBJ	= dmfe.o
obj-m	+= $(OBJ)

LINUX_SRC = $(PROJ_ROOT)/src/kernel/linux

CFLAGS := -D__KERNEL__ -Wall -Wstrict-prototypes -I$(LINUX_SRC)/include \
        -O2 -fomit-frame-pointer -fno-strict-aliasing -fno-common -pipe -DMODULE 
CFLAGS += -D_LED_ -D_DM9102_PROC_
all: modules install

modules: $(OBJ)
#	$(STRIP) --strip-debug --discard-all --discard-locals $(OBJ)
	$(CC) -c $(CFLAGS) -o $(OBJ) $(SRC)

install:
	install $(OBJ) $(PROJ_KMODULE)

clean: uninstall
	rm -f *~
	rm -rf $(OBJ)

gpl-clean: clean

uninstall:
	@rm -f $(PROJ_KMODULE)/$(OBJ)
