#	@(#)Makefile	1.1	(Berkeley)	9/24/86
#
DESTDIR=
CFLAGS=	-O

all:	cno phx

cno:	cno.c
	cc -O -o cno cno.c

phx:	phx.c
	cc -O -o phx phx.c

install: all
	install -c ching.sh ${DESTDIR}/usr/games/ching
	-mkdir ${DESTDIR}/usr/games/lib/ching.d
	install -c cno ${DESTDIR}/usr/games/lib/ching.d/cno
	install -c phx ${DESTDIR}/usr/games/lib/ching.d/phx
	install -c macros ${DESTDIR}/usr/games/lib/ching.d/macros
	install -c hexagrams ${DESTDIR}/usr/games/lib/ching.d/hexagrams

clean:
	rm -f cno phx a.out core *.s *.o
