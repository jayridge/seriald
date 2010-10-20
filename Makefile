BSON ?= ../src/mongo-c-driver

CFLAGS = -DMONGO_HAVE_STDINT -I. -I$(BSON)/src -Wall -g -std=c99
LIBS = -L. -L$(BSON) -ljson -lbson -ltcl
AR = ar
AR_FLAGS = rc
RANLIB = ranlib

srldtest: test.c json.c
	$(CC) $(CFLAGS) -o $@ test.c json.c core.c $(LIBS)

all: srldtest

clean:
	rm -rf *.a *.o srldtest *.dSYM
