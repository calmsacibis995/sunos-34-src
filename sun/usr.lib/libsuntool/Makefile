#
# @(#)Makefile 1.4 87/01/07 SMI
#

OBJSLIB=tool_select.o tool_display.o tool_draw_box.o tool_kbd.o\
	msgsw.o emptysw.o \
	tool_input.o wmgr_walkmenu.o wmgr_menu.o wmgr_state.o \
	fullscreen.o menu.o prompt.o \
	selection.o wmgr_rect.o wmgr_cursors.o \
	wmgr_policy.o wmgr_findspace.o wmgr_attic.o\
	tool_layout.o tool_pos.o tool_bdry.o \
	gfxsw.o gfxsw_nocur.o gfxsw_input.o \
	tool_create.o tool_destroy.o \
	tek.o teksw.o teksw_ui.o demorandom.o \
	cheap_text.o cursor_confirm.o icon.o \
	tla_find.o tla_get.o tla_parse.o tla_storage.o \
	tla_freelist.o tla_make.o tla_set.o tla_usage.o \
	tool_install.o tool_begin.o \
	icon_object.o icon_load.o expand_name.o expand_path.o \
	scrollbar_event.o scrollbar_public.o scrollbar_paint.o \
	sel_appl.o sel_clnt.o sel_common.o sel_debug.o \
	sel_svc.o sel_policy.o sel_writable_data.o sel_xdr.o
CFILESLIB=msgsw.c emptysw.c \
	selection.c tool_select.c \
	gfxsw.c gfxsw_nocur.c gfxsw_input.c demorandom.c \
	cheap_text.c tool_layout.c tool_pos.c \
	fullscreen.c menu.c prompt.c \
	tool_display.c tool_draw_box.c tool_kbd.c tool_input.c \
	wmgr_cursors.c wmgr_walkmenu.c wmgr_menu.c \
	wmgr_rect.c wmgr_state.c tool_bdry.c \
	wmgr_policy.c wmgr_findspace.c wmgr_attic.c\
	tool_create.c tool_destroy.c \
	tek.c teksw.c teksw_ui.c icon.c \
	tla_find.c tla_get.c tla_parse.c tla_storage.c \
	tla_freelist.c tla_make.c tla_set.c tla_usage.c \
	tool_install.c tool_begin.c \
	icon_object.c cursor_confirm.c icon_load.c \
	expand_name.c expand_path.c \
	scrollbar_event.c scrollbar_public.c scrollbar_paint.c \
	sel_appl.c sel_clnt.c sel_common.c sel_debug.c \
	sel_svc.c sel_policy.c sel_writable_data.c sel_xdr.c
GPROFFILESALL=tool_select.pg tool_display.pg tool_draw_box.pg tool_kbd.pg\
	msgsw.pg emptysw.pg \
	tool_input.pg wmgr_walkmenu.pg wmgr_menu.pg wmgr_state.pg \
	fullscreen.pg menu.pg prompt.pg \
	selection.pg wmgr_rect.pg wmgr_cursors.pg \
	wmgr_policy.pg wmgr_findspace.pg wmgr_attic.pg\
	tool_layout.pg tool_pos.pg tool_bdry.pg \
	gfxsw.pg gfxsw_nocur.pg gfxsw_input.pg \
	tool_create.pg tool_destroy.pg \
	tek.pg teksw.pg teksw_ui.pg demorandom.pg \
	cheap_text.pg cursor_confirm.pg icon.pg \
	tla_find.pg tla_get.pg tla_parse.pg tla_storage.pg \
	tla_freelist.pg tla_make.pg tla_set.pg tla_usage.pg \
	tool_install.pg tool_begin.pg \
	icon_object.pg icon_load.pg expand_name.pg expand_path.pg \
	scrollbar_event.pg scrollbar_public.pg scrollbar_paint.pg \
	sel_appl.pg sel_clnt.pg sel_common.pg sel_debug.pg \
	sel_svc.pg sel_policy.pg sel_writable_data.pg sel_xdr.pg
HDRSPUBLIC=emptysw.h expand_name.h fullscreen.h gfx_hs.h gfxsw.h \
	icon.h icon_load.h menu.h msgsw.h scrollbar.h \
	selection.h selection_attributes.h selection_svc.h seln.h\
	sunview.h tek.h teksw.h tool.h tool_hs.h tool_struct.h wmgr.h
HDRSPRIVATE= cheap_text.h gfxswimpl.h scrollbar_impl.h \
	selection_impl.h teksw_imp.h text_obj.h tool_impl.h wmgr_policy.h

