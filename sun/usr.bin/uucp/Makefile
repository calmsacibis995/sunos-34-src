#	Makefile	1.1	86/09/25
########################################################################
#
# Common object files assembled into a library (uulib.a)
#
# Targets defined are:
#	all(default)	Make all UUCP commands
#	install		Install everything
#	cp		Make and install executables
#	cpdirs:
#	cpuucp		Install uucp program
#	cpuux		Install uux program
#	cpuuxqt		Install uuxqt program
#	cpuucico	Install uucico program
#	cpuulog		Install uulog program
#	cpuuclean	Install uuclean program
#	cpuuname	Install uuname program
#	cpuustat	Install uustat program
#	cpuusub		Install uusub program
#	clean		Remove all .o files and executables
#	get:
#	cmp:		all
#	uucp		Make uucp program
#	uux		Make uux program
#	uuxqt		Make uuxqt program
#	uucico		Make uucico program
#	uulog		Make uulogp program
#	uuclean		Make uucleanp program
#	uuname		Make uunamep program
#	uustat		Make uustatp program
#	uusub		Make uusubp program
#	mkdirs		Create necessary directories
#	lint		Run lint on everything
#	lintuucp	Run lint on uucp sources
#	lintuucico	Run lint on uucico sources
#	lintuux		Run lint on uux sources
#	lintuuxqt	Run lint on uuxqt sources
#	lintuulog	Run lint on uulog sources
#	lintuuclean	Run lint on uuclean sources
#	lintuuname	Run lint on uuname sources
#	lintuustat	Run lint on uustat sources
#	lintuusub	Run lint on uusub sources
#	tar		Make exportable 'tar' archive of all
#			necessary sources
#	tags		Make tags file
########################################################################

#
# Files to export with tar
SOURCES=*.c
HEADRS=	*.h
METOO=	Makefile CHANGES* README TODO lib/*

#	/*  uucp.mk 3.18  4/4/80  */
#	/*  11/45, 11/70, and VAX version ('-i' has no effect on VAX)	*/
#	/* for 11/23, 11/34 (without separate I/D), IFLAG= */
CC=cc
CFLAGS=-O -DVMUNIX -DUUDIR
LDFLAGS=
IFLAG=
#
# if "UUDIR" is defined in CFLAGS, also link in the "syskludge" library
#
KLUDGELIB=syskludge/syskludge.a
FRC=
OWNER=uucp
INSDIR=${DESTDIR}/usr/lib/uucp
BIN=${DESTDIR}/usr/bin
PUBDIR=${DESTDIR}/usr/spool/uucppublic
SPOOL=${DESTDIR}/usr/spool/uucp
XQTDIR=${DESTDIR}/usr/lib/uucp/.XQTDIR
OLDLOG=${DESTDIR}/usr/spool/uucp/OLD
PKON=pkon.o
IOCTL=
CLEAN=
LINTOP=-hbauc
COMMANDS=uucp uux uuxqt uucico uulog uuclean uuname uustat uusub
UCBCMDS=uuencode uudecode uusend
COMMLIB=uulib.a
# Common object files
COMMON =anlwrk.o anyread.o assert.o chkpth.o \
	cpmv.o expfile.o gename.o getargs.o getopt.o getprm.o \
	getpw.o getpwinfo.o gio.o gnamef.o gnxseq.o gwd.o imsg.o \
	index.o lastpart.o logent.o mailst.o pk0.o pk1.o prefix.o \
	sdmail.o setline.o shio.o sysacct.o systat.o ub_sst.o ulockf.o \
	us_crs.o us_open.o us_rrs.o us_sst.o uucpdefs.o uucpname.o \
	versys.o xqt.o cfgets.o
# Device dependent dialing routines
DIALERS=dndial.o dfdial.o vendial.o hayesdial.o
CDIALERS=dndial.c dfdial.c vendial.c hayesdial.c
#
# Should steal "make depend" from kernel makefile
#
# Object files which depend on "pk.h"
#
PKDEP=	gio.o pk0.o pk1.o
#
# Object files which depend on "uucp.h"
#
UUCPDEP=anlwrk.o anyread.o assert.o chkpth.o cico.o cntrl.o conn.o \
	cpmv.o expfile.o gename.o getpwinfo.o gio.o gnamef.o \
	gnsys.o gnxseq.o gwd.o imsg.o ioctl.o logent.o mailst.o \
	sdmail.o setline.o shio.o systat.o ub_sst.o ulockf.o us_crs.o \
	us_open.o us_rrs.o us_sst.o uuclean.o uucp.o uucpdefs.o \
	uucpname.o uulog.o uuname.o uustat.o uusub.o uux.o uuxqt.o \
	versys.o xqt.o dndial.o dfdial.o vendial.o hayesdial.o
