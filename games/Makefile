#
# @(#)Makefile 1.3 87/01/20 SMI; from UCB 5.2 85/08/26
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
DESTDIR=
CFLAGS=	-O

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	adventure backgammon battlestar boggle chess ching cribbage fortune \
	hack hangman hunt monop quiz robots sail snake tool trek 

# Shell scripts that need only be installed and are never removed.
#
#SCRIPT=	

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	arithmetic btlgammon banner bcd bj cfscores craps factor \
	fish moo number random wump

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	canfield primes rain worm worms

all:	${SUBDIR} ${STD} ${NSTD}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

${STD}:
	${CC} ${CFLAGS} -o $@ $@.c

FRC:

install:
	-mkdir ${DESTDIR}/usr/games && chown bin ${DESTDIR}/usr/games && \
		chmod 755 ${DESTDIR}/usr/games
	-mkdir ${DESTDIR}/usr/games/lib && \
		chown bin ${DESTDIR}/usr/games/lib && \
		chmod 755 ${DESTDIR}/usr/games/lib
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
#	for i in ${SCRIPT}; do (install -c $$i ${DESTDIR}/usr/games/$$i); done
	for i in ${STD} ${NSTD}; do \
		(install -s $$i ${DESTDIR}/usr/games/$$i); \
	done
	install -c -m 644 backrules.lib ${DESTDIR}/usr/games/lib/backrules
	install -c -m 666 /dev/null ${DESTDIR}/usr/games/lib/cfscores

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
	rm -f ${STD} ${NSTD}

depend:
	rm -f makedep
	for i in ${STD} ${NSTD}; do \
	    ${CC} -M ${INCPATH} $$i.c | sed 's/\.o//' | \
	    awk ' { if ($$1 != prev) \
		{ if (rec != "") print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec } ' >> makedep; done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# Files listed in ${NSTD} have explicit make lines given below.

canfield: canfield.c
	${CC} -o canfield ${CFLAGS} canfield.c -lcurses -ltermcap

primes: primes.c
	${CC} -o primes ${CFLAGS} primes.c -lm

rain: rain.c
	${CC} -o rain ${CFLAGS} rain.c -lcurses -ltermcap

worm: worm.c
	${CC} -o worm ${CFLAGS} worm.c -lcurses -ltermcap

worms: worms.c
	${CC} -o worms ${CFLAGS} worms.c -lcurses -ltermcap

# DO NOT DELETE THIS LINE -- make depend uses it

arithmetic: arithmetic.c /usr/include/stdio.h /usr/include/signal.h
btlgammon: btlgammon.c /usr/include/stdio.h
banner: banner.c /usr/include/stdio.h
bcd: bcd.c /usr/include/stdio.h /usr/include/ctype.h
bj: bj.c /usr/include/stdio.h /usr/include/signal.h /usr/include/setjmp.h
cfscores: cfscores.c /usr/include/pwd.h
craps: craps.c /usr/include/stdio.h /usr/include/signal.h
factor: factor.c
fish: fish.c /usr/include/stdio.h
moo: moo.c /usr/include/stdio.h
number: number.c
random: random.c /usr/include/stdio.h
wump: wump.c /usr/include/stdio.h /usr/include/sgtty.h /usr/include/sys/ioctl.h
wump: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
wump: /usr/include/sgtty.h
canfield: canfield.c /usr/include/curses.h /usr/include/stdio.h
canfield: /usr/include/sgtty.h /usr/include/sys/ioctl.h
canfield: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
canfield: /usr/include/sgtty.h /usr/include/ctype.h /usr/include/signal.h
canfield: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
primes: primes.c /usr/include/stdio.h /usr/include/math.h
rain: rain.c /usr/include/stdio.h /usr/include/sgtty.h /usr/include/sys/ioctl.h
rain: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
rain: /usr/include/sgtty.h /usr/include/signal.h
worm: worm.c /usr/include/ctype.h /usr/include/curses.h /usr/include/stdio.h
worm: /usr/include/sgtty.h /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
worm: /usr/include/sys/ttydev.h /usr/include/sgtty.h /usr/include/signal.h
worms: worms.c /usr/include/stdio.h /usr/include/sgtty.h
worms: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
worms: /usr/include/sys/ttydev.h /usr/include/sgtty.h /usr/include/signal.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
