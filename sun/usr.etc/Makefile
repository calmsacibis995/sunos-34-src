#
# @(#)Makefile 1.1 86/09/25 SMI
#
DESTDIR=
CFLAGS=	-O

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	gpone setup upgrade
SUN2ONLY= sky
SUN3ONLY= eeprom fpa

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=extract_release

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	gpconfig mc68881version

all:	${SUBDIR} ${SUN2ONLY} ${SUN3ONLY} ${STD} ${NSTD}
 
${SUN2ONLY}: FRC
	if sun2; then cd $@; make ${MFLAGS}; fi

${SUN3ONLY}: FRC
	if sun3; then cd $@; make ${MFLAGS}; fi

${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

#${STD}:
#	cc ${CFLAGS} -o $@ $@.c

install: ${STD} ${NSTD}
	-mkdir ${DESTDIR}/usr/etc && chown bin ${DESTDIR}/usr/etc && \
		chmod 755 ${DESTDIR}/usr/etc
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	if sun2; then \
		for i in ${SUN2ONLY}; do \
			(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); \
		done; fi
	if sun3; then \
		for i in ${SUN3ONLY}; do \
			(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); \
		done; fi
	for i in ${SCRIPT}; do (install -c $$i.sh ${DESTDIR}/usr/etc/$$i); done
	for i in ${STD} ${NSTD}; do \
		(install -s $$i ${DESTDIR}/usr/etc/$$i); done

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
	rm -f ${STD} ${NSTD}

FRC:

depend:
	cat </dev/null >x.c
	for i in ${STD} ${NSTD}; do \
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
gpconfig: gpconfig.c ./gpone/libgp1/libgp1.a
	cc ${CFLAGS} -o gpconfig gpconfig.c ./gpone/libgp1/libgp1.a

mc68881version: mc68881version.c mc68881subroutine.s
	cc $(CFLAGS) -o mc68881version mc68881version.c mc68881subroutine.s

# DO NOT DELETE THIS LINE -- make depend uses it

gpconfig: gpconfig.c
gpconfig: /usr/include/pixrect/pixrect_hs.h
gpconfig: /usr/include/pixrect/gp1reg.h
gpconfig: /usr/include/sys/ioctl.h
gpconfig: /usr/include/sys/file.h
gpconfig: /usr/include/sun/fbio.h
gpconfig: /usr/include/sun/gpio.h
gpconfig: /usr/include/stdio.h
gpconfig: /usr/include/strings.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
