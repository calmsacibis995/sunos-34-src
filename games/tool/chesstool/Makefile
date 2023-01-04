#
# @(#)Makefile 1.4 86/12/17 Copyr 1984 Sun Micro
#
#
CFLAGS=	-O
LIBS=	-lsuntool -lsunwindow -lpixrect
OBJS =	chesstool.o pieces.o chessprog.o clock.o
NO_MAIN_OBJS =	pieces.o chessprog.o clock.o

all: chesstool
chesstool: ${OBJS}
	cc ${CFLAGS} -o chesstool ${OBJS} ${LIBS}

install: chesstool
	install -s chesstool ${DESTDIR}/usr/games/chesstool

clean:
	rm -f ${OBJS} chesstool

standalone:       ${NO_MAIN_OBJS}  ../life/prompt.o
	cc -DSTANDALONE ${CFLAGS} -o chesstool chesstool.c \
	 ../life/prompt.o ${NO_MAIN_OBJS} ${LIBS}

chessprog.o: chesstool.h
clock.o: chesstool.h clockhands.h
chesstool.o: pieces.h chesstool.h
pieces.o: pieces.h
