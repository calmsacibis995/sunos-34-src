#
# @(#)Makefile 1.1 86/09/25 SMI
#
CFLAGS=-g -DPAINT -c
SRCS= main.c sym.c scan.c assemble.c output.c
OBJS= main.o sym.o scan.o assemble.o output.o


ppas: ${OBJS}
	cc -g -o ppas ${OBJS}

main.o: ../main.c
	cc $(CFLAGS) ../main.c

sym.o: ../sym.c
	cc $(CFLAGS) ../sym.c

scan.o: ../scan.c
	cc $(CFLAGS) ../scan.c

assemble.o: ../assemble.c
	cc $(CFLAGS) ../assemble.c

output.o: ../output.c
	cc $(CFLAGS) ../output.c

clean:
	rm -f ${OBJS}
