#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 5.6 85/08/26
#
DESTDIR=
CFLAGS=	-O
LDFLAGS= -n

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	dump files fsck getty rpc.lockd rpc.statd in.routed restore

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=	fastboot fasthalt

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	arp biod chown clri cron dmesg fsirand \
	halt ifconfig inetd init \
	mknod newfs nfsd portmap reboot renice rmt \
	vipw ypbind

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	pstat update \
	mkfs in.rlogind in.rshd

# C programs that use the rpcsvc library
#
RPCSVC=	mount shutdown umount

# C programs that must run set-group-id kmem.
#
KMEM=	pstat arp dmesg

all:	${SUBDIR} ${STD} ${NSTD} ${RPCSVC}
 
${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

${STD}:
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $@.c

${RPCSVC}:
	${CC} ${CFLAGS} -o $@ $@.c -lrpcsvc

install: ${STD} ${NSTD} ${RPCSVC}
	-mkdir ${DESTDIR}/etc && chown bin ${DESTDIR}/etc && \
		chmod 755 ${DESTDIR}/etc
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	for i in ${SCRIPT}; do (install -c $$i.sh ${DESTDIR}/etc/$$i); done
	for i in ${STD} ${NSTD} ${RPCSVC}; do \
		(install -s $$i ${DESTDIR}/etc/$$i); done
	-for i in ${KMEM}; do (chgrp kmem ${DESTDIR}/etc/$$i; \
		chmod 2755 ${DESTDIR}/etc/$$i); done
	rm -f ${DESTDIR}/etc/termcap
	ln -s /usr/etc/termcap ${DESTDIR}/etc/termcap

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
	rm -f ${STD} ${NSTD} ${RPCSVC}

FRC:

# beware of the non-standard stuff for pstat
depend:
	rm -f makedep
	for i in ${STD} update mkfs in.rlogind in.rshd ${RPCSVC}; do \
	    ${CC} -M ${INCPATH} $$i.c | sed 's/\.o//' | \
	    awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec } ' >> makedep; done
	for i in pstat; do \
	    ${CC} -M ${INCPATH} -I/usr/src/sun/sys/sys $$i.c | sed 's/\.o//' | \
	    awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
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

# The -I flag below is needed because pstat.c defines KERNEL before including
# file.h (and thus would include "../h/fcntl.h")....fixed in 4.0 header re-org
pstat:	pstat.c
	${CC} ${CFLAGS} -I/usr/src/sun/sys/sys -o pstat pstat.c

update:	update.c
	${CC} ${CFLAGS} -o update -N update.c

# Following files have explicit make lines because they
# are big and should be linked to be demand paged.

mkfs:	mkfs.c
	${CC} ${CFLAGS} -o mkfs mkfs.c

in.rlogind:	in.rlogind.c
	${CC} ${CFLAGS} -o in.rlogind in.rlogind.c

in.rshd:	in.rshd.c
	${CC} ${CFLAGS} -o in.rshd in.rshd.c

# DO NOT DELETE THIS LINE -- make depend uses it


arp: arp.c /usr/include/stdio.h /usr/include/sys/types.h
arp: /usr/include/sys/sysmacros.h /usr/include/sys/socket.h
arp: /usr/include/netinet/in.h /usr/include/sys/ioctl.h
arp: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h /usr/include/sgtty.h
arp: /usr/include/errno.h /usr/include/sys/errno.h /usr/include/netdb.h
arp: /usr/include/nlist.h /usr/include/net/if.h /usr/include/netinet/if_ether.h

biod: biod.c /usr/include/stdio.h /usr/include/sys/file.h
biod: /usr/include/sys/fcntl.h /usr/include/sys/ioctl.h
biod: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
biod: /usr/include/sgtty.h

chown: chown.c /usr/include/stdio.h /usr/include/ctype.h
chown: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
chown: /usr/include/sys/stat.h /usr/include/pwd.h /usr/include/sys/dir.h
chown: /usr/include/grp.h /usr/include/strings.h

