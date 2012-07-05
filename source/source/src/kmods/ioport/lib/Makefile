
CC		= arm-linux-gcc
AR		= arm-linux-ar
CFLAGS		= -Wall

MYFLAGS		+= -DDEBUG
#ADDFLAGS	+= -DIOPORT_ADDOUT
MYFLAGS		+= -DIOPORT_TEST $(ADDFLAGS)

CFLAGS		+= -I./ -I../inc/ -I../../../sysapps/apps/cgi/libcgicomm/inc/
LDFLAGS		+= -L./ -lPRO_file

SRC1		= ioapi.c
OBJ1		= ioapi.o

SRC2		= ioconfig.c
OBJ2		= ioconfig.o

SRC3		= iorun.c
OBJ3		= iorun.o

SRC4		= evtrun.c
OBJ4		= evtrun.o

SRCALL		= $(SRC1) $(SRC2) $(SRC3) $(SRC4)
OBJALL		= $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4)

LIBFILE		= libioport.a
BIN1		= iotest
BIN2		= ioconf
BIN3		= iorun
BIN4		= evtrun

BINALL		= $(BIN1) $(BIN2) $(BIN3) $(BIN4)


$(OBJ1): $(SRC1)
	@$(CC) -c $^ $(CFLAGS) $(ADDFLAGS) -o $@

$(BIN1): $(SRC1)
	$(CC) $^ $(CFLAGS) $(LDFLAGS) $(MYFLAGS) -o $(BIN1)

$(BIN2): $(SRC2) $(OBJ1)
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -DIOCONF_TEST -o $(BIN2)

$(BIN3): $(SRC3)
	$(CC) $^ -o $(BIN3) -lpthread

$(BIN4): $(SRC4) $(OBJ1)
	$(CC) $^ $(ADDFLAGS) $(CFLAGS) -o $(BIN4)

clean:
	rm -f $(OBJALL) $(LIBFILE) $(BINALL)

gpl-clean:
	rm -f $(SRCALL) $(OBJALL) *.h *.a
