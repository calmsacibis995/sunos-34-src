#
# @(#)Makefile 1.1 86/09/24 Copyr 1985 Sun Micro
#
#
CFLAGS=	-O
LIBS=	-lsuntool -lsunwindow -lpixrect -lm

OBJS =	chesstool.o pieces.o chessprog.o clock.o \
	gammontool.o human.o computer.o draw.o cubes.o dice.o \
	bkg.o evalmove.o score.o boggle.o lists.o sand.o \
	search.o decode.o input.o canfieldtool.o \
	life.o lifetool.o circle.o patterns.o prompt.o \
	menu.o misc.o pipe.o sendmove.o gamemerge.o

all: gamemerge boggledict gammonscore

boggledict: FRC
	cd boggletool; make boggledict

gammonscore: FRC
	cd gammontool; make gammonscore

FRC:

gamemerge: ${OBJS}
	cc ${CFLAGS} -o gamemerge ${OBJS} ${LIBS}

chesstool.o: chesstool/chesstool.c chesstool/pieces.h chesstool/chesstool.h
	cc $(CFLAGS) -c chesstool/chesstool.c
pieces.o: chesstool/pieces.c chesstool/pieces.h
	cc $(CFLAGS) -c chesstool/pieces.c
chessprog.o: chesstool/chessprog.c chesstool/chesstool.h
	cc $(CFLAGS) -c chesstool/chessprog.c
clock.o: chesstool/clock.c chesstool/chesstool.h chesstool/clockhands.h
	cc $(CFLAGS) -c chesstool/clock.c
gammontool.o: gammontool/gammontool.c gammontool/defs.h \
	gammontool/bkgcodes.h gammontool/cursors.h
	cc $(CFLAGS) -c gammontool/gammontool.c
human.o: gammontool/human.c gammontool/defs.h
	cc $(CFLAGS) -c gammontool/human.c
computer.o: gammontool/computer.c gammontool/defs.h gammontool/bkgcodes.h
	cc $(CFLAGS) -c gammontool/computer.c
draw.o: gammontool/draw.c gammontool/defs.h
	cc $(CFLAGS) -c gammontool/draw.c
cubes.o: gammontool/cubes.c
	cc $(CFLAGS) -c gammontool/cubes.c
dice.o: gammontool/dice.c
	cc $(CFLAGS) -c gammontool/dice.c
bkg.o: gammontool/bkg.c
	cc $(CFLAGS) -c gammontool/bkg.c
evalmove.o: gammontool/evalmove.c
	cc $(CFLAGS) -c gammontool/evalmove.c
score.o: gammontool/score.c
	cc $(CFLAGS) -c gammontool/score.c
boggle.o: boggletool/boggle.c boggletool/defs.h boggletool/images.h
	cc $(CFLAGS) -c boggletool/boggle.c
lists.o: boggletool/lists.c boggletool/defs.h
	cc $(CFLAGS) -c boggletool/lists.c
sand.o: boggletool/sand.c boggletool/defs.h boggletool/outline.h \
	    boggletool/sandframes.h
	cc $(CFLAGS) -c boggletool/sand.c
search.o: boggletool/search.c boggletool/defs.h
	cc $(CFLAGS) -c boggletool/search.c
decode.o: canfieldtool/decode.c canfieldtool/defs.h
	cc $(CFLAGS) -c canfieldtool/decode.c
input.o: canfieldtool/input.c canfieldtool/defs.h
	cc $(CFLAGS) -c canfieldtool/input.c
canfieldtool.o: canfieldtool/canfieldtool.c canfieldtool/icon.h \
	canfieldtool/outline.h canfieldtool/defs.h
	cc $(CFLAGS) -c canfieldtool/canfieldtool.c
menu.o: canfieldtool/menu.c canfieldtool/defs.h canfieldtool/text.h
	cc $(CFLAGS) -c canfieldtool/menu.c
misc.o: canfieldtool/misc.c canfieldtool/defs.h canfieldtool/cards.h
	cc $(CFLAGS) -c canfieldtool/misc.c
pipe.o: canfieldtool/pipe.c canfieldtool/defs.h
	cc $(CFLAGS) -c canfieldtool/pipe.c
sendmove.o: canfieldtool/sendmove.c canfieldtool/defs.h
	cc $(CFLAGS) -c canfieldtool/sendmove.c
lifetool.o: life/lifetool.c life/life.h
	cc $(CFLAGS) -c life/lifetool.c
life.o: life/life.c life/life.h
	cc $(CFLAGS) -c life/life.c
circle.o: life/circle.c
	cc $(CFLAGS) -c life/circle.c
patterns.o: life/patterns.c life/life.h
	cc $(CFLAGS) -c life/patterns.c
prompt.o: life/prompt.c life/ml.icon life/mr.icon
	cc $(CFLAGS) -c life/prompt.c

clean:
	rm -f $(OBJS) gamemerge
install:
	install -s -m 4755 -o bin gamemerge $(DESTDIR)/usr/games/chesstool
	ln -s chesstool $(DESTDIR)/usr/games/gammontool
	ln -s chesstool $(DESTDIR)/usr/games/boggletool
	ln -s chesstool $(DESTDIR)/usr/games/canfieldtool
	ln -s chesstool $(DESTDIR)/usr/games/life
	install -s gammontool/gammonscore $(DESTDIR)/usr/games/gammonscore
	install boggletool/boggledict  $(DESTDIR)/usr/games/boggledict
