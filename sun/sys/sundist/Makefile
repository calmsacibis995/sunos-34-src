#	Makefile	1.1	86/09/25
#
DESTDIR=
CFLAGS=	-O

STD= 	tapefile
all: ${STD}

install:
	install -c -m 755 MAKEDEV ${DESTDIR}/dev
	install -c -m 755 MAKEHOSTS ${DESTDIR}/usr/hosts

clean:
	rm -f a.out core *.s *.o
	rm -f ${STD}

${STD}:
	cc ${CFLAGS} -o $@ $@.c

depend:
	cat </dev/null >x.c
	for i in ${STD} ${NSTD}; do \
		(echo $$i: $$i.c >>makedep; \
		/bin/grep '^#[ 	]*include' x.c $$i.c | \
		/bin/grep -v '/vax.*\.h' | sed \
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

FRC:

# DO NOT DELETE THIS LINE -- make depend uses it
tapefile: tapefile.c
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
