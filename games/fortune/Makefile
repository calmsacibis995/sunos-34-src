#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB
#
FORTUNES=	scene obscene
SOURCE=		fortune.c strfile.h strfile.c unstr.c ${FORTUNES}
DESTDIR=
LIBDIR=		${DESTDIR}/usr/games/lib
BINDIR=		${DESTDIR}/usr/games
OWN=		arnold
GRP=		arpa
CFLAGS=		-O
TARF=		fortunes.tar

.DEFAULT:
	sccs get $@

all: fortune strfile unstr fortunes.dat

fortune: strfile.h fortune.c
	${CC} ${CFLAGS} -DFORTFILE='"${LIBDIR}/fortunes.dat"' -o fortune fortune.c

strfile: strfile.h strfile.c
	${CC} ${CFLAGS} -o strfile strfile.c

unstr: strfile.h unstr.c
	${CC} ${CFLAGS} -o unstr unstr.c

fortunes.dat: fortunes strfile
	./strfile fortunes

fortunes: $(FORTUNES)
	cat scene > fortunes
	echo "%-" >> fortunes
	cat obscene >> fortunes
	echo "%%" >> fortunes

install: all
	install -m 600 -o daemon fortunes.dat ${LIBDIR}/
	install -m 4711 -o daemon fortune ${BINDIR}/

tar:
	tar crvf ${TARF} Makefile ${SOURCE} fortune.6

clean:
	rm -f fortune fortunes fortunes.dat fortunes.tar strfile unstr
