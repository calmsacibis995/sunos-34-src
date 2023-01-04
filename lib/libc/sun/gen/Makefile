#
# @(#)Makefile 1.1 86/09/24 SMI
#
OBJS=	_setjmp.o \
	abs.o alloca.o atof.o \
	bcopy.o bzero.o \
	ecvt.o \
	fabs.o ffs.o \
	insque.o isinf.o \
	memcpy.o memset.o \
	q.o \
	remque.o \
	setjmp.o strcat.o strcpy.o strlen.o strncat.o strncpy.o strtod.o \
	unpkd.o upottab.o \
        Vaints.o Vanints.o Varints.o \
        Vrems.o Vmods.o Vfloors.o Vceils.o Vsqrts.o Vlength2s.o Vpowis.o \
        Faints.o Frems.o Fsqrts.o Fpowis.o Flength2s.o \
        Saints.o Srems.o Ssqrts.o Spowis.o Slength2s.o \
        Mgens.o Mpowis.o \
        Wgens.o Wpowis.o \
        frexp.o ldexp.o modf.o fmod.o \
        Vaintd.o Vanintd.o Varintd.o \
        Vremd.o Vmodd.o Vfloord.o Vceild.o Vsqrtd.o Vlength2d.o Vpowid.o \
        Faintd.o Fremd.o Fsqrtd.o Fpowid.o Flength2d.o \
        Saintd.o Sremd.o Ssqrtd.o Spowid.o Slength2d.o \
        Mgend.o Mpowid.o \
        Wgend.o Wpowid.o

CFLAGS=	-O -fsoft
CPP=/lib/cpp
AS=/bin/as
ASFLAGS= -ten
LD=/bin/ld
INCLUDE= -I.. -I../crt

genlib genlib_p genlibs5 genlibs5_p: ${OBJS}
	@echo "building profiled Sys5 genlib"
	@cd profiled-sys5; ar cru ../genlibs5_p ${OBJS}
	@echo "building normal Sys5 genlib"
	@cd sys5; ar cru ../genlibs5 ${OBJS}
	@echo "building profiled genlib"
	@cd profiled; ar cru ../genlib_p ${OBJS}
	@echo "building normal genlib"
	@ar cru genlib ${OBJS}

Wpowis.o Wgens.o Wpowid.o Wgend.o: Wpowis.s Wgens.s Wpowid.s Wgend.s \
	        ../crt/fpcrtdefs.h ../crt/Wdefs.h
	        $(CPP) -DPROF -DS5EMUL -I/usr/5include $(INCLUDE) $*.s >$*.pp
	        $(AS) -20 -ten $*.pp -o $*.o
	        $(LD) -X -r $*.o
	        mv a.out profiled-sys5/$*.o; rm $*.pp
	        $(CPP) -DS5EMUL -I/usr/5include $(INCLUDE) $*.s >$*.pp
	        $(AS) -20 -ten $*.pp -o $*.o
	        $(LD) -X -r $*.o
	        mv a.out sys5/$*.o; rm $*.pp
	        $(CPP) -DPROF $(INCLUDE) $*.s >$*.pp
	        $(AS) -20 -ten $*.pp -o $*.o
	        $(LD) -X -r $*.o
	        mv a.out profiled/$*.o; rm $*.pp
	        $(CPP) $(INCLUDE) $*.s >$*.pp
	        $(AS) -20 -ten $*.pp -o $*.o
	        $(LD) -X -r $*.o
	        mv a.out $*.o; rm $*.pp

.c.o:
	$(CC) -p -c $(CFLAGS) -DS5EMUL $(INCLUDE) -I/usr/5include $*.c
	$(LD) -x -r $*.o
	mv a.out profiled-sys5/$*.o
	$(CC) -c $(CFLAGS) -DS5EMUL $(INCLUDE) -I/usr/5include $*.c
	$(LD) -x -r $*.o
	mv a.out sys5/$*.o
	$(CC) -p -c $(CFLAGS) $(INCLUDE) $*.c
	$(LD) -x -r $*.o
	mv a.out profiled/$*.o
	$(CC) -c $(CFLAGS) $(INCLUDE) $*.c
	$(LD) -x -r $*.o
	mv a.out $*.o

.s.o:
	$(CPP) -DPROF $(INCLUDE) -DS5EMUL -I/usr/5include $*.s >$*.pp
	$(AS) $(ASFLAGS) $*.pp -o $*.o
	$(LD) -x -r $*.o
	mv a.out profiled-sys5/$*.o; rm $*.pp
	$(CPP) $(INCLUDE) -DS5EMUL -I/usr/5include $*.s >$*.pp
	$(AS) $(ASFLAGS) $*.pp -o $*.o
	$(LD) -x -r $*.o
	mv a.out sys5/$*.o; rm $*.pp
	$(CPP) -DPROF $(INCLUDE) $*.s >$*.pp
	$(AS) $(ASFLAGS) $*.pp -o $*.o
	$(LD) -x -r $*.o
	mv a.out profiled/$*.o; rm $*.pp
	$(CPP) $(INCLUDE) $*.s >$*.pp
	$(AS) $(ASFLAGS) $*.pp -o $*.o
	$(LD) -x -r $*.o
	mv a.out $*.o; rm $*.pp

clean:
	rm -f genlib genlib_p genlibs5 genlibs5_p
	rm -f *.pp *.o profiled/*.o sys5/*.o profiled-sys5/*.o errs a.out core 

atof.o ecvt.o upottab.o:	unpkd.h
