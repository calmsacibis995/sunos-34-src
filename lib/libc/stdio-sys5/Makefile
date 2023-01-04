#
# @(#)Makefile 1.1 86/09/24 SMI
#
CFLAGS	= -O -DS5EMUL -I/usr/5include

OBJS=	doscan.o \
	filbuf.o fopen.o fprintf.o \
	printf.o \
	setbuf.o sprintf.o \
	tmpnam.o \
	vfprintf.o vprintf.o vsprintf.o

.c.o:
	$(CC) -p $(CFLAGS) -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled/$*.o
	$(CC) $(CFLAGS) -c $*.c
	${LD} -x -r $*.o
	mv a.out $*.o

stdio-sys5lib stdio-sys5lib_p: ${OBJS}
	@echo "building profiled stdio-sys5lib"
	@cd profiled; ar cru ../stdio-sys5lib_p ${OBJS}
	@echo "building normal stdio-sys5lib"
	@ar cru stdio-sys5lib ${OBJS}

clean:
	rm -f stdio-sys5lib stdio-sys5lib_p *.o profiled/*.o errs a.out core
