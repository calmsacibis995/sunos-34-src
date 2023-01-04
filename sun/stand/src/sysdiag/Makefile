#
#	Makefile for sysdiag, the system exerciser diagnostic
#
#     @(#) Makefile 1.1 9/25/86 1985 Sun Microsystems Inc.
#
ARCH=`arch`
DESTDIR=
COPTS=-O -D`arch` 
CFLAGS=$(COPTS)
FFLAGS=	-w

FPAOPTS=        -O -D$(ARCH) -DFPA -ffpa
68881OPTS=      -O -D$(ARCH) -DMC68881 -DMCNSOFT -f68881
SOFTOPTS=       -O -D$(ARCH) -DSOFT -DMCNSOFT -fsoft
XFPAOPTS=       -Dsun3 -ffpa -Dmc68020 -DTOKEN_BUF_SIZE=256
YFPAOPTS=		-Dsun2 -Dmc68010 -DTOKEN_BUF_SIZE=256

MAIN    = linsub
ROLL    =
FP      = ffpa
OUTPUT  = $(ROLL)$(FP)
F77     = f77 -u
DEFS    = -D$(ROLL)
OPTS    = -O
SRCS    = $(MAIN).F

cmd=	echo
opt=
file=	/dev/null
tail=
MLIB=           /usr/lib/libm.a /lib/libc.a
LIB=		../../lib
CLIBS=		/usr/lib/libsuntool.a /usr/lib/libsunwindow.a /usr/lib/libpixrect.a /lib/libc.a

OFILES=         S.$(OUTPUT).$(MAIN).o D.$(OUTPUT).$(MAIN).o linpack.o\
		fpa_systest.o


RCFILES=	.cshrc .login .suntools .suntools-ipc
SCRIPTS=	memtop disktop tapetop devtop dev setterm endt sysdiag \
		sysdiag.help nextlog ipctop options
BINARIES=	devtest dcptest skyprobe probe vmem pmem disk gpmtest sptest \
		reply c160 ipctest ffpusr softfp
BINARIES3=	fpatest mc68881
UCODE=          gpmtest.all.2p gpmtest.allbutgb.2p gpmtest.fifo_vme.2p \
		gpmtest.fifo_vme_dec.2p gpmtest.fpalu.2p \
		gpmtest.fpmult.2p gpmtest.fprega.2p gpmtest.fpregb.2p \
		gpmtest.gbnorm.2p gpmtest.gbrmw.2p gpmtest.int_flag.2p \
		gpmtest.pp_29116.2p gpmtest.ppfifo.2p gpmtest.ppprom.2p \
		gpmtest.scrpad.2p gpmtest.shmem.2p gpmtest.vme_byte.2p \
		gpmtest.vme_read.2p gpmtest.vme_read_byte.2p \
		gpmtest.vp_29116.2p gpmtest.vpprom.2p gpmtest.xoperand.2p \
		gpmtest.yoperand.2p
SOURCES=	devtest.c dcptest.c skyprobe.c probe.c vmem.c pmem.c disk.c \
		gpmtest.c sptest.c reply.c c160.c ipctest.c  \
		ffpusr.f ffpc.c ffpusr.inc \
		fpatest.c linpack.c fpa_systest.c linsub.F

TAPEFILES=	Makefile $(RCFILES) $(SCRIPTS) $(BINARIES) $(UCODE)

all:	$(TAPEFILES)
#	for i in ${SCRIPTS}; do (install $$i $$i.csh); done
#	for i in ${SCRIPTS}; do (install $$i.csh $$i); done
#	rm -f *.csh

clean:
	rm -f *.o errs $(BINARIES)

