#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 4.1 83/07/04
#
DESTDIR=
CFLAGS=	-O
LIBS=	-lm -ltermcap
BIN=	$(DESTDIR)/usr/games
LIB=	$(DESTDIR)/usr/games/lib
USER=	daemon
UTILS=	snscore
OBJS=	snake.o move.o
ALL=	snake ${UTILS}

all:	${ALL}

snake:	$(OBJS)
	cc $(OBJS) -o snake $(LIBS)

snake.o move.o:snake.h

snscore: snscore.c
	cc $(CFLAGS) snscore.c -o snscore

busy:	busy.c
	cc $(CFLAGS) busy.c -o busy

install: all
	install -s -m 4755 -o ${USER} snake ${BIN}/snake
	install -s -m 755 -o ${USER} snscore ${BIN}/snscore
#	install -s -m 755 -o ${USER} busy ${BIN}/busy
	cat /dev/null >> $(LIB)/snakerawscores
	chmod 644 $(LIB)/snakerawscores
	chown $(USER) $(LIB)/snakerawscores

clean:
	rm -f *.o ${ALL}
