#
# @(#)Makefile 1.1 86/09/24 SMI
#
# printf & co are here because they are machine dependent
# (although only in the sense that the changes to get
# around the full "varargs" mechanism may only work on
# some machines)
#
CFLAGS=	-O
CPP=/lib/cpp

SRCS=	fprintf.c printf.c sprintf.c
OBJS=	fprintf.o printf.o sprintf.o

.c.o:
	${CC} -p ${CFLAGS} -c -DS5EMUL -I/usr/5include $*.c
	$(LD) -x -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c -DS5EMUL -I/usr/5include $*.c
	$(LD) -x -r $*.o
	mv a.out $*.o

stdio-sys5lib stdio-sys5lib_p: ${OBJS}
	@echo "building profiled stdio-sys5lib"
	@cd profiled; ar cru ../stdio-sys5lib_p ${OBJS}
	@echo "building normal stdio-sys5lib"
	@ar cru stdio-sys5lib ${OBJS}

clean:
	rm -f stdio-sys5lib stdio-sys5lib_p
	rm -f ${OBJS} profiled/*.o *.pp
