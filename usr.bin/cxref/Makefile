#
# @(#)Makefile 1.1 86/09/25 SMI; from S5R2 1.7
#
FLG=
INS=install
OWNER=$(DESTDIR)/usr/lib
M=../../5bin/lint/mip
L=../../5bin/lint
INCS=-I. -I$M -I$L
DEFS=-DCXREF -DFLEXNAMES
LINTF = -p
LINT = lint
FFLAG =
CFLAGS = -O $(DEFS) $(INCS)
CPASS1 =	cgram.c $(M)/comm1.c $(M)/optim.c $(M)/pftn.c $(M)/scan.c $(M)/trees.c \
		$(M)/xdefs.c xlocal.c $(L)/lint.c $(M)/messages.c $(M)/yyerror.c
OPASS1 =	cgram.o comm1.o optim.o pftn.o scan.o trees.o \
		xdefs.o xlocal.o lint.o messages.o yyerror.o

XPASS =	$(M)/cgram.y $(M)/comm1.c $(M)/common $(L)/lint.c $(L)/lmanifest macdefs \
	$(M)/manifest $(M)/mfile1 $(M)/optim.c $(M)/pftn.c $(M)/scan.c $(M)/trees.c \
	$(M)/xdefs.c xlocal.c $(M)/messages.c $(M)/messages.h

all :	cxref xpass

# CXREF

cxref :	cxr.o
	$(CC) $(FFLAG) $(LDFLAGS) cxr.o -o cxref

cxr.o:	cxr.c owner.h
	$(CC) -c $(CFLAGS) cxr.c

# XPASS

xpass:	$(OPASS1)
	$(CC) $(FFLAG) $(LFLAG1) $(LDFLAGS) $(OPASS1) -o xpass

$(OPASS1): $(M)/manifest macdefs $(M)/mfile1

cgram.c: $(M)/cgram.y
	sed -e 's/\/\*CXREF\(.*\)\*\//\1/' $(M)/cgram.y > gram.y
	yacc gram.y
	@echo '(expect 6 shift/reduce)'
	mv y.tab.c cgram.c
	-rm -f gram.y

cgram.o:	cgram.c
	$(CC) -c -DBUG4 $(FFLAG) $(CFLAGS) cgram.c

comm1.o: $(M)/common
	$(CC) -c -DBUG4 $(FFLAG) $(CFLAGS) $(M)/comm1.c

lint.o:	$(L)/lmanifest $(L)/lint.c
	$(CC) -c $(FFLAG) $(CFLAGS) $(L)/lint.c
	
optim.o:	$(M)/optim.c
	$(CC) -c -DBUG4 $(FFLAG) $(CFLAGS) $(M)/optim.c
	
pftn.o:		$(M)/pftn.c
	$(CC) -c -DBUG4 $(FFLAG) $(CFLAGS) $(M)/pftn.c
	
scan.o: $(M)/scan.c
	$(CC) -c -DBUG4 $(FFLAG) $(CFLAGS) $(M)/scan.c

trees.o:	$(M)/trees.c
	$(CC) -c -DBUG4 $(FFLAG) $(CFLAGS) $(M)/trees.c

xdefs.o: $(M)/xdefs.c
	$(CC) -c -DBUG4 $(FFLAG) $(CFLAGS) $(M)/xdefs.c
	
xlocal.o:	xlocal.c $(L)/lmanifest
	$(CC) -c $(FFLAG) $(CFLAGS) xlocal.c

messages.o:	$(M)/messages.c $(M)/messages.h
	$(CC) -c $(FFLAG) $(CFLAGS) $(M)/messages.c

yyerror.o:	$(M)/yyerror.c
	$(CC) -c $(FFLAG) $(CFLAGS) $(M)/yyerror.c

# UTILITIES

install :	all
	$(INS) -s cxref $(DESTDIR)/usr/bin
	$(INS) -s xpass $(OWNER)

clean:
	-rm -f *.o
	-rm -f xpass cgram.c cxref

lint:
	$(LINT) $(LINTF) cxr.c
	$(LINT) $(LINTF) -DBUG4 $(CPASS1)
	$(LINT) $(LINTF) -Dunix=1 $(CPP)/cpp.c cpy.c
