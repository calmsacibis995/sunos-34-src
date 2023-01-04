#
#	@(#)Makefile	87/01/08 1.6 SMI
#
DESTDIR=
CPUFLAG=
PRODLIBDIR=/usr/lib
PRODINCLUDEDIR=/usr/include/suntool

INCLUDE=/usr/include
INSTALL=install -m 644

DBX=
CFLAGS=	-O -I$(INCLUDE)
CFLAGS_LOCAL=

CFLAGS_NOT_ROI_DATA= $(CFLAGS) $(DBX) $(CFLAGS_LOCAL) $(CPUFLAG)

HDRSPUBLIC=	ttysw.h ttytlsw.h
HDRSPRIVATE=	charimage.h charscreen.h ttyansi.h ttysw_impl.h ttytlsw_impl.h
       
CFILES=		cim_change.c cim_size.c csr_change.c csr_init.c         \
		csr_sig.c ttyansi.c ttyhist.c ttysw_gtty.c ttysw_init.c \
		ttysw_notify.c ttysw_stty.c ttysw_tio.c ttysw_tsw.c     \
		ttytlsw.c ttytlsw_notify.c ttytlsw_tio.c ttyselect.c 	\
		ttysw_main.c ttysw_menu.c ttysw_mapkey.c ttysw_modes.c \
		ttysw_entity_stream.c

GPROFFILESALL=${CFILES:.c=.pg}
CFILESALL=$(CFILES)
HDRSALL=$(HDRSPUBLIC) $(HDRSPRIVATE)
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile
GPROFLIBFILE= libttysw_p.a
.SUFFIXES: .pg

AWK=	/bin/awk

OBJFILES=	cim_change.o cim_size.o csr_change.o csr_init.o         \
		csr_sig.o ttyansi.o ttyhist.o ttysw_gtty.o ttysw_init.o \
		ttysw_notify.o ttysw_stty.o ttysw_tio.o ttysw_tsw.o     \
		ttytlsw.o ttytlsw_notify.o ttytlsw_tio.o ttyselect.o 	\
		ttysw_main.o ttysw_menu.o ttysw_mapkey.o ttysw_modes.o \
		ttysw_entity_stream.o
        
LIBFILE=	libttysw.a

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
all:	${LIBFILE}

${LIBFILE}:	${OBJFILES}
	ar uv ${LIBFILE} $?
	ranlib ${LIBFILE}

gprof:: ${GPROFLIBFILE}

${GPROFLIBFILE}:: ${GPROFFILESALL}
	ar uvl ${GPROFLIBFILE} $?
	ranlib ${GPROFLIBFILE}

#	Change scrlins in ttyansi.c
ttyansi.o:
	cc ${CFLAGS_NOT_ROI_DATA} -c ttyansi.c

#	Dynamic menu in ttysw_menu.c
ttysw_menu.o:
	cc ${CFLAGS_NOT_ROI_DATA} -c ttysw_menu.c

#	Might want to change ttysw_bell_tv in csr_change.c
csr_change.o:
	cc ${CFLAGS_NOT_ROI_DATA} -c csr_change.c

clean:
	-rm -f ${LIBFILE} core errs *.o *% *.BAK *.pg

install:	install_h

install_h:	FRC
	-for i in ${HDRSPUBLIC}; do \
		cmp -s $$i ${DESTDIR}${PRODINCLUDEDIR}/$$i || \
		${INSTALL} $$i ${DESTDIR}${PRODINCLUDEDIR}/$$i; \
	done;
	cmp -s ttysw_impl.h \
	${DESTDIR}${PRODINCLUDEDIR}/.UNSUPPORTED.ttysw_impl.h || \
	${INSTALL} ttysw_impl.h \
	${DESTDIR}${PRODINCLUDEDIR}/.UNSUPPORTED.ttysw_impl.h; \
	cmp -s ttytlsw_impl.h \
	${DESTDIR}${PRODINCLUDEDIR}/.UNSUPPORTED.ttytlsw_impl.h || \
	${INSTALL} ttysw_impl.h \
	${DESTDIR}${PRODINCLUDEDIR}/.UNSUPPORTED.ttytlsw_impl.h;


FRC:
# non-existent so always younger

depend:
	make_depend -I${INCLUDE} ${CFILESALL}

