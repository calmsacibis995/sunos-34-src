#
# @(#)Makefile 1.3 87/02/17 SMI
#
# printf & co are here because they are machine dependent
#
CFLAGS=	-O
CPP=/lib/cpp

SRCS=	doprnt.c _itoa.s scanf.c
OBJS=	doprnt.o _itoa.o scanf.o

.c.o:
	${CC} -p ${CFLAGS} -c -DS5EMUL -I/usr/5include $*.c
	$(LD) -x -r $*.o
	mv a.out profiled-sys5/$*.o
	${CC} ${CFLAGS} -c -DS5EMUL -I/usr/5include $*.c
	$(LD) -x -r $*.o
	mv a.out sys5/$*.o
	${CC} -p ${CFLAGS} -c $*.c
	$(LD) -x -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	$(LD) -x -r $*.o
	mv a.out $*.o

.s.o:
	$(CPP) -DPROF -I.. -DS5EMUL -I/usr/5include $*.s >$*.pp
	$(AS) $*.pp -o $*.o
	$(LD) -x -r $*.o
	mv a.out profiled-sys5/$*.o; rm $*.pp
	$(CPP) -I.. -DS5EMUL -I/usr/5include $*.s >$*.pp
	$(AS) $*.pp -o $*.o
	$(LD) -x -r $*.o
	mv a.out sys5/$*.o; rm $*.pp
	$(CPP) -DPROF -I.. $*.s >$*.pp
	$(AS) $*.pp -o $*.o
	$(LD) -x -r $*.o
	mv a.out profiled/$*.o; rm $*.pp
	$(CPP) -I.. $*.s >$*.pp
	$(AS) $*.pp -o $*.o
	$(LD) -x -r $*.o
	mv a.out $*.o; rm $*.pp

stdiolib stdiolib_p stdiolibs5 stdiolibst_p: ${OBJS}
	@echo "building profiled Sys5 stdiolib"
	@cd profiled-sys5; ar cru ../stdiolibs5_p ${OBJS}
	@echo "building normal Sys5 stdiolib"
	@cd sys5; ar cru ../stdiolibs5 ${OBJS}
	@echo "building profiled stdiolib"
	@cd profiled; ar cru ../stdiolib_p ${OBJS}
	@echo "building normal stdiolib"
	@ar cru stdiolib ${OBJS}

clean:
	rm -f stdiolib stdiolib_p stdiolibs5 stdiolibs5_p
	rm -f ${OBJS} profiled/*.o sys5/*.o profiled-sys5/*.o *.pp
