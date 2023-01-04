#
#	makefile for sun 1 color board running on Sun 2 multibus CPU
#
#	@(#)Makefile 1.1 86/09/25 SMI
#
LIB=	../../lib
INCLUDE=../../include
CFLAGS=	-O -I$(INCLUDE)
OFILES= cdiag.o colorbuf.o checker.o testfunc.o testmem.o colorvect.o \
	autotest.o find_bds.o manual.o tcmap.o testcmap.o tfbuf.o cramp.o

all:	color.diag

color.diag:	$(LIB)/stack.o $(OFILES) $(LIB)/libdiag.a
		ld -T 4000 -o color.diag $(LIB)/stack.o $(OFILES) \
		$(LIB)/libdiag.a -lm -lc

clean:
	rm -f color.diag *.o errs

install:
	install -s color.diag ${DESTDIR}/usr/stand

colorbuf.o: colorbuf.h m68000.h vectors.h reentrant.h
checker.o: colorbuf.h
colorvect.o: colorbuf.h
autotest.o: cdiag.h colorbuf.h m68000.h vectors.h reentrant.h
		cc -I$(INCLUDE) -c autotest.c
testfunc.o: cdiag.h colorbuf.h
testmem.o: cdiag.h colorbuf.h
find_bds.o: cdiag.h colorbuf.h
manual.o: cdiag.h colorbuf.h
		cc -I$(INCLUDE) -c manual.c
tcmap.o: cdiag.h colorbuf.h
testcmap.o: cdiag.h colorbuf.h
tfbuf.o: cdiag.h colorbuf.h
cramp.o: cdiag.h colorbuf.h
