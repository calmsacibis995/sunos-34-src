#
#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 4.2 83/02/11
#
DESTDIR=
CFLAGS=-O -DWORD32
SRCS = Makefile dextern files yaccpar \
	y1.c y2.c y3.c y4.c \
	yaccdiffs yaccnews

all: yacc

yacc: y1.o y2.o y3.o y4.o
	$(CC) -o yacc y?.o 
y1.o y2.o y3.o y4.o: dextern files
install:
	install -s yacc $(DESTDIR)/usr/bin
	install -c yaccpar $(DESTDIR)/usr/lib
clean :
	-rm -f *.o yacc

srcs: $(SRCS)
$(SRCS):
	sccs get $@
