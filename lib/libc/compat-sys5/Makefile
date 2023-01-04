#
# @(#)Makefile 1.1 86/09/24 SMI
#
CFLAGS=	-O -DS5EMUL -I/usr/5include
STD=	nice.o rand.o times.o utime.o
# these should just be pitched, but in the interest of compatibility...
TRASH=	getpw.o 
OBJS=	${STD} ${TRASH}

.c.o:
	${CC} -p ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out $*.o

compat-sys5lib compat-sys5lib_p: ${OBJS}
	@echo "building profiled compat-sys5lib"
	@cd profiled; ar cru ../compat-sys5lib_p ${OBJS}
	@echo "buiding normal compat-sys5lib"
	@ar cru compat-sys5lib ${OBJS}

clean:
	rm -f compat-sys5lib compat-sys5lib_p ${OBJS} profiled/*.o

times.o: epoch.h

epoch.h: mkepoch
	./mkepoch >epoch.h

mkepoch: mkepoch.c
	${CC} -O -o mkepoch mkepoch.c