clri: clri.c /usr/include/sys/param.h /usr/include/machine/param.h
clri: /usr/include/signal.h /usr/include/sys/types.h
clri: /usr/include/sys/sysmacros.h /usr/include/sys/time.h /usr/include/time.h
clri: /usr/include/sys/vnode.h /usr/include/ufs/inode.h /usr/include/ufs/fs.h

cron: cron.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
cron: /usr/include/stdio.h /usr/include/ctype.h /usr/include/signal.h
cron: /usr/include/sys/time.h /usr/include/time.h /usr/include/sys/stat.h
cron: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
cron: /usr/include/sys/ttydev.h /usr/include/sgtty.h /usr/include/sys/file.h
cron: /usr/include/sys/fcntl.h /usr/include/sys/wait.h

dmesg: dmesg.c /usr/include/stdio.h /usr/include/sys/param.h
dmesg: /usr/include/machine/param.h /usr/include/signal.h
dmesg: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
dmesg: /usr/include/nlist.h /usr/include/signal.h /usr/include/sys/vm.h
dmesg: /usr/include/sys/vmparam.h /usr/include/machine/vmparam.h
dmesg: /usr/include/sys/vmmac.h /usr/include/sys/vmmeter.h
dmesg: /usr/include/sys/vmsystm.h /usr/include/sys/msgbuf.h

fsirand: fsirand.c /usr/include/stdio.h /usr/include/sys/param.h
fsirand: /usr/include/machine/param.h /usr/include/signal.h
fsirand: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
fsirand: /usr/include/sys/time.h /usr/include/time.h /usr/include/ufs/fs.h
fsirand: /usr/include/sys/vnode.h /usr/include/ufs/inode.h

halt: halt.c /usr/include/stdio.h /usr/include/sys/reboot.h
halt: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
halt: /usr/include/sys/time.h /usr/include/time.h /usr/include/errno.h
halt: /usr/include/sys/errno.h /usr/include/signal.h /usr/include/utmp.h

ifconfig: ifconfig.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
ifconfig: /usr/include/sys/socket.h /usr/include/sys/ioctl.h
ifconfig: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
ifconfig: /usr/include/sgtty.h /usr/include/netinet/in.h /usr/include/net/if.h
ifconfig: /usr/include/stdio.h /usr/include/errno.h /usr/include/sys/errno.h
ifconfig: /usr/include/ctype.h /usr/include/netdb.h

inetd: inetd.c /usr/include/rpc/rpc.h /usr/include/rpc/types.h
inetd: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
inetd: /usr/include/netinet/in.h /usr/include/rpc/xdr.h /usr/include/rpc/auth.h
inetd: /usr/include/rpc/clnt.h /usr/include/rpc/rpc_msg.h
inetd: /usr/include/rpc/auth_unix.h /usr/include/rpc/svc.h
inetd: /usr/include/rpc/svc_auth.h /usr/include/rpc/pmap_prot.h
inetd: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
inetd: /usr/include/sys/ttydev.h /usr/include/sgtty.h /usr/include/sys/socket.h
inetd: /usr/include/sys/file.h /usr/include/sys/fcntl.h /usr/include/sys/wait.h
inetd: /usr/include/arpa/inet.h /usr/include/errno.h /usr/include/sys/errno.h
inetd: /usr/include/stdio.h /usr/include/signal.h /usr/include/netdb.h

init: init.c /usr/include/signal.h /usr/include/sys/types.h
init: /usr/include/sys/sysmacros.h /usr/include/utmp.h /usr/include/setjmp.h
init: /usr/include/sys/reboot.h /usr/include/errno.h /usr/include/sys/errno.h
init: /usr/include/sys/file.h /usr/include/sys/fcntl.h /usr/include/sys/ioctl.h
init: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
init: /usr/include/sgtty.h

