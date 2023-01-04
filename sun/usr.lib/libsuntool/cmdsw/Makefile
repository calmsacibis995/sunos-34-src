#
#	@(#)Makefile	86/09/25 1.1 SMI
#
DESTDIR=
PRODLIBDIR=/usr/lib
PRODINCLUDEDIR=/usr/include/suntool

INCLUDE=/usr/include
INSTALL=install -m 644
CFLAGS=	-O -I${INCLUDE}
CFLAGSPLUS= ${CFLAGS} -DCMDSW
DBX=

# NOTE -- all files in this directory are links to ../ttysw
#	      except for the Makefile

HDRSPUBLIC=	
HDRSPRIVATE=	charimage.h charscreen.h ttyansi.h ttysw_impl.h ttytlsw_impl.h
       
CFILES=		ttyansi.c ttysw_init.c ttysw_notify.c ttysw_main.c 

CFILESALL=$(CFILES)
HDRSALL=$(HDRSPUBLIC) $(HDRSPRIVATE)
SRCSALL=$(CFILESALL) $(HDRSALL) 

AWK=	/bin/awk

OBJFILES=	ttyansi.o ttysw_init.o ttysw_notify.o ttysw_main.o
        
LIBFILE=	libcmdsw.a

.c.o:
	cc $(CFLAGSPLUS) $(DBX) -c $<

#Note that the following two rules must be in this order so that make
#(or make all) does the sccs gets first!
all:	${LIBFILE}

${LIBFILE}:	${OBJFILES}
	ar uv ${LIBFILE} $?
	ranlib ${LIBFILE}

clean:
	-rm -f ${LIBFILE} core errs *.o *% *.BAK

install: all
	${INSTALL} ${LIBFILE} ${DESTDIR}/usr/lib
	ranlib ${DESTDIR}/usr/lib/${LIBFILE}

install_h:

charimage.h: ../ttysw/SCCS/s.charimage.h
	sccs get ../ttysw/charimage.h

charscreen.h: ../ttysw/SCCS/s.charscreen.h
	sccs get ../ttysw/charscreen.h

ttyansi.h: ../ttysw/SCCS/s.ttyansi.h
	sccs get ../ttysw/ttyansi.h

ttysw_impl.h: ../ttysw/SCCS/s.ttysw_impl.h
	sccs get ../ttysw/ttysw_impl.h

ttytlsw_impl.h: ../ttysw/SCCS/s.ttytlsw_impl.h
	sccs get ../ttysw/ttytlsw_impl.h

ttyansi.c: ../ttysw/SCCS/s.ttyansi.c
	sccs get ../ttysw/ttyansi.c

ttysw_init.c: ../ttysw/SCCS/s.ttysw_init.c
	sccs get ../ttysw/ttysw_init.c

ttysw_notify.c: ../ttysw/SCCS/s.ttysw_notify.c
	sccs get ../ttysw/ttysw_notify.c

ttysw_main.c: ../ttysw/SCCS/s.ttysw_main.c
	sccs get ../ttysw/ttysw_main.c

depend:
	make_depend -I${INCLUDE} ${CFILES}

# DO NOT DELETE THIS LINE -- make depend uses it

ttyansi.o:  ttyansi.c    \
	${INCLUDE}/suntool/ttysw.h  ttysw_impl.h  ttyansi.h  \
	${INCLUDE}/suntool/textsw.h  \
	${INCLUDE}/sunwindow/rect.h  \
	${INCLUDE}/sunwindow/rectlist.h  \
	${INCLUDE}/sunwindow/pixwin.h  \
	${INCLUDE}/sunwindow/attr.h  \
	${INCLUDE}/suntool/tool.h

ttysw_init.o:  ttysw_init.c   \
	${INCLUDE}/sunwindow/rect.h  \
	${INCLUDE}/sunwindow/rectlist.h  \
	${INCLUDE}/sunwindow/pixwin.h  \
	${INCLUDE}/sunwindow/win_cursor.h  \
	${INCLUDE}/sunwindow/attr.h  \
	${INCLUDE}/suntool/ttysw.h  ttysw_impl.h  \
	${INCLUDE}/suntool/textsw.h  \
	${INCLUDE}/suntool/tool.h  

ttysw_notify.o:  ttysw_notify.c   \
	${INCLUDE}/sunwindow/notify.h  \
	${INCLUDE}/sunwindow/rect.h  \
	${INCLUDE}/sunwindow/rectlist.h  \
	${INCLUDE}/sunwindow/pixwin.h  \
	${INCLUDE}/sunwindow/win_input.h  \
	${INCLUDE}/sundev/vuid_event.h  \
	${INCLUDE}/suntool/ttysw.h  ttysw_impl.h  \
	${INCLUDE}/suntool/tool.h  \
	${INCLUDE}/sunwindow/attr.h  \
	${INCLUDE}/suntool/textsw.h  \
	charimage.h charscreen.h  

ttysw_main.o:  ttysw_main.c    \
	${INCLUDE}/sundev/kbd.h  \
	${INCLUDE}/sunwindow/rect.h  \
	${INCLUDE}/sunwindow/win_input.h  \
	${INCLUDE}/sundev/vuid_event.h  \
	${INCLUDE}/sunwindow/win_struct.h  \
	${INCLUDE}/sunwindow/win_cursor.h  \
	${INCLUDE}/sunwindow/attr.h  \
	${INCLUDE}/suntool/menu.h  \
	${INCLUDE}/suntool/ttysw.h  ttysw_impl.h  ttytlsw_impl.h  \
	${INCLUDE}/suntool/selection_svc.h  \
	${INCLUDE}/suntool/tool.h  \
	${INCLUDE}/suntool/textsw.h  \
	${INCLUDE}/sunwindow/rectlist.h  \
	${INCLUDE}/sunwindow/pixwin.h

# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# SEE HOW make depend WORKS FOR FURTHER INFO
