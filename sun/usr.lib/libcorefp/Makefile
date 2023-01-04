#
# @(#)Makefile 1.1 86/09/27 SMI
#
DESTDIR=
CFLAGS=	-O

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	libcore68881 libcorefpa

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
