#
# @(#)Makefile 1.1 86/09/24 SMI
#
CFLAGS=	-O
STD=	gtty.o lockf.o pause.o stty.o tell.o ulimit.o
OBJS=	${STD}

.c.o:
	${CC} -p ${CFLAGS} -c -I/usr/5include $*.c
	${LD} -x -r $*.o
	mv a.out profiled-sys5/$*.o
	${CC} ${CFLAGS} -c -I/usr/5include $*.c
	${LD} -x -r $*.o
	mv a.out sys5/$*.o
	${CC} -p ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out $*.o

compatlib compatlib_p compatlibs5 compatlibs5_p: ${OBJS}
	@echo "building profiled Sys5 compatlib"
	@cd profiled-sys5; ar cru ../compatlibs5_p ${OBJS}
	@echo "buiding normal Sys5 compatlib"
	@cd sys5; ar cru ../compatlibs5 ${OBJS}
	@echo "building profiled compatlib"
	@cd profiled; ar cru ../compatlib_p ${OBJS}
	@echo "buiding normal compatlib"
	@ar cru compatlib ${OBJS}

clean:
	rm -f compatlib compatlib_p compatlibs5 compatlibs5_p
	rm -f ${OBJS} profiled/*.o sys5/*.o profiled-sys5/*.o
