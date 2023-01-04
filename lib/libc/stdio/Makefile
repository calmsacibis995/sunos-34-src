#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 4.8 83/06/30
#
CFLAGS=	-O

OBJS=	clrerr.o ctermid.o cuserid.o doprnt.o \
	fdopen.o fgetc.o fgets.o findiop.o flsbuf.o \
	fputc.o fputs.o fread.o fseek.o ftell.o fwrite.o \
	getchar.o gets.o getw.o putchar.o puts.o \
	putw.o rew.o scanf.o setbuffer.o setvbuf.o \
	tempnam.o tmpfile.o ungetc.o

.c.o:
	${CC} -p ${CFLAGS} -DS5EMUL -c -I/usr/5include $*.c
	${LD} -x -r $*.o
	mv a.out profiled-sys5/$*.o
	${CC} ${CFLAGS} -DS5EMUL -c -I/usr/5include $*.c
	${LD} -x -r $*.o
	mv a.out sys5/$*.o
	${CC} -p ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out $*.o

stdiolib stdiolib_p stdiolibs5 stdiolibs5_p: ${OBJS}
	@echo "building profiled Sys5 stdiolib"
	@cd profiled-sys5; ar cru ../stdiolibs5_p ${OBJS}
	@echo "building normal Sys5 stdiolib"
	@cd sys5; ar cru ../stdiolibs5 ${OBJS}
	@echo "building profiled stdiolib"
	@cd profiled; ar cru ../stdiolib_p ${OBJS}
	@echo "building normal stdiolib"
	@ar cru stdiolib ${OBJS}

clean:
	rm -f stdiolib stdiolib_p stdiolibs5 stdiolibs5_p
	rm -f *.o profiled/*.o sys5/*.o profiled-sys5/*.o errs a.out core

fgets.o flsbuf.o fputs.o fread.o fwrite.o gets.o puts.o: stdiom.h

doprnt.o:	print.h
