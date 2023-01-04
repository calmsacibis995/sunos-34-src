#
# @(#)Makefile 1.1 86/09/25 SMI; Copyr 1986 by Sun Microsystems, Inc.
#

CFLAGS=-g -c
SRCS= main.c sym.c scan.c assemble.c output.c
OBJS= main.o sym.o scan.o assemble.o output.o


fpas: ${OBJS}
	cc -g -o fpas ${OBJS}

main.o: main.c micro.h
	cc $(CFLAGS) main.c

sym.o: sym.c micro.h
	cc $(CFLAGS) sym.c

scan.o: scan.c micro.h
	cc $(CFLAGS) scan.c

assemble.o: assemble.c micro.h
	cc $(CFLAGS) assemble.c

output.o: output.c micro.h
	cc $(CFLAGS) output.c

install:

clean:
	rm -f *.o fpas core errs a.out
