#
# @(#)Makefile 1.1 86/09/24 SMI
#
DESTDIR=

# Files which live in the current directory and are copied to the destination.
#
FILES=	group hosts hosts.equiv networks passwd protocols \
	rc rc.boot rc.local rpc servers services ttys ttytype

# Files which should exist but be empty in the destination
#
EMPTY=	dumpdates motd mtab utmp

all:	${FILES}

${FILES}:

install:
	for i in ${FILES}; do (install -c -m 644 $$i ${DESTDIR}/etc/$$i); done
	for i in ${EMPTY}; do \
		install -c -m 644 /dev/null ${DESTDIR}/etc/$$i; done
#	window system kludge, sigh...
	chmod 666 ${DESTDIR}/etc/utmp
#	doesn't really belong here, but cron is here so...
	install -c -m 644 crontab ${DESTDIR}/usr/lib/crontab

clean:

FRC:
