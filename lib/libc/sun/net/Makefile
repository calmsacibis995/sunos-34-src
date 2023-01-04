#
# @(#)Makefile 1.1 86/09/24 SMI
#
OBJS=	htonl.o htons.o ntohl.o ntohs.o
CFLAGS=	
LD=	/bin/ld

.c.o:
	${CC} -p -c $(CFLAGS) -DS5EMUL -I/usr/5include $*.c
	${LD} -x -r $*.o
	mv a.out profiled-sys5/$*.o
	${CC} -c $(CFLAGS) -DS5EMUL -I/usr/5include $*.c
	${LD} -x -r $*.o
	mv a.out sys5/$*.o
	${CC} -p -c $(CFLAGS) $*.c
	${LD} -x -r $*.o
	mv a.out profiled/$*.o
	${CC} -c $(CFLAGS) $*.c
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
