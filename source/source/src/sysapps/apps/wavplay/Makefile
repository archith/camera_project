CC	= arm-linux-gcc
CFLAGS	= -O2 -Wall -I$(PROJ_ROOT)/src/kernel/linux/include -I$(PROJ_ROOT)/include -L$(PROJ_ROOT)/lib 

BIN	= my_mixer
OBJ	= my_mixer.o

BIN2	= ac97preset
OBJ2	= ac97preset.o

BIN3	= vecho2
OBJ3	= vecho2.o input.o

all: $(BIN) $(BIN2) install

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(BIN2): $(OBJ2)
	$(CC) $(CFLAGS) $^ -o $@

$(BIN3): $(OBJ3)
	$(CC) $(CFLAGS) $^ -o $@

gpl-clean:
	rm *.c

clean:
	rm -f $(BIN) $(OBJ) $(BIN2) $(OBJ2) $(BIN3) $(OBJ3)

install:
	@install -D $(BIN) $(PROJ_INSTALL)/usr/local/bin/$(BIN)
	@install -D $(BIN2) $(PROJ_INSTALL)/usr/local/bin/$(BIN2)

uninstall:
	@rm -f $(PROJ_INSTALL)/usr/local/bin/$(BIN)
	@rm -f $(PROJ_INSTALL)/usr/local/bin/$(BIN2)
