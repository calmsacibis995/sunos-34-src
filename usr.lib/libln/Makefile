#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 4.2 85/05/30
#
CFLAGS = -O
SRCS=	allprint.c main.c reject.c yyless.c yywrap.c
OBJS=	allprint.o main.o reject.o yyless.o yywrap.o
TAGSFILE=tags

all: libln.a

libln.a: ${OBJS}
	ar rc libln.a ${OBJS}

install: all
	install -c -m 644 libln.a ${DESTDIR}/usr/lib
	rm -f ${DESTDIR}/usr/lib/libl.a
	ln ${DESTDIR}/usr/lib/libln.a ${DESTDIR}/usr/lib/libl.a
	ranlib ${DESTDIR}/usr/lib/libln.a

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f ${OBJS} libln.a tags
