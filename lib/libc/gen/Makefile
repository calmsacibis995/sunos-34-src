#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 4.23 83/07/01
#

CRYPT=-DCRYPT
CFLAGS=	-O
MAKE=	make CC="${CC}" AS="${AS}" CPP="${CPP}" ${MFLAGS}
GENLIBS= genlib genlibs5 genlib_p genlibs5_p

OBJS=	_perror.o _psignal.o \
	a64l.o abort.o abs.o alarm.o atof.o atoi.o atol.o \
	bcmp.o bcopy.o bsearch.o bzero.o \
	calloc.o clock.o closedir.o crypt.o ctype_.o \
	drand48.o ecvt.o errlst.o execvp.o exit.o \
	fakcu.o frexp.o fstab.o ftok.o ftw.o \
	getcwd.o getenv.o getgrent.o getopt.o getwd.o hsearch.o \
	index.o initgroups.o l64a.o ldexp.o lfind.o lsearch.o \
	malloc.o mallopt.o memalign.o memccpy.o memchr.o memcmp.o \
	memcpy.o memset.o mktemp.o mntent.o modf.o mkstemp.o \
	ndbm.o on_exit.o opendir.o \
	perror.o popen.o psignal.o putenv.o \
	qsort.o random.o readdir.o regex.o rindex.o \
	scandir.o seekdir.o setegid.o seteuid.o setgid.o setrgid.o \
	setruid.o siglist.o siginterrupt.o ssignal.o strcat.o strchr.o \
	strcmp.o strcpy.o strcspn.o strlen.o strncat.o strncmp.o strncpy.o \
	strpbrk.o strrchr.o strspn.o strtod.o strtok.o strtol.o \
	swab.o syslog.o system.o \
	telldir.o tfind.o time.o tsearch.o ttyname.o \
	ualarm.o usleep.o \
	valloc.o

.c.o:
	${CC} -p ${CFLAGS} ${CRYPT} -DS5EMUL -c -I/usr/5include $*.c
	${LD} -x -r $*.o
	mv a.out profiled-sys5/$*.o
	${CC} ${CFLAGS} ${CRYPT} -DS5EMUL -c -I/usr/5include $*.c
	${LD} -x -r $*.o
	mv a.out sys5/$*.o
	${CC} -p ${CFLAGS} ${CRYPT} -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} ${CRYPT} -c $*.c
	${LD} -x -r $*.o
	mv a.out $*.o

${GENLIBS}: ${OBJS} debugfiles
	@echo "building profiled Sys5 genlib"
	@cd profiled-sys5; ar cru ../genlibs5_p ${OBJS}
	@echo "building normal Sys5 genlib"
	@cd sys5; ar cru ../genlibs5 ${OBJS}
	@echo "building profiled genlib"
	@cd profiled; ar cru ../genlib_p ${OBJS}
	@echo "building normal genlib"
	@ar cru genlib ${OBJS}

debugfiles:
	@echo "building debug modules"
	cd debug; ${MAKE}

international: icrypt.o ${GENLIBS}
	mv icrypt.o crypt.o
	${MAKE} "CRYPT="
	ar rv ${DESTDIR}/lib/libc.a crypt.o
	mv pro_crypt.o profiled/crypt.o
	cd profiled; ar rv ${DESTDIR}/usr/lib/libc_p.a crypt.o
	mv sysv_crypt.o sys5/crypt.o
	cd sys5; ar rv ${DESTDIR}/usr/5lib/libc.a crypt.o
	mv pro_sysv_crypt.o profiled-sys5/crypt.o
	cd profiled-sys5; ar rv ${DESTDIR}/usr/5lib/libc_p.a crypt.o
	-rm ${DESTDIR}/usr/man/man3/encrypt.3
	-rm ${DESTDIR}/usr/man/man3/crypt.3
	-rm ${DESTDIR}/usr/man/man3/cbc_crypt.3
	-rm ${DESTDIR}/usr/man/man3/ecb_crypt.3
	-rm crypt.o sys5/crypt.o profiled/crypt.o profiled-sys5/crypt.o

icrypt.o: crypt.c
	-rm crypt.o
	${MAKE} crypt.o "CRYPT="
	mv crypt.o icrypt.o
	${CC} -p ${CFLAGS} -DS5EMUL -c -I/usr/5include crypt.c
	${LD} -x -r crypt.o
	mv a.out pro_sysv_crypt.o
	${CC} ${CFLAGS} -DS5EMUL -c -I/usr/5include crypt.c
	${LD} -x -r crypt.o
	mv a.out sysv_crypt.o
	${CC} -p ${CFLAGS} -c crypt.c
	${LD} -x -r crypt.o
	mv a.out pro_crypt.o

clean:
	rm -f ${GENLIBS}
	rm -f ${OBJS} profiled/*.o sys5/*.o profiled-sys5/*.o

errlst.o: errlst.c
	${CC} -S errlst.c
	-if vax; then ed - errlst.s <_errfix.vax; fi
	-if sun; then sh _errfix.sun errlst.s; fi
	${AS} -o errlst.o errlst.s
	rm -f errlst.s
	cp errlst.o profiled-sys5/errlst.o
	cp errlst.o sys5/errlst.o
	cp errlst.o profiled/errlst.o

siglist.o: siglist.c
	${CC} -c -R siglist.c
	cp siglist.o profiled-sys5/siglist.o
	cp siglist.o sys5/siglist.o
	cp siglist.o profiled/siglist.o

malloc.o: malloc.c mallint.h
memallign.o: memalign.c malint.h
mallopt.o: mallopt.c mallint.h
