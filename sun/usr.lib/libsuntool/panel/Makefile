#
# @(#)Makefile 1.3 87/01/14 SMI
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

HDRSPUBLIC=	panel.h
HDRSPRIVATE=	panel_impl.h
CFILES= 	panel_attr.c panel_line.c panel_notify.c \
		panel_select.c panel_text.c \
		panel_button.c panel_menu.c panel_public.c \
		panel_seln.c panel_tio.c \
		panel_choice.c panel_message.c panel_scroll.c \
		panel_slider.c panel_util.c \
		panel_create.c panel_paint.c panel_image.c panel_sw.c
GPROFFILESALL=${CFILES:.c=.pg}
CFILESALL=$(CFILES)
HDRSALL=$(HDRSPUBLIC) $(HDRSPRIVATE)
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile

OBJFILES=	panel_attr.o   panel_line.o  panel_notify.o \
		panel_select.o panel_text.o \
		panel_button.o panel_menu.o  panel_public.o \
		panel_seln.o   panel_tio.o \
		panel_choice.o panel_message.o panel_scroll.o \
		panel_slider.o panel_util.o \
		panel_create.o panel_paint.o panel_image.o panel_sw.o
LIBFILE=	libpanel.a
GPROFLIBFILE=	libpanel_p.a
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

#	FIXED so don't have to do a CFLAGS_NOT_ROI_DATA: panel_seln.c.

#	Dynamic functions initialized to panel_nullproc panel_util.c
panel_util.o:
	cc ${CFLAGS_NOT_ROI_DATA} -c panel_util.c

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
panel_attr.o: panel_attr.c \
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
	${INCLUDE}/suntool/selection_svc.h

panel_line.o: panel_line.c \
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
	${INCLUDE}/suntool/selection_svc.h

panel_notify.o: panel_notify.c \
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
	${INCLUDE}/suntool/window.h

panel_select.o: panel_select.c \
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
	${INCLUDE}/suntool/window.h

panel_text.o: panel_text.c \
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
	${INCLUDE}/suntool/menu.h \
	${INCLUDE}/sunwindow/defaults.h \
	${INCLUDE}/sunwindow/sun.h \
	${INCLUDE}/sundev/kbd.h \
	${INCLUDE}/images/panel_left_arrow.pr

panel_button.o: panel_button.c \
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
	${INCLUDE}/suntool/selection_svc.h

panel_menu.o: panel_menu.c \
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
	${INCLUDE}/suntool/menu.h \
	${INCLUDE}/images/panel_button.pr \
	${INCLUDE}/images/panel_choose_one.pr \
	${INCLUDE}/images/panel_choose_many.pr \
	${INCLUDE}/images/panel_switch.pr \
	${INCLUDE}/images/panel_text.pr

panel_public.o: panel_public.c \
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
	${INCLUDE}/suntool/window.h

panel_seln.o: panel_seln.c \
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
	${INCLUDE}/suntool/selection_attributes.h

panel_tio.o: panel_tio.c \
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
	${INCLUDE}/suntool/selection_svc.h

panel_choice.o: panel_choice.c \
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
	${INCLUDE}/sundev/kbd.h \
	${INCLUDE}/suntool/menu.h \
	${INCLUDE}/images/panel_check_off.pr \
	${INCLUDE}/images/panel_check_on.pr

panel_message.o: panel_message.c \
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
	${INCLUDE}/suntool/selection_svc.h

panel_scroll.o: panel_scroll.c \
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
	${INCLUDE}/suntool/selection_svc.h

panel_slider.o: panel_slider.c \
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
	${INCLUDE}/suntool/selection_svc.h

panel_util.o: panel_util.c \
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
	${INCLUDE}/suntool/menu.h

panel_create.o: panel_create.c \
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
	${INCLUDE}/images/panel_menu_on.pr \
	${INCLUDE}/images/cycle.pr

panel_paint.o: panel_paint.c \
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
	${INCLUDE}/suntool/selection_svc.h

panel_image.o: panel_image.c \
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
	${INCLUDE}/suntool/selection_svc.h

panel_sw.o: panel_sw.c \
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
	${INCLUDE}/suntool/selection_svc.h



# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# SEE HOW make depend WORKS FOR FURTHER INFO
