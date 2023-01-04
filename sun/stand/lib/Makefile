#
# @(#)Makefile 1.1 9/25/86 SMI
#
# Makefile for sun2 diagnostic library
#
all:	libdiag.a libmem.a stack.o pstack.o

INCLUDE=	../include
CFLAGS=		-O -I$(INCLUDE) -c -DTOKEN_BUF_SIZE=256

MEMFILES=	addr.o cmp.o copy.o db.o patt.o rand.o uniq.o unique.o
#			fill.o check.o

LIBFILES=	machdep.o prf.o saio.o timefrom.o delay.o \
		$(MEMFILES) loop.o token.o tokenbuf.o buserr.o \
		idprom.o getidprom.o ex.o exception.o

FLOATFILES=	lmodt.o ldivt.o lmult.o \
		ptwo.o float.o sfloat.o oldstuff.o safvector.o

libdiag.a:	$(LIBFILES) $(FLOATFILES)
		ar ruv libdiag.a $(LIBFILES) $(FLOATFILES)
		ranlib libdiag.a

libmem.a:	$(MEMFILES)
		ar ruv libmem.a $(MEMFILES)
		ranlib libmem.a

clean:
		rm -f a.out *.o errs core libdiag.a libmem.a

install:

.s.o:
	/lib/cpp -I$(CFLAGS) $*.s > $*.pp
	as $*.pp -o $*.o ; rm $*.pp
