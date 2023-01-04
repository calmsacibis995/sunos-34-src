#
# @(#)Makefile 1.4 85/06/28 Copyright Sun Microsystems, Inc.
#
DESTDIR=
DESTHOST=
CFLAGS=	-O

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	lib src
RELDIR= include doc $(SUBDIR)

all:	${SUBDIR}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

install:
	-mkdir ${DESTDIR}/usr && chmod 755 ${DESTDIR}/usr && \
		chown bin ${DESTDIR}/usr
	-mkdir ${DESTDIR}/usr/diag && chmod 755 ${DESTDIR}/usr/diag && \
		chown bin ${DESTDIR}/usr/diag
	-mkdir ${DESTDIR}/usr/stand && chmod 755 ${DESTDIR}/usr/stand && \
		chown bin ${DESTDIR}/usr/stand
	-mkdir ${DESTDIR}/stand && chmod 755 ${DESTDIR}/stand && \
		chown bin ${DESTDIR}/stand	
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done

release:
	for i in ${RELDIR}; do \
		(rcp -r $$i $(DESTHOST):/usr/src/sun/stand ); done
	rcp Makefile $(DESTHOST):/usr/src/sun/stand
	rcp README $(DESTHOST):/usr/src/sun/stand

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done

FRC:
