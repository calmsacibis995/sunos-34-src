#
# @(#)Makefile 1.1 86/09/24 SMI; from S5R2 1.8
#

FRC =
DESTDIR =
CFLAGS = -O -DRES
LDFLAGS =

OFILES = setbrk.o blok.o stak.o cmd.o fault.o main.o word.o string.o\
name.o args.o xec.o service.o error.o io.o print.o macro.o expand.o\
ctype.o msg.o test.o defs.o echo.o hash.o hashserv.o pwd.o func.o

all: sh

sh: $(OFILES)
	$(CC) $(LDFLAGS) $(OFILES) -o sh

$(OFILES):	defs.h $(FRC)

ctype.o:	ctype.h
		$(CC) -c $(CFLAGS) -R ctype.c

service.o:	service.c
	$(CC) $(CFLAGS) -I/usr/src/cmd/acct -c service.c

blok.o fault.o main.o stak.o:	brkincr.h

io.o main.o:	dup.h

xec.o hash.o hashserv.o:	hash.h

cwd.o:	mac.h

blok.o cmd.o defs.o hashserv.o main.o setbrk.o stak.o xec.o:	stak.h

cmd.o macro.o main.o msg.o word.o xec.o:	sym.h

main.o:	timeout.h


msg.o:		msg.c $(FRC)
		$(CC) -c $(CFLAGS) -R msg.c

install:  all
	  install -s sh $(DESTDIR)/bin/sh

clean:
	  -rm -f *.o a.out core errs sh

FRC:
