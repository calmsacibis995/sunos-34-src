CFLAGS=-O
GET= sccs get

all: get_validation_file fpatest

fpatest: fpatest.c fpa.h values.h
	${CC} fpatest.c -o fpatest

values.h: ../microcode/constants
	sed -f get_validation_file ../microcode/constants > values.h

clean:
	rm -f fpatest values.h core

install:	fpatest
	install -s fpatest ${DESTDIR}/usr/etc/fpa/fpaversion

get_validation_file:
	$(GET) $@
