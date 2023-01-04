#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 4.23 83/07/01
#

DESTDIR=
CRYPT=-DCRYPT
CFLAGS=	-O ${CRYPT} -DDEBUG
DEBUGDIR= /usr/lib/debug

OBJS=	malloc.o mallocmap.o

debug: ${OBJS}

malloc.o: ../malloc.c ../mallint.h
	${CC} ${CFLAGS} -c -I.. ../malloc.c

mallocmap.o: mallocmap.c ../mallint.h
	${CC} ${CFLAGS} -c -I.. mallocmap.c
	
clean:
	rm -f ${OBJS}

install: debug
	rm -fr ${DESTDIR}${DEBUGDIR}
	mkdir ${DESTDIR}${DEBUGDIR}
	-for i in ${OBJS}; do \
		install -m 644 $${i} ${DESTDIR}${DEBUGDIR}; \
	done
