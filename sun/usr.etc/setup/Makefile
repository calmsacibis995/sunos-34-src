#
# @(#)Makefile 1.1 86/09/25 SMI
#
DESTDIR=
CFLAGS=	-O

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=runtime startup tty window

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=

# C programs that live in the current directory and need explicit make lines.
#
NSTD=

all:	${SUBDIR} ${STD} ${NSTD}

tags:	FRC
	rm -f tags
	for i in ${SUBDIR}; do \
		(cd $$i; rm -f tags; ctags -w ../$$i/*.[ch]; \
		 cat tags >>../tags; rm -f tags; ln -s ../tags tags); done
	sort tags -o tags
 
${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

#${STD}:
#	cc ${CFLAGS} -o $@ $@.c

install: ${STD} ${NSTD}
	-mkdir ${DESTDIR}/etc && chown bin ${DESTDIR}/etc && \
		chmod 755 ${DESTDIR}/etc
	-mkdir ${DESTDIR}/usr && chown bin ${DESTDIR}/usr && \
		chmod 755 ${DESTDIR}/usr
	-mkdir ${DESTDIR}/usr/etc && chown bin ${DESTDIR}/usr/etc && \
		chmod 755 ${DESTDIR}/usr/etc
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
#	for i in ${SCRIPT}; do (install -c $$i.sh ${DESTDIR}/usr/etc/$$i); done
#	for i in ${STD} ${NSTD}; do \
#		(install -s $$i ${DESTDIR}/usr/etc/$$i); done

clean:
#	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
#	rm -f ${STD} ${NSTD}

FRC:

depend:
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} depend); done
#	cat </dev/null >x.c
#	for i in ${STD} ${NSTD}; do \
#		(echo $$i: $$i.c >>makedep; \
#		/bin/grep '^#[ 	]*include' x.c $$i.c | sed \
#			-e '/\.\.\/h/d' \
#			-e '/\.\.\/ufs/d' \
#			-e 's,<\(.*\)>,"/usr/include/\1",' \
#			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
#			-e 's/\.c//' >>makedep); done
#	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
#	echo '$$r makedep' >>eddep
#	echo 'w' >>eddep
#	cp Makefile Makefile.bak
#	ed - Makefile < eddep
#	rm eddep makedep x.c
#	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
#	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
#	echo '# see make depend above' >> Makefile

# Files listed in ${NSTD} have explicit make lines given below.

# DO NOT DELETE THIS LINE -- make depend uses it

# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
