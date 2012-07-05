CROSS = arm-linux-
CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip
STRIPCMD = $(STRIP) --remove-section=.note --remove-section=.comment


#CFLAGS  =   -Wall -march=armv4 -O2 -I./include -I$(PROJ_ROOT)/src/kernel/linux/include  -I./ 
CFLAGS  =   -Wall -Werrorr -march=armv4 -O2 -I$(PROJ_ROOT)/src/kernel/linux/include  -I./ -I../include  -I../rtp/  -I../rtsp/include 


CFLAGS  += -I$(PROJ_ROOT)/src/sysapps/apps/cgi2/inc
LDFLAGS = -L$(PROJ_ROOT)/src/sysapps/apps/cgi2/bin -lpond 
CFLAGS += -I$(PROJ_ROOT)/src/kernel/linux/include

CFLAGS  += -I../jpeg/
CFLAGS  += -D_DAEMON_
CFLAGS  += -D_MP4V_

CFLAGS			+= -D_SW_ENCODE_JPEG_

#CFLAGS			+= -D_DEBUG_
CFLAGS			+= -D_SNAPSHOT_

# Encode source form test raw image data(YUV400/YUV422)
#CFLAGS                  += -D_TEST_READ_FILE_YUV_

#CFLAGS			+= -D_YUV400_  
CFLAGS			+= -D_YUV422_
## NOT SUPPORT YET ##
#CFLAGS			+= -D_YUV420_ 

CFLAGS			+= -D_SCALE_FACTOR_FROM_SHMEM_
#CFLAGS			+= -D_SCALE_128X96_
#CFLAGS			+= -D_SCALE_64X48_

CFLAGS			+= -D_SCALE_EN_ 
#CFLAGS			+= -D_OUTPUT_SCALED_YUV_

#CFLAGS			+= -D_ALGORITHM_AVERAGE_
CFLAGS			+= -D_ALGORITHM_SKIP_

LDFLAGS			= -s


BIN			= sw_jpegen

SRCS			= main.c encoder.c dct.c huffman.c marker.c quant.c readYUV.c img_scaling400_avg.c img_scaling420_avg.c img_scaling422_avg.c  img_gen_yuv.c img_scaling400_skip.c img_scaling422_skip.c
OBJS			= $(SRCS:.c=.o)

STRIPCMD		= $(STRIP) --remove-section=.note --remove-section=.comment

all: $(BIN) install

$(OBJS): %.o: %.c
	@$(CC) -c $(CFLAGS) $< -o $@

$(BIN): $(OBJS)
	@$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@$(STRIPCMD) $@

clean: 
	@rm -f $(BIN) *.o *.jpg 

mrproper:clean 

install:
	@install -D $(BIN) $(PROJ_INSTALL)/usr/local/bin/$(BIN)
