OBJ = main.o scanner.o bb.o error.o stmt.o
TCOV_SRC = tcov.c
DBX =
CFLAGS = -O ${DBX}

all:	bb_count tcov bb_link.o

bb_count:	${OBJ}
	${CC} ${CFLAGS} -o bb_count ${OBJ}

tcov:		${TCOV_SRC}
	${CC} ${CFLAGS} -o tcov ${TCOV_SRC}

bb_link.o:	bb_link.c
	${CC} ${CFLAGS} -c bb_link.c

install:	tcov bb_count bb_link.o
	install -s tcov ${DESTDIR}/usr/bin/tcov
	install -s bb_count ${DESTDIR}/usr/lib/bb_count
	install bb_link.o ${DESTDIR}/usr/lib/bb_link.o

clean:
	rm -f ${OBJ} tcov bb_count bb_link.o
