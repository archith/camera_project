
#USETHREAD = y
CROSS_COMPILE=arm-linux-
CC=$(CROSS_COMPILE)gcc

ifeq ($(USETHREAD),y)
  THREADLIB = -lpthread
  BINARY = openssl_sample openssl_thread
else
  THREADLIB =
  BINARY = openssl_sample
endif

all: $(BINARY)

CPPFLAGS = -I$(PROJ_ROOT)/include
CFLAGS = -Wall -O2
LIBS = -L$(PROJ_ROOT)/lib -lcrypto -laes


openssl_sample: openssl_sample.o
	$(CC) -o $@ $< $(CPPFLAGS) $(CFLAGS) $(LIBS)
openssl_thread: openssl_thread.o
	$(CC) -o $@ $< $(CPPFLAGS) $(CFLAGS) $(LIBS) $(THREADLIB)
clean:
	rm -f *.o *~ $(BINARY)
