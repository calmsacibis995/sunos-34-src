#
# @(#)Makefile 1.1 86/09/25 SMI
#
# to add a new script add its name to SCRIPTS and run make depend
#
# to check whether SCRIPTS is up to date run 'make check'
#
SCRIPTS = buf callout callout.nxt cblock cblock.nxt \
	clist dino dir dir.nxt file filsys ifnet inode \
	inpcb iovec ipreass ipreass.nxt mact mact.nxt \
	mbstat mbuf mbuf.nxt mbufs mbufs.nxt \
	mount pcb proc protosw rawcb rtentry rusage \
	setproc setproc.done setproc.nop setproc.nxt socket stat \
	tcpcb tcpip tcpreass tcpreass.nxt text traceall \
	traceall.nxt tty u ucred uio vnode vtimes

DESTDIR =
FLAGS	=-w

all:	adbgen1 adbsub.o adbgen3 adbgen4 ${SCRIPTS}

${SCRIPTS}:
	sh adbgen.sh ${FLAGS} $@.adb

adbgen1:	adbgen1.c
	cc -O -o adbgen1 adbgen1.c

adbsub.o:	adbsub.c
	cc -O -c adbsub.c

adbgen3:	adbgen3.c
	cc -O -o adbgen3 adbgen3.c

adbgen4:	adbgen4.c
	cc -O -o adbgen4 adbgen4.c

install: adbgen1 adbgen3 adbgen4 ${SCRIPTS} adbsub.o
	-mkdir ${DESTDIR}/usr/lib/adb && chown bin ${DESTDIR}/usr/lib/adb && \
		chmod 755 ${DESTDIR}/usr/lib/adb
	-for i in adbgen1 adbgen3 adbgen4; do \
		install -s $$i ${DESTDIR}/usr/lib/adb/$$i; done
	install -m 755 -c adbgen.sh ${DESTDIR}/usr/lib/adb/adbgen
	cp ${SCRIPTS} adbsub.o ${DESTDIR}/usr/lib/adb

clean:
	rm -f ${SCRIPTS} adbgen adbgen[134] adbgen2* adbsub.o \
		script.files actual.files

check:
	echo ${SCRIPTS} | tr ' ' '\012' | sed 's/$$/&.adb/' > script.files
	ls *.adb > actual.files
	diff script.files actual.files
	rm script.files actual.files

maclist:
	@dir=`pwd`; for i in ${SCRIPTS}; do\
		echo $$dir/$$i; done

depend:
	cp /dev/null x.c
	for i in ${SCRIPTS}; do \
		(egrep '^#[         ]*include' x.c $$i.adb | sed \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.adb//' >> makedep; \
		echo $$i: $$i.adb >> makedep); done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep x.c
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it

