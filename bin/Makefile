#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 5.13 85/09/09
#
DESTDIR=
MACHINES= iAPX286 m68k pdp11 sun u370 u3b u3b2 u3b5 vax
PROCESSORS= mc68010 mc68020
ARCHITECTURE= sun2 sun3
CFLAGS=	-O
LDFLAGS= -n
CC=	/bin/cc

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	awk csh diff ld make sed sh

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=	false true arch mach

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	ar cat chgrp chmod cmp date df domainname dd du echo env \
	grep hostid hostname kill line ln mkdir mt nice \
	nm od pagesize passwd pr pwd rm rmail rmdir size stty sync \
	tar tee test time wall who write

# Programs that must run setuid to root
#
SETUID=	df login mail passwd su

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	ps ed expr \
	login ls mail su

# Programs that must run set-group-id kmem
#
KMEM=	ps

all:	${SUBDIR} ${STD} ${NSTD} cp mv strip

${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

${STD} mv cp strip:
	${CC} ${CFLAGS} -o $@ $@.c ${LDFLAGS}

# take care with cp, strip, and mv, install uses them
install: ${STD} ${NSTD} cp mv strip
	-mkdir ${DESTDIR}/bin && chown bin ${DESTDIR}/bin && \
		chmod 755 ${DESTDIR}/bin
	install -s cp ${DESTDIR}/bin/newcp; \
		mv ${DESTDIR}/bin/newcp ${DESTDIR}/bin/cp;
	install -s strip ${DESTDIR}/bin/newstrip; \
		mv ${DESTDIR}/bin/newstrip ${DESTDIR}/bin/strip;
	rm -f ${DESTDIR}/bin/mv; cp mv ${DESTDIR}/bin/mv; \
		strip ${DESTDIR}/bin/mv; chmod 755 ${DESTDIR}/bin/mv
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	-for i in ${SCRIPT}; do (install -c $$i.sh ${DESTDIR}/bin/$$i); done
	-for i in ${STD} ${NSTD}; do (install -s $$i ${DESTDIR}/bin/$$i); done
	rm -f ${DESTDIR}/bin/cc; ln -s /lib/compile ${DESTDIR}/bin/cc
	-for i in ${SETUID}; do (chown root ${DESTDIR}/bin/$$i; \
		chmod 4755 ${DESTDIR}/bin/$$i); done
	-for i in ${KMEM}; do (chgrp kmem ${DESTDIR}/bin/$$i; \
		chmod 2755 ${DESTDIR}/bin/$$i); done
	rm -f "${DESTDIR}/bin/["; ln ${DESTDIR}/bin/test "${DESTDIR}/bin/["
	rm -f ${DESTDIR}/bin/e; ln ${DESTDIR}/bin/ed ${DESTDIR}/bin/e
	-for i in ${MACHINES}; do \
		rm -f ${DESTDIR}/bin/$$i; \
		if $$i; then ln ${DESTDIR}/bin/true ${DESTDIR}/bin/$$i; \
		else ln ${DESTDIR}/bin/false ${DESTDIR}/bin/$$i; fi; done

	-for i in ${PROCESSORS}; do \
		rm -f ${DESTDIR}/bin/$$i; \
		if $$i; then ln ${DESTDIR}/bin/true ${DESTDIR}/bin/$$i; \
		else ln ${DESTDIR}/bin/false ${DESTDIR}/bin/$$i; fi; done

	-for i in ${ARCHITECTURE}; do \
		rm -f ${DESTDIR}/bin/$$i; \
		if $$i; then ln ${DESTDIR}/bin/true ${DESTDIR}/bin/$$i; \
		else ln ${DESTDIR}/bin/false ${DESTDIR}/bin/$$i; fi; done

international:
	-rm ed
	${CC} ${CFLAGS} -o ed ed.c
	install -s ed ${DESTDIR}/bin
	rm ed

clean:
	rm -f a.out core *.s *.o errs
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
	rm -f ${STD} ${NSTD} cp mv strip

FRC:

# beware of the non-standard stuff for expr 
depend:
	rm -f makedep
	for i in ${STD} ps ed login ls mail su cp mv strip; do \
	    ${CC} -M ${INCPATH} $$i.c | sed 's/\.o//' | \
	    awk ' { if ($$1 != prev) \
		{ if (rec != "") print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec } ' >> makedep; done
	for i in expr; do \
	    ${CC} -M ${INCPATH} $$i.y=.c | sed 's/\.o//' | \
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

ps:
	${CC} ${CFLAGS} -o ps ps.c -lm

ed:
	${CC} ${CFLAGS} -DCRYPT -o ed ed.c

expr:
	${YACC} ${YFLAGS} $@.y
	${CC} -o $@ ${CFLAGS} y.tab.c ${LDFLAGS}
	rm -f y.tab.c

# Following files have explicit make lines because they
# are big and should be linked to be demand paged.

login:
	${CC} ${CFLAGS} -o login login.c

ls:
	${CC} ${CFLAGS} -o ls ls.c

mail:
	${CC} ${CFLAGS} -o mail mail.c

su:
	${CC} ${CFLAGS} -o su su.c

# DO NOT DELETE THIS LINE -- make depend uses it

ar: ar.c /usr/include/sys/param.h /usr/include/machine/param.h
ar: /usr/include/signal.h /usr/include/sys/types.h /usr/include/sys/sysmacros.h
ar: /usr/include/sys/stat.h /usr/include/sys/time.h /usr/include/time.h
ar: /usr/include/stdio.h /usr/include/ar.h
cat: cat.c /usr/include/stdio.h /usr/include/sys/types.h
cat: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
chgrp: chgrp.c /usr/include/stdio.h /usr/include/ctype.h
chgrp: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
chgrp: /usr/include/sys/stat.h /usr/include/grp.h /usr/include/pwd.h
chgrp: /usr/include/sys/dir.h
chmod: chmod.c /usr/include/stdio.h /usr/include/sys/types.h
chmod: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
chmod: /usr/include/sys/dir.h
cmp: cmp.c /usr/include/stdio.h /usr/include/ctype.h
date: date.c /usr/include/stdio.h /usr/include/sys/time.h /usr/include/time.h
date: /usr/include/utmp.h
df: df.c /usr/include/sys/param.h /usr/include/machine/param.h
df: /usr/include/signal.h /usr/include/sys/types.h /usr/include/sys/sysmacros.h
df: /usr/include/errno.h /usr/include/sys/errno.h /usr/include/ufs/fs.h
df: /usr/include/sys/stat.h /usr/include/sys/vfs.h /usr/include/stdio.h
df: /usr/include/mntent.h
domainname: domainname.c /usr/include/stdio.h
dd: dd.c /usr/include/stdio.h /usr/include/signal.h
du: du.c /usr/include/stdio.h /usr/include/sys/param.h
du: /usr/include/machine/param.h /usr/include/signal.h /usr/include/sys/types.h
du: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h /usr/include/sys/dir.h
echo: echo.c /usr/include/stdio.h
env: env.c /usr/include/stdio.h
grep: grep.c /usr/include/stdio.h /usr/include/ctype.h
hostid: hostid.c
hostname: hostname.c /usr/include/stdio.h
kill: kill.c /usr/include/stdio.h /usr/include/signal.h /usr/include/ctype.h
line: line.c
ln: ln.c /usr/include/stdio.h /usr/include/sys/types.h
ln: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h /usr/include/errno.h
ln: /usr/include/sys/errno.h
mkdir: mkdir.c /usr/include/stdio.h
mt: mt.c /usr/include/stdio.h /usr/include/ctype.h /usr/include/sys/types.h
mt: /usr/include/sys/sysmacros.h /usr/include/sys/mtio.h
mt: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
mt: /usr/include/sys/ttydev.h /usr/include/sgtty.h /usr/include/sundev/tmreg.h
mt: /usr/include/sundev/xtreg.h /usr/include/sundev/arreg.h
nice: nice.c /usr/include/stdio.h /usr/include/ctype.h /usr/include/sys/time.h
nice: /usr/include/time.h /usr/include/sys/resource.h
nm: nm.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
nm: /usr/include/ar.h /usr/include/stdio.h /usr/include/ctype.h
nm: /usr/include/a.out.h /usr/include/sys/exec.h /usr/include/stab.h
nm: /usr/include/sys/stat.h
od: od.c /usr/include/stdio.h /usr/include/sys/types.h
od: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
pagesize: pagesize.c
passwd: passwd.c /usr/include/sys/file.h /usr/include/sys/fcntl.h
passwd: /usr/include/stdio.h /usr/include/signal.h /usr/include/pwd.h
passwd: /usr/include/errno.h /usr/include/sys/errno.h
pr: pr.c /usr/include/stdio.h /usr/include/signal.h /usr/include/sys/types.h
pr: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
pwd: pwd.c /usr/include/stdio.h /usr/include/sys/param.h
pwd: /usr/include/machine/param.h /usr/include/signal.h
pwd: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
rm: rm.c /usr/include/stdio.h /usr/include/sys/param.h
rm: /usr/include/machine/param.h /usr/include/signal.h /usr/include/sys/types.h
rm: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h /usr/include/sys/dir.h
rm: /usr/include/sys/file.h /usr/include/sys/fcntl.h
rmail: rmail.c /usr/include/stdio.h /usr/include/sysexits.h
rmail: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
rmail: /usr/include/sys/stat.h
rmdir: rmdir.c /usr/include/stdio.h
size: size.c /usr/include/stdio.h /usr/include/a.out.h /usr/include/sys/exec.h
stty: stty.c /usr/include/stdio.h /usr/include/sgtty.h /usr/include/sys/ioctl.h
stty: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
stty: /usr/include/sgtty.h
sync: sync.c
tar: tar.c /usr/include/stdio.h /usr/include/sys/param.h
tar: /usr/include/machine/param.h /usr/include/signal.h
tar: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
tar: /usr/include/sys/stat.h /usr/include/sys/dir.h /usr/include/sys/ioctl.h
tar: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h /usr/include/sgtty.h
tar: /usr/include/sys/mtio.h /usr/include/sys/time.h /usr/include/time.h
tar: /usr/include/signal.h /usr/include/errno.h /usr/include/sys/errno.h
tar: /usr/include/fcntl.h
tee: tee.c /usr/include/signal.h /usr/include/stdio.h /usr/include/sys/types.h
tee: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h /usr/include/errno.h
tee: /usr/include/sys/errno.h /usr/include/fcntl.h
test: test.c /usr/include/stdio.h /usr/include/sys/types.h
test: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
time: time.c /usr/include/stdio.h /usr/include/signal.h
time: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
time: /usr/include/sys/time.h /usr/include/time.h /usr/include/sys/resource.h
wall: wall.c /usr/include/stdio.h /usr/include/utmp.h /usr/include/sys/time.h
wall: /usr/include/time.h /usr/include/signal.h
who: who.c /usr/include/stdio.h /usr/include/utmp.h /usr/include/pwd.h
who: /usr/include/ctype.h
write: write.c /usr/include/stdio.h /usr/include/ctype.h
write: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
write: /usr/include/sys/stat.h /usr/include/signal.h /usr/include/utmp.h
write: /usr/include/sys/time.h /usr/include/time.h
ps: ps.c /usr/include/stdio.h /usr/include/ctype.h /usr/include/nlist.h
ps: /usr/include/pwd.h /usr/include/sys/param.h /usr/include/machine/param.h
ps: /usr/include/signal.h /usr/include/sys/types.h /usr/include/sys/sysmacros.h
ps: /usr/include/sys/tty.h /usr/include/sys/ttychars.h
ps: /usr/include/sys/ttydev.h /usr/include/sys/dir.h /usr/include/sys/user.h
ps: /usr/include/machine/pcb.h /usr/include/machine/reg.h
ps: /usr/include/sys/dmap.h /usr/include/sys/time.h /usr/include/time.h
ps: /usr/include/sys/resource.h /usr/include/sys/exec.h /usr/include/errno.h
ps: /usr/include/sys/errno.h /usr/include/sys/proc.h /usr/include/machine/pte.h
ps: /usr/include/sys/vm.h /usr/include/sys/vmparam.h
ps: /usr/include/machine/vmparam.h /usr/include/sys/vmmac.h
ps: /usr/include/sys/vmmeter.h /usr/include/sys/vmsystm.h
ps: /usr/include/sys/text.h /usr/include/sys/stat.h /usr/include/sys/mbuf.h
ps: /usr/include/math.h
ed: ed.c /usr/include/signal.h /usr/include/sgtty.h /usr/include/sys/ioctl.h
ed: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h /usr/include/sgtty.h
ed: /usr/include/setjmp.h
login: login.c /usr/include/sys/param.h /usr/include/machine/param.h
login: /usr/include/signal.h /usr/include/sys/types.h
login: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
login: /usr/include/sys/time.h /usr/include/time.h /usr/include/sys/resource.h
login: /usr/include/sgtty.h /usr/include/sys/ioctl.h
login: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
login: /usr/include/sgtty.h /usr/include/utmp.h /usr/include/signal.h
login: /usr/include/pwd.h /usr/include/stdio.h /usr/include/lastlog.h
login: /usr/include/errno.h /usr/include/sys/errno.h
ls: ls.c /usr/include/sys/param.h /usr/include/machine/param.h
ls: /usr/include/signal.h /usr/include/sys/types.h /usr/include/sys/sysmacros.h
ls: /usr/include/sys/stat.h /usr/include/sys/dir.h /usr/include/stdio.h
ls: /usr/include/sgtty.h /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
ls: /usr/include/sys/ttydev.h /usr/include/sgtty.h /usr/include/pwd.h
ls: /usr/include/grp.h /usr/include/utmp.h
mail: mail.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
mail: /usr/include/sys/stat.h /usr/include/sys/file.h /usr/include/sys/fcntl.h
mail: /usr/include/ctype.h /usr/include/stdio.h /usr/include/pwd.h
mail: /usr/include/utmp.h /usr/include/signal.h /usr/include/setjmp.h
mail: /usr/include/sysexits.h /usr/include/sys/socket.h
mail: /usr/include/netinet/in.h
su: su.c /usr/include/stdio.h /usr/include/pwd.h /usr/include/sys/types.h
su: /usr/include/sys/sysmacros.h /usr/include/sys/time.h /usr/include/time.h
su: /usr/include/sys/resource.h /usr/include/syslog.h
cp: cp.c /usr/include/stdio.h /usr/include/sys/param.h
cp: /usr/include/machine/param.h /usr/include/signal.h /usr/include/sys/types.h
cp: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h /usr/include/sys/dir.h
cp: /usr/include/sys/time.h /usr/include/time.h
mv: mv.c /usr/include/sys/param.h /usr/include/machine/param.h
mv: /usr/include/signal.h /usr/include/sys/types.h /usr/include/sys/sysmacros.h
mv: /usr/include/sys/stat.h /usr/include/sys/time.h /usr/include/time.h
mv: /usr/include/stdio.h /usr/include/sys/dir.h /usr/include/errno.h
mv: /usr/include/sys/errno.h /usr/include/signal.h
strip: strip.c /usr/include/a.out.h /usr/include/sys/exec.h
strip: /usr/include/signal.h /usr/include/stdio.h /usr/include/sys/file.h
strip: /usr/include/sys/fcntl.h
expr: expr.y /usr/include/stdio.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
