#
# @(#)Makefile 1.1 86/09/25 SMI; from S5R2 1.2
#
#	Makefile for file command -- necessary to install /etc/magic
#
CFLAGS=	-O

file:	file.c
	$(CC) $(CFLAGS) $(LDFLAGS) file.c -o file

install: file
	install -s file $(DESTDIR)/usr/bin
	install -c -m 644 magic $(DESTDIR)/etc

clean:
	-rm -f file.o
	-rm -f file
