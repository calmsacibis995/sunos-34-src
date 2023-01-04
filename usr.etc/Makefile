#
# @(#)Makefile 1.3 87/02/13 SMI; from UCB 4.27 83/07/06
#
DESTDIR=
CFLAGS=	-O
LDFLAGS= -n

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	config htable implog in.ftpd in.named in.rwhod in.syslog rexd \
 termcap yp ypserv

# Shell scripts that need only be installed and are never removed.
#
SCRIPT= in.tftpbootd

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	accton catman chroot \
	devnm dumpfs edquota gettable grpck \
	in.comsat in.fingerd in.rexecd in.telnetd \
  	in.timed in.tnamed in.tftpd kgmon link mkproto nfsstat \
	ping pwck quotaon rarpd repquota \
	route rpcinfo \
	sa savecore swapon trpt tunefs unlink

# Programs that must run setuid to root
#
SETUID=	ping 

# C programs that live in the current directory and need explicit make lines.
#
NSTD= \
	ac analyze dcheck etherfind icheck ncheck quot quotacheck

# C programs that use the rpcsvc library
#
RPCSVC=	rpc.etherd rpc.mountd rpc.rquotad rpc.rstatd rpc.rwalld rpc.rusersd \
	rpc.sprayd rpc.yppasswdd rwall showmount spray

# C programs that must run set-group-id kmem.
#
KMEM=	kgmon nfsstat trpt

all:	${SUBDIR} ${STD} ${NSTD} ${RPCSVC}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

${STD}:
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $@.c

${RPCSVC}:
	${CC} ${CFLAGS} -o $@ $@.c -lrpcsvc

install: ${STD} ${NSTD} ${RPCSVC} ${SCRIPT}
	-mkdir ${DESTDIR}/usr/etc && chown bin ${DESTDIR}/usr/etc && \
		chmod 755 ${DESTDIR}/usr/etc
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	for i in ${SCRIPT}; do (install -c $$i ${DESTDIR}/usr/etc/$$i); done
	for i in ${STD} ${NSTD} ${RPCSVC}; do \
		if [ $$i = "rpc.mountd" ]; then \
			(install $$i ${DESTDIR}/usr/etc/$$i); \
		else \
			(install -s $$i ${DESTDIR}/usr/etc/$$i);\
		fi \
	done
	rm -f ${DESTDIR}/usr/etc/quotaoff
	for i in ${SETUID}; do (chown root ${DESTDIR}/usr/etc/$$i; \
		chmod 4755 ${DESTDIR}/usr/etc/$$i); done
	-for i in ${KMEM}; do (chgrp kmem ${DESTDIR}/usr/etc/$$i; \
		chmod 2755 ${DESTDIR}/usr/etc/$$i); done
	ln ${DESTDIR}/usr/etc/quotaon ${DESTDIR}/usr/etc/quotaoff

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
	rm -f ${STD} ${NSTD} ${RPCSVC}

FRC:

depend:
	rm -f makedep
	for i in ${STD} ${NSTD} ${RPCSVC}; do \
	    ${CC} -M ${INCPATH} $$i.c | sed 's/\.o//' | \
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

# Following files have explicit make lines because they
# are big and should be linked to be demand paged.

ac:	ac.c
	${CC} ${CFLAGS} -o ac ac.c

analyze:	analyze.c
	${CC} ${CFLAGS} -o analyze analyze.c

dcheck:	dcheck.c
	${CC} ${CFLAGS} -o dcheck dcheck.c

etherfind:	etherfind.c
	${CC} ${CFLAGS} -o etherfind etherfind.c

icheck:	icheck.c
	${CC} ${CFLAGS} -o icheck icheck.c

ncheck:	ncheck.c
	${CC} ${CFLAGS} -o ncheck ncheck.c

quot:	quot.c
	${CC} ${CFLAGS} -o quot quot.c

quotacheck:	quotacheck.c
	${CC} ${CFLAGS} -o quotacheck quotacheck.c

# DO NOT DELETE THIS LINE -- make depend uses it


accton: accton.c

catman: catman.c /usr/include/stdio.h /usr/include/sys/types.h
catman: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
catman: /usr/include/sys/time.h /usr/include/time.h /usr/include/sys/param.h
catman: /usr/include/machine/param.h /usr/include/signal.h
catman: /usr/include/sys/types.h /usr/include/sys/dir.h /usr/include/ctype.h

chroot: chroot.c /usr/include/stdio.h

devnm: devnm.c /usr/include/stdio.h /usr/include/sys/types.h
devnm: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
devnm: /usr/include/sys/dir.h

dumpfs: dumpfs.c /usr/include/sys/param.h /usr/include/machine/param.h
dumpfs: /usr/include/signal.h /usr/include/sys/types.h
dumpfs: /usr/include/sys/sysmacros.h /usr/include/sys/time.h
dumpfs: /usr/include/time.h /usr/include/sys/vnode.h /usr/include/ufs/inode.h
dumpfs: /usr/include/ufs/fs.h /usr/include/stdio.h /usr/include/fstab.h

edquota: edquota.c /usr/include/stdio.h /usr/include/signal.h
edquota: /usr/include/errno.h /usr/include/sys/errno.h /usr/include/pwd.h
edquota: /usr/include/ctype.h /usr/include/mntent.h /usr/include/sys/param.h
edquota: /usr/include/machine/param.h /usr/include/signal.h
edquota: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
edquota: /usr/include/sys/stat.h /usr/include/sys/file.h
edquota: /usr/include/sys/fcntl.h /usr/include/ufs/quota.h

gettable: gettable.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
gettable: /usr/include/sys/socket.h /usr/include/netinet/in.h
gettable: /usr/include/stdio.h /usr/include/netdb.h

grpck: grpck.c /usr/include/stdio.h /usr/include/ctype.h /usr/include/pwd.h

in.comsat: in.comsat.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
in.comsat: /usr/include/sys/socket.h /usr/include/sys/stat.h
in.comsat: /usr/include/sys/wait.h /usr/include/netinet/in.h
in.comsat: /usr/include/stdio.h /usr/include/sgtty.h /usr/include/sys/ioctl.h
in.comsat: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
in.comsat: /usr/include/sgtty.h /usr/include/utmp.h /usr/include/signal.h
in.comsat: /usr/include/errno.h /usr/include/sys/errno.h /usr/include/netdb.h

in.fingerd: in.fingerd.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
in.fingerd: /usr/include/netinet/in.h /usr/include/stdio.h /usr/include/ctype.h

in.rexecd: in.rexecd.c /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
in.rexecd: /usr/include/sys/ttydev.h /usr/include/sgtty.h
in.rexecd: /usr/include/sys/param.h /usr/include/machine/param.h
in.rexecd: /usr/include/signal.h /usr/include/sys/types.h
in.rexecd: /usr/include/sys/sysmacros.h /usr/include/sys/socket.h
in.rexecd: /usr/include/sys/wait.h /usr/include/netinet/in.h
in.rexecd: /usr/include/stdio.h /usr/include/errno.h /usr/include/sys/errno.h
in.rexecd: /usr/include/pwd.h /usr/include/signal.h /usr/include/netdb.h

in.telnetd: in.telnetd.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
in.telnetd: /usr/include/sys/socket.h /usr/include/sys/wait.h
in.telnetd: /usr/include/sys/file.h /usr/include/sys/fcntl.h
in.telnetd: /usr/include/sys/stat.h /usr/include/netinet/in.h
in.telnetd: /usr/include/arpa/telnet.h /usr/include/stdio.h
in.telnetd: /usr/include/signal.h /usr/include/errno.h /usr/include/sys/errno.h
in.telnetd: /usr/include/sgtty.h /usr/include/sys/ioctl.h
in.telnetd: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
in.telnetd: /usr/include/sgtty.h /usr/include/netdb.h /usr/include/syslog.h
in.telnetd: /usr/include/utmp.h

in.timed: in.timed.c /usr/include/sys/time.h /usr/include/time.h
in.timed: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
in.timed: /usr/include/sys/socket.h /usr/include/stdio.h

in.tnamed: in.tnamed.c /usr/include/stdio.h /usr/include/strings.h
in.tnamed: /usr/include/signal.h /usr/include/sys/time.h /usr/include/time.h
in.tnamed: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
in.tnamed: /usr/include/sys/socket.h /usr/include/netinet/in.h
in.tnamed: /usr/include/netdb.h

