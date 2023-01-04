#
# @(#)Makefile 1.1 86/09/24 SMI
#
CFLAGS= -O

CPP=/lib/cpp
OBJS=	sigvec.o sigtramp.o

.c.o:
	${CPP} -I../crt -E -DPROF -DS5EMUL -I/usr/5include $*.c > $*.pp
	${AS} -o $*.o $*.pp
	rm -f $*.pp
	${LD} -x -r $*.o
	mv a.out profiled-sys5/$*.o
	${CPP} -I../crt -E -DS5EMUL -I/usr/5include $*.c > $*.pp
	${AS} -o $*.o $*.pp
	rm -f $*.pp
	${LD} -x -r $*.o
	mv a.out sys5/$*.o
	${CPP} -I../crt -E -DPROF $*.c > $*.pp
	${AS} -o $*.o $*.pp
	rm -f $*.pp
	${LD} -x -r $*.o
	mv a.out profiled/$*.o
	${CPP} -I../crt -E $*.c > $*.pp
	${AS} -o $*.o $*.pp
	rm -f $*.pp
	${LD} -x -r $*.o
	mv a.out $*.o

syslib syslib_p syslibs5 syslibs5_p: ${OBJS}
	@echo "building profiled Sys5 syslib_p"
	@cd profiled-sys5; ar cru ../syslibs5_p ${OBJS}
	@echo "building normal Sys5 syslib"
	@cd sys5; ar cru ../syslibs5 ${OBJS}
	@echo "building profiled syslib_p"
	@cd profiled; ar cru ../syslib_p ${OBJS}
	@echo "building normal syslib"
	@ar cru syslib ${OBJS}

sigvec.o: sigvec.c
	${CC} -DPROF ${CFLAGS} -c -DS5EMUL -I/usr/5include $*.c
	${LD} -x -r $*.o
	mv a.out profiled-sys5/sigvec.o
	${CC} -c ${CFLAGS} -DS5EMUL -I/usr/5include $*.c
	${LD} -x -r $*.o
	mv a.out sys5/$*.o
	${CC} -DPROF ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled/sigvec.o
	${CC} -c ${CFLAGS} $*.c
	${LD} -x -r $*.o
	mv a.out $*.o

clean:
	rm -f syslib syslib_p syslibs5 syslibs5_p
	rm -f ${OBJS} profiled/*.o sys5/*.o profiled-sys5/*.o *.pp
