#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 1.1 12/15/82
#

CFLAGS=-O
m4:	m4.o m4y.o
	$(CC) -o m4 m4.o m4y.o 

install:
	install -s m4 $(DESTDIR)/usr/bin

clean:
	rm -f *.o m4 a.out core errs
