#
#       @(#)Makefile 1.4 87/03/12 SMI
#
#	Makefile for the example programs in the SunView Programmer's
#	Guide and SunView System Programme's Guide.
#	Some of the programs have been modified since the manuals
#	were printed.

#	This is the order they appear in the Example Programs chapter,
#	with the addition of the  selection demo program from the SunView
#	System Programmer's Guide
DESTDIR=
CFILES= filer.c image_browser_2.c resize_demo.c \
	coloredit.c animatecolor.c tty_io.c font_menu.c \
	dctool.c typein.c bounce.c spheres.c \
	seln_demo.c confirm.c
CC= cc
INSTALL= install
SCCS= sccs
CFLAGS= -O
LIBS = -lsuntool -lsunwindow -lpixrect

.c~.c:
	sccs get $<
		
all:  programmer system

programmer:  filer image_browser_2 resize_demo \
	coloredit animatecolor tty_io font_menu \
	dctool typein bounce spheres

system: seln_demo

spheres.icon: SCCS/s.spheres.icon
	sccs get spheres.icon

filer: filer.o confirm.o
	${CC} ${CFLAGS} ${LDFLAGS} -o filer filer.o confirm.o ${LIBS}

image_browser_2: image_browser_2.o
	${CC} ${CFLAGS} ${LDFLAGS} -o image_browser_2 image_browser_2.o ${LIBS}

resize_demo: resize_demo.o
	${CC} ${CFLAGS} ${LDFLAGS} -o resize_demo resize_demo.o ${LIBS}

coloredit: coloredit.o
	${CC} ${CFLAGS} ${LDFLAGS} -o coloredit coloredit.o ${LIBS}

animatecolor: animatecolor.o
	${CC} ${CFLAGS} ${LDFLAGS} -o animatecolor animatecolor.o ${LIBS}

tty_io: tty_io.o
	${CC} ${CFLAGS} ${LDFLAGS} -o tty_io tty_io.o ${LIBS}

font_menu: font_menu.o
	${CC} ${CFLAGS} ${LDFLAGS} -o font_menu font_menu.o ${LIBS}

dctool: dctool.o
	${CC} ${CFLAGS} ${LDFLAGS} -o dctool dctool.o ${LIBS}

typein: typein.o
	${CC} ${CFLAGS} ${LDFLAGS} -o typein typein.o ${LIBS}

bounce: bounce.o
	${CC} ${CFLAGS} ${LDFLAGS} -o bounce bounce.o ${LIBS}

spheres.o: spheres.c spheres.icon
	${CC} -c ${CFLAGS} spheres.c

spheres: spheres.o spheres.icon
	${CC} ${CFLAGS} ${LDFLAGS} -o spheres spheres.o ${LIBS}

seln_demo: seln_demo.o
	${CC} ${CFLAGS} ${LDFLAGS} -o seln_demo seln_demo.o ${LIBS}

install_sources: ${CFILES} spheres.icon
	-for i in Makefile ${CFILES} spheres.icon; do \
		${INSTALL} -m 444 $$i ${DESTDIR}/usr/src/sun/suntool/examples; \
	done
