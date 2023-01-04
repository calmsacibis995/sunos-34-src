#
# @(#)Makefile 1.1 86/09/24 SMI
#

CPP	= /lib/cpp
CFLAGS	= -O -DS5EMUL -I/usr/5include

OBJS=	_getpgrp.o \
	_ioctl.o \
	_kill.o \
	_lseek.o \
	_mount.o \
	_nap.o \
	_open.o \
	_read.o \
	_setpgrp.o \
	_write.o \
	getpgrp.o \
	ioctl.o \
	kill.o \
	lseek.o \
	mount.o \
	open.o \
	plock.o \
	read.o \
	setpgrp.o stime.o \
	umount.o uname.o ustat.o \
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

sys-sys5lib sys-sys5lib_p: ${OBJS}
	@echo "building profiled sys-sys5lib"
	@cd profiled; ar cru ../sys-sys5lib_p ${OBJS}
	@echo "building normal sys-sys5lib"
	@ar cru sys-sys5lib ${OBJS}

clean:
	rm -f sys-sys5lib sys-sys5lib_p *.pp *.o profiled/*.o errs a.out core
