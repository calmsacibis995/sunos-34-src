#
# @(#)Makefile 1.1 86/09/24 SMI
#
DESTDIR=

# Files which should exist but be empty in the destination
#
EMPTY=	wtmp shutdownlog messages lastlog msgbuf usracct

all:

install:
	-mkdir ${DESTDIR}/usr/adm && chown bin ${DESTDIR}/usr/adm && \
		chmod 755 ${DESTDIR}/usr/adm
	for i in ${EMPTY}; do (touch ${DESTDIR}/usr/adm/$$i); done

clean:
