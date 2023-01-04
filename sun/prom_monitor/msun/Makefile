#
# @(#)Makefile 4.13 86/01/27 SMI
#
DESTDIR=
CFLAGS=	-O

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	mon

all:	${SUBDIR}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

install:
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done

FRC:
