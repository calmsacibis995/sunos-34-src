#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB
#
# make file for tip device drivers
#
# Current drivers:
#	BIZCOMP
#	DEC DF02-AC, DF03-AC
#	DEC DN-11/Able Quadracall
#	VENTEL 212+ (w/o echo)
#	VADIC 831 RS232 adaptor
#	VADIC 3451
#	HAYES SmartModem

CFLAGS=	-O -I../
OBJS=	biz22.o biz31.o df.o dn11.o hayes.o ventel.o v831.o v3451.o

aculib.a: ${OBJS}
	ar cu aculib.a *.o
	ranlib aculib.a

${OBJS}: ../tip.h

clean:
	rm -f aculib.a *.o core errs a.out
