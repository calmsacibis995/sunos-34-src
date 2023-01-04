#
#	@(#)Makefile	87/01/07 1.3
#
DESTDIR=
CPUFLAG=
PRODLIBDIR=/usr/lib
PRODINCLUDEDIR=/usr/include/sunwindow

INCLUDE=/usr/include

DBX=
CFLAGS=	-O -I$(INCLUDE)
CFLAGS_LOCAL=

CFLAGS_NOT_ROI_DATA= $(CFLAGS) $(DBX) $(CFLAGS_LOCAL) $(CPUFLAG)

INSTALL=install -m 644

HEADERFILES=	io_stream.h
CFILES=		input.c output.c io.c \
		string_streams.c file_streams.c other_streams.c
OBJFILES=	input.o output.o io.o \
		string_streams.o file_streams.o other_streams.o
GPROFFILESALL=  ${CFILES:.c=.pg}

LIBFILE=	libio_stream.a
GPROFLIBFILE=	libio_stream_p.a

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
input.o: input.c \
	io_stream.h \
	${INCLUDE}/sunwindow/sun.h

output.o: output.c \
	io_stream.h \
	${INCLUDE}/sunwindow/sun.h

io.o: io.c \
	io_stream.h \
	${INCLUDE}/sunwindow/sun.h

string_streams.o: string_streams.c \
	io_stream.h \
	${INCLUDE}/sunwindow/sun.h

file_streams.o: file_streams.c \
	io_stream.h \
	${INCLUDE}/sunwindow/sun.h

other_streams.o: other_streams.c \
	io_stream.h \
	${INCLUDE}/sunwindow/sun.h



# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# SEE HOW make depend WORKS FOR FURTHER INFO
