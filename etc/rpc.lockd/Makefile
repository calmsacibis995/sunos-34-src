#
# @(#)Makefile 1.1 86/09/24 Copyright Sun Micro 1986
#
# Makefile for rpc.lockd - the network lock manager

LIB = -lrpcsvc
CFLAGS = -O
CC = cc

ALL = rpc.lockd
HDRS=   prot_lock.h prot_time.h sm_res.h priv_prot.h

SRCS=   prot_main.c prot_lock.c prot_pklm.c prot_pnlm.c prot_msg.c\
	prot_proc.c prot_libr.c prot_alloc.c prot_priv.c\
	tcp.c udp.c xdr_nlm.c xdr_klm.c xdr_sm.c sm_monitor.c\
	svc_udp.c hash.c

OBJS=	prot_main.o prot_lock.o prot_pklm.o prot_pnlm.o prot_msg.o\
	prot_proc.o prot_libr.o prot_alloc.o prot_priv.o\
	tcp.o udp.o xdr_nlm.o xdr_klm.o xdr_sm.o sm_monitor.o \
	svc_udp.o hash.o




all: $(ALL)


rpc.lockd: $(OBJS)
	$(CC) $(CFLAGS) -o rpc.lockd $(OBJS) $(LIB)

.c.o:
	$(CC) $(CFLAGS) -c $*.c

clean:
	rm -f *.o $(ALL) $(OBJS) a.out

install: all
	for i in ${ALL}; do \
		(install -s $$i ${DESTDIR}/etc/$$i); done
	

depend:
	cat </dev/null >x.c
	for i in ${SRCS} ${TESTSRCS}; do \
		(/bin/grep '^#[ 	]*include' x.c $$i | sed \
			-e '/\.\.\/h/d' \
			-e '/\.\.\/ufs/d' \
			-e 's/\.c:/.o:/' \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' >>makedep); done
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

prot_main.o: /usr/include/stdio.h
prot_main.o: /usr/include/signal.h
prot_main.o: /usr/include/sys/file.h
prot_main.o: /usr/include/sys/ioctl.h
prot_main.o: prot_lock.h
prot_main.o: priv_prot.h
prot_main.o: prot_time.h
prot_lock.o: /usr/include/stdio.h
prot_lock.o: /usr/include/sys/file.h
prot_lock.o: prot_lock.h
prot_lock.o: priv_prot.h
prot_lock.o: /usr/include/rpcsvc/sm_inter.h
prot_lock.o: sm_res.h
prot_pklm.o: /usr/include/stdio.h
prot_pklm.o: prot_lock.h
prot_pnlm.o: /usr/include/stdio.h
prot_pnlm.o: prot_lock.h
prot_msg.o: prot_lock.h
prot_msg.o: prot_time.h
prot_proc.o: /usr/include/stdio.h
prot_proc.o: /usr/include/sys/file.h
prot_proc.o: prot_lock.h
prot_libr.o: /usr/include/stdio.h
prot_libr.o: /usr/include/sys/file.h
prot_libr.o: /usr/include/signal.h
prot_libr.o: prot_lock.h
prot_libr.o: prot_time.h
prot_alloc.o: prot_lock.h
prot_priv.o: /usr/include/stdio.h
prot_priv.o: prot_lock.h
prot_priv.o: priv_prot.h
prot_priv.o: /usr/include/rpcsvc/sm_inter.h
tcp.o: /usr/include/stdio.h
tcp.o: /usr/include/netdb.h
tcp.o: /usr/include/rpc/rpc.h
tcp.o: /usr/include/sys/socket.h
tcp.o: /usr/include/sys/time.h
udp.o: /usr/include/stdio.h
udp.o: /usr/include/rpc/rpc.h
udp.o: /usr/include/sys/socket.h
udp.o: /usr/include/sys/time.h
udp.o: /usr/include/netdb.h
xdr_nlm.o: prot_lock.h
xdr_klm.o: prot_lock.h
xdr_sm.o: /usr/include/rpc/rpc.h
xdr_sm.o: /usr/include/rpcsvc/sm_inter.h
sm_monitor.o: prot_lock.h
sm_monitor.o: priv_prot.h
sm_monitor.o: /usr/include/stdio.h
sm_monitor.o: /usr/include/netdb.h
sm_monitor.o: /usr/include/rpc/rpc.h
sm_monitor.o: /usr/include/rpcsvc/sm_inter.h
sm_monitor.o: sm_res.h
svc_udp.o: /usr/include/stdio.h
svc_udp.o: /usr/include/rpc/rpc.h
svc_udp.o: /usr/include/sys/socket.h
svc_udp.o: /usr/include/netinet/in.h
svc_udp.o: /usr/include/errno.h
hash.o: /usr/include/rpc/rpc.h
hash.o: /usr/include/stdio.h
hash.o: /usr/include/sys/socket.h
hash.o: /usr/include/sys/time.h
hash.o: /usr/include/netdb.h
hash.o: prot_lock.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
