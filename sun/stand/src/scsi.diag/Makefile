# Makefile for Sun-2 SCSI diagnostic.
#       @(#)Makefile 1.1 9/25/86 Copyright Sun Micro

LIB=	 	../../lib
INCLUDE=	../../include
OFILES=		scsi.o sc_tests.o sc_funct.o sc_scc.o sc_tools.o sc_util.o
COPTS=-O
CFLAGS=$(COPTS) -I$(INCLUDE) -DSUN2COMPILE

all:		scsi.diag

scsi.diag:	$(LIB)/stack.o $(OFILES) $(LIB)/libdiag.a
		ld -T 4000 -o scsi.diag $(LIB)/stack.o $(OFILES) \
        		$(LIB)/libdiag.a -lc 
clean:
	        rm -f scsi.diag *.o errs

install:
		install -s scsi.diag $(DESTDIR)/usr/stand

scsi.o: 	scsi.c sc_reg.h 
sc_tests.o:	sc_tests.c sc_reg.h  
sc_funct.o:	sc_reg.h
			cc -I$(INCLUDE) -DSUN2COMPILE -c sc_funct.c
sc_scc.o :	sc_scc.c sc_reg.h 
sc_tools.o:	sc_reg.h
			cc -I$(INCLUDE) -DSUN2COMPILE -c sc_tools.c 
sc_util.o:	sc_util.c sc_reg.h sc_util.h