buf: ../h/types.h
buf: ../h/buf.h
buf: buf.adb
callout: ../h/types.h
callout: ../h/callout.h
callout: callout.adb
callout.nxt: ../h/types.h
callout.nxt: ../h/callout.h
callout.nxt: callout.nxt.adb
cblock: ../h/param.h
cblock: ../h/clist.h
cblock: cblock.adb
cblock.nxt: ../h/param.h
cblock.nxt: ../h/clist.h
cblock.nxt: cblock.nxt.adb
clist: ../h/types.h
clist: ../h/tty.h
clist: clist.adb
dino: ../h/types.h
dino: ../h/time.h
dino: ../h/vnode.h
dino: ../ufs/inode.h
dino: dino.adb
dir: dir.adb
dir.nxt: ../h/types.h
dir.nxt: ../h/dir.h
dir.nxt: dir.nxt.adb
file: ../h/types.h
file: ../h/file.h
file: file.adb
filsys: ../h/param.h
filsys: ../ufs/fs.h
filsys: filsys.adb
ifnet: ../h/types.h
ifnet: ../h/socket.h
ifnet: ../net/if.h
ifnet: ifnet.adb
inode: ../h/types.h
inode: ../h/time.h
inode: ../h/vnode.h
inode: ../ufs/inode.h
inode: inode.adb
inpcb: ../h/types.h
inpcb: ../h/socket.h
inpcb: ../net/route.h
inpcb: ../netinet/in.h
inpcb: ../netinet/in_pcb.h
inpcb: inpcb.adb
iovec: ../h/types.h
iovec: ../h/uio.h
iovec: iovec.adb
ipreass: ../h/types.h
ipreass: ../netinet/in.h
ipreass: ../netinet/ip_var.h
ipreass: ipreass.adb
ipreass.nxt: ../h/types.h
ipreass.nxt: ../netinet/in.h
ipreass.nxt: ../netinet/ip_var.h
ipreass.nxt: ipreass.nxt.adb
mact: ../h/types.h
mact: ../h/mbuf.h
mact: mact.adb
mact.nxt: ../h/types.h
mact.nxt: ../h/mbuf.h
mact.nxt: mact.nxt.adb
mbstat: ../h/types.h
mbstat: ../h/mbuf.h
mbstat: mbstat.adb
mbuf: ../h/types.h
mbuf: ../h/mbuf.h
mbuf: mbuf.adb
mbuf.nxt: ../h/types.h
mbuf.nxt: ../h/mbuf.h
mbuf.nxt: mbuf.nxt.adb
mbufs: ../h/types.h
mbufs: ../h/mbuf.h
mbufs: mbufs.adb
mbufs.nxt: ../h/types.h
mbufs.nxt: ../h/mbuf.h
mbufs.nxt: mbufs.nxt.adb
mount: ../h/types.h
mount: ../h/vfs.h
mount: ../ufs/mount.h
mount: mount.adb
pcb: ../h/param.h
pcb: ../h/dir.h
pcb: ../h/user.h
pcb: pcb.adb
proc: ../h/types.h
proc: ../h/time.h
proc: ../h/proc.h
proc: proc.adb
protosw: ../h/types.h
protosw: ../h/protosw.h
protosw: protosw.adb
rawcb: ../h/types.h
rawcb: ../h/socket.h
rawcb: ../net/route.h
rawcb: ../net/raw_cb.h
rawcb: rawcb.adb
rtentry: ../h/types.h
rtentry: ../h/socket.h
rtentry: ../net/route.h
rtentry: rtentry.adb
rusage: ../h/types.h
rusage: ../h/time.h
rusage: ../h/resource.h
rusage: rusage.adb
setproc: setproc.adb
setproc.done: setproc.done.adb
setproc.nop: setproc.nop.adb
setproc.nxt: ../h/types.h
setproc.nxt: ../h/time.h
setproc.nxt: ../h/proc.h
setproc.nxt: setproc.nxt.adb
socket: ../h/types.h
socket: ../h/socket.h
socket: ../h/socketvar.h
socket: socket.adb
stat: ../h/types.h
stat: ../h/stat.h
stat: stat.adb
tcpcb: ../h/types.h
tcpcb: ../netinet/tcp.h
tcpcb: ../netinet/tcp_timer.h
tcpcb: ../netinet/tcp_var.h
tcpcb: tcpcb.adb
tcpip: ../h/types.h
tcpip: ../netinet/tcp.h
tcpip: ../netinet/in.h
tcpip: ../netinet/in_systm.h
tcpip: ../netinet/ip.h
tcpip: ../netinet/ip_var.h
tcpip: ../netinet/tcpip.h
tcpip: tcpip.adb
tcpreass: ../h/types.h
tcpreass: ../netinet/tcp.h
tcpreass: ../netinet/in.h
tcpreass: ../netinet/in_systm.h
tcpreass: ../netinet/ip.h
tcpreass: ../netinet/ip_var.h
tcpreass: ../netinet/tcpip.h
tcpreass: tcpreass.adb
tcpreass.nxt: ../h/types.h
tcpreass.nxt: ../netinet/tcp.h
tcpreass.nxt: ../netinet/in.h
tcpreass.nxt: ../netinet/in_systm.h
tcpreass.nxt: ../netinet/ip.h
tcpreass.nxt: ../netinet/ip_var.h
tcpreass.nxt: ../netinet/tcpip.h
tcpreass.nxt: tcpreass.nxt.adb
text: ../h/types.h
text: ../h/text.h
text: text.adb
traceall: traceall.adb
traceall.nxt: ../h/types.h
traceall.nxt: ../h/time.h
traceall.nxt: ../h/proc.h
traceall.nxt: traceall.nxt.adb
tty: ../h/types.h
tty: ../h/tty.h
tty: tty.adb
u: ../h/param.h
u: ../h/dir.h
u: ../h/user.h
u: u.adb
ucred: ../h/param.h
ucred: ../h/dir.h
ucred: ../h/user.h
ucred: ucred.adb
uio: ../h/types.h
uio: ../h/uio.h
uio: uio.adb
vnode: ../h/types.h
vnode: ../h/time.h
vnode: ../h/vnode.h
vnode: vnode.adb
vtimes: ../h/vtimes.h
vtimes: vtimes.adb
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
