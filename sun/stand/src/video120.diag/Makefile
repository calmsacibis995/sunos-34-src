#
# Makefile for Sun-2/120 Video Board Diagnostic source code.
#
# @(#)Makefile 1.1 86/09/25 SMI
#

DESTDIR= 
LIB=     ../../lib
INCLUDE= ../../include
OFILES=	crcpy.o crdef.o crint.o crjmp.o crm.o crmem.o crscr.o errlog.o gotomm.o        help.o loop.o mmall.o mmdef.o quit.o scc.o sccdef.o sccm.o vmaddrs.o           vmchekr.o vmconst.o cp.o vmdef.o vmm.o pem.o vmrandm.o soe.o vmuniq.o          woe.o vmbowm.o byteeh.o bsloop.o wordeh.o wsloop.o prompt.o strcat.o           pmenu.o inrange.o pause.o vidint.o

COPTS=-O
CFLAGS=$(COPTS) -I$(INCLUDE)

all:	video120.diag

video120.diag:	$(LIB)/stack.o mm.o $(OFILES) $(LIB)/libdiag.a
	        ld -N -T 4000 -o video120.diag $(LIB)/stack.o mm.o $(OFILES)                   $(LIB)/libdiag.a -lc

clean:
		rm -f *.o video120.diag

install:
		install -s video120.diag $(DESTDIR)/usr/stand
