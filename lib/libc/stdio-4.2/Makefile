#
# @(#)Makefile 1.1 86/09/24 SMI
#
CFLAGS=	-O

OBJS=	doscan.o \
	filbuf.o fopen.o fprintf.o \
	printf.o \
	setbuf.o sprintf.o \
	tmpnam.o \
	vfprintf.o vprintf.o vsprintf.o

.c.o:
	${CC} -p ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out $*.o

stdio-4.2lib stdio-4.2lib_p: ${OBJS}
	@echo "building profiled stdio-4.2lib"
	@cd profiled; ar cru ../stdio-4.2lib_p ${OBJS}
	@echo "building normal stdio-4.2lib"
	@ar cru stdio-4.2lib ${OBJS}

clean:
	rm -f stdio-4.2lib stdio-4.2lib_p ${OBJS} profiled/*.o errs a.out core
