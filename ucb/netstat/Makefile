#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 4.4 12/18/82
#
OBJS=	host.o inet.o if.o main.o mbuf.o route.o
CFLAGS=-O
DESTDIR=

netstat: ${OBJS}
	${CC} ${OBJS} -o netstat

install: netstat
	install -m 2755 -g kmem -s netstat ${DESTDIR}/usr/ucb

clean:
	rm -f netstat *.o core a.out errs
