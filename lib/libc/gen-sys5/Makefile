#
# @(#)Makefile 1.1 86/09/24 SMI
#

CFLAGS=	-O -DS5EMUL -I/usr/5include

OBJS=	assert.o \
	ctime.o \
	gcvt.o getlogin.o getpass.o getpwent.o \
	isatty.o \
	nlist.o \
	putpwent.o \
	setuid.o signal.o sleep.o \
	tolower.o toupper.o ttyslot.o

.c.o:
	${CC} -p ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out $*.o

gen-sys5lib gen-sys5lib_p: ${OBJS}
	@echo "building profiled gen-sys5lib"
	@cd profiled; ar cru ../gen-sys5lib_p ${OBJS}
	@echo "buiding normal gen-sys5lib"
	@ar cru gen-sys5lib ${OBJS}

clean:
	rm -f gen-sys5lib gen-sys5lib_p ${OBJS} profiled/*.o
