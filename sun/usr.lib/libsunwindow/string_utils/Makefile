#
#	@(#)Makefile	86/09/25 1.1 SMI
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

HEADERFILES=	string_utils.h
CFILES=		string_utils.c white_space.c
OBJFILES=	string_utils.o white_space.o
LIBFILE=	libstring_utils.a

CFILESALL=$(CFILES)
HDRSALL=$(HEADERFILES)
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile

.c.o:
	cc $(CFLAGS_NOT_ROI_DATA) -A-R -c $<

${LIBFILE}:	${OBJFILES}
	ar uv ${LIBFILE} $?
	ranlib ${LIBFILE}

clean:
	-rm -f ${LIBFILE} core errs *.o *% *.BAK

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
string_utils.o: string_utils.c \
	${INCLUDE}/sunwindow/sun.h \
	${INCLUDE}/sunwindow/string_utils.h

white_space.o: white_space.c \
	string_utils.h \
	${INCLUDE}/sunwindow/sun.h



# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# SEE HOW make depend WORKS FOR FURTHER INFO
