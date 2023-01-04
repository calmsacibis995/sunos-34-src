#
# @(#)Makefile 1.1 9/25/86 Sun Microsystems Inc.
#
DESTDIR=
CFLAGS=	-O

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	loopback.diag sysdiag vid.pat diag standamong
#	color.diag video120.diag vmad.diag mem.diag sc.diag scsi.diag sky.diag 

all:	${SUBDIR}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

install:
	-mkdir ${DESTDIR}/usr && chmod 755 ${DESTDIR}/usr && \
		chown bin ${DESTDIR}/usr
	-mkdir ${DESTDIR}/usr/diag && chmod 755 ${DESTDIR}/usr/diag && \
		chown bin ${DESTDIR}/usr/diag
	-mkdir ${DESTDIR}/usr/stand && chmod 755 ${DESTDIR}/stand && \
		chown bin ${DESTDIR}/usr/stand
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	cp extract_diags ${DESTDIR}/usr/stand/extract_diags
	chown bin ${DESTDIR}/usr/stand/extract_diags
	chmod +x ${DESTDIR}/usr/stand/extract_diags
clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do (echo $$i; cd $$i; make ${MFLAGS} clean); done

FRC:

