#
# @(#)Makefile 1.1 86/09/25 SMI
#
DESTDIR=

# Files which live in the current directory and are copied to the destination.
#
FILES=	tmac.a tmac.an tmac.cp tmac.e tmac.imagen tmac.os tmac.r \
	tmac.s tmac.scover tmac.sdisp tmac.skeep tmac.srefs tmac.vcat \
	tmac.sun tmac.indent

all:	${FILES}

${FILES}:

install:
	-mkdir ${DESTDIR}/usr/lib/tmac && \
		chown bin ${DESTDIR}/usr/lib/tmac && \
		chmod 755 ${DESTDIR}/usr/lib/tmac
	for i in ${FILES}; do \
		(install -c -m 644 $$i ${DESTDIR}/usr/lib/tmac/$$i); done

clean:

FRC:
