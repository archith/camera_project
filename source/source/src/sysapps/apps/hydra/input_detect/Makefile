include ../Defs.mak

STRIPCMD	= $(STRIP) --remove-section=.comment
INSTALL_DIR	= $(PROJ_ROOT)/img/fs/shadow_root/usr/local/bin

LDFLAGS		= -s
LIBS		+= -L$(APPS_DIR)/cgi2/bin -lpond -lpthread
LDFLAGS		+= $(LIBS)

BIN		= input_detect
SRC		= $(BIN).c
OBJ		= $(SRC:.c=.o)


all: $(BIN) install

%.o: %.c
	@$(CC) -c $(CFLAGS) $< -o $@

$(BIN): $(OBJ)
	@$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@$(STRIPCMD) $@

clean: uninstall
	@rm -f $(BIN) *.o 

uninstall: 
	@rm -f $(INSTALL_DIR)/$(BIN);

install:
	@install -D $(BIN) $(INSTALL_DIR)/$(BIN)

gpl-clean:
	@rm -f $(SRC) $(BIN)

