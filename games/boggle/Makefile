#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 5.3 85/09/07
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
DESTDIR=
CC = /bin/cc
CFLAGS = -O

all:	boggle bogdict

bogdict: comp sfile
	sed -f sfile /usr/dict/words | uniq | comp >bogdict

install: boggle bogdict
	install -s boggle ${DESTDIR}/usr/games/boggle
	install bogdict ${DESTDIR}/usr/games/bogdict

boggle: boggle.c
	cc -o boggle ${CFLAGS} boggle.c

comp: comp.c
	cc -o comp ${CFLAGS} comp.c

clean:
	rm -f bogdict boggle comp
