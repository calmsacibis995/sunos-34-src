#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 4.1 83/06/27
#
CFLAGS=-O

libdbm.a: dbm.o
	mv dbm.o libdbm.a

dbm.o : dbm.h

install:
	install -c -m 644 libdbm.a $(DESTDIR)/usr/lib
	install -c -m 644 dbm.h $(DESTDIR)/usr/include

clean:
	rm -f libdbm.a
