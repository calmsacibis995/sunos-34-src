#
# @(#)Makefile 1.1 86/09/24 SMI
#
DESTDIR=
CFLAGS=	-O
CC=	/usr/5bin/cc
MAKE=	make CC=${CC}

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	cxref diff3 ed lint m4 sed

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=	cflow dircmp

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	banner cat du echo expr grep nohup od pr stty sum time touch tr uname

# Programs that must run setuid to root
#
SETUID=	

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	ls pg tabs tput

# C programs that live in the current directory but whose source is in /bin.
SHAREDBIN=	chmod date

# C programs that live in the current directory but whose source is in /usr/bin.
SHAREDUSRBIN=	col sort

all:	${SUBDIR} ${STD} ${NSTD} ${SHAREDBIN} ${SHAREDUSRBIN}

${SUBDIR}: FRC
	cd $@; ${MAKE} ${MFLAGS}

${STD}:
	${CC} ${CFLAGS} -DS5EMUL -o $@ $@.c

${SHAREDBIN}:
	${CC} ${CFLAGS} -DS5EMUL -o $@ ../bin/$@.c

${SHAREDUSRBIN}:
	${CC} ${CFLAGS} -DS5EMUL -o $@ ../usr.bin/$@.c

install: ${STD} ${NSTD} ${SHAREDBIN} ${SHAREDUSRBIN}
	-mkdir ${DESTDIR}/usr/5bin && chown bin ${DESTDIR}/usr/5bin && \
		chmod 755 ${DESTDIR}/usr/5bin 
	-for i in ${SUBDIR}; do \
		(cd $$i; ${MAKE} ${MFLAGS} DESTDIR=${DESTDIR} install); done
	-for i in ${SCRIPT}; do (install -c $$i.sh ${DESTDIR}/usr/5bin/$$i); done
	-for i in ${STD} ${NSTD} ${SHAREDBIN} ${SHAREDUSRBIN}; do \
		(install -s $$i ${DESTDIR}/usr/5bin/$$i); \
	done
	rm -f ${DESTDIR}/usr/5bin/cc; ln -s /usr/5lib/compile ${DESTDIR}/usr/5bin/cc
	#-for i in ${SETUID}; do (chown root ${DESTDIR}/usr/5bin/$$i; \
	#	chmod 4755 ${DESTDIR}/usr/5bin/$$i); done

clean:
	rm -f a.out y.tab.* core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; ${MAKE} ${MFLAGS} clean); done
	rm -f ${STD} ${NSTD} ${SHAREDBIN} ${SHAREDUSRBIN}

# beware of the non-standard stuff for ls and pg
depend:
	rm -f makedep
	for i in ${STD} tabs tput; do \
	    ${CC} -M ${INCPATH} -DS5EMUL $$i.c | sed 's/\.o//' | \
	    awk ' { if ($$1 != prev) \
		{ if (rec != "") print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec } ' >> makedep; done
	for i in ${SHAREDBIN}; do \
	    ${CC} -M ${INCPATH} -DS5EMUL ../bin/$$i.c | sed 's/\.o//' | \
	    awk ' { if ($$1 != prev) \
		{ if (rec != "") print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec } ' >> makedep; done
	for i in ${SHAREDUSRBIN}; do \
	    ${CC} -M ${INCPATH} -DS5EMUL ../usr.bin/$$i.c | sed 's/\.o//' | \
	    awk ' { if ($$1 != prev) \
		{ if (rec != "") print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec } ' >> makedep; done
	for i in ls pg; do \
	    ${CC} -M ${INCPATH} -DSINGLE -DS5EMUL $$i.c | sed 's/\.o//' | \
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

FRC:

# Files listed in ${NSTD} have explicit make lines given below.

ls:	ls.c
	${CC} -o ls ${CFLAGS} -DSINGLE -DS5EMUL ls.c -lcurses

pg:	pg.c
	${CC} -o pg ${CFLAGS} -DSINGLE -DS5EMUL pg.c -lcurses

tabs:	tabs.c
	${CC} -o tabs ${CFLAGS} -DS5EMUL tabs.c -lcurses

tput:	tput.c
	${CC} -o tput ${CFLAGS} -DS5EMUL tput.c -lcurses

