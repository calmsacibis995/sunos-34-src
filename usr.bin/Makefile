#
# @(#)Makefile 1.2 86/10/04 SMI; from UCB 4.38 85/08/26
#
DESTDIR=
CFLAGS=	-O
LDFLAGS= -n

# Programs that live in subdirectories, and have makefiles of their own.
# "lint" should be made before things it depends on, like "cxref".
#
SUBDIR=lint at calendar cflow ctrace cxref dc diff3 eqn f77 file find \
	graph lex m4 neqn nroff plot prof ratfor refer spell tbl tip \
	troff uucp xsend yacc

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=	diffmk dirname install lorder nohup

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	basename cal cb checkeq comm csplit cut \
	deroff getopt id iostat ipcrm ipcs join logname look \
	mesg nl pack paste ptx \
	ranlib rev sdiff sleep spline split sum touch tr tsort tty \
	uniq units unpack xargs ypcat ypwhich ypmatch

# Programs that must run setuid to root
#
SETUID=	

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	bc col egrep sort yppasswd \
	fgrep

# C programs that must run set-group-id kmem.
#
KMEM=	iostat ipcs

all:	${SUBDIR} ${STD} ${NSTD}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

${STD}:
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $@.c

install: ${STD} ${NSTD}
	-mkdir ${DESTDIR}/usr/bin && chown bin ${DESTDIR}/usr/bin && \
		chmod 755 ${DESTDIR}/usr/bin 
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	-for i in ${SCRIPT}; do (install -c $$i.sh ${DESTDIR}/usr/bin/$$i); done
	-for i in ${STD} ${NSTD}; do \
		(install -s $$i ${DESTDIR}/usr/bin/$$i); \
	done
#	-for i in ${SETUID}; do (chown root ${DESTDIR}/usr/bin/$$i; \
#		chmod 4755 ${DESTDIR}/usr/bin/$$i); done
	-for i in ${KMEM}; do (chgrp kmem ${DESTDIR}/usr/bin/$$i; \
		chmod 2755 ${DESTDIR}/usr/bin/$$i); done
	install -c units.lib ${DESTDIR}/usr/lib/units
	install -c ptx.eign ${DESTDIR}/usr/lib/eign
	rm -f ${DESTDIR}/usr/bin/pcat
	ln ${DESTDIR}/usr/bin/unpack ${DESTDIR}/usr/bin/pcat

international:
	rm ${DESTDIR}/usr/bin/crypt
	rm ${DESTDIR}/usr/man/man1/crypt.1

clean:
	rm -f a.out y.tab.* core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
	rm -f ${STD} ${NSTD}

# beware of the non-standard stuff for col, sort, bc, and egrep
depend:
	rm -f makedep
	for i in ${STD} yppasswd fgrep; do \
	    ${CC} -M ${INCPATH} $$i.c | sed 's/\.o//' | \
	    awk ' { if ($$1 != prev) \
		{ if (rec != "") print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec } ' >> makedep; done
	for i in col; do \
	    ${CC} -M ${INCPATH} -DBSD $$i.c | sed 's/\.o//' | \
	    awk ' { if ($$1 != prev) \
		{ if (rec != "") print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec } ' >> makedep; done
	for i in sort; do \
	    ${CC} -M ${INCPATH} -DPRE_S5R2 $$i.c | sed 's/\.o//' | \
	    awk ' { if ($$1 != prev) \
		{ if (rec != "") print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec } ' >> makedep; done
	for i in bc egrep; do \
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

FRC:

# Files listed in ${NSTD} have explicit make lines given below.

bc egrep:
	${YACC} ${YFLAGS} $@.y
	${CC} -o $@ ${CFLAGS} -w y.tab.c
	rm -f y.tab.c

col:	col.c
	${CC} -o col ${CFLAGS} ${LDFLAGS} -DBSD col.c

sort:	sort.c
	${CC} -o sort ${CFLAGS} -DPRE_S5R2 sort.c

yppasswd: yppasswd.c
	${CC} ${CFLAGS} -o yppasswd yppasswd.c -lrpcsvc

# Following files have explicit make lines because they
# are big and should be linked to be demand paged.

fgrep: fgrep.c
	${CC} ${CFLAGS} -o fgrep fgrep.c

# DO NOT DELETE THIS LINE -- make depend uses it

basename: basename.c /usr/include/stdio.h
cal: cal.c /usr/include/time.h
cb: cb.c /usr/include/stdio.h
checkeq: checkeq.c /usr/include/stdio.h
comm: comm.c /usr/include/stdio.h
csplit: csplit.c /usr/include/stdio.h /usr/include/errno.h
csplit: /usr/include/sys/errno.h /usr/include/signal.h /usr/include/regexp.h
cut: cut.c /usr/include/stdio.h
deroff: deroff.c /usr/include/stdio.h
getopt: getopt.c /usr/include/stdio.h
id: id.c /usr/include/stdio.h /usr/include/pwd.h /usr/include/grp.h
iostat: iostat.c /usr/include/stdio.h /usr/include/nlist.h
iostat: /usr/include/sys/param.h /usr/include/machine/param.h
iostat: /usr/include/signal.h /usr/include/sys/types.h
iostat: /usr/include/sys/sysmacros.h /usr/include/sys/buf.h
iostat: /usr/include/sys/dk.h /usr/include/sundev/mbvar.h
ipcrm: ipcrm.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
ipcrm: /usr/include/sys/ipc.h /usr/include/sys/msg.h /usr/include/sys/sem.h
ipcrm: /usr/include/sys/shm.h /usr/include/errno.h /usr/include/sys/errno.h
ipcrm: /usr/include/stdio.h
ipcs: ipcs.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
ipcs: /usr/include/sys/ipc.h /usr/include/sys/msg.h /usr/include/sys/sem.h
ipcs: /usr/include/sys/shm.h /usr/include/nlist.h /usr/include/fcntl.h
ipcs: /usr/include/time.h /usr/include/grp.h /usr/include/pwd.h
ipcs: /usr/include/stdio.h
join: join.c /usr/include/stdio.h
logname: logname.c /usr/include/stdio.h
look: look.c /usr/include/stdio.h /usr/include/ctype.h
mesg: mesg.c /usr/include/stdio.h /usr/include/sys/types.h
mesg: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
nl: nl.c /usr/include/stdio.h /usr/include/regexp.h
pack: pack.c /usr/include/stdio.h /usr/include/sys/types.h
pack: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
paste: paste.c /usr/include/stdio.h
ptx: ptx.c /usr/include/stdio.h /usr/include/ctype.h /usr/include/signal.h
ranlib: ranlib.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
ranlib: /usr/include/sys/stat.h /usr/include/ar.h /usr/include/ranlib.h
ranlib: /usr/include/a.out.h /usr/include/sys/exec.h /usr/include/stdio.h
rev: rev.c /usr/include/stdio.h
sdiff: sdiff.c /usr/include/stdio.h /usr/include/ctype.h /usr/include/signal.h
sdiff: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
sdiff: /usr/include/sys/stat.h
sleep: sleep.c /usr/include/stdio.h
spline: spline.c /usr/include/stdio.h /usr/include/math.h
split: split.c /usr/include/stdio.h
sum: sum.c /usr/include/stdio.h
touch: touch.c /usr/include/stdio.h /usr/include/sys/types.h
touch: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
tr: tr.c /usr/include/stdio.h
tsort: tsort.c /usr/include/stdio.h
tty: tty.c
uniq: uniq.c /usr/include/stdio.h /usr/include/ctype.h
units: units.c /usr/include/stdio.h /usr/include/signal.h
unpack: unpack.c /usr/include/stdio.h /usr/include/setjmp.h
unpack: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
unpack: /usr/include/sys/stat.h
xargs: xargs.c
ypcat: ypcat.c /usr/include/stdio.h /usr/include/rpc/rpc.h
ypcat: /usr/include/rpc/types.h /usr/include/sys/types.h
ypcat: /usr/include/sys/sysmacros.h /usr/include/netinet/in.h
ypcat: /usr/include/rpc/xdr.h /usr/include/rpc/auth.h /usr/include/rpc/clnt.h
ypcat: /usr/include/rpc/rpc_msg.h /usr/include/rpc/auth_unix.h
ypcat: /usr/include/rpc/svc.h /usr/include/rpc/svc_auth.h
ypcat: /usr/include/rpcsvc/ypclnt.h /usr/include/rpcsvc/yp_prot.h
ypcat: /usr/include/rpcsvc/ypv1_prot.h
ypwhich: ypwhich.c /usr/include/stdio.h /usr/include/ctype.h
ypwhich: /usr/include/rpc/rpc.h /usr/include/rpc/types.h
ypwhich: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
ypwhich: /usr/include/netinet/in.h /usr/include/rpc/xdr.h
ypwhich: /usr/include/rpc/auth.h /usr/include/rpc/clnt.h
ypwhich: /usr/include/rpc/rpc_msg.h /usr/include/rpc/auth_unix.h
ypwhich: /usr/include/rpc/svc.h /usr/include/rpc/svc_auth.h
ypwhich: /usr/include/netdb.h /usr/include/sys/time.h /usr/include/time.h
ypwhich: /usr/include/sys/socket.h /usr/include/rpcsvc/yp_prot.h
ypwhich: /usr/include/rpcsvc/ypv1_prot.h /usr/include/rpcsvc/ypclnt.h
ypmatch: ypmatch.c /usr/include/stdio.h /usr/include/rpc/rpc.h
ypmatch: /usr/include/rpc/types.h /usr/include/sys/types.h
ypmatch: /usr/include/sys/sysmacros.h /usr/include/netinet/in.h
ypmatch: /usr/include/rpc/xdr.h /usr/include/rpc/auth.h /usr/include/rpc/clnt.h
ypmatch: /usr/include/rpc/rpc_msg.h /usr/include/rpc/auth_unix.h
ypmatch: /usr/include/rpc/svc.h /usr/include/rpc/svc_auth.h
ypmatch: /usr/include/netdb.h /usr/include/sys/time.h /usr/include/time.h
ypmatch: /usr/include/sys/socket.h /usr/include/rpcsvc/yp_prot.h
ypmatch: /usr/include/rpcsvc/ypclnt.h
yppasswd: yppasswd.c /usr/include/stdio.h /usr/include/signal.h
yppasswd: /usr/include/pwd.h /usr/include/rpc/rpc.h /usr/include/rpc/types.h
yppasswd: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
yppasswd: /usr/include/netinet/in.h /usr/include/rpc/xdr.h
yppasswd: /usr/include/rpc/auth.h /usr/include/rpc/clnt.h
yppasswd: /usr/include/rpc/rpc_msg.h /usr/include/rpc/auth_unix.h
yppasswd: /usr/include/rpc/svc.h /usr/include/rpc/svc_auth.h
yppasswd: /usr/include/rpcsvc/yppasswd.h /usr/include/netdb.h
yppasswd: /usr/include/sys/socket.h /usr/include/sys/file.h
yppasswd: /usr/include/sys/fcntl.h /usr/include/errno.h
yppasswd: /usr/include/sys/errno.h
fgrep: fgrep.c /usr/include/stdio.h /usr/include/ctype.h
fgrep: /usr/include/sys/param.h /usr/include/machine/param.h
fgrep: /usr/include/signal.h /usr/include/sys/types.h
fgrep: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
col: col.c /usr/include/stdio.h
sort: sort.c /usr/include/stdio.h /usr/include/ctype.h /usr/include/signal.h
sort: /usr/include/sys/param.h /usr/include/machine/param.h
sort: /usr/include/signal.h /usr/include/sys/types.h
sort: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
sort: /usr/include/values.h
bc: bc.y /usr/include/stdio.h
egrep: egrep.y /usr/include/stdio.h /usr/include/ctype.h
egrep: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
egrep: /usr/include/sys/stat.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
