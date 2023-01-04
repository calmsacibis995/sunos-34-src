#
# @(#)Makefile 1.3 87/01/08 SMI; from UCB 4.2 10/25/82
#
DESTDIR=
MACHINES= vax sun

# Programs that live in subdirectories, and have makefiles of their own.
# Order here is critical for make install.
#
SUBDIR=	include lib usr.lib etc bin usr.etc usr.bin ucb 5include 5lib 5bin \
	adm files man pub sccs games sunpro

# Directories that must exist before make install
#
DIRS=	bin lib etc tmp dev mnt usr
USRDIRS= bin ucb etc include 5bin 5lib 5include adm tmp lib spool crash hosts sunpro

all:	${SUBDIR} ${MACHINES}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

${MACHINES}: FRC
	-if $@; then cd $@; make ${MFLAGS}; fi

install:
	-for i in ${DIRS}; do \
		mkdir ${DESTDIR}/$$i && chown bin ${DESTDIR}/$$i && \
		chmod 755 ${DESTDIR}/$$i; done
	-for i in ${USRDIRS}; do \
		mkdir ${DESTDIR}/usr/$$i && chown bin ${DESTDIR}/usr/$$i && \
		chmod 755 ${DESTDIR}/usr/$$i; done
	-chmod 777 ${DESTDIR}/tmp ${DESTDIR}/usr/tmp
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	-for i in ${MACHINES}; do \
		if $$i; then \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); \
		fi; done

clean:
	rm -f a.out core *.s *.o
	-for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
	-for i in ${MACHINES}; do \
		if $$i; then (cd $$i; make ${MFLAGS} clean); fi; done

FRC:
