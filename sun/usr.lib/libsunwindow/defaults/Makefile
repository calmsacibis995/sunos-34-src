#
#	@(#)Makefile	87/01/07 1.3 SMI
#
DESTDIR=
CPUFLAG=
PRODLIBDIR=/usr/lib
PRODINCLUDEDIR=/usr/include/sunwindow

INCLUDE=/usr/include
INSTALL=install -m 644

DBX=
CFLAGS=	-O -I$(INCLUDE)
CFLAGS_LOCAL=

CFLAGS_NOT_ROI_DATA= $(CFLAGS) $(DBX) $(CFLAGS_LOCAL) $(CPUFLAG)

HDRSPUBLIC=	defaults.h
HDRSPRIVATE=	hash.h parse.h
CFILES=		defaults.c parse.c hash.c
OBJFILES=	defaults.o parse.o hash.o
GPROFFILESALL=	defaults.pg parse.pg hash.pg
LIBFILE=	libdefaults.a
GPROFLIBFILE=	libdefaults_p.a
.SUFFIXES: .pg

CFILESALL=	$(CFILES)
HDRSALL=	${HDRSPUBLIC} ${HDRSPRIVATE}
SRCSALL=	$(CFILESALL) $(HDRSALL) Makefile

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

#	FIXED so don't have to do a CFLAGS_NOT_ROI_DATA: defaults.c
#	get_login_directory was doing unnecessary "directory" init.

clean:
	-rm -f ${LIBFILE} core err* *.o *% *.BAK *.pg

install:	install_h

install_h:	FRC
	-for i in ${HDRSPUBLIC}; do \
		cmp -s $$i ${DESTDIR}${PRODINCLUDEDIR}/$$i || \
		${INSTALL} $$i ${DESTDIR}${PRODINCLUDEDIR}/$$i; \
	done

install_lib: ${LIBFILE}
	${INSTALL} ${LIBFILE} ${DESTDIR}${PRODLIBDIR}
	ranlib ${DESTDIR}${PRODLIBDIR}/${LIBFILE}

hash_test:	hash_test.o hash.o parse.o
	cc $(DBX) hash_test.o hash.o parse.o -o hash_test

defaults_test:	defaults_test.o ${LIBFILE}
	cc $(DBX) defaults_test.o ${LIBFILE} -o defaults_test

FRC:
# non-existent so always younger

depend:
	make_depend -I${INCLUDE} ${CFILESALL}

#include Make.depend


# DO NOT DELETE THIS LINE -- make depend uses it
defaults.o: defaults.c \
	${INCLUDE}/sunwindow/sun.h \
	hash.h \
	parse.h \
	${INCLUDE}/sunwindow/defaults.h

parse.o: parse.c \
	${INCLUDE}/sunwindow/sun.h \
	parse.h

hash.o: hash.c \
	${INCLUDE}/sunwindow/sun.h \
	hash.h



# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# SEE HOW make depend WORKS FOR FURTHER INFO
