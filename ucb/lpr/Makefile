#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 4.11 83/07/02
#
# makefile for line printer spooling system
#
# Ownerships {see note at the top of lpd.c}
# ROOT		a user that can access any and all files
# DAEMON	someone special
# SPGRP		the group id of the spooling programs
#
CFLAGS=	-O
LIBDIR=	/usr/lib
BINDIR=	/usr/ucb
SPOOLDIR=/usr/spool/lpd
CXREF=	/usr/ucb/ctags -x
ROOT=	root
DAEMON=	daemon
SPGRP=	daemon
DESTDIR=
SRCS=	lpd.c lpr.c lpq.c lprm.c pac.c lpd.c cmds.c cmdtab.c \
	printjob.c recvjob.c displayq.c rmjob.c \
	startdaemon.c common.c printcap.c lpdchar.c
ALL=	lpd lpr lpq lprm pac lpc

all:	${ALL} filters

lpd:	lpd.o printjob.o recvjob.o displayq.o rmjob.o
lpd:	startdaemon.o lpdchar.o common.o printcap.o
	${CC} -o lpd lpd.o printjob.o recvjob.o displayq.o rmjob.o \
		startdaemon.o lpdchar.o common.o printcap.o

lpr:	lpr.o startdaemon.o printcap.o
	${CC} -o lpr lpr.o startdaemon.o printcap.o

lpq:	lpq.o displayq.o common.o printcap.o
	${CC} -o lpq lpq.o displayq.o common.o printcap.o -ltermcap

lprm:	lprm.o rmjob.o startdaemon.o common.o printcap.o
	${CC} -o lprm lprm.o rmjob.o startdaemon.o common.o printcap.o

lpc:	lpc.o cmds.o cmdtab.o startdaemon.o common.o printcap.o
	${CC} -o lpc lpc.o cmds.o cmdtab.o startdaemon.o common.o printcap.o

pac:	pac.o printcap.o
	${CC} -o pac pac.o printcap.o

lpd.o lpr.o lpq.o lprm.o pac.o: lp.h lp.local.h
recvjob.o printjob.o displayq.o rmjob.o common.o: lp.h lp.local.h
startdaemon.o: lp.local.h
lpc.o cmdtab.o: lpc.h
cmds.o: lp.h lp.local.h

filters: FRC
	cd filters; make ${MFLAGS}

install: all
	install -s -g ${SPGRP} -m 6711 lpr ${DESTDIR}${BINDIR}/lpr
	install -s -g ${SPGRP} -m 6711 lpq ${DESTDIR}${BINDIR}/lpq
	install -s -g ${SPGRP} -m 6711 lprm ${DESTDIR}${BINDIR}/lprm
	install -s -g ${SPGRP} -m 6711 lpd ${DESTDIR}${LIBDIR}/lpd
	install -s -g ${SPGRP} -m 6711 lpc ${DESTDIR}/usr/etc/lpc
	install -s pac ${DESTDIR}/usr/etc/pac
	install -c -m 644 printcap.sample ${DESTDIR}/etc/printcap
	-mkdir ${DESTDIR}${SPOOLDIR} && \
		chown ${DAEMON} ${DESTDIR}${SPOOLDIR} && \
		chgrp ${SPGRP} ${DESTDIR}${SPOOLDIR} && \
		chmod 775 ${DESTDIR}${SPOOLDIR}
	cd filters; make ${MFLAGS} DESTDIR=${DESTDIR} install

clean:
	rm -f ${ALL} *.o a.out errs core
	cd filters; make ${MFLAGS} clean

print:
	@pr makefile
	@${CXREF} *.c | pr -h XREF
	@pr *.h *.c

FRC:
