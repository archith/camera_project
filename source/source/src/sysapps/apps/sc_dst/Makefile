#-------------------------------------------------------
#  Daylight Saving Time  Makefile
#-------------------------------------------------------

CC			= arm-linux-gcc
LD			= arm-linux-ld

STRIP = arm-linux-strip
STRIPCMD = $(STRIP) --remove-section=.note --remove-section=.comment

CFLAGS			=
LDFLAGS			= -s
WD			= ${shell pwd}
TOPDIR			= $(WD)/../..

CFLAGS			+= -I../cgi2/inc
LIBS += -L../cgi2/bin -lpond -lpthread

BIN_DIR =  

BIN1  = sc_dst_setcron
# BIN2  = sc_set_dst_flag
# BIN3  = sc_adjust_time

SRCS1			= sc_dst_setcron.c sc_localtime.c sc_adjust_time.c
OBJS1			= $(SRCS1:.c=.o) 

# SRCS2			= sc_set_dst_flag.c sc_localtime.c
# OBJS2			= $(SRCS2:.c=.o)

# SRCS3			= sc_adjust_time.c sc_localtime.c
# OBJS3			= $(SRCS3:.c=.o)	

#all: $(BIN1) $(BIN2) $(BIN3)
all: $(BIN1) install

$(BIN1) : $(OBJS1) 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(STRIPCMD) $@
	
#$(BIN2) : $(OBJS2)  
#	$(CC) $(LDFLAGS) -o $@ $^
#	$(STRIPCMD) $@
	
#$(BIN3) : $(OBJS3)  
#	$(CC) $(LDFLAGS) -o $@ $^
#	$(STRIPCMD) $@

clean: 
	rm -f $(BIN1)  $(OBJS1) # $(BIN2)  $(OBJS2) $(BIN3)  $(OBJS3) *~


	
install:
	@install -D $(BIN1) $(PROJ_INSTALL)/usr/local/bin/$(BIN1)
	cp  -rf zoneinfo $(PROJ_INSTALL)/usr/local/ 
	chmod 777 $(PROJ_INSTALL)/usr/local/zoneinfo 

gpl-clean:
	rm -rf *.c