in.tftpd: in.tftpd.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
in.tftpd: /usr/include/sys/socket.h /usr/include/sys/ioctl.h
in.tftpd: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
in.tftpd: /usr/include/sgtty.h /usr/include/sys/wait.h /usr/include/sys/stat.h
in.tftpd: /usr/include/netinet/in.h /usr/include/arpa/tftp.h
in.tftpd: /usr/include/signal.h /usr/include/stdio.h /usr/include/errno.h
in.tftpd: /usr/include/sys/errno.h /usr/include/ctype.h /usr/include/netdb.h
in.tftpd: /usr/include/setjmp.h

kgmon: kgmon.c /usr/include/sys/param.h /usr/include/machine/param.h
kgmon: /usr/include/signal.h /usr/include/sys/types.h
kgmon: /usr/include/sys/sysmacros.h /usr/include/machine/pte.h
kgmon: /usr/include/sys/vm.h /usr/include/sys/vmparam.h
kgmon: /usr/include/machine/vmparam.h /usr/include/sys/vmmac.h
kgmon: /usr/include/sys/vmmeter.h /usr/include/sys/vmsystm.h
kgmon: /usr/include/stdio.h /usr/include/nlist.h /usr/include/ctype.h
kgmon: /usr/include/sys/gprof.h

link: link.c

mkproto: mkproto.c /usr/include/stdio.h /usr/include/sys/param.h
mkproto: /usr/include/machine/param.h /usr/include/signal.h
mkproto: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
mkproto: /usr/include/sys/time.h /usr/include/time.h /usr/include/sys/vnode.h
mkproto: /usr/include/ufs/inode.h /usr/include/ufs/fs.h
mkproto: /usr/include/ufs/fsdir.h

nfsstat: nfsstat.c /usr/include/stdio.h /usr/include/sys/param.h
nfsstat: /usr/include/machine/param.h /usr/include/signal.h
nfsstat: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
nfsstat: /usr/include/sys/types.h /usr/include/sys/socket.h
nfsstat: /usr/include/net/if.h /usr/include/netinet/in.h
nfsstat: /usr/include/netinet/if_ether.h /usr/include/sun/ndio.h
nfsstat: /usr/include/nlist.h

ping: ping.c /usr/include/stdio.h /usr/include/errno.h /usr/include/sys/errno.h
ping: /usr/include/sys/time.h /usr/include/time.h /usr/include/sys/param.h
ping: /usr/include/machine/param.h /usr/include/signal.h
ping: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
ping: /usr/include/sys/socket.h /usr/include/sys/file.h
ping: /usr/include/sys/fcntl.h /usr/include/netinet/in_systm.h
ping: /usr/include/netinet/in.h /usr/include/netinet/ip.h
ping: /usr/include/netinet/ip_icmp.h /usr/include/netdb.h

pwck: pwck.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
pwck: /usr/include/sys/param.h /usr/include/machine/param.h
pwck: /usr/include/signal.h /usr/include/sys/types.h /usr/include/sys/signal.h
pwck: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h /usr/include/stdio.h
pwck: /usr/include/ctype.h

quotaon: quotaon.c /usr/include/sys/param.h /usr/include/machine/param.h
quotaon: /usr/include/signal.h /usr/include/sys/types.h
quotaon: /usr/include/sys/sysmacros.h /usr/include/sys/file.h
quotaon: /usr/include/sys/fcntl.h /usr/include/ufs/quota.h /usr/include/stdio.h
quotaon: /usr/include/mntent.h

rarpd: rarpd.c /usr/include/stdio.h /usr/include/sys/file.h
rarpd: /usr/include/sys/fcntl.h /usr/include/sys/ioctl.h
rarpd: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
rarpd: /usr/include/sgtty.h /usr/include/errno.h /usr/include/sys/errno.h
rarpd: /usr/include/sys/time.h /usr/include/time.h /usr/include/sys/types.h
rarpd: /usr/include/sys/sysmacros.h /usr/include/sys/socket.h
rarpd: /usr/include/net/if.h /usr/include/net/nit.h /usr/include/netinet/in.h
rarpd: /usr/include/netinet/if_ether.h /usr/include/netinet/ip_var.h
rarpd: /usr/include/netinet/udp.h /usr/include/netinet/udp_var.h
rarpd: /usr/include/netdb.h

