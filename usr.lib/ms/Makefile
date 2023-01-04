#
# @(#)Makefile 1.1 86/09/25 SMI
#
DESTDIR=

# Files which live in the current directory and are copied to the destination.
#
FILES=	ms.acc ms.cov ms.eqn ms.ref ms.tbl ms.ths ms.toc

all:	${FILES}

${FILES}:

install:
	-mkdir ${DESTDIR}/usr/lib/ms && \
		chown bin ${DESTDIR}/usr/lib/ms && \
		chmod 755 ${DESTDIR}/usr/lib/ms
	for i in ${FILES}; do \
		(install -c -m 644 $$i ${DESTDIR}/usr/lib/ms/$$i); done

clean:

FRC:
