#
# @(#)Makefile 1.1 86/09/24 SMI
#
INS=install
INCS=-I../../usr.bin/cxref
DEFS=-DCXREF -DFLEXNAMES -DS5EMUL
LINTF = -p -I../../usr.bin/cxref
LINT = lint
FFLAG =
CFLAGS = -O $(DEFS) $(INCS)

all :	cxref

# CXREF

cxref :	cxr.o
	$(CC) $(FFLAG) $(LDFLAGS) cxr.o -o cxref

cxr.o:	../../usr.bin/cxref/cxr.c ../../usr.bin/cxref/owner.h
	$(CC) -c $(CFLAGS) ../../usr.bin/cxref/cxr.c

# UTILITIES

install :	all
	$(INS) -s cxref $(DESTDIR)/usr/5bin

clean:
	-rm -f *.o
	-rm -f cxref

lint:
	$(LINT) $(LINTF) cxr.c