# DO NOT DELETE THIS LINE -- make depend uses it

banner: banner.c /usr/5include/stdio.h
cat: cat.c /usr/5include/stdio.h /usr/include/sys/types.h
cat: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
du: du.c /usr/5include/stdio.h /usr/include/sys/param.h
du: /usr/include/machine/param.h /usr/include/signal.h /usr/include/sys/types.h
du: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h /usr/include/sys/dir.h
echo: echo.c
expr: expr.c /usr/include/regexp.h
grep: grep.c /usr/5include/stdio.h /usr/5include/ctype.h /usr/include/regexp.h
nohup: nohup.c /usr/5include/stdio.h /usr/include/signal.h
od: od.c /usr/include/sys/param.h /usr/include/machine/param.h
od: /usr/include/signal.h /usr/include/sys/types.h /usr/include/sys/sysmacros.h
od: /usr/5include/stdio.h
pr: pr.c /usr/5include/stdio.h /usr/include/signal.h /usr/5include/ctype.h
pr: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
pr: /usr/include/sys/stat.h
stty: stty.c /usr/5include/stdio.h /usr/include/sys/types.h
stty: /usr/include/sys/sysmacros.h /usr/5include/sys/termio.h
stty: /usr/5include/sys/ioccom.h
sum: sum.c /usr/5include/stdio.h
time: time.c /usr/5include/stdio.h /usr/include/signal.h /usr/include/errno.h
time: /usr/include/sys/errno.h /usr/include/sys/param.h
time: /usr/include/machine/param.h /usr/include/signal.h
time: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
touch: touch.c /usr/5include/stdio.h /usr/5include/ctype.h
touch: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
touch: /usr/include/sys/stat.h /usr/5include/time.h
tr: tr.c /usr/5include/stdio.h
uname: uname.c /usr/5include/stdio.h /usr/5include/sys/utsname.h
tabs: tabs.c /usr/5include/stdio.h /usr/include/signal.h
tabs: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
tabs: /usr/include/sys/stat.h /usr/5include/curses.h /usr/5include/stdio.h
tabs: /usr/5include/unctrl.h /usr/5include/termio.h /usr/5include/sys/ioccom.h
tabs: /usr/5include/term.h
tput: tput.c /usr/5include/curses.h /usr/5include/stdio.h
tput: /usr/5include/unctrl.h /usr/5include/termio.h /usr/5include/sys/ioccom.h
tput: /usr/5include/term.h
chmod: ../bin/chmod.c /usr/5include/stdio.h /usr/include/sys/types.h
chmod: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
chmod: /usr/include/sys/dir.h
date: ../bin/date.c /usr/5include/stdio.h /usr/include/sys/time.h
date: /usr/5include/time.h /usr/include/utmp.h
col: ../usr.bin/col.c /usr/5include/stdio.h
sort: ../usr.bin/sort.c /usr/5include/stdio.h /usr/5include/ctype.h
sort: /usr/include/signal.h /usr/include/sys/param.h
sort: /usr/include/machine/param.h /usr/include/signal.h
sort: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
sort: /usr/include/sys/stat.h /usr/include/values.h
ls: ls.c /usr/include/sys/param.h /usr/include/machine/param.h
ls: /usr/include/signal.h /usr/include/sys/types.h /usr/include/sys/sysmacros.h
ls: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
ls: /usr/include/sys/stat.h /usr/include/sys/dir.h /usr/5include/stdio.h
ls: /usr/5include/curses.h /usr/5include/stdio.h /usr/5include/unctrl.h
ls: /usr/5include/termio.h /usr/5include/sys/ioccom.h /usr/5include/term.h
ls: /usr/5include/pwd.h /usr/include/grp.h /usr/include/utmp.h
pg: pg.c /usr/include/signal.h /usr/include/setjmp.h /usr/include/sys/types.h
pg: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h /usr/5include/ctype.h
pg: /usr/5include/stdio.h /usr/include/values.h /usr/5include/curses.h
pg: /usr/5include/stdio.h /usr/5include/unctrl.h /usr/5include/termio.h
pg: /usr/5include/sys/ioccom.h /usr/5include/term.h /usr/include/regexp.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
