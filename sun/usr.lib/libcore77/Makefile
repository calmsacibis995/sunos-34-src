#
# @(#)Makefile 1.1 86/09/25 SMI
#

#   Build libcore77.a -- Fortran 77 interface routines for SunCore library

DESTDIR =

CFLAGS = -O

OBJECTS = \
	batch77.o credelseg77.o errors77.o get_view_surface77.o\
	init_termin77.o inprims77.o inqinput77.o inqprimatt77.o\
	inqsegatt77.o inqtextent77.o inquiry77.o\
	ndctowld77.o newframe77.o outprim277.o outprim377.o polygon277.o\
	polygon377.o rasterfileio77.o rasterprim77.o\
	savesegment77.o segatt377.o\
	segdefaults77.o segments77.o setprimatt77.o setsegatt77.o\
	textmark77.o view_surface77.o view_trans77.o xformattrib77.o

libcore77.a:	$(OBJECTS)
	ar ru libcore77.a $(OBJECTS)
	ranlib libcore77.a

clean:
	rm -f *.o libcore77.a

install:	libcore77.a
	install -c -m 644 libcore77.a ${DESTDIR}/usr/lib
	ranlib ${DESTDIR}/usr/lib/libcore77.a
	-mkdir ${DESTDIR}/usr/include/f77 && \
		chown bin ${DESTDIR}/usr/include/f77 && \
		chmod 755 ${DESTDIR}/usr/include/f77
	install -c -m 644 usercore77.h ${DESTDIR}/usr/include/f77
