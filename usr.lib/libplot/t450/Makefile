#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 4.2 83/07/09
#
CFLAGS=	-O
OBJS=	arc.o box.o circle.o close.o dot.o erase.o label.o \
	line.o linmod.o move.o open.o point.o space.o subr.o

../lib450:	${OBJS}
	ar cu ../lib450 ${OBJS}

clean:
	rm -f ${OBJS} errs a.out core
