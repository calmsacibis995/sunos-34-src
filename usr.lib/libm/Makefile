#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 4.1 12/25/82
#

.SUFFIXES:      .cf .cs .cm .cw .ff .fs .fw

AS=/bin/as
ASFLAGS= -ten
CPP=/lib/cpp
LD=/bin/ld

CFLAGS=-O -DIEEE -fsingle

F77 = /usr/bin/f77
FFLAGS=	-U -u -O -DIEEE
FF =         -fsoft	/usr/lib/fsoft.il
FS = -m68010 -fsky	/usr/lib/fsky.il
FM =         -f68881	/usr/lib/f68881.il
FV =         -fswitch	/usr/lib/fswitch.il
FW = -m68020 -ffpa	/usr/lib/ffpa.il
CC =/bin/cc

INSTALL= install
SUNLIB=	../../lib/libc/sun/crt

CSOURCES = \
	asin.c atan.c exp.c hypot.c \
	log.c matherr.c pow.c sin.c sinh.c sqrt.c tan.c \
	gamma.c j0.c j1.c jn.c erf.c utilities.c \
	acosh.c asinh.c atanh.c exp__E.c lgamma.c log__L.c \
	cbrt.c

CFSOURCES = \
        CFatans.cf CFpows.cf \
        CFexpd.cf CFpowd.cf CFlogd.cf  CFsinhd.cf CFtanhd.cf \
        CFasind.cf CFatand.cf

CSSOURCES = \
        CSpows.cs \
        CSexpd.cs CSpowd.cs CSlogd.cs  CSsinhd.cs CStanhd.cs \
        CSasind.cs CSatand.cs

CMSOURCES =

CWSOURCES =

FSOURCES =

FFSOURCES = \
        FFexps.ff FFlogs.ff \
	FFpow2s.ff FFpow10s.ff FFlog10s.ff \
        FFtrigs.ff FFasins.ff FFacoss.ff FFsinhs.ff FFtanhs.ff \
        FFtrigd.ff FFpow2d.ff FFpow10d.ff FFlog10d.ff

FSSOURCES = \
        FSexp1s.fs FSlog1s.fs FSpow2s.fs FSpow10s.fs FSlog10s.fs\
        FSasins.fs FSacoss.fs FSsinhs.fs FStanhs.fs \
        FStrigd.fs FSpow2d.fs FSpow10d.fs FSlog10d.fs

FWSOURCES =

SSOURCES = \
        Vexps.s Vexp1s.s Vpow10s.s Vpow2s.s Vpows.s \
        Vlogs.s Vlog1s.s Vlog10s.s Vlog2s.s Vsinhs.s Vtanhs.s \
        Vsins.s Vcoss.s Vtans.s Vasins.s Vacoss.s Vatans.s Vatan2s.s \
        Fexps.s Fpows.s Flogs.s \
        Ftrigs.s Fatans.s Fatan2s.s \
        Fpow2s.s Fpow10s.s Flog10s.s \
        Fasins.s Facoss.s Fsinhs.s Ftanhs.s \
        Sexp1s.s Slog1s.s Spow2s.s Spow10s.s Slog10s.s \
        Sasins.s Sacoss.s Satan2s.s Ssinhs.s Stanhs.s \
        Sfuncs.s \
        Mpows.s Matan2s.s Mfuncs.s \
	Watan2s.s Wfuncs.s \
        CopySign.s \
        Cfunc.s Cfloor.s Cceil.s Ccabs.s Ctanh.s \
        Ccopysign.s Cdrem.s Cexpm1.s Cfinite.s Clog1p.s Clogb.s \
	Crint.s Cscalb.s \
	Vexpd.s Vexp1d.s Vpow10d.s Vpow2d.s Vpowd.s \
        Vlogd.s Vlog1d.s Vlog10d.s Vlog2d.s Vsinhd.s Vtanhd.s \
        Vsind.s Vcosd.s Vtand.s Vasind.s Vacosd.s Vatand.s Vatan2d.s \
        Fexpd.s Fexp1d.s Fpow10d.s Fpow2d.s Fpowd.s \
        Flogd.s Flog1d.s Flog10d.s Fsinhd.s Ftanhd.s \
        Ftrigd.s Fasind.s Facosd.s Fatand.s Fatan2d.s \
        Flibm.s \
        Sexpd.s Sexp1d.s Spow10d.s Spow2d.s Spowd.s \
        Slogd.s Slog1d.s Slog10d.s Ssinhd.s Stanhd.s \
        Strigd.s Sasind.s Sacosd.s Satand.s Satan2d.s \
        Mpowd.s Matan2d.s Mfuncd.s \
        Wfuncd.s \
        Watan2d.s \
        Fscales.s Fscaled.s \
	pow_support.s

