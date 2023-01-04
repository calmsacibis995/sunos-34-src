#
#	sky.diag (Stand-Alone Sky Board Diagnostic) @(#)Makefile 1.16 4/29/85
#

DESTDIR=
CFLAGS=		-O -I$(INCLUDE)
INCLUDE=	../../include
LIB=		../../lib
FILES=		sky.diag.o skyucode.o

all:		sky.diag

clean:
		rm -f a.out core *.s *.o sky.diag

skyucode:	skyucode.c
		cc -c skyucode.c

sky.diag:	$(FILES)
		ld -M -N -T 4000 -o sky.diag $(LIB)/stack.o $(FILES) $(LIB)/libdiag.a -lc

install:
		install -s sky.diag $(DESTDIR)/usr/stand ; sync


