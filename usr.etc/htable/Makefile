#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile 1.1 86/09/25 SMI from 5.2 9/18/85
#
CFLAGS=	-O
DESTDIR=
ALL=	htable

all:	${ALL}

htable:	htable.o parse.o scan.o
	${CC} htable.o parse.o scan.o -o htable

htable.o: htable.c htable.h
	${CC} ${CFLAGS} -c htable.c

scan.o:	scan.c htable.h y.tab.h

parse.o: parse.c htable.h

scan.c:	scan.l

parse.c: parse.y

y.tab.h:
	${YACC} -d parse.y
	rm y.tab.c

clean:
	rm -f ${ALL} *.o *.s parse.c scan.c y.tab.h errs

install: htable
	install -s htable ${DESTDIR}/usr/etc/htable
