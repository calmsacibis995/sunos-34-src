#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 4.17 83/07/02
#

CPP=	/lib/cpp

OBJS=	Ovfork.o Ovadvise.o \
	accept.o access.o acct.o adjtime.o async_daemon.o \
	bind.o brk.o \
	cerror.o chdir.o chmod.o chown.o chroot.o close.o connect.o \
	creat.o \
	dup.o dup2.o \
	execl.o execle.o execv.o execve.o _exit.o exportfs.o \
	fchmod.o fchown.o fcntl.o flock.o fork.o fstat.o fstatfs.o \
	fsync.o ftruncate.o \
	getdirentries.o getdomainname.o getdtablesize.o \
	getegid.o geteuid.o getfh.o getgid.o \
	getgroups.o gethostid.o gethostname.o getitimer.o \
	getpagesize.o getpeername.o getpid.o getppid.o getpriority.o \
	getrlimit.o getrusage.o getsockname.o getsockopt.o \
	gettimeofday.o getuid.o \
	killpg.o \
	link.o listen.o lstat.o \
	mkdir.o mknod.o mmap.o msgsys.o munmap.o \
	nfssvc.o \
	pipe.o profil.o ptrace.o \
	quotactl.o \
	readlink.o readv.o reboot.o recv.o recvfrom.o \
	recvmsg.o rename.o rmdir.o \
	sbrk.o select.o \
	semsys.o send.o sendmsg.o sendto.o setdomainname.o \
	setgroups.o sethostname.o setitimer.o setpriority.o \
	setregid.o setreuid.o setrlimit.o setsockopt.o settimeofday.o \
	shmsys.o shutdown.o sigblock.o sigpause.o \
	sigsetmask.o sigstack.o socket.o socketpair.o \
	stat.o statfs.o swapon.o symlink.o sync.o syscall.o \
	truncate.o \
	umask.o unlink.o unmount.o utimes.o \
	vhangup.o \
	wait.o wait3.o writev.o

.c.o:
	${CPP} -E -DPROF -I/usr/5include $*.c > $*.pp
	${AS} -o $*.o $*.pp
	rm -f $*.pp
	${LD} -x -r $*.o
	mv a.out profiled-sys5/$*.o
	${CPP} -E -I/usr/5include $*.c > $*.pp
	${AS} -o $*.o $*.pp
	rm -f $*.pp
	${LD} -x -r $*.o
	mv a.out sys5/$*.o
	${CPP} -E -DPROF $*.c > $*.pp
	${AS} -o $*.o $*.pp
	rm -f $*.pp
	${LD} -x -r $*.o
	mv a.out profiled/$*.o
	${CPP} -E $*.c > $*.pp
	${AS} -o $*.o $*.pp
	rm -f $*.pp
	${LD} -x -r $*.o
	mv a.out $*.o

syslib syslib_p syslibs5 syslibs5_p: ${OBJS}
	@echo "building profiled Sys5 syslib"
	@cd profiled-sys5; ar cru ../syslibs5_p ${OBJS}
	@echo "building normal Sys5 syslib"
	@cd sys5; ar cru ../syslibs5 ${OBJS}
	@echo "building profiled syslib"
	@cd profiled; ar cru ../syslib_p ${OBJS}
	@echo "building normal syslib"
	@ar cru syslib ${OBJS}

execl.o: execl.c
	${CC} -DPROF ${CFLAGS} -c -I/usr/5include $*.c
	${LD} -x -r $*.o
	mv a.out profiled-sys5/execl.o
	${CC} ${CFLAGS} -I/usr/5include -c $*.c
	${LD} -x -r $*.o
	mv a.out sys5/$*.o
	${CC} -DPROF ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled/execl.o
	${CC} ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out $*.o

execle.o: execle.c
	${CC} -DPROF ${CFLAGS} -I/usr/5include -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled-sys5/execle.o
	${CC} ${CFLAGS} -I/usr/5include -c $*.c
	${LD} -x -r $*.o
	mv a.out sys5/$*.o
	${CC} -DPROF ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled/execle.o
	${CC} ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out $*.o

execv.o: execv.c
	${CC} -DPROF ${CFLAGS} -I/usr/5include -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled-sys5/execv.o
	${CC} ${CFLAGS} -I/usr/5include -c $*.c
	${LD} -x -r $*.o
	mv a.out sys5/$*.o
	${CC} -DPROF ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled/execv.o
	${CC} ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out $*.o

msgsys.o: msgsys.c
	${CC} -DPROF ${CFLAGS} -I/usr/5include -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled-sys5/$*.o
	${CC} ${CFLAGS} -I/usr/5include -c $*.c
	${LD} -x -r $*.o
	mv a.out sys5/$*.o
	${CC} -DPROF ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out $*.o

semsys.o: semsys.c
	${CC} -DPROF ${CFLAGS} -I/usr/5include -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled-sys5/$*.o
	${CC} ${CFLAGS} -I/usr/5include -c $*.c
	${LD} -x -r $*.o
	mv a.out sys5/$*.o
	${CC} -DPROF ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out $*.o

shmsys.o: shmsys.c
	${CC} -DPROF ${CFLAGS} -I/usr/5include -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled-sys5/$*.o
	${CC} ${CFLAGS} -I/usr/5include -c $*.c
	${LD} -x -r $*.o
	mv a.out sys5/$*.o
	${CC} -DPROF ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out $*.o

clean:
	rm -f syslib syslib_p syslibs5 syslibs5_p
	rm -f ${OBJS} profiled/*.o sys5/*.o profiled-sys5/*.o *.pp
