CROSS   =arm-linux-
CC      =$(CROSS)gcc
LD      =$(CROSS)ld
STRIP	=$(CROSS)strip

AUDIO_OUT_DIR	= $(PROJ_INSTALL)/usr/local/www/camera_api
ADMIN_OUT_DIR	= $(PROJ_INSTALL)/usr/local/www/adm

#SUB_NAME	= .cgi
SUB_NAME	= 

CFLAGS		+= -I../../../../inc
CFLAGS		+= -D_RUN_AS_CGI_

SRC		=  ra_debug.c g711.c ra_dsp.c ra_response.c ra_create.c ra_destroy.c ra_start.c ra_stop.c ra_func.c ra_main.c 
OBJ		=  ra_debug.o g711.o ra_dsp.o ra_response.o ra_create.o ra_destroy.o ra_start.o ra_stop.o ra_func.o ra_main.o
BIN		=  rau_srv

SRC2		=  ra_client.c 
OBJ2		=  ra_client.o
BIN2		=  rau_cli

SRC3		=  audioctrl.c
OBJ3		=  audioctrl.o
BIN3		=  audioctrl.cgi

all: $(BIN) $(BIN3) install

$(BIN): $(OBJ)
	$(CC) -o $(BIN) $(OBJ) -L../../../../bin -lpthread -lpond 

$(BIN2): $(OBJ2)
	$(CC) -o $(BIN2) $(OBJ2)

$(BIN3): $(OBJ3)
	$(CC) -o $(BIN3) $(OBJ3) -L../../../../bin -lpthread -lpond 

install:
	mkdir -p $(AUDIO_OUT_DIR)
	install -D --mode=0755 $(BIN) $(AUDIO_OUT_DIR)/CreateAudioRTP$(SUB_NAME)
	install -D --mode=0755 $(BIN) $(AUDIO_OUT_DIR)/StartAudioRTP$(SUB_NAME)
	install -D --mode=0755 $(BIN) $(AUDIO_OUT_DIR)/StopAudioRTP$(SUB_NAME)
	install -D --mode=0755 $(BIN) $(AUDIO_OUT_DIR)/DestroyAudioRTP$(SUB_NAME)
	mkdir -p $(ADMIN_OUT_DIR)
	install -D --mode=0755 $(BIN3) $(ADMIN_OUT_DIR)/$(BIN3)

distclean clean:
	rm -rf $(OBJ) $(OBJ2) $(OBJ3) $(BIN) $(BIN2) $(BIN3)

h info:

gpl-clean:
	@echo "GPL-CLEAR" $(PWD)
	@rm -rf *.c

