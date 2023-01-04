#       @(#)Makefile 1.1 9/25/86 Copyright Sun Microsystems
#
#       makefile for sun3 fparel  diagnostic sources
#
#       @(#)Makefile 1.1 2/8/85 SMI
#


MAIN 	= linsub
ROLL    =
FP 	= ffpa
OUTPUT 	= $(ROLL)$(FP)
F77 	= f77 -u
DEFS	= -D$(ROLL)
OPTS	= -O
SRCS 	= $(MAIN).F

COPTS=          -O
CFLAGS=         -Dsun3 -ffpa -Dmc68020 -DTOKEN_BUF_SIZE=256

OFILES=         memory.o reg_ram.o lock.o register.o simpleins.o \
		ust_reg.o wbranching.o wdp.o woperation.o wlwf.o \
		wstatus.o nack2.o wtiming.o contexts.o pointers.o \
		ptr_incdec.o winitfp.o fpa.systest.o linpack.o \
		S.$(OUTPUT).$(MAIN).o D.$(OUTPUT).$(MAIN).o

fparel:	$(OFILES)
	ld  -N -o fparel /lib/crt0.o /lib/Wcrt1.o $(OFILES) -lc

S.$(OUTPUT).$(MAIN).o: $(SRCS)
	$(F77) -DS $(DEFS) $(OPTS) -$(FP) -c $(MAIN).F
	mv $(MAIN).o S.$(OUTPUT).$(MAIN).o

D.$(OUTPUT).$(MAIN).o: $(SRCS)
	$(F77) -DD $(DEFS) $(OPTS) -$(FP) -c $(MAIN).F
	mv $(MAIN).o D.$(OUTPUT).$(MAIN).o


all:    fparel

fparel: $(OFILES) 
		ld  -N -o fparel /lib/crt0.o /lib/Wcrt1.o \
		$(OFILES) /lib/libc.a -lc


clean:
		rm -f *.o errs fparel

install:
		install -s fparel  $(DESTDIR)/usr/etc/fpa

.c.o:
		cc $(CFLAGS) -c $*.c