FILES=  \
        Vexps.o Vexp1s.o Vpow10s.o Vpow2s.o Vpows.o \
        Vlogs.o Vlog1s.o Vlog10s.o Vlog2s.o Vsinhs.o Vtanhs.o \
        Vsins.o Vcoss.o Vtans.o Vasins.o Vacoss.o Vatans.o Vatan2s.o \
        Fexps.o Fpows.o Flogs.o \
        Ftrigs.o FFtrigs.o Fatans.o Fatan2s.o \
        Fpow2s.o Fpow10s.o Flog10s.o \
        Fasins.o Facoss.o Fsinhs.o Ftanhs.o \
        CFatans.o CFpows.o \
        FFexps.o FFlogs.o \
        FFpow2s.o FFpow10s.o FFlog10s.o \
        FFasins.o FFacoss.o FFsinhs.o FFtanhs.o \
        Sexp1s.o Slog1s.o Spow2s.o Spow10s.o Slog10s.o \
        Sasins.o Sacoss.o Satan2s.o Ssinhs.o Stanhs.o \
        Sfuncs.o \
        CSpows.o \
        FSexp1s.o FSlog1s.o FSpow2s.o FSpow10s.o FSlog10s.o \
        FSasins.o FSacoss.o FSsinhs.o FStanhs.o \
        Mpows.o Matan2s.o Mfuncs.o \
	Watan2s.o Wfuncs.o \
        CopySign.o \
	asin.o atan.o exp.o hypot.o \
	log.o matherr.o pow.o sin.o sinh.o sqrt.o tan.o \
	jn.o j0.o j1.o gamma.o erf.o \
	acosh.o asinh.o atanh.o exp__E.o lgamma.o log__L.o \
	cbrt.o \
        Cfunc.o Cfloor.o Cceil.o Ccabs.o Ctanh.o \
        Ccopysign.o Cdrem.o Cexpm1.o Cfinite.o Clog1p.o Clogb.o \
	Crint.o Cscalb.o \
        Vexpd.o Vexp1d.o Vpow10d.o Vpow2d.o Vpowd.o \
        Vlogd.o Vlog1d.o Vlog10d.o Vlog2d.o Vsinhd.o Vtanhd.o \
        Vsind.o Vcosd.o Vtand.o Vasind.o Vacosd.o Vatand.o Vatan2d.o \
        Fexpd.o Fexp1d.o Fpow10d.o Fpow2d.o Fpowd.o \
        Flogd.o Flog1d.o Flog10d.o Fsinhd.o Ftanhd.o \
        Ftrigd.o FFtrigd.o Fasind.o Facosd.o Fatand.o Fatan2d.o \
        Flibm.o CFexpd.o CFpowd.o CFlogd.o CFsinhd.o CFtanhd.o \
        CFasind.o CFatand.o \
        FFpow2d.o FFpow10d.o FFlog10d.o \
        Sexpd.o Sexp1d.o Spow10d.o Spow2d.o Spowd.o \
        Slogd.o Slog1d.o Slog10d.o Ssinhd.o Stanhd.o \
        Strigd.o FStrigd.o Sasind.o Sacosd.o Satand.o Satan2d.o \
        CSexpd.o CSpowd.o CSlogd.o CSsinhd.o CStanhd.o \
        CSasind.o CSatand.o \
        FSpow2d.o FSpow10d.o FSlog10d.o \
        Mpowd.o Matan2d.o Mfuncd.o \
        Wfuncd.o \
        Watan2d.o \
        Fscales.o Fscaled.o \
	pow_support.o utilities.o

libm.a libm_p.a : $(FILES)
	rm -f libm_p.a
	cd profiled; ar cru ../libm_p.a $(FILES)
	ranlib libm_p.a
	rm -f libm.a
	ar cru libm.a $(FILES)
	ranlib libm.a

Watan2d.o Wfuncd.o :
	$(CPP) -DPROF -I$(SUNLIB) $*.s > tmp.pp
	${AS} -20 -ten tmp.pp -o tmp.o
	${LD} -x -r tmp.o
	mv a.out profiled/$*.o
	$(CPP) -I$(SUNLIB) $*.s > tmp.pp
	${AS} -20 -ten tmp.pp -o tmp.o
	${LD} -x -r tmp.o
	mv a.out $*.o

.s.o:
	$(CPP) -DPROF -I$(SUNLIB) $*.s > tmp.pp
	${AS} ${ASFLAGS} tmp.pp -o tmp.o
	${LD} -x -r tmp.o
	mv a.out profiled/$*.o 
	$(CPP) -I$(SUNLIB) $*.s > tmp.pp
	${AS} ${ASFLAGS} tmp.pp -o tmp.o
	${LD} -x -r tmp.o
	mv a.out $*.o

.c.o:	 
	rm -f tmp.c
	cp $*.c tmp.c
	${CC} -p ${CFLAGS} $(FV) -S -I$(SUNLIB) tmp.c
	$(AS) $(ASFLAGS) tmp.s -o tmp.o
	-$(LD) -X -r tmp.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} $(FV) -S -I$(SUNLIB) tmp.c
	$(AS) $(ASFLAGS) tmp.s -o tmp.o
	-$(LD) -x -r tmp.o
	mv a.out $*.o