install:
	strip $(BINARIES3)
	strip $(BINARIES)
	/bin/csh -c "if (-e $(DESTDIR)/usr/diag/sysdiag) rm -rf $(DESTDIR)/usr/diag/sysdiag"
	/bin/csh -c "if (-e $(DESTDIR)/usr/diag/sysdiag == 0) mkdir $(DESTDIR)/usr/diag/sysdiag"
	chmod g+w $(DESTDIR)/usr/diag/sysdiag
	for i in ${RCFILES}; do (install $$i $(DESTDIR)/usr/diag/sysdiag; chmod g+w $$i); done
	for i in ${BINARIES}; do (install $$i $(DESTDIR)/usr/diag/sysdiag); done
	if [ $(ARCH) = "sun3" ]; then \
		for i in ${BINARIES3}; do \
			(install $$i $(DESTDIR)/usr/diag/sysdiag); \
		done \
	fi		
	for i in ${UCODE}; do (install $$i $(DESTDIR)/usr/diag/sysdiag); done
	for i in ${SCRIPTS}; do (install $$i $(DESTDIR)/usr/diag/sysdiag); done
	chmod g+w $(DESTDIR)/usr/diag/sysdiag/*

cmdall:	FoRCe
	$(cmd) $(opt) $(TAPEFILES)

cmdrc:	FoRCe
	$(cmd) $(opt) $(RCFILES)

cmdsrc:	FoRCe
	$(cmd) $(opt) $(SOURCES)

cmdscr:	FoRCe
	$(cmd) $(opt) $(SCRIPTS)

cmduc:	FoRCe
	$(cmd) $(opt) $(UCODE)

$(SCRIPTS):
	sccs get $@
	chmod +x $@

devtest:devtest.o
	cc $(CFLAGS) -o devtest devtest.o $(LIB)/libmem.a

probe:	probe.o
	cc $(CFLAGS) -o probe probe.o

skyprobe:skyprobe.o
	cc $(CFLAGS) -o skyprobe skyprobe.o

pmem:	pmem.o
	cc $(CFLAGS) -o pmem pmem.o

vmem:	vmem.o
	cc $(CFLAGS) -o vmem vmem.o $(LIB)/libmem.a

sptest:	sptest.o
	cc $(CFLAGS) -o sptest sptest.o 

reply:	reply.o
	cc $(CFLAGS) -o reply reply.o 

c160:	c160.o
	cc $(CFLAGS) -o c160 c160.o $(CLIBS)

disk:	disk.o
	cc $(CFLAGS) -o disk disk.o $(LIB)/libmem.a

gpmtest:gpmtest.o
	cc $(CFLAGS) -o gpmtest gpmtest.o -O

dcptest:dcptest.o
	cc $(CFLAGS) -o dcptest dcptest.o

ipctest:ipctest.o
	cc $(CFLAGS) -o ipctest ipctest.o

linpack.o:	linpack.c
	if [ $(ARCH) = "sun3" ]; then \
		cc $(XFPAOPTS) -c linpack.c; \
	fi
	if [ $(ARCH) = "sun2" ]; then \
		cc $(YFPAOPTS) -c linpack.c; \
	fi

S.$(OUTPUT).$(MAIN).o: $(SRCS)
	$(F77) -DS $(DEFS) $(OPTS) -$(FP) -c $(MAIN).F
	mv $(MAIN).o S.$(OUTPUT).$(MAIN).o

D.$(OUTPUT).$(MAIN).o: $(SRCS)
	$(F77) -DD $(DEFS) $(OPTS) -$(FP) -c $(MAIN).F
	mv $(MAIN).o D.$(OUTPUT).$(MAIN).o

fpa_systest.o:  fpa_systest.c
	@if [ $(ARCH) != "sun2" ]; then \
		cc $(XFPAOPTS) -c fpa_systest.c; \
	fi

fpatest:        fpatest.c $(OFILES)
	@if [ $(ARCH) != "sun2" ]; then \
		echo "TARGET operation is fpatest"; \
		cc $(FPAOPTS) -c fpatest.c; \
		cc $(FPAOPTS) -o fpatest fpatest.o $(OFILES) $(MLIB); \
		size fpatest; \
	fi

mc68881:        fpatest.c 
	@if [ $(ARCH) != "sun2" ]; then \
		echo "TARGET operation is mc68881"; \
		cc $(68881OPTS) -c fpatest.c; \
		cc $(68881OPTS) -o mc68881 fpatest.o $(MLIB); \
		size mc68881; \
	fi

softfp: fpatest.c
	@echo "TARGET operation is softfp"
	cc $(SOFTOPTS) -c fpatest.c
	cc $(SOFTOPTS) -o softfp fpatest.o $(MLIB)
	size softfp

ffpusr: ffpusr.inc ffpusr.o ffpc.o
	f77 -w -o ffpusr ffpusr.o ffpc.o

aroff:
	tar cvf /dev/rar0 $(TAPEFILES)
stoff tapeoff:
	tar cvf /dev/rst0 $(TAPEFILES)
mtoff:
	tar cvf /dev/rmt0 $(TAPEFILES)
taroff:
	tar cvf $(file) $(TAPEFILES) $(tail)

FoRCe:	
