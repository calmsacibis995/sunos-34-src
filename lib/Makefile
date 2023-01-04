#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 4.2 11/22/82
#
DESTDIR=
CFLAGS=	-O

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	cpp libc compile

all:	${SUBDIR}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

install:
	-mkdir ${DESTDIR}/lib && chown bin ${DESTDIR}/lib && \
		chmod 755 ${DESTDIR}/lib
	-mkdir ${DESTDIR}/usr/5lib && chown bin ${DESTDIR}/usr/5lib && \
		chmod 755 ${DESTDIR}/usr/5lib
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done

FRC:
