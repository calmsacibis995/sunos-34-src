#
# @(#)Makefile 1.1 86/09/25 SMI
#
ALL=	eeprom.o getdate.o
SRC=	eeprom.c getdate.y
DESTDIR=
CFLAGS=	-O

eeprom:	${ALL}
	${CC} ${LDFLAGS} -o eeprom ${ALL}

tags:	${SRC}
	ctags ${SRC}

install: eeprom
	install -s eeprom ${DESTDIR}/usr/etc/eeprom

clean:
	rm -f eeprom *.o *.s errs core a.out yacc.* y.tab.c
