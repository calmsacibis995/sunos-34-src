#
#	@(#)Makefile	1.4	87/01/08 SMI
#
DESTDIR=
CPUFLAG=
PRODLIBDIR=/usr/lib
PRODINCLUDEDIR=/usr/include/suntool

INCLUDE=/usr/include
INSTALL=install -m 644
AWK=/bin/awk

DBX=
CFLAGS=	-O -I$(INCLUDE)
CFLAGS_LOCAL=

CFLAGS_NOT_ROI_DATA= $(CFLAGS) $(DBX) $(CFLAGS_LOCAL) $(CPUFLAG)

HDRSPUBLIC=	image.h walkmenu.h 
HDRSPRIVATE=	image_impl.h walkmenu_impl.h
CFILES= 	image.c walkmenu_public.c walkmenu_render.c \
		 walkmenu_set.c walkmenu_get.c walkmenu_customizable.c
GPROFFILESALL=${CFILES:.c=.pg}
.SUFFIXES: .pg

CFILESALL=$(CFILES)
HDRSALL=$(HDRSPUBLIC) $(HDRSPRIVATE)
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile

OBJFILES=	image.o walkmenu_public.o walkmenu_render.o \
		walkmenu_set.o walkmenu_get.o walkmenu_customizable.o

LIBFILE=	libmenu.a
GPROFLIBFILE=	libmenu_p.a

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


#Note that the following two rules must be in this order so that make
#(or make all) does the sccs gets first!
all: ${LIBFILE}

${LIBFILE}:	${OBJFILES}
	ar uv ${LIBFILE} $?
	ranlib ${LIBFILE}

gprof:: ${GPROFLIBFILE}

${GPROFLIBFILE}:: ${GPROFFILESALL}
	ar uvl ${GPROFLIBFILE} $?
	ranlib ${GPROFLIBFILE}

#	FIXED so don't have to do a CFLAGS_NOT_ROI_DATA: image.c.

clean:
	-rm -f ${LIBFILE} core errs *.o *% *.BAK *.pg

install:	install_h

install_h:	FRC
	-for i in ${HDRSPUBLIC}; do \
		cmp -s $$i ${DESTDIR}${PRODINCLUDEDIR}/$$i || \
		${INSTALL} $$i ${DESTDIR}${PRODINCLUDEDIR}/$$i; \
	done

FRC:
# non-existent so always younger

depend:
	make_depend -I${INCLUDE} ${CFILESALL}

#include Make.depend

# DO NOT DELETE THIS LINE -- make depend uses it
image.o: image.c \
	image_impl.h \
	${INCLUDE}/suntool/image.h \
	${INCLUDE}/sunwindow/attr.h

walkmenu_public.o: walkmenu_public.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/defaults.h \
	${INCLUDE}/sunwindow/sun.h \
	${INCLUDE}/suntool/fullscreen.h \
	${INCLUDE}/suntool/window.h \
	walkmenu_impl.h \
	${INCLUDE}/suntool/walkmenu.h \
	image_impl.h \
	${INCLUDE}/suntool/image.h

walkmenu_render.o: walkmenu_render.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/suntool/fullscreen.h \
	${INCLUDE}/suntool/window.h \
	walkmenu_impl.h \
	${INCLUDE}/suntool/walkmenu.h \
	image_impl.h \
	${INCLUDE}/suntool/image.h

walkmenu_set.o: walkmenu_set.c \
	walkmenu_impl.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/sunwindow/attr.h \
	image_impl.h \
	${INCLUDE}/suntool/image.h

walkmenu_get.o: walkmenu_get.c \
	walkmenu_impl.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/sunwindow/attr.h \
	image_impl.h \
	${INCLUDE}/suntool/image.h



# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# SEE HOW make depend WORKS FOR FURTHER INFO
