#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile 1.1 86/09/25 SMI from UCB 5.7 9/14/85
#

OBJS=	gethostnamadr.o sethostent.o res_comp.o res_debug.o \
	res_init.o res_mkquery.o res_send.o
SRCS=	gethostnamadr.c sethostent.c res_comp.c res_debug.c \
	res_init.c res_mkquery.c res_send.c

CFLAGS = -DDEBUG -O -I.
DESTDIR=

libresolv.a: ${OBJS}
	-rm -f libresolv.a
	ar cru libresolv.a `lorder ${OBJS} | tsort`
	ranlib libresolv.a

clean:
	rm -f *.o errs libresolv.a

install: libresolv.a
	install -m 644 libresolv.a ${DESTDIR}/usr/lib/libresolv.a
	ranlib ${DESTDIR}/usr/lib/libresolv.a