#include Make.depend

# DO NOT DELETE THIS LINE -- make depend uses it
cim_change.o: cim_change.c \
	ttyansi.h \
	charimage.h \
	charscreen.h

cim_size.o: cim_size.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_notify.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool_struct.h \
	ttysw_impl.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/window.h \
	ttyansi.h \
	charimage.h \
	charscreen.h

csr_change.o: csr_change.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/suntool/ttysw.h \
	charimage.h \
	charscreen.h \
	ttyansi.h

csr_init.o: csr_init.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_environ.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	charimage.h \
	charscreen.h

csr_sig.o: csr_sig.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	charimage.h \
	charscreen.h

ttyansi.o: ttyansi.c \
	${INCLUDE}/suntool/ttysw.h \
	${INCLUDE}/sunwindow/win_struct.h \
	ttysw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	ttyansi.h \
	${INCLUDE}/suntool/selection_attributes.h

ttyhist.o: ttyhist.c \
	${INCLUDE}/suntool/ttysw.h \
	ttysw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	ttyansi.h \
	charimage.h

ttysw_gtty.o: ttysw_gtty.c

ttysw_init.o: ttysw_init.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/defaults.h \
	${INCLUDE}/sunwindow/sun.h \
	${INCLUDE}/suntool/ttysw.h \
	ttysw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	charscreen.h

ttysw_notify.o: ttysw_notify.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_notify.h \
	${INCLUDE}/sunwindow/defaults.h \
	${INCLUDE}/sunwindow/sun.h \
	${INCLUDE}/suntool/ttysw.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool_struct.h \
	ttysw_impl.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/window.h \
	charimage.h \
	charscreen.h

ttysw_stty.o: ttysw_stty.c

ttysw_tio.o: ttysw_tio.c \
	${INCLUDE}/sundev/kbd.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/menu.h \
	${INCLUDE}/suntool/ttysw.h \
	ttysw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h

ttysw_tsw.o: ttysw_tsw.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/ttysw.h

ttytlsw.o: ttytlsw.c \
	${INCLUDE}/sunwindow/window_hs.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/pw_util.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_environ.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/sunwindow/win_notify.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/suntool/icon_load.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/wmgr.h \
	${INCLUDE}/suntool/ttysw.h \
	ttysw_impl.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/window.h \
	ttytlsw_impl.h

ttytlsw_notify.o: ttytlsw_notify.c \
	${INCLUDE}/sunwindow/window_hs.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/pw_util.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_environ.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/sunwindow/win_notify.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/suntool/icon_load.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/wmgr.h \
	${INCLUDE}/suntool/ttysw.h \
	ttysw_impl.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/window.h \
	ttytlsw_impl.h

ttytlsw_tio.o: ttytlsw_tio.c \
	${INCLUDE}/sunwindow/window_hs.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/pw_util.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_environ.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/sunwindow/win_notify.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/suntool/icon_load.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/wmgr.h \
	${INCLUDE}/suntool/ttysw.h \
	ttysw_impl.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/window.h \
	ttytlsw_impl.h

ttyselect.o: ttyselect.c \
	${INCLUDE}/sundev/kbd.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/selection.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/ttysw.h \
	ttysw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/window.h \
	ttyansi.h \
	charimage.h \
	charscreen.h

ttysw_main.o: ttysw_main.c \
	${INCLUDE}/sundev/kbd.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/ttysw.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	ttysw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/window.h \
	ttytlsw_impl.h \
	${INCLUDE}/suntool/selection_svc.h

ttysw_menu.o: ttysw_menu.c \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/suntool/ttysw.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/suntool/menu.h \
	${INCLUDE}/suntool/selection.h \
	${INCLUDE}/suntool/selection_svc.h \
	ttysw_impl.h \
	${INCLUDE}/sunwindow/notify.h

ttysw_mapkey.o: ttysw_mapkey.c \
	${INCLUDE}/sundev/kbd.h \
	${INCLUDE}/suntool/tool_hs.h \
	${INCLUDE}/sunwindow/window_hs.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/pw_util.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_environ.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/sunwindow/win_notify.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/ttysw.h \
	ttysw_impl.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/window.h \
	ttytlsw_impl.h



# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# SEE HOW make depend WORKS FOR FURTHER INFO
