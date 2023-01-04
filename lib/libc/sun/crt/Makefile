#
# @(#)Makefile 1.1 86/09/24 SMI
#
AS=/bin/as
ASFLAGS= -ten
CFLAGS= -O -fsoft
CPP=/lib/cpp
LD=/bin/ld
INCLUDE = -I..

OFILES =	lmodt.o ldivt.o lmult.o ptwo.o \
                fp_globals.o start_float.o float_switch.o V_switch.o \
                Vinit.o vinitfp.o Vswitch.o \
                Vmode.o Vstatus.o \
                Vdtos.o Vstod.o \
                Vflts.o Vuns.o Vints.o Vnints.o Vrints.o \
                Vadds.o Vsubs.o Vmuls.o Vdivs.o Vcmps.o \
                Vsqrs.o Vscaleis.o \
                Vfltd.o Vund.o Vintd.o Vnintd.o Vrintd.o \
                Vaddd.o Vsubd.o Vmuld.o Vdivd.o Vcmpd.o \
                Vsqrd.o Vscaleid.o \
                Finit.o finitfp.o Fswitch.o \
                Fclass2d.o Fclass2s.o \
                Ffloat.o Fdtos.o Fstod.o \
                Fflts.o Fadds.o Fmuls.o \
                Ffltd.o Faddd.o Fmuld.o \
                Sinit.o sinitfp.o Sswitch.o \
                Sfloat.o \
                Snints.o Sscaleis.o \
                Snintd.o Sscaleid.o \
                Minit.o Mdefault.o minitfp.o Mswitch.o \
                Mfloat.o \
                Winit.o winitfp.o Wswitch.o \
                Wfloat.o \
		is68020.o \
                fpa_handler.o fpa_handle.o fpa_81comp.o fpa_trans.o \
                fp_save.o rel1support.o rel2support.o

SRCFILES =	lmodt.s ldivt.s divide.include lmult.s ptwo.s

crtlib crtlib_p crtlibs5 crtlibs5_p: $(OFILES)
	rm -f crtlibs5_p
	cd profiled-sys5; ar cru ../crtlibs5_p $(OFILES)
	rm -f crtlibs5
	cd sys5; ar cru ../crtlibs5 $(OFILES)
	rm -f crtlib_p
	cd profiled; ar cru ../crtlib_p $(OFILES)
	rm -f crtlib
	ar cru crtlib $(OFILES)

fpa_81comp.o:   fpa_recompute.c fpa_support.h
	       $(CC) -p -DF68881 -S $(EMULATOR) -DS5EMUL -I/usr/5include -O -m68020 -f68881 fpa_recompute.c
	       sed /f68881_used/d fpa_recompute.s > tmp.s
	       $(AS) -20 -ten tmp.s -o fpa_81comp.o
	       $(LD) -x -r fpa_81comp.o
	       mv a.out profiled-sys5/fpa_81comp.o
	       $(CC) -DF68881 -S $(EMULATOR) -DS5EMUL -I/usr/5include -O -m68020 -f68881 fpa_recompute.c
	       sed /f68881_used/d fpa_recompute.s > tmp.s
	       $(AS) -20 -ten tmp.s -o fpa_81comp.o
	       $(LD) -x -r fpa_81comp.o
	       mv a.out sys5/fpa_81comp.o
	       $(CC) -p -DF68881 -S $(EMULATOR) -O -m68020 -f68881 fpa_recompute.c
	       sed /f68881_used/d fpa_recompute.s > tmp.s
	       $(AS) -20 -ten tmp.s -o fpa_81comp.o
	       $(LD) -x -r fpa_81comp.o
	       mv a.out profiled/fpa_81comp.o
	       $(CC) -DF68881 -S $(EMULATOR) -O -m68020 -f68881 fpa_recompute.c
	       sed /f68881_used/d fpa_recompute.s > tmp.s
	       $(AS) -20 -ten tmp.s -o fpa_81comp.o
	       $(LD) -x -r fpa_81comp.o
	       mv a.out fpa_81comp.o
 
Wfloat.o: Wfloat.s ../DEFS.h fpcrtdefs.h Wdefs.h Mdefs.h
	       $(CPP) $(EMULATOR) -DPROF $(INCLUDE) -DS5EMUL -I/usr/5include $*.s > $*.pp
	       ${AS} -20 -ten $*.pp -o $*.o
	       ${LD} -x -r $*.o
	       mv a.out profiled-sys5/$*.o ; rm $*.pp
	       $(CPP) $(EMULATOR) $(INCLUDE) -DS5EMUL -I/usr/5include $*.s > $*.pp
	       ${AS} -20 -ten $*.pp -o $*.o
	       ${LD} -x -r $*.o
	       mv a.out sys5/$*.o; rm $*.pp
	       $(CPP) $(EMULATOR) -DPROF $(INCLUDE) $*.s > $*.pp
	       ${AS} -20 -ten $*.pp -o $*.o
	       ${LD} -x -r $*.o
	       mv a.out profiled/$*.o ; rm $*.pp
	       $(CPP) $(EMULATOR) $(INCLUDE) $*.s > $*.pp
	       ${AS} -20 -ten $*.pp -o $*.o
	       ${LD} -x -r $*.o
	       mv a.out $*.o; rm $*.pp

.c.o:
	$(CC) -p -c $(EMULATOR) $(CFLAGS) -DS5EMUL -I/usr/5include $*.c
	$(LD) -x -r $*.o
	mv a.out profiled-sys5/$*.o
	$(CC) -c $(EMULATOR) $(CFLAGS) -DS5EMUL -I/usr/5include $*.c
	$(LD) -x -r $*.o
	mv a.out sys5/$*.o
	$(CC) -p -c $(EMULATOR) $(CFLAGS) $*.c
	$(LD) -x -r $*.o
	mv a.out profiled/$*.o
	$(CC) -c $(EMULATOR) $(CFLAGS) $*.c
	$(LD) -x -r $*.o
	mv a.out $*.o

.s.o:
	$(CPP) $(EMULATOR) -DPROF $(INCLUDE) -DS5EMUL -I/usr/5include $*.s > $*.pp
	${AS} ${ASFLAGS} $*.pp -o $*.o
	${LD} -x -r $*.o
	mv a.out profiled-sys5/$*.o ; rm $*.pp
	$(CPP) $(EMULATOR) $(INCLUDE) -DS5EMUL -I/usr/5include $*.s > $*.pp
	${AS} ${ASFLAGS} $*.pp -o $*.o
	${LD} -x -r $*.o
	mv a.out sys5/$*.o; rm $*.pp
	$(CPP) $(EMULATOR) -DPROF $(INCLUDE) $*.s > $*.pp
	${AS} ${ASFLAGS} $*.pp -o $*.o
	${LD} -x -r $*.o
	mv a.out profiled/$*.o ; rm $*.pp
	$(CPP) $(EMULATOR) $(INCLUDE) $*.s > $*.pp
	${AS} ${ASFLAGS} $*.pp -o $*.o
	${LD} -x -r $*.o
	mv a.out $*.o; rm $*.pp

clean:	
	rm -f crtlib crtlib_p crtlibs5 crtlibs5_p
	rm -f *.pp $(OFILES) core errs profiled/* sys5/* profiled-sys5/*

lmodt.o: lmodt.s divide.include
ldivt.o: ldivt.s divide.include
