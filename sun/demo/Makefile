#
# @(#)Makefile 1.6 87/01/11 SMI
#

DESTDIR=

CORE_DEMOS= draw shaded showmap stringart suncube
WINDOW_DEMOS= cframedemo show maze
OTHER_DEMOS= goban
DEMOS= ${CORE_DEMOS} ${WINDOW_DEMOS} ${OTHER_DEMOS}

GOBANSRC=	goban.c goboard.c gopanel.c goservice.c gocapture.c gocircle.c \
	goprint.c gomaster.c
GOBANOBJ=	goban.o goboard.o gopanel.o goservice.o gocapture.o gocircle.o \
	goprint.o gomaster.o

CORELIBS= -lcore -lm
WINLIBS=-lsuntool -lsunwindow -lpixrect

FLOATFLAG= -fsingle -fswitch
CFLAGS = -O ${FLOATFLAG}

SUBDIR= GP1

all:	${DEMOS} FRC

FRC:
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS}); \
		done

.s.o:
	/lib/cpp -E $*.s >$*.pp
	${AS} -o $*.o $*.pp
	rm -f $*.pp

cframedemo: cframedemo.o
	cc cframedemo.o -o cframedemo ${WINLIBS} 

goban:	$(GOBANOBJ)
	cc -o goban $(CFLAGS) $(GOBANOBJ) -lsuntool -lsunwindow -lpixrect

$(GOBANOBJ):  goban.icon goban.h
	cc -c $(CFLAGS) $*.c

maze: maze.o
	cc maze.o -o maze ${WINLIBS} 

draw: draw.o
	cc draw.o -o draw ${CORELIBS} ${WINLIBS}

shaded: shaded.o
	cc shaded.o -o shaded ${CORELIBS} ${WINLIBS}

showmap: showmap.o
	cc showmap.o -o showmap ${CORELIBS} ${WINLIBS}

show: show.o
	cc show.o -o show ${WINLIBS}

stringart: stringart.o
	cc stringart.o -o stringart ${CORELIBS} ${WINLIBS}

suncube: suncube.o
	cc suncube.o -o suncube ${CORELIBS} ${WINLIBS} 

clean:
	-rm -f *.o errs core ${DEMOS} ${NOSRC_DEMOS} ${OTHER_DEMOS}
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} clean); \
		done

install: all
	-for i in '' /MAPS /DATA /COLORPIX /SRC /globeframes; do \
		mkdir ${DESTDIR}/usr/demo$$i && \
		chown bin ${DESTDIR}/usr/demo$$i && \
                chmod 755 ${DESTDIR}/usr/demo$$i; done
	-for i in ${CORE_DEMOS} ${WINDOW_DEMOS}; do \
		(cp $$i.c ${DESTDIR}/usr/demo/SRC); done
	-for i in ${GOBANSRC}; do \
		(cp $$i ${DESTDIR}/usr/demo/SRC); done
	-cd DATA; for i in *; do\
		(cp $$i ${DESTDIR}/usr/demo/DATA); done
	-cd COLORPIX2; for i in *; do\
		(cp $$i ${DESTDIR}/usr/demo/COLORPIX); done
	-cd MAPS; for i in *; do\
		(cp $$i ${DESTDIR}/usr/demo/MAPS); done
	-cd globeframes; for i in *; do\
		(cp $$i ${DESTDIR}/usr/demo/globeframes); done
	-(cp README ${DESTDIR}/usr/demo)
	-(cp Readme.goban ${DESTDIR}/usr/demo)
	-(cp demolib.h ${DESTDIR}/usr/demo/SRC)
	-(cp goban.h ${DESTDIR}/usr/demo/SRC)
	-(cp goban.icon ${DESTDIR}/usr/demo/SRC)
	-(cp Makefile.release ${DESTDIR}/usr/demo/Makefile)
	-(cp Makefile.src.release ${DESTDIR}/usr/demo/SRC/Makefile)
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
