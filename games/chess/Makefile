#
# @(#)Makefile 1.1 86/09/24 SMI
#

DESTDIR=
CFLAGS=	-O
OBJS=	agen.o attb.o attw.o bgen.o bheur.o bmove.o book.o bplay.o \
	clock.o ctrl.o data.o init.o io.o mater.o play.o qsort.o \
	savres.o setup.o stat.o stdin.o wgen.o wheur.o wmove.o wplay.o
CFILES=	agen.c bheur.c book.c bplay.c clock.c data.c init.c io.c \
	mater.c play.c savres.c setup.c stat.c stdin.c wheur.c wplay.c
SFILES=	attb.s attw.s bgen.s bmove.s ctrl.s qsort.s wgen.s wmove.s

all: chess
chess: ${OBJS}
	cc ${CFLAGS} -o chess ${OBJS}

${OBJS}: old.h

install: chess
	install -s chess ${DESTDIR}/usr/games/chess
	cp book ${DESTDIR}/usr/games/lib/chess.book
clean:
	rm -f ${OBJS} chess
