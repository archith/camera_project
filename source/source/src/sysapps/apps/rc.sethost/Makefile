CROSS = arm-linux-
CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip
STRIPCMD = $(STRIP) --remove-section=.note --remove-section=.comment
CFLAGS			+= -I../cgi2/inc
CFLAGS			+= -I../
#CFLAGS  		+= -D_AR2414_

LDFLAGS			= -s

BIN1			= rc.sethost
SRC1			= sethost.c
OBJ1			= sethost.o

OBJS			= $(SRCS:.c=.o)
LIBS 			+= -L../cgi2/bin -lpond -pthread

ifeq  ($(MODEL),PVC2300)
CFLAGS	+= -D_NO_WIRELESS_
endif

all: $(BIN1) install

$(OBJS): %.o: %.c
	@$(CC) -c $(CFLAGS) $< -o $@

$(BIN1): $(OBJ1)
	@$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@$(STRIPCMD) $@

clean: 
	@rm -f $(BIN1) $(OBJ1)

gpl-clean:
	@echo "GPL-CLEAR" $(PWD)
	@rm -f *.c *.h

mrproper:clean 

install:
	@install -D $(BIN1) $(PROJ_INSTALL)/etc/$(BIN1)




