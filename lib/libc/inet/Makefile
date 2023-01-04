#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 4.2 83/06/27
#

OBJS=	inet_addr.o inet_network.o \
	inet_netof.o inet_ntoa.o inet_lnaof.o \
	inet_makeaddr.o
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

inetlib inetlib_p inetlibs5 inetlibs5_p: ${OBJS}
	@echo "building profiled Sys5 inetlib"
	@cd profiled-sys5; ar cru ../inetlibs5_p ${OBJS}
	@echo "building normal Sys5 inetlib"
	@cd sys5; ar cru ../inetlibs5 ${OBJS}
	@echo "building profiled inetlib"
	@cd profiled; ar cru ../inetlib_p ${OBJS}
	@echo "building normal inetlib"
	@ar cru inetlib ${OBJS}

clean:
	rm -f inetlib inetlib_p inetlibs5 inetlibs5_p
	rm -f ${OBJS} profiled/*.o sys5/*.o profiled-sys5/*.o errs a.out core