repquota: repquota.c /usr/include/stdio.h /usr/include/errno.h
repquota: /usr/include/sys/errno.h /usr/include/sys/param.h
repquota: /usr/include/machine/param.h /usr/include/signal.h
repquota: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
repquota: /usr/include/ufs/quota.h /usr/include/sys/stat.h
repquota: /usr/include/sys/time.h /usr/include/time.h /usr/include/mntent.h
repquota: /usr/include/pwd.h

route: route.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
route: /usr/include/sys/socket.h /usr/include/sys/ioctl.h
route: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
route: /usr/include/sgtty.h /usr/include/sys/mbuf.h /usr/include/net/route.h
route: /usr/include/netinet/in.h /usr/include/stdio.h /usr/include/errno.h
route: /usr/include/sys/errno.h /usr/include/ctype.h /usr/include/netdb.h
route: /usr/include/nlist.h

rpcinfo: rpcinfo.c /usr/include/rpc/rpc.h /usr/include/rpc/types.h
rpcinfo: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
rpcinfo: /usr/include/netinet/in.h /usr/include/rpc/xdr.h
rpcinfo: /usr/include/rpc/auth.h /usr/include/rpc/clnt.h
rpcinfo: /usr/include/rpc/rpc_msg.h /usr/include/rpc/auth_unix.h
rpcinfo: /usr/include/rpc/svc.h /usr/include/rpc/svc_auth.h
rpcinfo: /usr/include/stdio.h /usr/include/sys/socket.h /usr/include/sys/time.h
rpcinfo: /usr/include/time.h /usr/include/netdb.h /usr/include/rpc/pmap_prot.h
rpcinfo: /usr/include/rpc/pmap_clnt.h /usr/include/ctype.h

sa: sa.c /usr/include/stdio.h /usr/include/sys/types.h
sa: /usr/include/sys/sysmacros.h /usr/include/sys/acct.h /usr/include/signal.h
sa: /usr/include/utmp.h /usr/include/pwd.h

savecore: savecore.c /usr/include/stdio.h /usr/include/nlist.h
savecore: /usr/include/sys/param.h /usr/include/machine/param.h
savecore: /usr/include/signal.h /usr/include/sys/types.h
savecore: /usr/include/sys/sysmacros.h /usr/include/sys/dir.h
savecore: /usr/include/sys/stat.h /usr/include/sys/vfs.h
savecore: /usr/include/sys/time.h /usr/include/time.h

swapon: swapon.c /usr/include/stdio.h /usr/include/fstab.h /usr/include/errno.h
swapon: /usr/include/sys/errno.h

trpt: trpt.c /usr/include/sys/param.h /usr/include/machine/param.h
trpt: /usr/include/signal.h /usr/include/sys/types.h
trpt: /usr/include/sys/sysmacros.h /usr/include/sys/socket.h
trpt: /usr/include/sys/socketvar.h /usr/include/sys/protosw.h
trpt: /usr/include/net/route.h /usr/include/net/if.h /usr/include/netinet/in.h
trpt: /usr/include/netinet/in_pcb.h /usr/include/netinet/in_systm.h
trpt: /usr/include/netinet/ip.h /usr/include/netinet/ip_var.h
trpt: /usr/include/netinet/tcp.h /usr/include/netinet/tcp_fsm.h
trpt: /usr/include/netinet/tcp_seq.h /usr/include/netinet/tcp_timer.h
trpt: /usr/include/netinet/tcp_var.h /usr/include/netinet/tcpip.h
trpt: /usr/include/netinet/tcp_debug.h /usr/include/stdio.h
trpt: /usr/include/errno.h /usr/include/sys/errno.h /usr/include/nlist.h

tunefs: tunefs.c /usr/include/sys/param.h /usr/include/machine/param.h
tunefs: /usr/include/signal.h /usr/include/sys/types.h
tunefs: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
tunefs: /usr/include/ufs/fs.h /usr/include/sys/time.h /usr/include/time.h
tunefs: /usr/include/sys/vnode.h /usr/include/ufs/inode.h /usr/include/stdio.h
tunefs: /usr/include/fstab.h

