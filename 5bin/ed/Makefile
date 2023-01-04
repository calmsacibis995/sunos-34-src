#
# @(#)Makefile 1.1 86/09/24 SMI; from S5R2 1.9
#
#	ed make file

CFLAGS = -O 

ed:	ed.c
	$(CC) $(CFLAGS) -o ed ed.c

install: ed
	install -s ed $(DESTDIR)/usr/5bin
#	-rm -f $(DESTDIR)/usr/5bin/red
#	ln $(DESTDIR)/usr/5bin/ed $(DESTDIR)/usr/5bin/red

clean:
	-rm -f *.o ed
