# @(#)Makefile	1.1	86/09/24	SMI
#
#	crt0	Normal C run time startoff
#	mcrt0	C run time start off for profiling, ``prof'' conventions
#	gcrt0	C run time start off for profiling, ``gprof'' conventions
#	f?crt0	C run time start off with sky board initialization.
#
LD= ld
CPP= /lib/cpp
SRCS=   crt0.s mcrt0.s gcrt0.s mon.c mcount.s gmon.c gmcount.s gmon.h \
	Fcrt1.s Scrt1.s Mcrt1.s Wcrt1.s
OBJS=	crt0.o mcrt0.o gcrt0.o mon.o gmon.o \
	Fcrt1.o Scrt1.o Mcrt1.o Wcrt1.o
CFLAGS= -O -m68010 -fsoft
ASFLAGS= -10
PRINT= print
INSTALL= install -m 644

all: ${OBJS}

install: ${OBJS}
	$(INSTALL)  crt0.o $(DESTDIR)/lib/crt0.o
	$(INSTALL) mcrt0.o $(DESTDIR)/lib/mcrt0.o
	$(INSTALL) gcrt0.o $(DESTDIR)/lib/gcrt0.o
	$(INSTALL)  sys5/crt0.o $(DESTDIR)/usr/5lib/crt0.o
	$(INSTALL) sys5/mcrt0.o $(DESTDIR)/usr/5lib/mcrt0.o
	$(INSTALL) sys5/gcrt0.o $(DESTDIR)/usr/5lib/gcrt0.o
	$(INSTALL) Fcrt1.o $(DESTDIR)/lib/Fcrt1.o
	$(INSTALL) Scrt1.o $(DESTDIR)/lib/Scrt1.o
	$(INSTALL) Mcrt1.o $(DESTDIR)/lib/Mcrt1.o
	$(INSTALL) Wcrt1.o $(DESTDIR)/lib/Wcrt1.o

crt0.o:	crt0.s
	$(CPP) crt0.s >tmp.s
	$(AS) $(ASFLAGS) -o crt0.o tmp.s && rm tmp.s
	$(LD) -x -r crt0.o
	mv a.out sys5/crt0.o
	$(CPP) -DOLD crt0.s >tmp.s
	$(AS) $(ASFLAGS) -o crt0.o tmp.s && rm tmp.s
	$(LD) -x -r crt0.o
	mv a.out crt0.o

mcrt0.o: mcrt0.s mon.o
	$(CPP) mcrt0.s >tmp.s
	$(AS) $(ASFLAGS) -o mcrt0.o tmp.s && rm tmp.s
	$(LD) -x -r mcrt0.o mon.o
	mv a.out sys5/mcrt0.o
	$(CPP) -DOLD mcrt0.s >tmp.s
	$(AS) $(ASFLAGS) -o mcrt0.o tmp.s && rm tmp.s
	$(LD) -x -r mcrt0.o mon.o
	mv a.out mcrt0.o

gcrt0.o: gcrt0.s gmon.o
	$(CPP) gcrt0.s >tmp.s
	$(AS) $(ASFLAGS) -o gcrt0.o tmp.s && rm tmp.s
	$(LD) -x -r gcrt0.o gmon.o
	mv a.out sys5/gcrt0.o
	$(CPP) -DOLD gcrt0.s >tmp.s
	$(AS) $(ASFLAGS) -o gcrt0.o tmp.s && rm tmp.s
	$(LD) -x -r gcrt0.o gmon.o
	mv a.out gcrt0.o

.s.o:
	$(CPP) $*.s >tmp.s
	$(AS) $(ASFLAGS) -o $*.o tmp.s && rm tmp.s
	$(LD) -x -r $*.o
	mv a.out $*.o
	
gmon.o: gmon.c gmon.h gmcount.s
	$(CC) -S $(CFLAGS) gmon.c && cat gmcount.s >>gmon.s
	$(AS) $(ASFLAGS) -o gmon.o gmon.s  && rm gmon.s
	$(LD) -x -r gmon.o && mv a.out gmon.o
	
mon.o: mon.c mon.h mcount.s
	$(CC) -S $(CFLAGS) mon.c && cat mcount.s >> mon.s
	$(AS) $(ASFLAGS) -o mon.o mon.s  && rm mon.s
	$(LD) -x -r mon.o && mv a.out mon.o
	
clean:
	rm -f ${OBJS}
print:
	$(PRINT) $(SRCS)
