#
# makefile for video pattern generator
#
# @(#)Makefile 1.1 86/09/25 SMI
#
DESTDIR=
COPTS=-O

all:	vid.120.pat

vid.120.pat:	vid.120.pat.c /usr/lib/libpixrect.a pat_icons.h
	cc -o vid.120.pat ${COPTS} vid.120.pat.c -lpixrect -lcurses -ltermcap

clean:
	rm -rf errs *.o vid.120.pat

install: all
	install -s vid.120.pat	$(DESTDIR)/usr/diag
