CROSS = arm-linux-
CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip
STRIPCMD = $(STRIP) --remove-section=.note --remove-section=.comment

CFLAGS			+= -D_IPV4_
LDFLAGS			= -s

BIN			= sap

SRCS			= $(BIN).c
OBJS			= $(SRCS:.c=.o)

all: $(BIN) install

$(OBJS): %.o: %.c
	@$(CC) -c $(CFLAGS) $< -o $@

$(BIN): $(OBJS) 
	@$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@$(STRIPCMD) $@

clean: 
	@rm -f $(BIN) *.o 

mrproper:clean 

install:
	@install -D $(BIN) $(PROJ_INSTALL)/usr/local/bin/$(BIN)
