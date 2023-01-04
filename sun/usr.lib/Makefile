#
# @(#)Makefile 1.3 87/01/13 SMI
#
DESTDIR=
CFLAGS=	-O

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	libcore libcore77 libcorepas libcorefp libg \
	libpixrect libsuntool libsunwindow libcgi libcgi77 \
	rasfilters bb_count iropt ilfiles

# Files that need only be installed and are never removed.
#
FILES=	

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	inline

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	

all:	${SUBDIR} ${STD} ${NSTD} gprof

${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

${STD}:
	cc ${CFLAGS} -o $@ $@.c

#${SCRIPT} ${FILES}:

gprof:
	for i in libsunwindow libsuntool; do \
		cd $$i; \
		make gprof; \
		cd ..; \
	done

install:
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
#	for i in ${FILES}; do (install -c -m 644 $$i ${DESTDIR}/usr/lib/$$i); done
#	for i in ${SCRIPT}; do (install -c $$i.sh ${DESTDIR}/usr/lib/$$i); done
	for i in ${STD} ${NSTD}; do (install -s $$i ${DESTDIR}/usr/lib/$$i); done

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
# DEPENDENCIES MUST END AT END OF FILE
inline: inline.c
inline: /usr/include/stdio.h
inline: /usr/include/ctype.h
inline: /usr/include/strings.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
