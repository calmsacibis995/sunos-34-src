#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB     06/12/83
#
ALL=	tftp
DESTDIR=
CFLAGS=-O

all: ${ALL}

tftp:	main.o tftp.o
	${CC} main.o tftp.o -o tftp

clean:
	rm -f ${ALL} *.o *.s errs core a.out t.?

install: 
	install -s tftp ${DESTDIR}/usr/ucb