unlink: unlink.c

ac: ac.c /usr/include/stdio.h /usr/include/ctype.h /usr/include/utmp.h
ac: /usr/include/sys/time.h /usr/include/time.h /usr/include/sys/types.h
ac: /usr/include/sys/sysmacros.h /usr/include/sys/timeb.h

analyze: analyze.c /usr/include/stdio.h /usr/include/sys/param.h
analyze: /usr/include/machine/param.h /usr/include/signal.h
analyze: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
analyze: /usr/include/sys/dir.h /usr/include/machine/pte.h /usr/include/nlist.h
analyze: /usr/include/sys/map.h /usr/include/sys/user.h
analyze: /usr/include/machine/pcb.h /usr/include/machine/reg.h
analyze: /usr/include/sys/dmap.h /usr/include/sys/time.h /usr/include/time.h
analyze: /usr/include/sys/resource.h /usr/include/sys/exec.h
analyze: /usr/include/errno.h /usr/include/sys/errno.h /usr/include/sys/proc.h
analyze: /usr/include/sys/text.h /usr/include/sys/cmap.h /usr/include/sys/vm.h
analyze: /usr/include/sys/vmparam.h /usr/include/machine/vmparam.h
analyze: /usr/include/sys/vmmac.h /usr/include/sys/vmmeter.h
analyze: /usr/include/sys/vmsystm.h

dcheck: dcheck.c /usr/include/sys/param.h /usr/include/machine/param.h
dcheck: /usr/include/signal.h /usr/include/sys/types.h
dcheck: /usr/include/sys/sysmacros.h /usr/include/sys/time.h
dcheck: /usr/include/time.h /usr/include/sys/vnode.h /usr/include/ufs/inode.h
dcheck: /usr/include/ufs/fs.h /usr/include/sys/dir.h /usr/include/stdio.h

etherfind: etherfind.c /usr/include/stdio.h /usr/include/netdb.h
etherfind: /usr/include/ctype.h /usr/include/sys/types.h
etherfind: /usr/include/sys/sysmacros.h /usr/include/sys/time.h
etherfind: /usr/include/time.h /usr/include/sys/socket.h
etherfind: /usr/include/sys/file.h /usr/include/sys/fcntl.h
etherfind: /usr/include/sys/signal.h /usr/include/net/if.h
etherfind: /usr/include/netinet/in.h /usr/include/netinet/in_systm.h
etherfind: /usr/include/netinet/ip.h /usr/include/netinet/if_ether.h
etherfind: /usr/include/netinet/udp.h /usr/include/net/route.h
etherfind: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
etherfind: /usr/include/sys/ttydev.h /usr/include/sgtty.h
etherfind: /usr/include/net/nit.h /usr/include/netinet/in_pcb.h
etherfind: /usr/include/netinet/ip_var.h /usr/include/netinet/udp_var.h
etherfind: /usr/include/netinet/ip_icmp.h

icheck: icheck.c /usr/include/stdio.h /usr/include/sys/param.h
icheck: /usr/include/machine/param.h /usr/include/signal.h
icheck: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
icheck: /usr/include/sys/time.h /usr/include/time.h /usr/include/sys/vnode.h
icheck: /usr/include/ufs/inode.h /usr/include/ufs/fs.h

ncheck: ncheck.c /usr/include/sys/param.h /usr/include/machine/param.h
ncheck: /usr/include/signal.h /usr/include/sys/types.h
ncheck: /usr/include/sys/sysmacros.h /usr/include/sys/time.h
ncheck: /usr/include/time.h /usr/include/sys/vnode.h /usr/include/ufs/inode.h
ncheck: /usr/include/ufs/fs.h /usr/include/sys/dir.h /usr/include/stdio.h

quot: quot.c /usr/include/stdio.h /usr/include/ctype.h /usr/include/fstab.h
quot: /usr/include/mntent.h /usr/include/sys/param.h
quot: /usr/include/machine/param.h /usr/include/signal.h
quot: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
quot: /usr/include/sys/time.h /usr/include/time.h /usr/include/sys/vnode.h
quot: /usr/include/ufs/inode.h /usr/include/ufs/fs.h /usr/include/sys/file.h
quot: /usr/include/sys/fcntl.h /usr/include/sys/stat.h /usr/include/pwd.h
quot: /usr/include/utmp.h