mknod: mknod.c /usr/include/stdio.h /usr/include/sys/types.h
mknod: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h

newfs: newfs.c /usr/include/sys/param.h /usr/include/machine/param.h
newfs: /usr/include/signal.h /usr/include/sys/types.h
newfs: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
newfs: /usr/include/ufs/fs.h /usr/include/sys/dir.h /usr/include/stdio.h
newfs: /usr/include/disktab.h /usr/include/sys/ioctl.h
newfs: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
newfs: /usr/include/sgtty.h /usr/include/sun/dklabel.h /usr/include/sun/dkio.h
newfs: /usr/include/sys/ioctl.h /usr/include/sun/dklabel.h

nfsd: nfsd.c /usr/include/sys/param.h /usr/include/machine/param.h
nfsd: /usr/include/signal.h /usr/include/sys/types.h
nfsd: /usr/include/sys/sysmacros.h /usr/include/rpc/rpc.h
nfsd: /usr/include/rpc/types.h /usr/include/sys/types.h
nfsd: /usr/include/netinet/in.h /usr/include/rpc/xdr.h /usr/include/rpc/auth.h
nfsd: /usr/include/rpc/clnt.h /usr/include/rpc/rpc_msg.h
nfsd: /usr/include/rpc/auth_unix.h /usr/include/rpc/svc.h
nfsd: /usr/include/rpc/svc_auth.h /usr/include/sys/socket.h
nfsd: /usr/include/sys/errno.h /usr/include/sys/time.h /usr/include/time.h
nfsd: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
nfsd: /usr/include/sys/ttydev.h /usr/include/sgtty.h /usr/include/sys/file.h
nfsd: /usr/include/sys/fcntl.h /usr/include/nfs/nfs.h /usr/include/stdio.h
nfsd: /usr/include/signal.h

portmap: portmap.c /usr/include/rpc/rpc.h /usr/include/rpc/types.h
portmap: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
portmap: /usr/include/netinet/in.h /usr/include/rpc/xdr.h
portmap: /usr/include/rpc/auth.h /usr/include/rpc/clnt.h
portmap: /usr/include/rpc/rpc_msg.h /usr/include/rpc/auth_unix.h
portmap: /usr/include/rpc/svc.h /usr/include/rpc/svc_auth.h
portmap: /usr/include/rpc/pmap_prot.h /usr/include/stdio.h /usr/include/netdb.h
portmap: /usr/include/sys/socket.h /usr/include/sys/time.h /usr/include/time.h
portmap: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
portmap: /usr/include/sys/ttydev.h /usr/include/sgtty.h

reboot: reboot.c /usr/include/stdio.h /usr/include/sys/reboot.h
reboot: /usr/include/errno.h /usr/include/sys/errno.h /usr/include/signal.h
reboot: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
reboot: /usr/include/sys/time.h /usr/include/time.h /usr/include/syscall.h
reboot: /usr/include/utmp.h

renice: renice.c /usr/include/sys/time.h /usr/include/time.h
renice: /usr/include/sys/resource.h /usr/include/stdio.h /usr/include/pwd.h

rmt: rmt.c /usr/include/stdio.h /usr/include/sgtty.h /usr/include/sys/ioctl.h
rmt: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h /usr/include/sgtty.h
rmt: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
rmt: /usr/include/sys/socket.h /usr/include/sys/mtio.h /usr/include/errno.h
rmt: /usr/include/sys/errno.h

vipw: vipw.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
vipw: /usr/include/sys/stat.h /usr/include/sys/file.h /usr/include/sys/fcntl.h
vipw: /usr/include/stdio.h /usr/include/errno.h /usr/include/sys/errno.h
vipw: /usr/include/signal.h

