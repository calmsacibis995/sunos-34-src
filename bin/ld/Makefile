#
# @(#)Makefile	1.1 (Sun) 9/24/86 19:31:43
#
CFLAGS  = -O -Dsun3
OBJS    = ld.o incl.o
PROGRAM = ld

${PROGRAM}:	${OBJS}
	${CC} -o ${PROGRAM} ${OBJS}

install:	${PROGRAM}
	install -s ${PROGRAM} ${DESTDIR}/bin/${PROGRAM}

clean:	
	rm -f ${PROGRAM} ${OBJS}