quotacheck: quotacheck.c /usr/include/stdio.h /usr/include/ctype.h
quotacheck: /usr/include/signal.h /usr/include/errno.h /usr/include/sys/errno.h
quotacheck: /usr/include/sys/param.h /usr/include/machine/param.h
quotacheck: /usr/include/signal.h /usr/include/sys/types.h
quotacheck: /usr/include/sys/sysmacros.h /usr/include/sys/time.h
quotacheck: /usr/include/time.h /usr/include/sys/vnode.h
quotacheck: /usr/include/ufs/inode.h /usr/include/ufs/fs.h
quotacheck: /usr/include/ufs/quota.h /usr/include/sys/stat.h
quotacheck: /usr/include/sys/wait.h /usr/include/mntent.h /usr/include/pwd.h

rpc.etherd: rpc.etherd.c /usr/include/stdio.h /usr/include/rpc/rpc.h
rpc.etherd: /usr/include/rpc/types.h /usr/include/sys/types.h
rpc.etherd: /usr/include/sys/sysmacros.h /usr/include/netinet/in.h
rpc.etherd: /usr/include/rpc/xdr.h /usr/include/rpc/auth.h
rpc.etherd: /usr/include/rpc/clnt.h /usr/include/rpc/rpc_msg.h
rpc.etherd: /usr/include/rpc/auth_unix.h /usr/include/rpc/svc.h
rpc.etherd: /usr/include/rpc/svc_auth.h /usr/include/netdb.h
rpc.etherd: /usr/include/ctype.h /usr/include/signal.h /usr/include/sys/ioctl.h
rpc.etherd: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
rpc.etherd: /usr/include/sgtty.h /usr/include/sys/socket.h
rpc.etherd: /usr/include/sys/file.h /usr/include/sys/fcntl.h
rpc.etherd: /usr/include/sys/time.h /usr/include/time.h
rpc.etherd: /usr/include/sys/errno.h /usr/include/net/if.h
rpc.etherd: /usr/include/netinet/in_systm.h /usr/include/netinet/ip.h
rpc.etherd: /usr/include/netinet/if_ether.h /usr/include/netinet/udp.h
rpc.etherd: /usr/include/net/route.h /usr/include/net/nit.h
rpc.etherd: /usr/include/netinet/in_pcb.h /usr/include/netinet/ip_var.h
rpc.etherd: /usr/include/netinet/udp_var.h /usr/include/netinet/ip_icmp.h
rpc.etherd: /usr/include/rpcsvc/ether.h

rpc.mountd: rpc.mountd.c /usr/include/sys/param.h /usr/include/machine/param.h
rpc.mountd: /usr/include/signal.h /usr/include/sys/types.h
rpc.mountd: /usr/include/sys/sysmacros.h /usr/include/ufs/fs.h
rpc.mountd: /usr/include/rpc/rpc.h /usr/include/rpc/types.h
rpc.mountd: /usr/include/sys/types.h /usr/include/netinet/in.h
rpc.mountd: /usr/include/rpc/xdr.h /usr/include/rpc/auth.h
rpc.mountd: /usr/include/rpc/clnt.h /usr/include/rpc/rpc_msg.h
rpc.mountd: /usr/include/rpc/auth_unix.h /usr/include/rpc/svc.h
rpc.mountd: /usr/include/rpc/svc_auth.h /usr/include/sys/stat.h
rpc.mountd: /usr/include/sys/socket.h /usr/include/sys/file.h
rpc.mountd: /usr/include/sys/fcntl.h /usr/include/sys/time.h
rpc.mountd: /usr/include/time.h /usr/include/stdio.h /usr/include/signal.h
rpc.mountd: /usr/include/sys/wait.h /usr/include/sys/ioctl.h
rpc.mountd: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
rpc.mountd: /usr/include/sgtty.h /usr/include/sys/errno.h
rpc.mountd: /usr/include/nfs/nfs.h /usr/include/rpcsvc/mount.h
rpc.mountd: /usr/include/netdb.h

