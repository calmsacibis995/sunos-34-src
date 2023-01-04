#
# @(#)Makefile 1.1 86/09/25 Copyright (c) 1984 by Sun Microsystems, Inc.
#
SERVERCFILES= ypserv.c ypserv_ancil.c ypserv_map.c ypserv_proc.c

SERVEROBJS = ypserv.o ypserv_ancil.o ypserv_map.o ypserv_proc.o

SERVERDOTH =  ypsym.h /usr/include/rpcsvc/yp_prot.h \
/usr/include/rpcsvc/ypclnt.h /usr/include/rpcsvc/ypv1_prot.h

CFLAGS=-O
# CFLAGS = -g
LDFLAGS = -ldbm ../in.named/res/libresolv.a

all: ypserv

ypserv: ${SERVEROBJS}
	${CC} ${SERVEROBJS} -o ypserv ${LDFLAGS}

clean:
	rm -f ${SERVEROBJS} core

install:	ypserv
	install -s ypserv ${DESTDIR}/usr/etc/ypserv

lint: ${SERVERCFILES}
	lint ${SERVERCFILES}

depend:
	cat </dev/null >x.c
	cat </dev/null >makedep
	for i in ${SERVERCFILES}; do \
		(/bin/grep '^#[ 	]*include' x.c $$i | sed \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.c/.o/' >>makedep); done
	for i in ${SERVERDOTH}; do \
		(/bin/grep '^#[ 	]*include' x.c $$i | sed \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' >>makedep); done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep x.c
#
# DO NOT DELETE THIS LINE -- make depend uses it
#
ypserv.o: ypsym.h
ypserv.o: /usr/include/sys/ioctl.h
ypserv_domain.o: ypsym.h
ypserv_peer.o: ypsym.h
ypserv_timer.o: ypsym.h
ypserv_ancil.o: ypsym.h
ypserv_map.o: ypsym.h
ypserv_map.o: /usr/include/ctype.h
ypserv_proc.o: ypsym.h
ypserv_xfr.o: ypsym.h
ypserv_xfr.o: /usr/include/sys/file.h
ypsym.h: /usr/include/dbm.h
ypsym.h: /usr/include/stdio.h
ypsym.h: /usr/include/errno.h
ypsym.h: /usr/include/signal.h
ypsym.h: /usr/include/rpc/rpc.h
ypsym.h: /usr/include/sys/socket.h
ypsym.h: /usr/include/sys/stat.h
ypsym.h: /usr/include/sys/dir.h
ypsym.h: /usr/include/sys/time.h
ypsym.h: /usr/include/sys/wait.h
ypsym.h: /usr/include/rpcsvc/yp_prot.h
ypsym.h: /usr/include/rpcsvc/ypclnt.h
