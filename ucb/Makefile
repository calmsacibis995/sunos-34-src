#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 5.12 85/09/17
#
DESTDIR=
CFLAGS=	-O
LDFLAGS= -n

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	Mail compact compress error ex eyacc ftp gprof \
	indent lpr more netstat pascal rdist talk tftp \
	tset vgrind

# Shell scripts that need only be installed and are never removed.
#
CSHSCRIPT= vtroff which

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	biff checknr chsh colcrt colrm ctags expand \
	fold fpr from fsplit gcore groups head last \
	leave man mkstr printenv prmail pti \
	rdate rsh rwho ruptime \
	script soelim strings symorder syslog \
	tail telnet unexpand unifdef users vmstat w whatis whereis whoami \
	whois what wc xstr yes

# Programs that must run setuid to root
#
SETUID=	chsh quota rcp rlogin rsh

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	clear ul \
	finger lastcomm rcp rlogin sccs

# C programs that use the rpcsvc library
#
RPCSVC=	quota rup rusers

# Programs that must run set-group-id kmem.
#
KMEM=	gcore vmstat w

all:	${SUBDIR} ${STD} ${NSTD} ${RPCSVC}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

${STD}:
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $@.c

${RPCSVC}:
	${CC} ${CFLAGS} -o $@ $@.c -lrpcsvc

install: ${STD} ${NSTD} ${RPCSVC}
	-mkdir ${DESTDIR}/usr/ucb && chown bin ${DESTDIR}/usr/ucb && \
		chmod 755 ${DESTDIR}/usr/ucb
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	for i in ${CSHSCRIPT}; do \
		(install -c $$i.csh ${DESTDIR}/usr/ucb/$$i); done
	for i in ${STD} ${NSTD} ${RPCSVC}; do \
		(install -s $$i ${DESTDIR}/usr/ucb/$$i); \
	done
	for i in ${SETUID}; do (chown root ${DESTDIR}/usr/ucb/$$i; \
		chmod 4755 ${DESTDIR}/usr/ucb/$$i); done
	-for i in ${KMEM}; do (chgrp kmem ${DESTDIR}/usr/ucb/$$i; \
		chmod 2755 ${DESTDIR}/usr/ucb/$$i); done
	rm -f ${DESTDIR}/usr/ucb/uptime ${DESTDIR}/usr/ucb/f
	ln ${DESTDIR}/usr/ucb/w ${DESTDIR}/usr/ucb/uptime

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
	rm -f ${STD} ${NSTD} ${RPCSVC}

FRC:

depend:
	rm -f makedep
	for i in ${STD} ${NSTD} ${RPCSVC}; do \
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

clear:
	${CC} -o clear ${CFLAGS} clear.c -ltermlib

ul:
	${CC} -o ul ${CFLAGS} ul.c -ltermlib

# Following files have explicit make lines because they
# are big and should be linked to be demand paged.

finger:
	${CC} -o finger ${CFLAGS} finger.c

lastcomm:
	${CC} -o lastcomm ${CFLAGS} lastcomm.c

rcp:
	${CC} -o rcp ${CFLAGS} rcp.c

rlogin:
	${CC} -o rlogin ${CFLAGS} rlogin.c

sccs:
	${CC} -o sccs ${CFLAGS} sccs.c

# DO NOT DELETE THIS LINE -- make depend uses it