#
# Object files which depend on "uust.h"
#
UUSTDEP=anlwrk.o cico.o cntrl.o us_crs.o us_open.o us_rrs.o us_sst.o \
	uucp.o uustat.o
#
# Ojbect files which depend on "uusub.h"
#
UUSUBDEP=cico.o ub_sst.o uusub.o
#
# Object files which depend on "uucpname.h"
#
UUCPNAMEDEP=uucpanem.o
#
# Object files which depend on "dial.h"
#
DIALDEP=conn.o dndial.o dfdial.o vendial.o hayesdial.o

LFILES=assert.c cpmv.c expfile.c gename.c getpwinfo.c index.c lastpart.c \
	prefix.c shio.c ulockf.c xqt.c
OUUCP=uucp.o $(COMMLIB)
LUUCP=uucpdefs.c uucp.c gwd.c chkpth.c getargs.c logent.c uucpname.c\
	versys.c us_crs.c us_open.c
OUUX=uux.o $(COMMLIB)
LUUX=uucpdefs.c uux.c gwd.c anyread.c chkpth.c getargs.c getprm.c\
	logent.c uucpname.c versys.c
OUUXQT=uuxqt.o $(COMMLIB)
LUUXQT=uucpdefs.c uuxqt.c mailst.c getprm.c gnamef.c logent.c uucpname.c \
	chkpth.c getargs.c anyread.c
OUUCICO=cico.o cntrl.o conn.o gnsys.o $(DIALERS) $(COMMLIB)
LUUCICO=uucpdefs.c cico.c cntrl.c conn.c pk0.c pk1.c gio.c anyread.c \
	anlwrk.c chkpth.c getargs.c gnamef.c gnsys.c gnxseq.c \
	imsg.c logent.c sysacct.c systat.c \
	mailst.c uucpname.c us_rrs.c us_sst.c us_open.c ub_sst.c setline.c \
	$(CDIALERS)
OUULOG=uulog.o $(COMMLIB)
LUULOG=uucpdefs.c uulog.c prefix.c xqt.c ulockf.c gnamef.c assert.c
OUUCLEAN=uuclean.o $(COMMLIB)
LUUCLEAN=uucpdefs.c uuclean.c gnamef.c prefix.c mailst.c getpwinfo.c\
	 getargs.c
OUUNAME=uuname.o $(COMMLIB)
LUUNAME=uuname.c uucpname.c uucpdefs.c
OUUSTAT=uustat.o $(COMMLIB)
LUUSTAT=uucpdefs.c uustat.c gnamef.c getpwinfo.c us_open.c getopt.c \
	lastpart.c cpmv.c
OUUSUB=uusub.o $(COMMLIB)
LUUSUB=uucpdefs.c uusub.c getpwinfo.c us_open.c xqt.c getopt.c

all:	$(COMMANDS) $(UCBCMDS)

$(COMMANDS):	$(COMMLIB)

$(COMMLIB):	$(COMMON)
	rm -f $@ ; ar cr $@ $(COMMON); ranlib $@

$(KLUDGELIB): FRC
	cd syskludge; make ${MFLAGS}

install:	mkdirs all cp $(CLEAN)

cp:	all cpdirs cpuucp cpuux cpuuxqt cpuucico cpuulog cpuuclean cpuuname \
	cpuustat cpuusub cpucb cplib

cpdirs:
	chown $(OWNER) $(INSDIR)
	chmod 755 $(INSDIR)

cpuucp:
	install -s -m 4111 -o $(OWNER) uucp $(BIN)

cpuux:
	install -s -m 4111 -o $(OWNER) uux $(BIN)

cpuuxqt:
	install -s -m 4111 -o $(OWNER) uuxqt $(INSDIR)

cpuucico:
	install -s -m 4111 -o $(OWNER) uucico $(INSDIR)

cpuulog:
	install -s -m 4111 -o $(OWNER) uulog $(BIN)

cpuuclean:
	install -s -m 4111 -o $(OWNER) uuclean $(INSDIR)

cpuuname:
	install -s -m 4111 -o $(OWNER) uuname $(BIN)

cpuustat:
	install -s -m 4111 -o $(OWNER) uustat $(BIN)

cpuusub:
	install -s -m 100 -o $(OWNER) uusub $(INSDIR)

cpucb:
	install -s -m 4111 -o $(OWNER) uuencode $(BIN)
	install -s -m 4111 -o $(OWNER) uudecode $(BIN)
	install -s -m 4111 -o $(OWNER) uusend $(BIN)

cplib:
	-for i in `ls lib | grep -v SCCS`; do \
		(cd lib; install -c -o uucp -m 600 $$i $(INSDIR)); \
	done
	-chmod 700 $(INSDIR)/uuck
	-chmod 4755 $(INSDIR)/uupoll
	-for i in C. D. D.noname OLD; do \
		mkdir $(SPOOL)/$$i; chown uucp $(SPOOL)/$$i; \
		chmod 711 $(SPOOL)/$$i; done
	-for i in LOGFILE SYSLOG; do \
		install -c -m 644 -o uucp /dev/null $(SPOOL)/$$i; done
	cp /dev/null $(PUBDIR)/.hushlogin

