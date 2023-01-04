#
# @(#)Makefile 1.1 86/09/25 SMI
#

DESTDIR=
CFLAGS=	-O

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	libgp1 vp pp

# Files that need only be installed and are never removed.
#
FILES= gp1cg2.1152.ucode gp1cg2.1024.ucode

all:	${SUBDIR} ${FILES}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

gp1cg2.1024.ucode: vp/gp1cg2.1024.vp.o pp/gp1cg2.1024.pp.o
	rm -f gp1cg2.1024.ucode
	cat vp/gp1cg2.1024.vp.o pp/gp1cg2.1024.pp.o > gp1cg2.1024.ucode

gp1cg2.1152.ucode: vp/gp1cg2.1152.vp.o pp/gp1cg2.1152.pp.o
	rm -f gp1cg2.1152.ucode
	cat vp/gp1cg2.1152.vp.o pp/gp1cg2.1152.pp.o > gp1cg2.1152.ucode

install:
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	for i in ${FILES}; do (install -c -m 644 $$i ${DESTDIR}/usr/lib/$$i); done

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done

FRC:
