#	Makefile for Sun-2 SCSI diagnostic.
#       @(#)Makefile 1.1 9/25/86 Copyright Sun Micro

DESTDIR=
LIB=	 	../../lib
INCLUDE=	../../include
COPTS=-O
CFLAGS=$(COPTS) -I$(INCLUDE) -DSUN2COMPILE
OFILES=		vme_adap.o  vme_tests.o  vme_funct.o  vme_util.o  vme_tools.o \
		pmgn.o ansi_pos.o

all:		vmad.diag

vmad.diag:	$(LIB)/stack.o $(OFILES) $(LIB)/libdiag.a
		ld -T 4000 -o vmad.diag $(LIB)/stack.o $(OFILES) \
        		$(LIB)/libdiag.a -lc 

vme_funct.o:	vme_funct.c
		cc -I$(INCLUDE) -DSUN2COMPILE -c vme_funct.c



vme_adap.o: 	vme_adap.c vme_adap.h  
vme_tests.o:	vme_tests.c vme_adap.h  
vme_funct.o:	vme_funct.c vme_adap.h 
vme_util.o :	vme_util.c  vme_adap.h 
vme_tools.o:	vme_tools.c vme_adap.h 
pmgn.o:		pmgn.c
ansi_pos.o:	ansi_pos.c

install:
		install -s vmad.diag $(DESTDIR)/usr/stand ; sync


clean:
		rm -f a.out core *.s *.o vmad.diag
