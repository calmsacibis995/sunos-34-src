#
# Makefile for color board tests.
#
#	This directory and its include above are an inherited kludge,
#	and someday will be fixed or lost.
#
#	@(#)Makefile 1.1 9/25/86 Copyright Sun Microsystems, Inc.
OBJ=	sc.diag.o scdac.o scfind.o schecker.o scint.o scman.o screg.o \
	scutils.o scvect.o autocmap.o automem.o scrop.o briefmon.o \
	mancmap.o memtest.o plmask.o manmem.o sczoom.o s2map.o checkerfastb.o \
	autodst.o autosrc.o bwmon.o checkerfast.o machdep2.o prf.o

SRC=	assym.h scbuf.h sc.diag.h autocmap.c autodst.c \
	automem.c autosrc.c briefmon.c bwmon.c \
	checkerfast.c checkerfastb.c machdep2.c mancmap.c manmem.c \
	memtest.c plmask.c prf.c s2map.s sc.diag.c scdac.c scfind.c \
	schecker.c scint.c scman.c screg.c scrop.c scutils.c scvect.c \
	sczoom.c

all:	sc.diag

$(SRC):
	sccs get $@

sc.diag:assym.h scbuf.h sc.diag.h $(OBJ)
	ld -N -T 4000 -o sc.diag $(OBJ) -lc

clean:
	rm -f *.o *.pp core errs sc.diag

install:
	install -s sc.diag $(DESTDIR)/usr/stand

.s.o:
	/lib/cpp $*.s > $*.pp
	as $*.pp -o $*.o ; rm $*.pp
