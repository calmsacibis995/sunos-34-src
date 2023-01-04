#
#	@(#)Makefile	1.5	87/01/13 SMI
#
DESTDIR=
CPUFLAG=
PRODLIBDIR=/usr/lib
PRODINCLUDEDIR=/usr/include/suntool

INCLUDE=/usr/include
INSTALL=install -m 644
AWK=	/bin/awk

DBX=
CFLAGS=	-O -I$(INCLUDE)
CFLAGS_LOCAL=

CFLAGS_NOT_ROI_DATA= $(CFLAGS) $(DBX) $(CFLAGS_LOCAL) $(CPUFLAG)

HDRSPUBLIC=	window.h frame.h tty.h canvas.h text.h
HDRSPRIVATE=	window_impl.h frame_impl.h canvas_impl.h panel_impl.h

WCFILES=	window.c window_get.c window_set.c window_loop.c
FCFILES=	frame.c frame_get.c frame_set.c frame_layout.c
CFILES=         ${WCFILES} ${FCFILES} panel.c text.c tty.c canvas.c \
		canvas_scroll.c canvas_repaint.c canvas_resize.c
GPROFFILESALL= window.pg window_get.pg window_set.pg window_loop.pg\
		frame.pg frame_get.pg frame_set.pg frame_layout.pg \
		panel.pg text.pg tty.pg canvas.pg canvas_scroll.pg \
		canvas_repaint.pg canvas_resize.pg

CFILESALL=$(CFILES)
HDRSALL=$(HDRSPUBLIC) $(HDRSPRIVATE)
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile

WOBJFILES=	window.o window_get.o window_set.o window_loop.o
FOBJFILES=	frame.o frame_get.o frame_set.o frame_layout.o
OBJFILES=       ${WOBJFILES} ${FOBJFILES} panel.o text.o tty.o canvas.o \
		canvas_scroll.o canvas_repaint.o canvas_resize.o

LIBFILE=	libwindow.a
GPROFLIBFILE=	libwindow_p.a
.SUFFIXES: .pg

LIBDIR = ${DESTDIR}/usr/src/sun/usr.lib
LIBS   = ${LIBDIR}/libsuntool.a ${LIBDIR}/libsunwindow.a -lpixrect

.c.o:
	cc $(CFLAGS_NOT_ROI_DATA) -A-R -c $<

.c.pg:
	if [ -f $*.o ]; then \
		mv $*.o $*.o.tmp; \
	fi
	${CC} ${CFLAGS_NOT_ROI_DATA} -I../text -I../ttysw -pg -c $<
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
text.o: text.h text.c
	${CC} ${CFLAGS_NOT_ROI_DATA} -I../text -c text.c

tty.o: tty.c
	${CC} ${CFLAGS_NOT_ROI_DATA} -I../ttysw -c tty.c

#	FIXED so don't have to do a CFLAGS_NOT_ROI_DATA: window.c.

demo.o ${OBJS}: window.h frame.h

demo: demo.o ${LIBFILE}
	${CC} ${CFLAGS} ${DBX} ${LDFLAGS} ${CPUFLAG} -o demo demo.o ${LIBFILE} ${LIBS}

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
window.o: window.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/suntool/frame.h \
	${INCLUDE}/sunwindow/attr.h \
	window_impl.h \
	${INCLUDE}/suntool/window.h

window_get.o: window_get.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_screen.h \
	window_impl.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/attr.h

window_set.o: window_set.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_struct.h \
	window_impl.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/attr.h

window_loop.o: window_loop.c \
	${INCLUDE}/suntool/sunview.h \
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
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/suntool/frame.h \
	${INCLUDE}/suntool/fullscreen.h

frame.o: frame.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/wmgr.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/window.h \
	tool_impl.h \
	frame_impl.h \
	${INCLUDE}/suntool/frame.h

frame_get.o: frame_get.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/attr.h \
	tool_impl.h \
	frame_impl.h \
	${INCLUDE}/suntool/frame.h

frame_set.o: frame_set.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/wmgr.h \
	${INCLUDE}/suntool/walkmenu.h \
	tool_impl.h \
	frame_impl.h \
	${INCLUDE}/suntool/frame.h

frame_layout.o: frame_layout.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/wmgr.h \
	tool_impl.h \
	frame_impl.h \
	${INCLUDE}/suntool/frame.h

panel.o: panel.c \
	panel_impl.h \
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
	${INCLUDE}/suntool/panel.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/suntool/frame.h

text.o: text.c \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/text.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/suntool/tool_struct.h

tty.o: tty.c \
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
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/suntool/frame.h \
	${INCLUDE}/suntool/tty.h \
	${INCLUDE}/suntool/ttysw.h

canvas.o: canvas.c \
	canvas_impl.h \
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
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/suntool/canvas.h

canvas_scroll.o: canvas_scroll.c \
	canvas_impl.h \
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
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/suntool/canvas.h

canvas_repaint.o: canvas_repaint.c \
	canvas_impl.h \
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
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/suntool/canvas.h

canvas_resize.o: canvas_resize.c \
	canvas_impl.h \
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
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/suntool/canvas.h



# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# SEE HOW make depend WORKS FOR FURTHER INFO
