#
# @(#)Makefile 1.1 86/09/27 SMI
#
DESTDIR=
CFLAGS=	-O

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR= carrera m25 sirius prism

all:	${SUBDIR}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

install:
	-echo "do nothing here"

clean:
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done

FRC:
