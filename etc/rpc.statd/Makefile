#
# @(#)Makefile 1.1 86/09/24 Copyright Sun Micro 1986
#
# Makefile for rpc.statd - the network status monitor

LIB = -lrpcsvc
CFLAGS = -O
CC = cc

ALL = rpc.statd
HDRS=  sm_statd.h  

SRCS= sm_svc.c sm_proc.c sm_statd.c ../rpc.lockd/tcp.c 

OBJS= sm_svc.o sm_proc.o sm_statd.o tcp.o

all: $(ALL)

rpc.statd: $(OBJS)
	$(CC) $(CFLAGS) -o rpc.statd $(OBJS) $(LIB)

.c.o:
	$(CC) $(CFLAGS) -c $*.c

.c:
	$(CC) $(CFLAGS) $@.c $(LIB) -o $@

tcp.o: ../rpc.lockd/tcp.c
	$(CC) $(CFLAGS) -c ../rpc.lockd/tcp.c

clean:
	rm -f *.o $(ALL) $(OBJS) a.out

install: all
	for i in $(ALL); do \
		(install -s $$i ${DESTDIR}/etc/$$i); done

depend:
	cat </dev/null >x.c
	for i in ${SRCS} ; do \
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

sm_svc.o: /usr/include/stdio.h
sm_svc.o: /usr/include/signal.h
sm_svc.o: /usr/include/rpc/rpc.h
sm_svc.o: /usr/include/sys/ioctl.h
sm_svc.o: /usr/include/rpcsvc/sm_inter.h
sm_svc.o: sm_statd.h
sm_proc.o: /usr/include/stdio.h
sm_proc.o: /usr/include/rpc/rpc.h
sm_proc.o: /usr/include/rpcsvc/sm_inter.h
sm_statd.o: /usr/include/stdio.h
sm_statd.o: /usr/include/sys/types.h
sm_statd.o: /usr/include/sys/file.h
sm_statd.o: /usr/include/sys/dir.h
sm_statd.o: /usr/include/rpc/rpc.h
sm_statd.o: /usr/include/rpcsvc/sm_inter.h
sm_statd.o: /usr/include/errno.h
sm_statd.o: sm_statd.h
../rpc.lockd/tcp.o: /usr/include/stdio.h
../rpc.lockd/tcp.o: /usr/include/netdb.h
../rpc.lockd/tcp.o: /usr/include/rpc/rpc.h
../rpc.lockd/tcp.o: /usr/include/sys/socket.h
../rpc.lockd/tcp.o: /usr/include/sys/time.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
