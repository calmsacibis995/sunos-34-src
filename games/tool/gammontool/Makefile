#
# @(#)Makefile 1.1 86/09/24 SMI
#

OBJECTS = gammontool.o human.o computer.o draw.o cubes.o dice.o \
	bkg.o evalmove.o score.o
SOURCES = gammontool.c human.c computer.c draw.c cubes.c dice.c \
	bkg.c evalmove.c score.c
CMDS = gammontool gammonscore
LIBS = -lsuntool -lsunwindow -lpixrect
CFLAGS = -O

all: $(CMDS)

gammonscore: gammonscore.c score.h
	cc $(CFLAGS) -o gammonscore gammonscore.c

gammontool: $(OBJECTS)
	cc $(CFLAGS) -o gammontool $(OBJECTS) $(LIBS)

gammontool.o: gammontool.c
	mv gammontool.c gammontool.c-
	sed -e "s/gammontool_main/main/" gammontool.c- > gammontool.c
	cc $(CFLAGS) -c gammontool.c
	mv gammontool.c- gammontool.c

gammontool.o: defs.h bkgcodes.h cursors.h

human.o draw.o: defs.h

computer.o: defs.h bkgcodes.h

lint:
	lint -hb $(SOURCES) $(LIBS)

install: $(CMDS)
	install -s -m 4755 -o daemon gammontool ${DESTDIR}/usr/games/gammontool
	install -s gammonscore ${DESTDIR}/usr/games/gammonscore

clean:
	rm -f $(OBJECTS) gammontool
