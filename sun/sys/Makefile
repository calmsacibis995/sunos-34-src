#
# @(#)Makefile 1.1 86/09/25 SMI
#
DESTDIR=

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	GENERIC stand dist conf adb kadb

all:	${SUBDIR} LINT

${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

LINT: FRC
	cd $@; make ${MFLAGS} lint | 2>&1 tee lint.errs

install:
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR} LINT; do (cd $$i; make ${MFLAGS} clean); done

FRC:
