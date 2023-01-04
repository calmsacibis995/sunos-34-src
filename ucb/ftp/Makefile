#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 4.6 06/15/83
#
ALL=	ftp
DESTDIR=
CFLAGS=	-O

all:	${ALL}

ftp:	cmds.o cmdtab.o ftp.o getpass.o glob.o main.o
	${CC} cmds.o cmdtab.o ftp.o getpass.o glob.o main.o -o ftp 

ftp.o:	/usr/include/arpa/ftp.h ftp_var.h
cmds.o:	/usr/include/arpa/ftp.h ftp_var.h
cmdtab.o: ftp_var.h
ftp.o:	ftp_var.h
main.o: ftp_var.h

install: all
	install -s ftp ${DESTDIR}/usr/ucb/ftp

clean:
	rm -f ${ALL} *.o *.s errs core a.out t.?
