#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB
#
DESTDIR=
BINDIR=usr/ucb
CFLAGS=-O -DUCB_NTTY

tset:	tset.c
	${CC} ${CFLAGS} tset.c -o tset -ltermlib

reset:	tset
	cp tset reset

install:
	-rm -f ${DESTDIR}/${BINDIR}/tset
	-rm -f ${DESTDIR}/${BINDIR}/reset
	install -s tset ${DESTDIR}/${BINDIR}
	ln ${DESTDIR}/${BINDIR}/tset ${DESTDIR}/${BINDIR}/reset 

clean:
	rm -f *.o tset reset
