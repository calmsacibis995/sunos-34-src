#
# @(#)Makefile 1.1 86/09/24 SMI
#

OBJECTS = decode.o input.o canfieldtool.o menu.o misc.o pipe.o sendmove.o 

LIBS =  -lsuntool -lsunwindow -lpixrect

CFLAGS = -O

canfieldtool:	$(OBJECTS)
	cc -o canfieldtool $(CFLAGS) $(OBJECTS) $(LIBS)

canfieldtool.o: canfieldtool.c
	mv canfieldtool.c canfieldtool.c-
	sed -e "s/canfieldtool_main/main/" canfieldtool.c- > canfieldtool.c
	cc $(CFLAGS) -c canfieldtool.c
	mv canfieldtool.c- canfieldtool.c

decode.o: defs.h
input.o: defs.h
canfieldtool.o: defs.h icon.h outline.h
menu.o: defs.h text.h
misc.o: defs.h cards.h
pipe.o: defs.h

clean: 
	-rm $(OBJECTS) core canfieldtool

install: canfieldtool
	install -s -o daemon canfieldtool ${DESTDIR}/usr/games/canfieldtool

