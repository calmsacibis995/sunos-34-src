#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 4.1 83/02/10
#

CFLAGS=-O

diff3: diff3.o
	cc -o diff3 diff3.o

install: diff3
	install -s diff3 $(DESTDIR)/usr/lib
	install -c diff3.sh $(DESTDIR)/usr/bin/diff3

clean:
	rm -f diff3.o diff3	
