#
# @(#)Makefile 1.1 86/09/24 SMI
#

SRCS = boggle.c lists.c sand.c search.c
OBJS = boggle.o lists.o sand.o search.o
LIBS = -lsuntool -lsunwindow -lpixrect
CFLAGS = -O

all: boggletool boggledict

boggletool: $(OBJS)
	cc -o boggletool $(CFLAGS) $(OBJS) $(LIBS)

boggle.o: boggle.c
	mv boggle.c boggle.c-
	sed -e "s/boggletool_main/main/" boggle.c- > boggle.c
	cc $(CFLAGS) -c boggle.c
	mv boggle.c- boggle.c

boggle.o: defs.h images.h
lists.o: defs.h
sand.o: defs.h outline.h sandframes.h
search.o: defs.h

boggledict: bogglewords comp
	cat bogglewords | spellin > boggledict
	cat bogglewords | comp >> boggledict

comp: comp.c
	cc -o comp $(CFLAGS) comp.c

lint:
	lint -hb $(SRCS) $(LIBS)

install: all
	install -s boggletool $(DESTDIR)/usr/games/boggletool
	install boggledict $(DESTDIR)/usr/games/boggledict

clean:
	rm -f $(OBJS) boggletool