biff: biff.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
biff: /usr/include/sys/stat.h /usr/include/stdio.h
checknr: checknr.c /usr/include/stdio.h /usr/include/ctype.h
chsh: chsh.c /usr/include/stdio.h /usr/include/signal.h /usr/include/pwd.h
chsh: /usr/include/sys/file.h /usr/include/sys/fcntl.h /usr/include/sys/time.h
chsh: /usr/include/time.h /usr/include/sys/resource.h
colcrt: colcrt.c /usr/include/stdio.h
colrm: colrm.c /usr/include/stdio.h
ctags: ctags.c /usr/include/stdio.h /usr/include/ctype.h
expand: expand.c /usr/include/stdio.h
fold: fold.c /usr/include/stdio.h
fpr: fpr.c /usr/include/stdio.h
from: from.c /usr/include/stdio.h /usr/include/ctype.h /usr/include/pwd.h
fsplit: fsplit.c /usr/include/ctype.h /usr/include/stdio.h
fsplit: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
fsplit: /usr/include/sys/stat.h
gcore: gcore.c /usr/include/stdio.h /usr/include/nlist.h
gcore: /usr/include/sys/param.h /usr/include/machine/param.h
gcore: /usr/include/signal.h /usr/include/sys/types.h
gcore: /usr/include/sys/sysmacros.h /usr/include/sys/dir.h
gcore: /usr/include/sys/user.h /usr/include/machine/pcb.h
gcore: /usr/include/machine/reg.h /usr/include/sys/dmap.h
gcore: /usr/include/sys/time.h /usr/include/time.h /usr/include/sys/resource.h
gcore: /usr/include/sys/exec.h /usr/include/errno.h /usr/include/sys/errno.h
gcore: /usr/include/sys/proc.h /usr/include/machine/pte.h /usr/include/sys/vm.h
gcore: /usr/include/sys/vmparam.h /usr/include/machine/vmparam.h
gcore: /usr/include/sys/vmmac.h /usr/include/sys/vmmeter.h
gcore: /usr/include/sys/vmsystm.h /usr/include/setjmp.h /usr/include/sys/core.h
gcore: /usr/include/sys/exec.h /usr/include/machine/reg.h
gcore: /usr/include/sys/file.h /usr/include/sys/fcntl.h
gcore: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
gcore: /usr/include/sys/ttydev.h /usr/include/sgtty.h
gcore: /usr/include/sundev/fpareg.h /usr/include/machine/reg.h
groups: groups.c /usr/include/sys/param.h /usr/include/machine/param.h
groups: /usr/include/signal.h /usr/include/sys/types.h
groups: /usr/include/sys/sysmacros.h /usr/include/grp.h /usr/include/pwd.h
groups: /usr/include/stdio.h
head: head.c /usr/include/stdio.h
last: last.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
last: /usr/include/stdio.h /usr/include/signal.h /usr/include/sys/stat.h
last: /usr/include/utmp.h
leave: leave.c /usr/include/stdio.h /usr/include/ctype.h /usr/include/signal.h
man: man.c /usr/include/stdio.h /usr/include/ctype.h /usr/include/sgtty.h
man: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
man: /usr/include/sys/ttydev.h /usr/include/sgtty.h /usr/include/sys/param.h
man: /usr/include/machine/param.h /usr/include/signal.h
man: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
man: /usr/include/sys/types.h /usr/include/sys/stat.h /usr/include/signal.h
man: /usr/include/strings.h
mkstr: mkstr.c /usr/include/stdio.h /usr/include/sys/types.h
mkstr: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
printenv: printenv.c
prmail: prmail.c /usr/include/pwd.h /usr/include/sys/types.h
prmail: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
pti: pti.c
rdate: rdate.c /usr/include/signal.h /usr/include/sys/types.h
rdate: /usr/include/sys/sysmacros.h /usr/include/sys/time.h /usr/include/time.h
rdate: /usr/include/sys/socket.h /usr/include/netinet/in.h /usr/include/netdb.h
rdate: /usr/include/stdio.h
rsh: rsh.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
rsh: /usr/include/sys/socket.h /usr/include/sys/ioctl.h
rsh: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h /usr/include/sgtty.h
rsh: /usr/include/sys/file.h /usr/include/sys/fcntl.h /usr/include/netinet/in.h
rsh: /usr/include/stdio.h /usr/include/errno.h /usr/include/sys/errno.h
rsh: /usr/include/signal.h /usr/include/pwd.h /usr/include/netdb.h
rwho: rwho.c /usr/include/sys/param.h /usr/include/machine/param.h
rwho: /usr/include/signal.h /usr/include/sys/types.h
rwho: /usr/include/sys/sysmacros.h /usr/include/stdio.h /usr/include/sys/dir.h
rwho: /usr/include/protocols/rwhod.h
ruptime: ruptime.c /usr/include/sys/param.h /usr/include/machine/param.h
ruptime: /usr/include/signal.h /usr/include/sys/types.h
ruptime: /usr/include/sys/sysmacros.h /usr/include/stdio.h
ruptime: /usr/include/sys/dir.h /usr/include/protocols/rwhod.h
script: script.c /usr/include/stdio.h /usr/include/signal.h
script: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
script: /usr/include/sys/stat.h /usr/include/sys/ioctl.h
script: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
script: /usr/include/sgtty.h /usr/include/sgtty.h /usr/include/sys/time.h
script: /usr/include/time.h /usr/include/sys/file.h /usr/include/sys/fcntl.h
script: /usr/include/sys/wait.h
soelim: soelim.c /usr/include/stdio.h
strings: strings.c /usr/include/stdio.h /usr/include/a.out.h
strings: /usr/include/sys/exec.h /usr/include/ctype.h /usr/include/sys/file.h
strings: /usr/include/sys/fcntl.h
symorder: symorder.c /usr/include/stdio.h /usr/include/a.out.h
symorder: /usr/include/sys/exec.h
syslog: syslog.c /usr/include/stdio.h /usr/include/ctype.h
syslog: /usr/include/syslog.h
tail: tail.c /usr/include/stdio.h /usr/include/ctype.h /usr/include/sys/types.h
tail: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
tail: /usr/include/sys/file.h /usr/include/sys/fcntl.h /usr/include/errno.h
tail: /usr/include/sys/errno.h
telnet: telnet.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
telnet: /usr/include/sys/socket.h /usr/include/sys/ioctl.h
telnet: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
telnet: /usr/include/sgtty.h /usr/include/netinet/in.h
telnet: /usr/include/arpa/telnet.h /usr/include/stdio.h /usr/include/ctype.h
telnet: /usr/include/errno.h /usr/include/sys/errno.h /usr/include/signal.h
telnet: /usr/include/setjmp.h /usr/include/netdb.h
unexpand: unexpand.c /usr/include/stdio.h
unifdef: unifdef.c /usr/include/stdio.h /usr/include/ctype.h
users: users.c /usr/include/stdio.h /usr/include/utmp.h
vmstat: vmstat.c /usr/include/stdio.h /usr/include/sys/param.h
vmstat: /usr/include/machine/param.h /usr/include/signal.h
vmstat: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
vmstat: /usr/include/sys/vm.h /usr/include/sys/vmparam.h
vmstat: /usr/include/machine/vmparam.h /usr/include/sys/vmmac.h
vmstat: /usr/include/sys/vmmeter.h /usr/include/sys/vmsystm.h
vmstat: /usr/include/sys/dk.h /usr/include/nlist.h /usr/include/sys/buf.h
vmstat: /usr/include/sundev/mbvar.h
w: w.c /usr/include/sys/param.h /usr/include/machine/param.h
w: /usr/include/signal.h /usr/include/sys/types.h /usr/include/sys/sysmacros.h
w: /usr/include/nlist.h /usr/include/stdio.h /usr/include/ctype.h
w: /usr/include/utmp.h /usr/include/sys/stat.h /usr/include/sys/dir.h
w: /usr/include/sys/user.h /usr/include/machine/pcb.h
w: /usr/include/machine/reg.h /usr/include/sys/dmap.h /usr/include/sys/time.h
w: /usr/include/time.h /usr/include/sys/resource.h /usr/include/sys/exec.h
w: /usr/include/errno.h /usr/include/sys/errno.h /usr/include/sys/proc.h
w: /usr/include/machine/pte.h /usr/include/sys/vm.h /usr/include/sys/vmparam.h
w: /usr/include/machine/vmparam.h /usr/include/sys/vmmac.h
w: /usr/include/sys/vmmeter.h /usr/include/sys/vmsystm.h
whatis: whatis.c /usr/include/stdio.h /usr/include/ctype.h
whereis: whereis.c /usr/include/sys/param.h /usr/include/machine/param.h
whereis: /usr/include/signal.h /usr/include/sys/types.h
whereis: /usr/include/sys/sysmacros.h /usr/include/sys/dir.h
whereis: /usr/include/stdio.h /usr/include/ctype.h
whoami: whoami.c /usr/include/pwd.h
whois: whois.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
whois: /usr/include/sys/socket.h /usr/include/netinet/in.h /usr/include/stdio.h
whois: /usr/include/netdb.h
what: what.c /usr/include/stdio.h
wc: wc.c /usr/include/stdio.h
xstr: xstr.c /usr/include/stdio.h /usr/include/ctype.h /usr/include/sys/types.h
xstr: /usr/include/sys/sysmacros.h /usr/include/signal.h
yes: yes.c
clear: clear.c /usr/include/stdio.h /usr/include/sgtty.h
clear: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
clear: /usr/include/sys/ttydev.h /usr/include/sgtty.h
ul: ul.c /usr/include/stdio.h
finger: finger.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
finger: /usr/include/sys/stat.h /usr/include/utmp.h /usr/include/sys/signal.h
finger: /usr/include/pwd.h /usr/include/stdio.h /usr/include/lastlog.h
finger: /usr/include/ctype.h /usr/include/sys/time.h /usr/include/time.h
finger: /usr/include/sys/socket.h /usr/include/netinet/in.h
finger: /usr/include/netdb.h
lastcomm: lastcomm.c /usr/include/sys/param.h /usr/include/machine/param.h
lastcomm: /usr/include/signal.h /usr/include/sys/types.h
lastcomm: /usr/include/sys/sysmacros.h /usr/include/sys/acct.h
lastcomm: /usr/include/sys/file.h /usr/include/sys/fcntl.h /usr/include/stdio.h
lastcomm: /usr/include/pwd.h /usr/include/sys/stat.h /usr/include/utmp.h
lastcomm: /usr/include/struct.h /usr/include/ctype.h /usr/include/sys/dir.h
rcp: rcp.c /usr/include/sys/param.h /usr/include/machine/param.h
rcp: /usr/include/signal.h /usr/include/sys/types.h
rcp: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
rcp: /usr/include/sys/time.h /usr/include/time.h /usr/include/sys/ioctl.h
rcp: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h /usr/include/sgtty.h
rcp: /usr/include/netinet/in.h /usr/include/stdio.h /usr/include/signal.h
rcp: /usr/include/pwd.h /usr/include/ctype.h /usr/include/netdb.h
rcp: /usr/include/errno.h /usr/include/sys/errno.h /usr/include/sys/dir.h
rlogin: rlogin.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
rlogin: /usr/include/sys/file.h /usr/include/sys/fcntl.h
rlogin: /usr/include/sys/socket.h /usr/include/sys/wait.h
rlogin: /usr/include/netinet/in.h /usr/include/stdio.h /usr/include/sgtty.h
rlogin: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
rlogin: /usr/include/sys/ttydev.h /usr/include/sgtty.h /usr/include/errno.h
rlogin: /usr/include/sys/errno.h /usr/include/pwd.h /usr/include/signal.h
rlogin: /usr/include/setjmp.h /usr/include/netdb.h
sccs: sccs.c /usr/include/stdio.h /usr/include/sys/param.h
sccs: /usr/include/machine/param.h /usr/include/signal.h
sccs: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
sccs: /usr/include/sys/stat.h /usr/include/sys/dir.h /usr/include/errno.h
sccs: /usr/include/sys/errno.h /usr/include/signal.h /usr/include/sysexits.h
sccs: /usr/include/pwd.h
quota: quota.c /usr/include/stdio.h /usr/include/mntent.h /usr/include/ctype.h
quota: /usr/include/pwd.h /usr/include/errno.h /usr/include/sys/errno.h
quota: /usr/include/sys/param.h /usr/include/machine/param.h
quota: /usr/include/signal.h /usr/include/sys/types.h
quota: /usr/include/sys/sysmacros.h /usr/include/sys/file.h
quota: /usr/include/sys/fcntl.h /usr/include/sys/stat.h /usr/include/sys/time.h
quota: /usr/include/time.h /usr/include/ufs/quota.h /usr/include/rpc/rpc.h
quota: /usr/include/rpc/types.h /usr/include/sys/types.h
quota: /usr/include/netinet/in.h /usr/include/rpc/xdr.h /usr/include/rpc/auth.h
quota: /usr/include/rpc/clnt.h /usr/include/rpc/rpc_msg.h
quota: /usr/include/rpc/auth_unix.h /usr/include/rpc/svc.h
quota: /usr/include/rpc/svc_auth.h /usr/include/rpc/pmap_prot.h
quota: /usr/include/sys/socket.h /usr/include/netdb.h
quota: /usr/include/rpcsvc/rquota.h
rup: rup.c /usr/include/stdio.h /usr/include/netdb.h /usr/include/sys/param.h
rup: /usr/include/machine/param.h /usr/include/signal.h
rup: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
rup: /usr/include/sys/stat.h /usr/include/sys/time.h /usr/include/time.h
rup: /usr/include/sys/socket.h /usr/include/rpcsvc/rstat.h
rup: /usr/include/sys/dk.h /usr/include/rpc/rpc.h /usr/include/rpc/types.h
rup: /usr/include/sys/types.h /usr/include/netinet/in.h /usr/include/rpc/xdr.h
rup: /usr/include/rpc/auth.h /usr/include/rpc/clnt.h /usr/include/rpc/rpc_msg.h
rup: /usr/include/rpc/auth_unix.h /usr/include/rpc/svc.h
rup: /usr/include/rpc/svc_auth.h /usr/include/rpc/pmap_clnt.h
rusers: rusers.c /usr/include/stdio.h /usr/include/utmp.h /usr/include/netdb.h
rusers: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
rusers: /usr/include/sys/socket.h /usr/include/rpc/rpc.h
rusers: /usr/include/rpc/types.h /usr/include/sys/types.h
rusers: /usr/include/netinet/in.h /usr/include/rpc/xdr.h
rusers: /usr/include/rpc/auth.h /usr/include/rpc/clnt.h
rusers: /usr/include/rpc/rpc_msg.h /usr/include/rpc/auth_unix.h
rusers: /usr/include/rpc/svc.h /usr/include/rpc/svc_auth.h
rusers: /usr/include/rpc/pmap_clnt.h /usr/include/rpcsvc/rusers.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