clean:
	-rm -f *.o core uulib.a $(COMMANDS) $(UCBCMDS)

cmp:	all
	cmp uucp $(BIN)
	rm uucp
	cmp uux $(BIN)
	rm uux
	cmp uuxqt $(INSDIR)/uuxqt
	rm uuxqt
	cmp uucico $(INSDIR)/uucico
	rm uucico
	cmp uulog $(BIN)
	rm uulog
	cmp uuclean $(INSDIR)/uuclean
	rm uuclean
	cmp uuname $(BIN)
	rm uuname
	cmp uustat $(BIN)
	rm uustat
	cmp uusub $(INSDIR)
	rm uusub
	rm *.o


uucp:	$(OUUCP) $(KLUDGELIB)
	$(CC) $(LDFLAGS) $(OUUCP)  $(KLUDGELIB) -o uucp

uux:	$(OUUX) $(KLUDGELIB)
	$(CC) $(LDFLAGS) $(OUUX)  $(KLUDGELIB) -o uux

uuxqt:	$(OUUXQT) $(KLUDGELIB)
	$(CC) $(LDFLAGS) $(OUUXQT)  $(KLUDGELIB) -o uuxqt

uucico:	$(OUUCICO)  $(IOCTL) $(PKON) $(KLUDGELIB)
	$(CC) $(IFLAG) $(LDFLAGS) $(OUUCICO)  $(IOCTL) $(PKON) $(KLUDGELIB)\
	 -o uucico

uulog:	$(OUULOG) $(KLUDGELIB)
	$(CC) $(LDFLAGS) $(OUULOG) $(KLUDGELIB) -o uulog

uuclean:  $(OUUCLEAN)
	$(CC) $(LDFLAGS) $(OUUCLEAN) -o uuclean

uuname:	$(OUUNAME)
	$(CC) $(LDFLAGS) $(OUUNAME) -o uuname
 
uustat:	$(OUUSTAT) $(KLUDGELIB)
	$(CC) $(LDFLAGS) $(OUUSTAT) $(KLUDGELIB) -o uustat

uusub:	$(OUUSUB) ${KLUDGELIB}
	$(CC) $(LDFLAGS) $(OUUSUB) $(KLUDGELIB) -o uusub

uuencode:	uuencode.o
	$(CC) $(LDFLAGS) uuencode.o -o uuencode

uudecode:	uudecode.o
	$(CC) $(LDFLAGS) uudecode.o -o uudecode

uusend:	uusend.o
	$(CC) $(LDFLAGS) uusend.o -o uusend


#
# Header file dependencies
#
$(PKDEP):	pk.h
$(UUCPDEP):	uucp.h
$(UUSTDEP):	uust.h
$(UUSUBDEP):	uusub.h
$(UUCPNAMEDEP):	uucpname.h
$(DIALDEP):	dial.h

FRC:

mkdirs:
	-mkdir $(INSDIR)
	chmod 755 $(INSDIR)
	-mkdir $(SPOOL)
	chmod 777 $(SPOOL)
	chown $(OWNER) $(SPOOL)
	-mkdir $(PUBDIR)
	chmod 777 $(PUBDIR)
	chown $(OWNER) $(PUBDIR)
	-mkdir $(XQTDIR)
	chmod 777 $(XQTDIR)
	chown $(OWNER) $(XQTDIR)
	-mkdir $(OLDLOG)
	chmod 777 $(OLDLOG)
	chown $(OWNER) $(OLDLOG)

lint:	lintuucp lintuucico lintuux lintuuxqt lintuulog lintuuclean\
	lintuuname lintuustat lintuusub
lintuucp:
	lint $(LINTOP) $(LUUCP) $(LFILES)

lintuucico:
	lint $(LINTOP) $(LUUCICO) $(LFILES)

lintuux:
	lint $(LINTOP) $(LUUX) $(LFILES)

lintuuxqt:
	lint $(LINTOP) $(LUUXQT) $(LFILES)

lintuulog:
	lint $(LINTOP) $(LUULOG)

lintuuclean:
	lint $(LINTOP) $(LUUCLEAN)

lintuuname:
	lint $(LINTOP) $(LUUNAME)

lintuustat:
	lint $(LINTOP) $(LUUSTAT) $(LFILES)

lintuusub:
	lint $(LINTOP) $(LUUSUB) $(LFILES)

# Make exportable 'tar' archive package
#
tar:
	tar cbf 1 uucp.tar $(METOO) $(SOURCES) $(HEADRS)

# Make tags file
#
tags:
	ctags $(HEADRS) $(SOURCES)
