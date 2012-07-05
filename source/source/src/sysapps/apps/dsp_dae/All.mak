# Default Configure

CC			= arm-linux-gcc
LD			= arm-linux-ld

STRIP = arm-linux-strip
STRIPCMD = $(STRIP) --remove-section=.note --remove-section=.comment

CFLAGS			=
LDFLAGS			= -s
WD			= ${shell pwd}
TOPDIR			= $(WD)/../..


TEXTOVLY_FUN = 1
NVSION_FUN   = 0	

OBJS1= CMOS_functions.o AUDIO_functions.o SYS_log.o PRO_file.o IMG_conf.o system_config.o rtc.o
MOT_OBJS = maf_mot.o  mot_conf.o mot_thread.o 


ifeq ($(TEXTOVLY_FUN),1)
	TEXTOVLY_FONT_OBJS = ./font/font_8x16.o
	TEXTOVLY_OBJS= textOvlyAPI.o fonts.o $(TEXTOVLY_FONT_OBJS)
	OBJS1+= $(TEXTOVLY_OBJS)
	CFLAGS += -DTXTOVLY_EN -DTXTOVLY_BlackStyle
endif
ifeq ($(NVSION_FUN),1)
	NVSION_OBJS = nvsion_conf.o nvsionAPI.o schedule.o IR.o
	OBJS1+= NVSION_OBJS
	CFLAGS += -DNVSION_EN
endif

CFLAGS += -DAUD
CFLAGS += -DDUMP_INFO
#CFLAGS += -DSAVE_AUDIO
#CFLAGS += -DSAVE_SNAPSHOOT

CFLAGS += -DWVC54G_MOT
#CFLAGS += -DMOT_DEBUG
CFLAGS += -I../cgi2/inc
LIBS += -L../cgi2/bin -lpond -lpthread



BIN_DIR =  $(PROJ_INSTALL)/usr/local/bin


all: dsp_dae install

%.o: %.c
	@$(CC) -c $(CFLAGS) $< -o $@

dsp_dae: dsp_dae.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	$(STRIPCMD) $@
	
install: all
	install -D --mode=0755 dsp_dae $(BIN_DIR)/dsp_dae

clean: uninstall
	@rm -f AUDIO_commands.h AUDIO_functions.c CMOS_commands.h CMOS_functions.c IMG_conf.h IMG_conf.c PRO_file.c SYS_log.c mot_conf.c mot_conf.h motconf_def.h
	rm -f $(OBJS1) $(TEXTOVLY_OBJS) $(MOT_OBJS) $(NVSION_OBJS) *.o *~ core dsp_srv dsp_cli dsp_dae video dsp_dump mjpeg.cgi *.elf dump
	rm -rf nvsion_conf.h nvsion_conf.c schedule.h schedule.c 

gpl-clean: 
	@echo "GPL-CLEAR" $(PWD)
	@rm -rf *.c *.h
uninstall:
	@rm -f $(BIN_DIR)/dsp_dae
