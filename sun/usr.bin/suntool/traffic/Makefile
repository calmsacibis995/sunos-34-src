#
# @(#)Makefile 1.3 87/01/07 SMI
#
CPUFLAG= 
DESTDIR=
LIBDIR= /usr/src/sun/usr.lib
LIBS=   -lsuntool -lsunwindow -lpixrect
LIBSPLUS=${LIBS} -lrpcsvc
        
INCLUDE=/usr/include
PRODLIBDIR=/usr/lib
PRODINCLUDEDIR=/usr/include/suntool

DBX= 
CFLAGS=	-O ${DBX} -I${INCLUDE}
LDFLAGS= ${DBX}
CFLAGS_NOT_ROI_DATA= $(CFLAGS) $(CFLAGSLOCAL) $(DBX) $(CPUFLAG)

HDRS=		traffic.h
CFILES= 	traffic.c draw.c panel.c rpc.c
OBJFILES=	traffic.o draw.o panel.o rpc.o
GPROFFILESALL= ${CFILES:.c=.pg}
NO_MAIN_OBJS=	draw.o panel.o rpc.o

CFILESALL=$(CFILES)
HDRSALL=$(HDRS)
SRCSALL=$(CFILESALL) $(HDRSALL)
 
AWK=	/bin/awk
LINT=   /usr/bin/lint
LIBFILE= libtraffic.a
BASETOOLS_BIN= suntools
OTHERTOOLS_BIN= othertools
GPROF_MERGE_LIB= libtoolmerge_p.a

.SUFFIXES: .pg
.c.o:
	cc $(CFLAGS_NOT_ROI_DATA) -c $<

.c.pg:
	if [ -f $*.o ]; then \
		mv $*.o $*.o.tmp; \
	fi
	cc ${CFLAGS_NOT_ROI_DATA} -pg -c $<
	-mv $*.o $*.pg
	if [ -f $*.o.tmp ]; then \
		mv $*.o.tmp $*.o; \
	fi

${LIBFILE}:	${OBJFILES}
	ar uv ${LIBFILE} $?
	ranlib ${LIBFILE}

gprof:: ${GPROFFILESALL}
	ar uvl ../${GPROF_MERGE_LIB} ${GPROFFILESALL}

standalone:       ${NO_MAIN_OBJS} ${LIBS}
	${CC} -DSTANDALONE ${CFLAGS_NOT_ROI_DATA} -o traffic traffic.c \
	${NO_MAIN_OBJS} ${LIBSPLUS}

clean:	
	-rm -f core errs *.o *% *.BAK traffic *.pg

install:	
	rm -f ${DESTDIR}/usr/bin/traffic
	ln -s ${OTHERTOOLS_BIN} ${DESTDIR}/usr/bin/traffic

install_h:	NONEXISTENT_SO_ALWAYS_YOUNGER
	-for i in ${HFILES}; do \
		cmp -s $$i ${DESTDIR}${PRODINCLUDEDIR}/$$i || \
		install $$i ${DESTDIR}${PRODINCLUDEDIR}/$$i; \
	done
	
lint:
	${LINT} -hb ${CFILES} ${LIBSPLUS}
        
tags:   ${CFILES}
	ctags -w ${CFILES}

NONEXISTENT_SO_ALWAYS_YOUNGER:

depend:
	make_depend -I${INCLUDE} ${CFILES}

# DO NOT DELETE THIS LINE -- make depend uses it

# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
