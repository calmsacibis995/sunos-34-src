#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 1.4 83/06/30
#
DESTDIR=
SRCS=	dc.c dc.h
CFLAGS=	-O

all: dc

dc:	dc.c dc.h
	${CC} ${CFLAGS} dc.c -o dc

install: dc
	install -s dc ${DESTDIR}/usr/bin/dc

clean:
	rm -f *.o dc a.out errs core
