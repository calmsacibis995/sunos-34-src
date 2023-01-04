#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 4.10 83/06/12
#
# Copyright (c) 1982 Regents of the University of California
#
DESTDIR=
CFLAGS= -O
SRCS=	restore.h main.c restore.c dirs.c symtab.c tape.c utilities.c\
	dumprmt.c
OBJS=	main.o restore.o dirs.o symtab.o tape.o utilities.o dumprmt.o
TOUCH=	touch -cf

all: restore

restore: ${OBJS}
	cc ${LDFLAGS} ${CFLAGS} -o restore ${OBJS}

install: restore
	install -s restore ${DESTDIR}/etc/restore
	-rm -f ${DESTDIR}/etc/rrestore
	ln -s /etc/restore ${DESTDIR}/etc/rrestore

clean:
	rm -f restore *.o errs a.out core

lint:
	lint ${FILES}

print:
	@pr /usr/include/dumprestor.h ${SRCS}

dumprmt.c:
	ln -s ../dump/dumprmt.c .

restore.h:
	${TOUCH} restore.h

depend: ${SRCS}
	cp /dev/null x.c
	for i in ${SRCS} ; do \
		(/bin/grep '^#[ 	]*include' x.c $$i | sed \
			-e '/\.\.\/h/d' \
			-e '/\.\.\/ufs/d' \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.c/\.o/' >>makedep); done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep x.c
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it

restore.h: /usr/include/stdio.h
restore.h: /usr/include/sys/param.h
restore.h: /usr/include/sys/time.h
restore.h: /usr/include/sys/vnode.h
restore.h: /usr/include/ufs/inode.h
restore.h: /usr/include/ufs/fs.h
main.o: restore.h
main.o: /usr/include/signal.h
restore.o: restore.h
dirs.o: restore.h
dirs.o: /usr/include/dumprestor.h
dirs.o: /usr/include/sys/file.h
dirs.o: /usr/include/sys/dir.h
symtab.o: restore.h
symtab.o: /usr/include/sys/stat.h
symtab.o: /usr/include/sys/dir.h
tape.o: restore.h
tape.o: /usr/include/dumprestor.h
tape.o: /usr/include/sys/ioctl.h
tape.o: /usr/include/sys/mtio.h
tape.o: /usr/include/sys/file.h
tape.o: /usr/include/setjmp.h
tape.o: /usr/include/sys/stat.h
utilities.o: restore.h
dumprmt.o: /usr/include/sys/param.h
dumprmt.o: /usr/include/sys/mtio.h
dumprmt.o: /usr/include/sys/ioctl.h
dumprmt.o: /usr/include/netinet/in.h
dumprmt.o: /usr/include/stdio.h
dumprmt.o: /usr/include/netdb.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
