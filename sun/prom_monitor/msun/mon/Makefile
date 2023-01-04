#
# @(#)Makefile 1.5 85/02/21 SMI
#
# Makefile for Sun ROM Monitor toplevel directory.
#
DESTDIR=

#
# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	sunconfig
OBJDIR= RevQ2 RevQv RevQk RevQs

all:
	for i in ${OBJDIR}; do (make ${MFLAGS} $$i) ; done

${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

${OBJDIR}: sunconfig
	-mkdir $@
	cd conf; ./sunconfig $@
	cd $@; make ${MFLAGS}

install:
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done

clean:
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done

FRC:
