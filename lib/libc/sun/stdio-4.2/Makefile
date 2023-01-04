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
	${CC} -p ${CFLAGS} -c $*.c
	$(LD) -x -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	$(LD) -x -r $*.o
	mv a.out $*.o

stdio-4.2lib stdio-4.2lib_p: ${OBJS}
	@echo "building profiled stdio-4.2lib"
	@cd profiled; ar cru ../stdio-4.2lib_p ${OBJS}
	@echo "building normal stdio-4.2lib"
	@ar cru stdio-4.2lib ${OBJS}

clean:
	rm -f stdio-4.2lib stdio-4.2lib_p
	rm -f ${OBJS} profiled/*.o *.pp
