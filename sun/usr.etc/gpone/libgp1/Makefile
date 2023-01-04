#
# @(#)Makefile  1.1  86/09/25
#

DESTDIR=
CFLAGS=	-O

CFILES=	gp1.c

OBJS=	gp1.o

all:	libgp1.a

libgp1.a: ${OBJS}
	ar uv libgp1.a $?
	ranlib libgp1.a

install:
	echo 'Do nothing here'

clean:
	rm -f *.o *.bak libgp1.a

depend:
	cat </dev/null >x.c
	for i in ${CFILES}; do \
		(/bin/grep '^#[ 	]*include' x.c $$i | sed \
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
# DO NOT DELETE THIS LINE -- make depend uses it
# DEPENDENCIES MUST END AT END OF FILE
gp1: /usr/include/sys/types.h
gp1: /usr/include/sys/file.h
gp1: /usr/include/sys/mman.h
gp1: /usr/include/sys/ioctl.h
gp1: /usr/include/sun/gpio.h
gp1: /usr/include/stdio.h
gp1: /usr/include/pixrect/gp1reg.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
