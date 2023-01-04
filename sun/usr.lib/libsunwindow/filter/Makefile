#
#	@(#)Makefile	87/01/07 1.3 SMI
#
DESTDIR=
CPUFLAG=
PRODLIBDIR=/usr/lib
PRODINCLUDEDIR=/usr/include/sunwindow

INSTALL=install -m 644
INCLUDE=/usr/include

DBX=
CFLAGS=	-O -I$(INCLUDE)
CFLAGS_LOCAL=

CFLAGS_NOT_ROI_DATA= $(CFLAGS) $(DBX) $(CFLAGS_LOCAL) $(CPUFLAG)

HEADERFILES=	filter.h
CFILES=		filter.c
OBJFILES=	filter.o
GPROFFILESALL= filter.pg
LIBFILE=	libfilter.a
GPROFLIBFILE=	libfilter_p.a

CFILESALL=	$(CFILES)
HDRSALL=	$(HEADERFILES)
SRCSALL=	$(CFILESALL) $(HDRSALL) Makefile
.SUFFIXES: .pg

.c.o:
	cc $(CFLAGS_NOT_ROI_DATA) -A-R -c $<

.c.pg:
	if [ -f $*.o ]; then \
		mv $*.o $*.o.tmp; \
	fi
	${CC} ${CFLAGS_NOT_ROI_DATA} -pg -c $<
	-mv $*.o $*.pg
	if [ -f $*.o.tmp ]; then \
		mv $*.o.tmp $*.o; \
	fi


${LIBFILE}:	${OBJFILES}
	ar uv ${LIBFILE} $?
	ranlib ${LIBFILE}

gprof:: ${GPROFLIBFILE}

${GPROFLIBFILE}:: ${GPROFFILESALL}
	ar uvl ${GPROFLIBFILE} $?
	ranlib ${GPROFLIBFILE}

clean:
	-rm -f ${LIBFILE} core errs *.o *% *.BAK *.pg

install:	install_h

install_h:	FRC
	-for i in ${HEADERFILES}; do \
		cmp -s $$i ${DESTDIR}${PRODINCLUDEDIR}/$$i || \
		${INSTALL} $$i ${DESTDIR}${PRODINCLUDEDIR}/$$i; \
	done

install_lib: ${LIBFILE}
	${INSTALL} ${LIBFILE} ${DESTDIR}${PRODLIBDIR}
	ranlib ${DESTDIR}${PRODLIBDIR}/${LIBFILE}

FRC:
# non-existent so always younger

depend:
	make_depend -I${INCLUDE} ${CFILESALL}

#include Make.depend

# DO NOT DELETE THIS LINE -- make depend uses it
filter.o: filter.c \
	${INCLUDE}/sunwindow/io_stream.h \
	${INCLUDE}/sunwindow/sun.h \
	${INCLUDE}/sunwindow/string_utils.h \
	${INCLUDE}/sunwindow/filter.h



# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# SEE HOW make depend WORKS FOR FURTHER INFO
