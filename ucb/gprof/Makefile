#
# @(#)Makefile 1.3 87/02/26 SMI; from UCB 1.10 3/31/83
#

DFLAGS = 
CFLAGS = -O ${DFLAGS}
LINTFLAGS = -x -a -n ${DFLAGS}

RM=	/bin/rm -f
PR=pr

GPROFHDRS =	gprof.h vax.h sun.h gcrt0.h
GPROFSRCS =	gprof.c arcs.c dfn.c lookup.c calls.vax.c calls.sun.c \
		printgprof.c printlist.c
GPROFOBJS =	gprof.o arcs.o dfn.o lookup.o calls.o \
		printgprof.o printlist.o

gprof: ${GPROFOBJS}
	cc -o gprof ${CFLAGS} ${GPROFOBJS}

install: gprof flat.blurb callg.blurb
	install -s gprof ${DESTDIR}/usr/ucb/gprof
	install -c -m 644 flat.blurb ${DESTDIR}/usr/lib/gprof.flat.blurb
	install -c -m 644 callg.blurb ${DESTDIR}/usr/lib/gprof.callg.blurb

calls.o:
	-if vax; then ${CC} -c ${CFLAGS} calls.vax.c; mv calls.vax.o calls.o; fi
	-if sun; then ${CC} -c ${CFLAGS} calls.sun.c; mv calls.sun.o calls.o; fi

clean:
	${RM} ${GPROFOBJS} errs gprof

#gcrt0.h:
#
#        	cp /usr/src/libc/csu/gcrt0.h gcrt0.h
#
print:
	@ ls -l | ${PR}
	@ ${PR} makefile
	@ ${PR} gcrt0.h
	@ ${PR} ${GPROFHDRS} ${GPROFSRCS}
	@ ${PR} gprof.flat.blurb gprof.callg.blurb

lint:
	lint ${LINTFLAGS} ${DFLAGS} ${GPROFSRCS}

gprof.o: gprof.c ${GPROFHDRS}
arcs.o: arcs.c ${GPROFHDRS}
lookup.o: lookup.c ${GPROFHDRS}
dfn.o: dfn.c ${GPROFHDRS}
calls.o: calls.vax.c calls.sun.c ${GPROFHDRS}
printgprof.o: printgprof.c ${GPROFHDRS}
printlist.o: printlist.c ${GPROFHDRS}
