#
# @(#)Makefile 1.4 87/01/14 SMI
#
DESTDIR=
CFLAGS=	-O

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	screendump des suntool rpcgen

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	sun3cvt click 

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	cpio screenblank

all:	${SUBDIR} ${STD} ${NSTD} gprof

${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

gprof: FRC
	cd suntool; make ${MFLAGS} gprof

${STD}:
	cc ${CFLAGS} -o $@ $@.c

install: ${STD} ${NSTD}
	-mkdir ${DESTDIR}/usr/bin && chown bin ${DESTDIR}/usr/bin && \
		chmod 755 ${DESTDIR}/usr/bin 
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
#	for i in ${SCRIPT}; do (install -c $$i.sh ${DESTDIR}/usr/bin/$$i); done
	for i in ${STD} ${NSTD}; do \
		(install -s $$i ${DESTDIR}/usr/bin/$$i); \
	done

clean:
	rm -f a.out y.tab.* core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
	rm -f ${STD} ${NSTD}

depend:
	cat </dev/null >x.c
	for i in ${STD} ${NSTD}; do \
		(echo $$i: $$i.c >>makedep; \
		/bin/grep '^#[ 	]*include' x.c $$i.c | \
		/bin/grep -v '/vax.*\.h' | sed \
			-e '/\.\.\/h/d' \
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

FRC:

# Files listed in ${NSTD} have explicit make lines given below.

cpio:	cpio.c
	cc -DVMUNIX ${CFLAGS} -o cpio cpio.c

screenblank: screenblank.c
	cc ${CFLAGS} -o screenblank screenblank.c -lm
# DO NOT DELETE THIS LINE -- make depend uses it
# DEPENDENCIES MUST END AT END OF FILE
sun3cvt: sun3cvt.c
sun3cvt: /usr/include/stdio.h
sun3cvt: /usr/include/sys/file.h
sun3cvt: /usr/include/a.out.h
cpio: cpio.c
cpio: /usr/include/stdio.h
cpio: /usr/include/signal.h
cpio: /usr/include/sys/types.h
cpio: /usr/include/sys/stat.h
cpio: /usr/include/sys/errno.h
cpio: /usr/include/pwd.h
click.c: /usr/include/sys/types.h
click.c: /usr/include/sys/time.h
click.c: /usr/include/sys/ioctl.h
click.c: /usr/include/sundev/kbio.h
click.c: /usr/include/sundev/kbd.h
click.c: /usr/include/stdio.h
screenblank.c: /usr/include/sys/types.h
screenblank.c: /usr/include/sys/time.h
screenblank.c: /usr/include/sys/stat.h
screenblank.c: /usr/include/sys/file.h
screenblank.c: /usr/include/sys/ioctl.h
screenblank.c: /usr/include/errno.h
screenblank.c: /usr/include/stdio.h
screenblank.c: /usr/include/sun/fbio.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
