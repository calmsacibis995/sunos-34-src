#
# @(#)Makefile 1.1 86/09/25 SMI; from S5R2 1.3
#
CFLAGS = -O
FILES = main.o sub1.o sub2.o header.o

lex:	$(FILES) y.tab.o
	$(CC) $(LDFLAGS) $(FILES) y.tab.o -o lex

$(FILES): ldefs.c
main.o:	  once.c
y.tab.c:  parser.y
	$(YACC) parser.y

install: lex
	install -s lex $(DESTDIR)/usr/bin
	-mkdir $(DESTDIR)/usr/lib/lex && chown bin $(DESTDIR)/usr/lib/lex && \
		chmod 755 $(DESTDIR)/usr/lib/lex 
	install -c -m 644 ncform $(DESTDIR)/usr/lib/lex
	install -c -m 644 nrform $(DESTDIR)/usr/lib/lex

clean:
	rm -f *.o lex y.tab.c
