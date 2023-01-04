#
# @(#)Makefile 1.1 86/09/25 SMI
#
DESTDIR=
CFLAGS=	-O

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=

# Files that need only be installed and are never removed.
#
FILES=	nd.local

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	dkinfo nd

# C programs that live in the current directory and need explicit make lines.
#
NSTD=

all:	${SUBDIR} ${STD} ${NSTD}

#${SUBDIR}: FRC
#	cd $@; make ${MFLAGS}

${STD}:
	cc ${CFLAGS} -o $@ $@.c

install:
#	for i in ${SUBDIR}; do \
#		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	for i in ${FILES}; do (install -c -m 644 $$i ${DESTDIR}/etc/$$i); done
	for i in ${STD} ${NSTD}; do (install -s $$i ${DESTDIR}/etc/$$i); done

clean:
	rm -f a.out core *.s *.o
#	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
	rm -f ${STD} ${NSTD}

FRC:

depend:
	cat </dev/null >x.c
	for i in ${STD} ${NSTD}; do \
		(echo $$i: $$i.c >>makedep; \
		/bin/grep '^#[ 	]*include' x.c $$i.c | sed \
			-e '/\.\.\/h/d' \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.c//' >>makedep); done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep x.c
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# Files listed in ${NSTD} have explicit make lines given below.

# DO NOT DELETE THIS LINE -- make depend uses it
# DEPENDENCIES MUST END AT END OF FILE
dkinfo: dkinfo.c
dkinfo: /usr/include/stdio.h
dkinfo: /usr/include/sys/types.h
dkinfo: /usr/include/sys/stat.h
dkinfo: /usr/include/sys/ioctl.h
dkinfo: /usr/include/sun/dklabel.h
dkinfo: /usr/include/sun/dkio.h
nd: nd.c
nd: /usr/include/sys/types.h
nd: /usr/include/sys/socket.h
nd: /usr/include/net/if.h
nd: /usr/include/netinet/in.h
nd: /usr/include/netinet/if_ether.h
nd: /usr/include/sys/ioctl.h
nd: /usr/include/sun/ndio.h
nd: /usr/include/stdio.h
nd: /usr/include/netdb.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
