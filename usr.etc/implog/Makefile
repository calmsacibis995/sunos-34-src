#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 4.5 06/12/83
#
ALL=	implog implogd
DESTDIR=
CFLAGS=-O

all:	${ALL}

implog:	implog.o
	${CC} implog.o -o implog

implogd: implogd.o
	${CC} implogd.o -o implogd

clean:
	rm -f ${ALL} *.o *.s errs core a.out t.?

install: ${ALL}
	install -s implog ${DESTDIR}/usr/etc
	install -s implogd ${DESTDIR}/usr/etc
