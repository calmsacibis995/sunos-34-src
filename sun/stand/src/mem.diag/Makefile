#
#	makefile for sun2 memory diagnostic sources
#
#	@(#)Makefile 1.1 86/09/25 SMI
#
LIB=		../../lib
INCLUDE=	../../include
OFILES=		mem_addr.o mem_check.o mem_const.o mem_diag.o mem_disp.o \
		mem_errs.o mem_fill.o mem_micr.o mem_opts.o mem_rand.o \
		mem_uniq.o address.o uunique.o
COPTS=-O
CFLAGS=$(COPTS) -I$(INCLUDE)

all:	mem.diag

mem.diag:	$(LIB)/stack.o mem.diag.o $(OFILES) $(LIB)/libdiag.a
		ld -N -T 4000 -o mem.diag $(LIB)/stack.o mem.diag.o \
			$(OFILES) $(LIB)/libdiag.a -lc

clean:
		rm -f *.o errs mem.diag

install:
		install -s mem.diag $(DESTDIR)/usr/stand
