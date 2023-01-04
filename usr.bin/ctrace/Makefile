#
# @(#)Makefile 1.1 86/09/25 SMI; from S5R2 1.2
#
#	ctrace makefile
# installation directories:
BIN = /usr/bin
LIB = /usr/lib/ctrace
INS = install

# setting preprocessor symbols:
# set for UNIX/370:
# U370 = -b1,0
CFLAGS	= -O -DLIB=\"$(LIB)

YFLAGS	= -d

SOURCE	 = constants.h global.h main.c parser.y scanner.l lookup.c trace.c \
	   runtime.c ctcr
OTHER	 = Makefile
OBJECTS	 = main.o parser.o scanner.o lookup.o trace.o

ctrace:	$(OBJECTS)
	@echo '(expect 13 shift/reduce)'
	$(CC) -o ctrace $(LDFLAGS) $(OBJECTS)

install: ctrace
	$(INS) -s ctrace $(DESTDIR)$(BIN)
	-mkdir $(DESTDIR)$(LIB) && chown bin $(DESTDIR)$(LIB) && chmod 755 $(DESTDIR)$(LIB)
	$(INS) -c runtime.c $(DESTDIR)$(LIB)

clean:
	-rm -f *.o y.* yacc.* y.tab.h y.output
	-rm -f ctrace