ypbind: ypbind.c /usr/include/stdio.h /usr/include/errno.h
ypbind: /usr/include/sys/errno.h /usr/include/signal.h /usr/include/sys/time.h
ypbind: /usr/include/time.h /usr/include/sys/wait.h /usr/include/rpc/rpc.h
ypbind: /usr/include/rpc/types.h /usr/include/sys/types.h
ypbind: /usr/include/sys/sysmacros.h /usr/include/netinet/in.h
ypbind: /usr/include/rpc/xdr.h /usr/include/rpc/auth.h /usr/include/rpc/clnt.h
ypbind: /usr/include/rpc/rpc_msg.h /usr/include/rpc/auth_unix.h
ypbind: /usr/include/rpc/svc.h /usr/include/rpc/svc_auth.h
ypbind: /usr/include/sys/dir.h /usr/include/netdb.h /usr/include/sys/ioctl.h
ypbind: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
ypbind: /usr/include/sgtty.h /usr/include/sys/socket.h
ypbind: /usr/include/rpcsvc/yp_prot.h /usr/include/rpcsvc/ypv1_prot.h
ypbind: /usr/include/rpcsvc/ypclnt.h

update: update.c /usr/include/signal.h

mkfs: mkfs.c /usr/include/stdio.h /usr/include/a.out.h /usr/include/sys/exec.h
mkfs: /usr/include/sys/param.h /usr/include/machine/param.h
mkfs: /usr/include/signal.h /usr/include/sys/types.h
mkfs: /usr/include/sys/sysmacros.h /usr/include/sys/time.h /usr/include/time.h
mkfs: /usr/include/sys/vnode.h /usr/include/ufs/fsdir.h
mkfs: /usr/include/ufs/inode.h /usr/include/ufs/fs.h

in.rlogind: in.rlogind.c /usr/include/stdio.h /usr/include/sys/types.h
in.rlogind: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
in.rlogind: /usr/include/sys/socket.h /usr/include/sys/wait.h
in.rlogind: /usr/include/sys/file.h /usr/include/sys/fcntl.h
in.rlogind: /usr/include/netinet/in.h /usr/include/errno.h
in.rlogind: /usr/include/sys/errno.h /usr/include/pwd.h /usr/include/signal.h
in.rlogind: /usr/include/sgtty.h /usr/include/sys/ioctl.h
in.rlogind: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
in.rlogind: /usr/include/sgtty.h /usr/include/stdio.h /usr/include/netdb.h
in.rlogind: /usr/include/syslog.h /usr/include/strings.h /usr/include/utmp.h

in.rshd: in.rshd.c /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
in.rshd: /usr/include/sys/ttydev.h /usr/include/sgtty.h
in.rshd: /usr/include/sys/param.h /usr/include/machine/param.h
in.rshd: /usr/include/signal.h /usr/include/sys/types.h
in.rshd: /usr/include/sys/sysmacros.h /usr/include/sys/socket.h
in.rshd: /usr/include/sys/wait.h /usr/include/netinet/in.h /usr/include/stdio.h
in.rshd: /usr/include/errno.h /usr/include/sys/errno.h /usr/include/pwd.h
in.rshd: /usr/include/signal.h /usr/include/netdb.h

mount: mount.c /usr/include/sys/param.h /usr/include/machine/param.h
mount: /usr/include/signal.h /usr/include/sys/types.h
mount: /usr/include/sys/sysmacros.h /usr/include/rpc/rpc.h
mount: /usr/include/rpc/types.h /usr/include/sys/types.h
mount: /usr/include/netinet/in.h /usr/include/rpc/xdr.h /usr/include/rpc/auth.h
mount: /usr/include/rpc/clnt.h /usr/include/rpc/rpc_msg.h
mount: /usr/include/rpc/auth_unix.h /usr/include/rpc/svc.h
mount: /usr/include/rpc/svc_auth.h /usr/include/sys/errno.h
mount: /usr/include/sys/time.h /usr/include/time.h /usr/include/nfs/nfs.h
mount: /usr/include/rpcsvc/mount.h /usr/include/sys/socket.h
mount: /usr/include/sys/stat.h /usr/include/netdb.h /usr/include/stdio.h
mount: /usr/include/mntent.h /usr/include/sys/mount.h

