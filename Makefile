BSON ?= ../src/mongo-c-driver

CFLAGS = -DMONGO_HAVE_STDINT -I. -I$(BSON)/src -Wall -g -std=c99
LIBS = -L. -L$(BSON) -ljson -lbson -ltcl -ltokyocabinet
AR = ar
AR_FLAGS = rc
RANLIB = ranlib

libsrld.a: database.o iterator.o
	/bin/rm -f $@
	$(AR) $(AR_FLAGS) $@ $^
	$(RANLIB) $@

srldtest: test.o json.o core.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

all: libsrld.a srldtest

clean:
	rm -rf *.a *.o srldtest *.dSYM
