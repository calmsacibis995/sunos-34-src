#
# @(#)Makefile 1.1 86/09/24 SMI
#

CPP	= /lib/cpp
CFLAGS	= -O

OBJS=	getpgrp.o \
	ioctl.o \
	kill.o \
	lseek.o \
	mount.o \
	open.o \
	read.o \
	setpgrp.o \
	write.o

.c.o:
	${CC} -p $(CFLAGS) -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled/$*.o
	${CC} $(CFLAGS) -c $*.c
	${LD} -x -r $*.o
	mv a.out $*.o

.s.o:
	${CPP} -E -DPROF -I../sys $*.s > $*.pp
	${AS} -o $*.o $*.pp
	rm -f $*.pp
	${LD} -x -r $*.o
	mv a.out profiled/$*.o
	${CPP} -E -I../sys $*.s > $*.pp
	${AS} -o $*.o $*.pp
	rm -f $*.pp
	${LD} -x -r $*.o
	mv a.out $*.o

sys-4.2lib sys-4.2lib_p: ${OBJS}
	$(PROF)@echo "building profiled sys-4.2lib"
	$(PROF)@cd profiled; ar cru ../sys-4.2lib_p ${OBJS}
	@echo "building normal sys-4.2lib"
	@ar cru sys-4.2lib ${OBJS}

clean:
	rm -f sys-4.2lib sys-4.2lib_p ${OBJS} profiled/*.o *.pp
