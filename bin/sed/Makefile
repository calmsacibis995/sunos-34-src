#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 4.2 beta
#
#
CFLAGS=-O -w

sed:	sed0.o sed1.o
	cc -o sed *.o

sed0.o: sed0.c sed.h
sed1.o: sed1.c sed.h

install: sed
	install -s sed $(DESTDIR)/bin
clean:
	rm -f *.o sed
