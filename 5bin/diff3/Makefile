#
# @(#)Makefile 1.1 86/09/24 SMI; from S5R2 1.5
#
#	diff3 make file

IFLAG =
CFLAGS = -O
LDFLAGS = $(IFLAG)
MAKE = make

diff3prog:
	$(CC) $(CFLAGS) $(LDFLAGS) -o diff3prog diff3prog.c

install: diff3prog
	install -c diff3.sh $(DESTDIR)/usr/5bin/diff3
	install -s diff3prog $(DESTDIR)/usr/5lib

clean:
	rm -f diff3prog
