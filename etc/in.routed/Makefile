#
# @(#)Makefile 1.4 86/10/13 SMI; from UCB X.X 06/12/83
#
ALL=	in.routed
OBJS=	af.o if.o inet.o input.o main.o output.o startup.o tables.o timer.o \
	trace.o
CFLAGS=	-O
DESTDIR=
LDFLAGS=

all:	${ALL}

in.routed: ${OBJS}
	${CC} -o in.routed ${LDFLAGS} ${OBJS}

install: all
	install -s in.routed ${DESTDIR}/etc/in.routed

clean:
	rm -f in.routed *.o core a.out t.c linterrs typescript errs