rpc.rquotad: rpc.rquotad.c /usr/include/stdio.h /usr/include/mntent.h
rpc.rquotad: /usr/include/sys/param.h /usr/include/machine/param.h
rpc.rquotad: /usr/include/signal.h /usr/include/sys/types.h
rpc.rquotad: /usr/include/sys/sysmacros.h /usr/include/sys/time.h
rpc.rquotad: /usr/include/time.h /usr/include/sys/socket.h
rpc.rquotad: /usr/include/sys/stat.h /usr/include/sys/file.h
rpc.rquotad: /usr/include/sys/fcntl.h /usr/include/ufs/quota.h
rpc.rquotad: /usr/include/rpc/rpc.h /usr/include/rpc/types.h
rpc.rquotad: /usr/include/sys/types.h /usr/include/netinet/in.h
rpc.rquotad: /usr/include/rpc/xdr.h /usr/include/rpc/auth.h
rpc.rquotad: /usr/include/rpc/clnt.h /usr/include/rpc/rpc_msg.h
rpc.rquotad: /usr/include/rpc/auth_unix.h /usr/include/rpc/svc.h
rpc.rquotad: /usr/include/rpc/svc_auth.h /usr/include/rpcsvc/rquota.h

rpc.rstatd: rpc.rstatd.c /usr/include/signal.h /usr/include/stdio.h
rpc.rstatd: /usr/include/rpc/rpc.h /usr/include/rpc/types.h
rpc.rstatd: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
rpc.rstatd: /usr/include/netinet/in.h /usr/include/rpc/xdr.h
rpc.rstatd: /usr/include/rpc/auth.h /usr/include/rpc/clnt.h
rpc.rstatd: /usr/include/rpc/rpc_msg.h /usr/include/rpc/auth_unix.h
rpc.rstatd: /usr/include/rpc/svc.h /usr/include/rpc/svc_auth.h
rpc.rstatd: /usr/include/sys/socket.h /usr/include/nlist.h
rpc.rstatd: /usr/include/sys/dk.h /usr/include/sys/errno.h
rpc.rstatd: /usr/include/sys/vmmeter.h /usr/include/net/if.h
rpc.rstatd: /usr/include/sys/time.h /usr/include/time.h
rpc.rstatd: /usr/include/rpcsvc/rstat.h

rpc.rwalld: rpc.rwalld.c /usr/include/rpcsvc/rwall.h /usr/include/rpc/rpc.h
rpc.rwalld: /usr/include/rpc/types.h /usr/include/sys/types.h
rpc.rwalld: /usr/include/sys/sysmacros.h /usr/include/netinet/in.h
rpc.rwalld: /usr/include/rpc/xdr.h /usr/include/rpc/auth.h
rpc.rwalld: /usr/include/rpc/clnt.h /usr/include/rpc/rpc_msg.h
rpc.rwalld: /usr/include/rpc/auth_unix.h /usr/include/rpc/svc.h
rpc.rwalld: /usr/include/rpc/svc_auth.h /usr/include/stdio.h
rpc.rwalld: /usr/include/netdb.h /usr/include/sys/socket.h

rpc.rusersd: rpc.rusersd.c /usr/include/rpc/rpc.h /usr/include/rpc/types.h
rpc.rusersd: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
rpc.rusersd: /usr/include/netinet/in.h /usr/include/rpc/xdr.h
rpc.rusersd: /usr/include/rpc/auth.h /usr/include/rpc/clnt.h
rpc.rusersd: /usr/include/rpc/rpc_msg.h /usr/include/rpc/auth_unix.h
rpc.rusersd: /usr/include/rpc/svc.h /usr/include/rpc/svc_auth.h
rpc.rusersd: /usr/include/utmp.h /usr/include/rpcsvc/rusers.h
rpc.rusersd: /usr/include/stdio.h /usr/include/sys/socket.h
rpc.rusersd: /usr/include/sys/stat.h

