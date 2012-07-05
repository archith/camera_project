# set GPL as "yes" if you want to create the GPL file.
GPL	=no

CROSS   =arm-linux-
CC      =$(CROSS)gcc
LD      =$(CROSS)ld
STRIP	=$(CROSS)strip

AUDIO_OUT_DIR = $(PROJ_INSTALL)/usr/local/www/img

SRCS	= au_main.c au_misc.c au_debug.c au_signals.c g726.c g711.c shm.c
OBJS	= $(SRCS:.c=.o)

CFLAGS	+= -I../../../../inc
LIBS	= -L../../../../bin -lpthread -lpond

BIN	= audio_upload.cgi

all: $(BIN) install
ifeq    ($(GPL),yes)
	@echo "(GPL) clear the source."
	rm -rf *.c *.h
endif

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LIBS)

h info:

clean distclean: uninstall
ifeq    ($(GPL),no)
	rm -rf *.o $(BIN) $(BIN2) 
else
	@echo "(GPL) Don't clear the object file."
endif

gpl-clean:
	@echo "GPL-CLEAR" $(PWD)
	@rm -rf *.c

install: $(BIN)
	mkdir -p $(AUDIO_OUT_DIR)
	install -D --mode=0755 $(BIN) $(AUDIO_OUT_DIR)/$(BIN)
        
	cd $(AUDIO_OUT_DIR) ; \
		ln -sf $(BIN) g711a.cgi; \
		ln -sf $(BIN) g711u.cgi; \
		ln -sf $(BIN) g726.cgi; \
        cd -

uninstall: 
	rm -rf $(AUDIO_OUT_DIR)/$(BIN)
	rm -rf $(AUDIO_OUT_DIR)/g711a.cgi
	rm -rf $(AUDIO_OUT_DIR)/g711u.cgi
	rm -rf $(AUDIO_OUT_DIR)/g726.cgi