shutdown: shutdown.c /usr/include/stdio.h /usr/include/ctype.h
shutdown: /usr/include/signal.h /usr/include/setjmp.h /usr/include/utmp.h
shutdown: /usr/include/sys/time.h /usr/include/time.h
shutdown: /usr/include/sys/resource.h /usr/include/sys/types.h
shutdown: /usr/include/sys/sysmacros.h /usr/include/sys/errno.h
shutdown: /usr/include/rpc/rpc.h /usr/include/rpc/types.h
shutdown: /usr/include/sys/types.h /usr/include/netinet/in.h
shutdown: /usr/include/rpc/xdr.h /usr/include/rpc/auth.h
shutdown: /usr/include/rpc/clnt.h /usr/include/rpc/rpc_msg.h
shutdown: /usr/include/rpc/auth_unix.h /usr/include/rpc/svc.h
shutdown: /usr/include/rpc/svc_auth.h /usr/include/rpc/pmap_clnt.h
shutdown: /usr/include/nfs/nfs.h /usr/include/rpcsvc/mount.h
shutdown: /usr/include/rpcsvc/rwall.h /usr/include/sys/socket.h
shutdown: /usr/include/netdb.h

umount: umount.c /usr/include/sys/param.h /usr/include/machine/param.h
umount: /usr/include/signal.h /usr/include/sys/types.h
umount: /usr/include/sys/sysmacros.h /usr/include/sys/file.h
umount: /usr/include/sys/fcntl.h /usr/include/stdio.h /usr/include/mntent.h
umount: /usr/include/errno.h /usr/include/sys/errno.h /usr/include/sys/time.h
umount: /usr/include/time.h /usr/include/rpc/rpc.h /usr/include/rpc/types.h
umount: /usr/include/sys/types.h /usr/include/netinet/in.h
umount: /usr/include/rpc/xdr.h /usr/include/rpc/auth.h /usr/include/rpc/clnt.h
umount: /usr/include/rpc/rpc_msg.h /usr/include/rpc/auth_unix.h
umount: /usr/include/rpc/svc.h /usr/include/rpc/svc_auth.h
umount: /usr/include/nfs/nfs.h /usr/include/rpcsvc/mount.h
umount: /usr/include/sys/socket.h /usr/include/netdb.h

pstat: pstat.c /usr/include/stdio.h /usr/include/sys/param.h
pstat: /usr/include/machine/param.h /usr/include/signal.h
pstat: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
pstat: /usr/include/sys/dir.h /usr/include/sys/file.h
pstat: /usr/src/sun/sys/sys/../h/fcntl.h /usr/include/sys/user.h
pstat: /usr/include/machine/pcb.h /usr/include/machine/reg.h
pstat: /usr/include/sys/dmap.h /usr/include/sys/time.h /usr/include/time.h
pstat: /usr/include/sys/resource.h /usr/include/sys/exec.h /usr/include/errno.h
pstat: /usr/include/sys/errno.h /usr/include/sys/proc.h /usr/include/sys/text.h
pstat: /usr/include/sys/vnode.h /usr/include/ufs/inode.h /usr/include/sys/map.h
pstat: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
pstat: /usr/include/sys/ttydev.h /usr/include/sgtty.h /usr/include/sys/tty.h
pstat: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
pstat: /usr/include/sys/conf.h /usr/include/sys/vm.h /usr/include/sys/vmparam.h
pstat: /usr/include/machine/vmparam.h /usr/include/sys/vmmac.h
pstat: /usr/include/sys/vmmeter.h /usr/include/sys/vmsystm.h
pstat: /usr/include/nlist.h /usr/include/machine/pte.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
