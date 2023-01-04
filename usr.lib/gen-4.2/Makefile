#
# @(#)Makefile 1.1 86/09/24 SMI
#

CFLAGS=	-O

OBJS=	ctime.o \
	gcvt.o getlogin.o getpass.o getpwent.o \
	isatty.o \
	nlist.o \
	setuid.o signal.o sleep.o \
	timezone.o ttyslot.o

.c.o:
	${CC} -p ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out $*.o

gen-4.2lib gen-4.2lib_p: ${OBJS}
	@echo "building profiled gen-4.2lib"
	@cd profiled; ar cru ../gen-4.2lib_p ${OBJS}
	@echo "buiding normal gen-4.2lib"
	@ar cru gen-4.2lib ${OBJS}

clean:
	rm -f gen-4.2lib gen-4.2lib_p ${OBJS} profiled/*.o