.f.o:  $*.f
	$(F77) -p ${FFLAGS} $(FF) -S $*.f
	$(AS) $(ASFLAGS) $*.s -o tmp.o
	rm $*.s
	-$(LD) -X -r tmp.o
	mv a.out profiled/$*.o
	$(F77) ${FFLAGS} $(FF) -S $*.f
	$(AS) $(ASFLAGS) $*.s -o tmp.o
	rm $*.s
	-$(LD) -x -r tmp.o
	mv a.out $*.o

.cf.o:  $*.cf libmdefs.h
	rm -f tmp.c
	cp $*.cf tmp.c
	${CC} -p ${CFLAGS} $(FF) -c -S tmp.c
	$(AS) $(ASFLAGS) tmp.s -o tmp.o
	-$(LD) -X -r tmp.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} $(FF) -c -S tmp.c
	$(AS) $(ASFLAGS) tmp.s -o tmp.o
	-$(LD) -x -r tmp.o
	mv a.out $*.o

.cs.o:  $*.cs libmdefs.h Sdefs.h
	rm -f tmp.c
	cp $*.cs tmp.c
	${CC} -p ${CFLAGS} $(FS) -c -S tmp.c
	sed /fsky_used/d tmp.s > tmp2.s
	$(AS) $(ASFLAGS) -10 tmp2.s -o tmp.o
	-$(LD) -X -r tmp.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} $(FS) -c -S tmp.c
	sed /fsky_used/d tmp.s > tmp2.s
	$(AS) $(ASFLAGS) -10 tmp2.s -o tmp.o
	-$(LD) -x -r tmp.o
	mv a.out $*.o

.cm.o:  $*.cm libmdefs.h Mdefs.h
	rm -f tmp.c
	cp $*.cm tmp.c
	${CC} -p ${CFLAGS} $(FM) -c -S tmp.c
	sed /f68881_used/d tmp.s > tmp2.s
	$(AS) $(ASFLAGS) tmp2.s -o tmp.o
	-$(LD) -X -r tmp.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} $(FM) -c -S tmp.c
	sed /f68881_used/d tmp.s > tmp2.s
	$(AS) $(ASFLAGS) tmp2.s -o tmp.o
	-$(LD) -x -r tmp.o
	mv a.out $*.o

.cw.o:  $*.cw libmdefs.h Wdefs.h
	rm -f tmp.c
	cp $*.cw tmp.c
	${CC} -p ${CFLAGS} $(FW) -c -S tmp.c
	sed /ffpa_used/d tmp.s > tmp2.s
	$(AS) $(ASFLAGS) -20 tmp2.s -o tmp.o
	-$(LD) -X -r tmp.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} $(FW) -c -S tmp.c
	sed /ffpa_used/d tmp.s > tmp2.s
	$(AS) $(ASFLAGS) -20 tmp2.s -o tmp.o
	-$(LD) -x -r tmp.o
	mv a.out $*.o

.ff.o:  $*.ff
	rm -f tmp.f
	cp $*.ff tmp.f
	$(F77) -p ${FFLAGS} $(FF) -c -S tmp.f
	$(AS) $(ASFLAGS) tmp.s -o tmp.o
	-$(LD) -X -r tmp.o
	mv a.out profiled/$*.o
	$(F77) ${FFLAGS} $(FF) -c -S tmp.f
	$(AS) $(ASFLAGS) tmp.s -o tmp.o
	-$(LD) -x -r tmp.o
	mv a.out $*.o

.fs.o:  $*.fs
	rm -f tmp.f
	cp $*.fs tmp.f
	$(F77) -p ${FFLAGS} $(FS) -c -S tmp.f
	sed /fsky_used/d tmp.s > tmp2.s
	$(AS) $(ASFLAGS) -10 tmp2.s -o tmp.o
	-$(LD) -X -r tmp.o
	mv a.out profiled/$*.o
	$(F77) ${FFLAGS} $(FS) -c -S tmp.f
	sed /fsky_used/d tmp.s > tmp2.s
	$(AS) $(ASFLAGS) -10 tmp2.s -o tmp.o
	-$(LD) -x -r tmp.o
	mv a.out $*.o

.fw.o:  $*.fw
	rm -f tmp.f
	cp $*.fw tmp.f
	$(F77) -p ${FFLAGS} $(FW) -c -S tmp.f
	sed /ffpa_used/d tmp.s > tmp2.s
	$(AS) $(ASFLAGS) -20 tmp2.s -o tmp.o
	-$(LD) -X -r tmp.o
	mv a.out profiled/$*.o
	$(F77) ${FFLAGS} $(FW) -c -S tmp.f
	sed /ffpa_used/d tmp.s > tmp2.s
	$(AS) $(ASFLAGS) -20 tmp2.s -o tmp.o
	-$(LD) -x -r tmp.o
	mv a.out $*.o

install: libm.a libm_p.a
	${INSTALL} -c -m 644 libm.a ${DESTDIR}/usr/lib
	ranlib ${DESTDIR}/usr/lib/libm.a
	${INSTALL} -c -m 644 libm_p.a ${DESTDIR}/usr/lib
	ranlib ${DESTDIR}/usr/lib/libm_p.a

clean:
	rm -f *.o profiled/*.o libm.a libm_p.a [A-Z][A-Z]*.[fsc] tmp*
