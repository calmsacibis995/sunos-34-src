#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 4.27 83/07/06
#
DESTDIR=
CFLAGS=	-O

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	

# Files that need only be installed and are never removed.
#
FILES=	sky.ucode

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	skyrc skyversion	

# Programs that must run setuid to root
#
SETUID=

# C programs that live in the current directory and need explicit make lines.
#
NSTD=

# C programs that use the rpcsvc library
#
RPCSVC=	

all:	${SUBDIR} ${STD} ${NSTD} ${RPCSVC}

#${SUBDIR}: FRC
#	cd $@; make ${MFLAGS}

${STD}:
	cc ${CFLAGS} -o $@ $@.c

#${RPCSVC}:
#	cc ${CFLAGS} -o $@ $@.c -lrpcsvc

${SCRIPT} ${FILES}:

install: ${STD} ${NSTD} ${RPCSVC}
	-mkdir ${DESTDIR}/usr/etc && chown bin ${DESTDIR}/usr/etc && \
		chmod 755 ${DESTDIR}/usr/etc
#	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	for i in ${FILES}; do (install -c -m 644 $$i ${DESTDIR}/usr/lib/$$i); done
#	for i in ${SCRIPT}; do (install -c $$i.sh ${DESTDIR}/usr/etc/$$i); done
	for i in ${STD} ${NSTD} ${RPCSVC}; do \
		(install -s $$i ${DESTDIR}/usr/etc/$$i); done
#	rm -f ${DESTDIR}/usr/etc/quotaoff
#	for i in ${SETUID}; do (chown root ${DESTDIR}/usr/etc/$$i; \
		chmod 4755 ${DESTDIR}/usr/etc/$$i); done
#	ln ${DESTDIR}/usr/etc/quotaon ${DESTDIR}/usr/etc/quotaoff

clean:
	rm -f a.out core *.s *.o
#	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
	rm -f ${STD} ${NSTD} ${RPCSVC}

FRC:

depend:
	cat </dev/null >x.c
	for i in ${STD} ${NSTD} ${RPCSVC}; do \
		(echo $$i: $$i.c >>makedep; \
		/bin/grep '^#[ 	]*include' x.c $$i.c | sed \
			-e '/\.\.\/h/d' \
			-e '/\.\.\/ufs/d' \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.c//' >>makedep); done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep x.c
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# Files listed in ${NSTD} have explicit make lines given below.

# DO NOT DELETE THIS LINE -- make depend uses it

skyrc: skyrc.c
skyrc: /usr/include/stdio.h
skyrc: /usr/include/sys/file.h
skyrc: /usr/include/sys/mman.h
skyrc: /usr/include/sys/types.h
skyrc: /usr/include/sundev/skyreg.h
skyversion: skyversion.c
skyversion: /usr/include/stdio.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
