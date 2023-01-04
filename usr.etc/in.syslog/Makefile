#
# @(#)Makefile 1.1 86/09/25 SMI
#
DESTDIR=
CFLAGS=	-O
DIRS=${DESTDIR}/usr/spool/log

in.syslog:	in.syslog.o cache.o
	${CC} -o in.syslog in.syslog.o cache.o

install: in.syslog ${DIRS}
	install -c -o daemon -g daemon -m 755 \
		in.syslog ${DESTDIR}/usr/etc/in.syslog
	install -c -o daemon -g daemon -m 660 \
		syslog.conf ${DESTDIR}/etc/syslog.conf
	install -c -o daemon -g daemon -m 660 \
		/dev/null ${DESTDIR}/etc/syslog.pid
	install -c -o daemon -g daemon -m 660 \
		newsyslog.sh ${DESTDIR}/usr/adm/newsyslog
	for i in '' .0 .1 .2 .3 .4 .5 .6 .7; do \
		install -c -o daemon -g daemon -m 660 \
			/dev/null ${DESTDIR}/usr/spool/log/syslog$$i; \
	done

${DESTDIR}/usr/spool/log:
	mkdir ${DESTDIR}/usr/spool/log
	/etc/chown daemon ${DESTDIR}/usr/spool/log
	chgrp daemon ${DESTDIR}/usr/spool/log
	chmod 770 ${DESTDIR}/usr/spool/log

clean:
	rm -f in.syslog *.o *.s errs core a.out
