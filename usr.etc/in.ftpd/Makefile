#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 4.2 06/12/83
#
ALL=	ftpd.o ftpcmd.o glob.o vers.o
SRC=	ftpd.c ftpcmd.y glob.c vers.c
DESTDIR=
CFLAGS=	-O

ftpd:	${ALL}
	${CC} -o ftpd ${ALL}

tags:	${SRC}
	ctags ${SRC}

vers.o:	ftpd.c ftpcmd.y
	sh newvers.sh
	${CC} -c vers.c

install: ftpd
	install -s ftpd ${DESTDIR}/usr/etc/in.ftpd

clean:
	rm -f ftpd *.o *.s errs core a.out t.? yacc.* y.tab.c
