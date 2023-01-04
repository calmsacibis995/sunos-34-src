#
# @(#)Makefile 1.1 86/09/25 (Sun)
#

CFLAGS = -O -DLIBPFC

LIB = ${DESTDIR}/usr/lib
UCBDIR = ${DESTDIR}/usr/ucb
LIBPCDIR = ../libpc
LIBF77DIR = ../libF77

INSTALL= install -c -m 644
LD= /bin/ld
RM = /bin/rm -f
RANLIB= ranlib
AR= /bin/ar

PSRCS =	PCSTART.c
FSRCS = main.c

HDRS =	whoami.h h00vars.h

POBJS =	PCSTART.o
FOBJS = main.o

.c.o:
	${CC} -p ${CFLAGS} -c $*.c
	mv $@ tmp.o
	-${LD} -X -r -o profiled/$@ tmp.o
	${CC} ${CFLAGS} -c $*.c
	mv $@ tmp.o
	-${LD} -x -r -o $@ tmp.o

libpfc.a libpfc_p.a: ${POBJS} ${FOBJS}
	cd profiled; ${AR} cru ../libpfc_p.a ${POBJS} ${FOBJS}
	${RANLIB} libpfc_p.a
	${AR} cru libpfc.a ${POBJS} ${FOBJS}
	${RANLIB} libpfc.a

install: libpfc.a libpfc_p.a
	${INSTALL} libpfc_p.a ${LIB}/libpfc_p.a
	${RANLIB} ${LIB}/libpfc_p.a
	${INSTALL} libpfc.a ${LIB}/libpfc.a
	${RANLIB} ${LIB}/libpfc.a

sources: ${PSRCS} ${FSRCS} ${HDRS}

whoami.h:
	rm -f $@
	ln -s ${LIBPCDIR}/$@ .

${PSRCS}:
	rm -f $@
	ln -s ${LIBPCDIR}/$@ .

${FSRCS}:
	rm -f $@
	ln -s ${LIBF77DIR}/$@ .

clean:
	${RM} libpfc.a libpfc_p.a *.o profiled/*.o a.out lpr tmp core errs

print:
	ls -l >lst
	${UCBDIR}/vprint lst Makefile *.h [A-Z][A-Z]*.[cs] [a-z][a-z]*.[cs]
	${RM} lst

grind: sources
	${UCBDIR}/ctags -v *.h *.c | sort -f >index
	${UCBDIR}/vgrind -t -x index >lpr
	${UCBDIR}/vgrind -t -n Makefile >>lpr
	${RM} index
	${UCBDIR}/vgrind -t *.h *.c >>lpr
	${UCBDIR}/vpr -t lpr

depend:	sources
	/bin/grep '^#[ 	]*include' ${PSRCS} ${FSRCS} | sed \
		-e '/<.*>/d' \
		-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
		-e 's/\.c/.o/' >makedep
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it
# DEPENDENCIES MUST END AT END OF FILE
PCSTART.o: h00vars.h
PCSTART.o: libpc.h
PCEXIT.o: h00vars.h
main.o: ../libI77/fiodefs.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
