#
# @(#)Makefile 1.1 86/09/25 SMI
#
DESTDIR=

# Files which live in the current directory and are copied to the destination.
#
FILES=	acm.me chars.me deltext.me eqn.me float.me footnote.me \
	index.me local.me null.me refer.me revisions sh.me tbl.me thesis.me

all:	${FILES}

${FILES}:

install:
	-mkdir ${DESTDIR}/usr/lib/me && chown bin ${DESTDIR}/usr/lib/me && \
		chmod 755 ${DESTDIR}/usr/lib/me
	for i in ${FILES}; do \
		(install -c -m 644 $$i ${DESTDIR}/usr/lib/me/$$i); done

clean:

FRC:
