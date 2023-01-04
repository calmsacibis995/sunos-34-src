#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 4.4 6/10/83
#

OBJS=	ether_addr.o gethostent.o getnetent.o getnetgrent.o getprotoent.o \
	getrpcent.o getservent.o innetgr.o rcmd.o rexec.o ruserpass.o
CFLAGS=	-O

.c.o:
	${CC} -p -c ${CFLAGS} -I/usr/5include $*.c
	${LD} -x -r $*.o
	mv a.out profiled-sys5/$*.o
	${CC} ${CFLAGS} -c -I/usr/5include $*.c
	${LD} -x -r $*.o
	mv a.out sys5/$*.o
	${CC} -p -c ${CFLAGS} $*.c
	${LD} -x -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out $*.o

netlib netlib_p netlibs5 netlibs5_p: ${OBJS}
	@echo "building profiled Sys5 netlib"
	@cd profiled-sys5; ar cru ../netlibs5_p ${OBJS}
	@echo "building normal Sys5 netlib"
	@cd sys5; ar cru ../netlibs5 ${OBJS}
	@echo "building profiled netlib"
	@cd profiled; ar cru ../netlib_p ${OBJS}
	@echo "building normal netlib"
	@ar cru netlib ${OBJS}

clean:
	rm -f netlib netlib_p netlibs5 netlibs5_p
	rm -f ${OBJS} profiled/*.o sys5/*.o profiled-sys5/*.o errs a.out core
