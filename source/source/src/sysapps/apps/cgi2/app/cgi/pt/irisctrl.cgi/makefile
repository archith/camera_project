# set GPL as "yes" if you want to create the GPL file.
GPL	=no

CROSS   =arm-linux-
CC      =$(CROSS)gcc
LD      =$(CROSS)ld
STRIP	=$(CROSS)strip

IRIS_MT_DIR = $(PROJ_INSTALL)/usr/local/www/pt

SRCS	= irisctrl.c
OBJS	= $(SRCS:.c=.o)

BIN	= irisctrl.cgi

all: $(BIN) install
ifeq    ($(GPL),yes)
	@echo "(GPL) clear the source."
	rm -rf *.c *.h
endif

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LIBS)

distclean clean: uninstall
ifeq    ($(GPL),no)
	rm -rf *.o $(BIN) 
else
	@echo "(GPL) Don't clear the object file."
endif


install: $(BIN)
	mkdir -p $(IRIS_MT_DIR)
	install -D --mode=0755 $(BIN) $(IRIS_MT_DIR)/$(BIN)

uninstall: 
	rm -rf $(IRIS_MT_DIR)/$(BIN)

h info:

gpl-clean:
	@echo "GPL-CLEAR" $(PWD)
	@rm -rf *.c

