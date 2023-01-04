#
# @(#)Makefile 1.1 86/09/25 SMI;
#	Makefile for serial i/o loopback tests
#
#	Global descriptions
#
DESTDIR=
CFLAGS = -I -c

#	Main file descriptions

all:	loopback

loopback: loopback.c expars.o siodat.o
	cc -I loopback.c expars.o siodat.o -o loopback

#	Object descriptions

expars.o:
	cc $(CFLAGS) expars.c
siodat.o:
	cc $(CFLAGS) siodat.c

#	Utilities

doc:
	tbl loopbackusr.msun | itroff -msun 2>loopbackusrtc.msun -
	echo .UH CONTENTS > loopbackusrPages.msun
	grep '^\.CE [CANU]' loopbackusrtc.msun | itroff -rP1 -msun \
		loopbackusrPages.msun -
	rm -f *tc.msun *Pages.msun

clean:
	rm -f *tc.msun *Pages.msun
	rm -f *.o *.BAK *.CKP
	rm -f loopback

# 	This is a Unix-level test, so belongs in /usr/diag.

install:
	install -s loopback $(DESTDIR)/usr/diag