rpc.sprayd: rpc.sprayd.c /usr/include/stdio.h /usr/include/signal.h
rpc.sprayd: /usr/include/rpc/rpc.h /usr/include/rpc/types.h
rpc.sprayd: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
rpc.sprayd: /usr/include/netinet/in.h /usr/include/rpc/xdr.h
rpc.sprayd: /usr/include/rpc/auth.h /usr/include/rpc/clnt.h
rpc.sprayd: /usr/include/rpc/rpc_msg.h /usr/include/rpc/auth_unix.h
rpc.sprayd: /usr/include/rpc/svc.h /usr/include/rpc/svc_auth.h
rpc.sprayd: /usr/include/sys/time.h /usr/include/time.h
rpc.sprayd: /usr/include/rpcsvc/spray.h

rpc.yppasswdd: rpc.yppasswdd.c /usr/include/stdio.h /usr/include/signal.h
rpc.yppasswdd: /usr/include/sys/file.h /usr/include/sys/fcntl.h
rpc.yppasswdd: /usr/include/rpc/rpc.h /usr/include/rpc/types.h
rpc.yppasswdd: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
rpc.yppasswdd: /usr/include/netinet/in.h /usr/include/rpc/xdr.h
rpc.yppasswdd: /usr/include/rpc/auth.h /usr/include/rpc/clnt.h
rpc.yppasswdd: /usr/include/rpc/rpc_msg.h /usr/include/rpc/auth_unix.h
rpc.yppasswdd: /usr/include/rpc/svc.h /usr/include/rpc/svc_auth.h
rpc.yppasswdd: /usr/include/pwd.h /usr/include/rpcsvc/yppasswd.h
rpc.yppasswdd: /usr/include/errno.h /usr/include/sys/errno.h
rpc.yppasswdd: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
rpc.yppasswdd: /usr/include/sys/ttydev.h /usr/include/sgtty.h
rpc.yppasswdd: /usr/include/sys/socket.h /usr/include/sys/wait.h

rwall: rwall.c /usr/include/stdio.h /usr/include/utmp.h /usr/include/rpc/rpc.h
rwall: /usr/include/rpc/types.h /usr/include/sys/types.h
rwall: /usr/include/sys/sysmacros.h /usr/include/netinet/in.h
rwall: /usr/include/rpc/xdr.h /usr/include/rpc/auth.h /usr/include/rpc/clnt.h
rwall: /usr/include/rpc/rpc_msg.h /usr/include/rpc/auth_unix.h
rwall: /usr/include/rpc/svc.h /usr/include/rpc/svc_auth.h
rwall: /usr/include/rpc/pmap_clnt.h /usr/include/sys/socket.h
rwall: /usr/include/sys/time.h /usr/include/time.h /usr/include/netdb.h
rwall: /usr/include/rpcsvc/rwall.h

showmount: showmount.c /usr/include/stdio.h /usr/include/rpc/rpc.h
showmount: /usr/include/rpc/types.h /usr/include/sys/types.h
showmount: /usr/include/sys/sysmacros.h /usr/include/netinet/in.h
showmount: /usr/include/rpc/xdr.h /usr/include/rpc/auth.h
showmount: /usr/include/rpc/clnt.h /usr/include/rpc/rpc_msg.h
showmount: /usr/include/rpc/auth_unix.h /usr/include/rpc/svc.h
showmount: /usr/include/rpc/svc_auth.h /usr/include/sys/socket.h
showmount: /usr/include/netdb.h /usr/include/sys/time.h /usr/include/time.h
showmount: /usr/include/sys/errno.h /usr/include/nfs/nfs.h
showmount: /usr/include/rpcsvc/mount.h

spray: spray.c /usr/include/stdio.h /usr/include/ctype.h /usr/include/rpc/rpc.h
spray: /usr/include/rpc/types.h /usr/include/sys/types.h
spray: /usr/include/sys/sysmacros.h /usr/include/netinet/in.h
spray: /usr/include/rpc/xdr.h /usr/include/rpc/auth.h /usr/include/rpc/clnt.h
spray: /usr/include/rpc/rpc_msg.h /usr/include/rpc/auth_unix.h
spray: /usr/include/rpc/svc.h /usr/include/rpc/svc_auth.h
spray: /usr/include/sys/socket.h /usr/include/sys/time.h /usr/include/time.h
spray: /usr/include/netdb.h /usr/include/rpcsvc/spray.h
spray: /usr/include/netinet/in_systm.h /usr/include/netinet/ip.h
spray: /usr/include/netinet/ip_icmp.h /usr/include/signal.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