CFILESALL=$(CFILESLIB)
HDRSALL=$(HDRSPUBLIC) $(HDRSPRIVATE)
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile
GPROFLIBFILE= libsuntool_p.a

SUBDIRS=  text ttysw menu panel window
# Note that the order of the subdirs *IS* significant:
#  text must precede ttysw; window must be last

INCLUDE=/usr/include

AWK=	/bin/awk
CTAGS=	/usr/ucb/ctags -w
TOUCH=	touch -c
DESTDIR=
CPUFLAG=
PROFILE=

DBX=
CFLAGS=	-O -I$(INCLUDE)
CFLAGS_LOCAL=

CFLAGS_NOT_ROI_DATA= $(CFLAGS) $(DBX) $(CFLAGS_LOCAL) $(CPUFLAG)
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

# Warning: always do "make" or "make all"
# do NOT do "make libsuntool.a"
all: libsuntool.tmp.a
	rm -f libsuntool.a
	cp libsuntool.tmp.a libsuntool.a
	ranlib libsuntool.a

gprof:: ${GPROFFILESALL}
	ar uvl ${GPROFLIBFILE} ${GPROFFILESALL}
	ranlib ${GPROFLIBFILE}

gprof:: FRC
	-for i in ${SUBDIRS}; do \
	(	echo "Descending into directory: $$i"; \
		cd $$i; make ${MFLAGS} "INCLUDE=${INCLUDE}"\
		"CFLAGS=${CFLAGS}" "DBX=${DBX}" "CPUFLAG=${CPUFLAG}" gprof\
	); done
	-mkdir tmp
	-rm -rf tmp/*
	for i in ${SUBDIRS}; do \
		(cd tmp; ar x ../$$i/lib$${i}_p.a; rm -f __.SYMDEF); done;
	ar uvl ${GPROFLIBFILE} tmp/*
	-rm -rf tmp
	ranlib ${GPROFLIBFILE}

libsuntool.tmp.a::	$(OBJSLIB)
	ar uv libsuntool.tmp.a $?

libsuntool.tmp.a::	FRC
	-for i in ${SUBDIRS}; do \
	(	echo "Descending into directory: $$i"; \
		cd $$i; make ${MFLAGS} "INCLUDE=${INCLUDE}"\
		"CFLAGS=${CFLAGS}" "DBX=${DBX}" "CPUFLAG=${CPUFLAG}" \
	); done
	-mkdir tmp
	-rm -rf tmp/*
	for i in ${SUBDIRS}; do \
		(cd tmp; ar x ../$$i/lib$${i}.a; rm -f __.SYMDEF); done;
	ar uv libsuntool.tmp.a tmp/*
	-rm -rf tmp
# Remove each __.SYMDEF from tmp so that the ranlib of the parent .a will work

#	FIXED so don't have to do a CFLAGS_NOT_ROI_DATA: sel_clnt.c,

#	Might want to change cheap_text_ops in cheap_text.c (IS THIS TRUE?)
cheap_text.o:
	cc ${CFLAGS_NOT_ROI_DATA} -c cheap_text.c

#	Change I and J in demorandom.c
demorandom.o:
	cc ${CFLAGS_NOT_ROI_DATA} -c demorandom.c

#	This isolates the writable initialized data in the selection library
sel_writable_data.o:
	cc ${CFLAGS_NOT_ROI_DATA} -c sel_writable_data.c

#	Menu stuff is dynamically set in teksw_ui.c
teksw_ui.o:
	cc ${CFLAGS_NOT_ROI_DATA} -c teksw_ui.c

#	Menu stuff is dynamically set in wmgr_menu.c
wmgr_menu.o:
	cc ${CFLAGS_NOT_ROI_DATA} -c wmgr_menu.c

#	There is a static cursor into which is saved the original cursor
#	in fullscreen.c
fullscreen.o:
	cc ${CFLAGS_NOT_ROI_DATA} -c fullscreen.c

#	Uses initialized cursor struct to store cursor in scrollbar_event.c
#	Could rewrite.
scrollbar_event.o:
	cc ${CFLAGS_NOT_ROI_DATA} -c scrollbar_event.c


#	Note: Rewrote flushline in tool_display.c to not write into string.

install: install_lint
	install -c -m 644 libsuntool.a ${DESTDIR}/usr/lib
	ranlib ${DESTDIR}/usr/lib/libsuntool.a
	install -c -m 644 ${GPROFLIBFILE} ${DESTDIR}/usr/lib
	ranlib ${DESTDIR}/usr/lib/${GPROFLIBFILE}
	-mkdir ${DESTDIR}/usr/include/suntool && \
		chown bin ${DESTDIR}/usr/include/suntool && \
		chmod 755 ${DESTDIR}/usr/include/suntool
	make DESTDIR=${DESTDIR} install_h

install_lint: llib-lsuntool.ln
	install -c -m 644 llib-lsuntool ${DESTDIR}/usr/lib/lint
	install -c -m 644 llib-lsuntool.ln ${DESTDIR}/usr/lib/lint

llib-lsuntool.ln: llib-lsuntool
	lint -I${INCLUDE} -Csuntool llib-lsuntool

llib-lsuntool:
	@if [ ! -f llib-lsuntool ]; then \
		sccs get llib-lsuntool; \
	fi

install_h:
	-for i in ${SUBDIRS}; do (cd $$i; \
		make ${MFLAGS} DESTDIR=${DESTDIR} install_h); done
	-for i in ${HDRSPUBLIC}; do \
		cmp -s $$i ${DESTDIR}/usr/include/suntool/$$i || \
		install -c -m 644 $$i ${DESTDIR}/usr/include/suntool/$$i; \
	done

clean:
	-for i in ${SUBDIRS}; do (cd $$i; make ${MFLAGS} clean); done
	rm -f *.o libsuntool.tmp.a core .pg

tags:	$(CFILESALL)
	${CTAGS} $(CFILESALL);

print:	$(SRCSALL)
	pr -f $(SRCSALL) | rsh krypton lpr -Pimagen -r

lint:
	lint $(CFLAGS) $(CFILESALL) /usr/src/sun/usr.lib/libpixrect/*.c \
		/usr/src/sun/usr.lib/libsunwindow/*.c

FRC:
# non-existent so always younger

depend:
	make_depend -I${INCLUDE} ${CFILESALL}

#include Make.depend

# DO NOT DELETE THIS LINE -- make depend uses it
msgsw.o: msgsw.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/suntool/msgsw.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool_struct.h

emptysw.o: emptysw.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/emptysw.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h

selection.o: selection.c \
	${INCLUDE}/suntool/selection.h

tool_select.o: tool_select.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	tool_impl.h \
	${INCLUDE}/suntool/wmgr.h

gfxsw.o: gfxsw.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_environ.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/gfxsw.h \
	gfxswimpl.h

gfxsw_nocur.o: gfxsw_nocur.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_environ.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/gfxsw.h \
	gfxswimpl.h

gfxsw_input.o: gfxsw_input.c \
	${INCLUDE}/sundev/kbd.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_environ.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/gfxsw.h \
	gfxswimpl.h

demorandom.o: demorandom.c

cheap_text.o: cheap_text.c \
	cheap_text.h \
	text_obj.h

tool_layout.o: tool_layout.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/wmgr.h

tool_pos.o: tool_pos.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/wmgr.h

fullscreen.o: fullscreen.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/suntool/fullscreen.h

menu.o: menu.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/cms_mono.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/suntool/menu.h \
	${INCLUDE}/suntool/fullscreen.h

prompt.o: prompt.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/suntool/menu.h \
	${INCLUDE}/suntool/fullscreen.h

tool_display.o: tool_display.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/cms_mono.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	tool_impl.h \
	${INCLUDE}/suntool/wmgr.h

tool_draw_box.o: tool_draw_box.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h

tool_kbd.o: tool_kbd.c \
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
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/sunwindow/cms_mono.h \
	${INCLUDE}/suntool/wmgr.h \
	tool_impl.h

tool_input.o: tool_input.c \
	${INCLUDE}/sundev/kbd.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/cms_mono.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	tool_impl.h \
	${INCLUDE}/suntool/wmgr.h \
	${INCLUDE}/suntool/menu.h

wmgr_cursors.o: wmgr_cursors.c \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/images/confirm.cursor \
	${INCLUDE}/images/move.cursor \
	${INCLUDE}/images/move_h.cursor \
	${INCLUDE}/images/move_v.cursor \
	${INCLUDE}/images/stretch_v.cursor \
	${INCLUDE}/images/stretch_h.cursor \
	${INCLUDE}/images/stretchNW.cursor \
	${INCLUDE}/images/stretchNE.cursor \
	${INCLUDE}/images/stretchSE.cursor \
	${INCLUDE}/images/stretchSW.cursor

wmgr_walkmenu.o: wmgr_walkmenu.c \
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
	tool_impl.h \
	${INCLUDE}/suntool/menu.h

wmgr_menu.o: wmgr_menu.c \
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
	${INCLUDE}/suntool/menu.h

wmgr_rect.o: wmgr_rect.c \
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
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/suntool/menu.h \
	${INCLUDE}/suntool/wmgr.h \
	${INCLUDE}/suntool/fullscreen.h

wmgr_state.o: wmgr_state.c \
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
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/suntool/menu.h \
	${INCLUDE}/suntool/fullscreen.h \
	${INCLUDE}/suntool/wmgr.h

tool_bdry.o: tool_bdry.c \
	${INCLUDE}/sundev/kbd.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/cms_mono.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/menu.h

wmgr_policy.o: wmgr_policy.c \
	${INCLUDE}/sunwindow/defaults.h \
	${INCLUDE}/sunwindow/sun.h \
	wmgr_policy.h \
	${INCLUDE}/sunwindow/win_enum.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/suntool/tool_hs.h \
	${INCLUDE}/sunwindow/window_hs.h \
	${INCLUDE}/sunwindow/notify.h \
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
	${INCLUDE}/suntool/wmgr.h

wmgr_findspace.o: wmgr_findspace.c \
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
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/sunwindow/win_enum.h \
	wmgr_policy.h \
	${INCLUDE}/suntool/tool_hs.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/wmgr.h

wmgr_attic.o: wmgr_attic.c \
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
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/suntool/menu.h \
	${INCLUDE}/suntool/wmgr.h \
	${INCLUDE}/suntool/fullscreen.h

tool_create.o: tool_create.c \
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
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/suntool/wmgr.h \
	tool_impl.h

tool_destroy.o: tool_destroy.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool_struct.h \
	tool_impl.h

tek.o: tek.c \
	tek.h

teksw.o: teksw.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/menu.h \
	teksw.h \
	tek.h \
	teksw_imp.h

teksw_ui.o: teksw_ui.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/menu.h \
	tek.h \
	teksw_imp.h \
	${INCLUDE}/images/hglass.cursor

icon.o: icon.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/sunwindow/attr.h

tla_find.o: tla_find.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	tool_impl.h

tla_get.o: tla_get.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	tool_impl.h

tla_parse.o: tla_parse.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/icon_load.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	tool_impl.h

tla_storage.o: tla_storage.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h

tla_freelist.o: tla_freelist.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	tool_impl.h

tla_make.o: tla_make.c \
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
	${INCLUDE}/sunwindow/win_ioctl.h \
	tool_impl.h \
	${INCLUDE}/suntool/wmgr.h \
	${INCLUDE}/images/bullseye.cursor

tla_set.o: tla_set.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	tool_impl.h

tla_usage.o: tla_usage.c

tool_install.o: tool_install.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool_struct.h \
	tool_impl.h

tool_begin.o: tool_begin.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/sunwindow/win_notify.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	tool_impl.h \
	${INCLUDE}/suntool/wmgr.h

icon_object.o: icon_object.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/sunwindow/attr.h

cursor_confirm.o: cursor_confirm.c \
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
	${INCLUDE}/suntool/fullscreen.h

icon_load.o: icon_load.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/icon_load.h

expand_name.o: expand_name.c \
	${INCLUDE}/suntool/expand_name.h

expand_path.o: expand_path.c

scrollbar_event.o: scrollbar_event.c \
	scrollbar_impl.h \
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
	${INCLUDE}/suntool/scrollbar.h

scrollbar_public.o: scrollbar_public.c \
	${INCLUDE}/sunwindow/defaults.h \
	${INCLUDE}/sunwindow/sun.h \
	scrollbar_impl.h \
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
	${INCLUDE}/suntool/scrollbar.h

scrollbar_paint.o: scrollbar_paint.c \
	scrollbar_impl.h \
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
	${INCLUDE}/suntool/scrollbar.h

sel_appl.o: sel_appl.c \
	selection_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/sunwindow/attr.h

sel_clnt.o: sel_clnt.c \
	selection_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/sunwindow/attr.h

sel_common.o: sel_common.c \
	selection_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/sunwindow/attr.h

sel_debug.o: sel_debug.c \
	selection_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/sunwindow/attr.h

sel_svc.o: sel_svc.c \
	selection_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/sunwindow/attr.h

sel_policy.o: sel_policy.c \
	selection_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h

sel_writable_data.o: sel_writable_data.c \
	selection_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/sunwindow/attr.h

sel_xdr.o: sel_xdr.c \
	selection_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/sunwindow/attr.h



# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# SEE HOW make depend WORKS FOR FURTHER INFO
