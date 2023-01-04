#	Makefile	1.1	86/09/24
#
DESTDIR=

# Files which live in the current directory and are copied to the destination.
#
FILES=	cshrc login profile

# Files which should exist but be empty in the destination
#
EMPTY=	rhosts

all:	${FILES}

${FILES}:

install:
	for i in ${FILES}; do (install -c -m 644 $$i ${DESTDIR}/.$$i); done
	for i in ${EMPTY}; do (touch ${DESTDIR}/.$$i); done

clean:

FRC:
