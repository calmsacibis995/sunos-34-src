#
# @(#)Makefile 1.3 87/01/07 SMI
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

ENTITYHDRS= 	entity_interpreter.h entity_stream.h entity_view.h ev_impl.h
MISCHDRS=	primal.h ps_impl.h finger_table.h
TEXTHDRS=	textsw_impl.h
HDRSPRIVATE=	$(ENTITYHDRS) $(MISCHDRS) $(TEXTHDRS)
HDRSPUBLIC=	textsw.h

ESCFILESLIB=	es_file.c es_mem.c es_util.c es_attr.c ps_impl.c
EVCFILESLIB=	ev_display.c ev_op_bdry.c ev_edit.c ev_once.c ev_attr.c
MISCCFILESLIB=	copy_file.c ei_plain_text.c ei_attr.c finger_table.c
TEXTCFILESLIB=	textsw_once.c textsw_input.c textsw_edit.c textsw_menu.c \
		textsw_display.c textsw_event.c textsw_find.c textsw_file.c \
		textsw_selection.c textsw_seln_svc.c textsw_again.c \
		textsw_get_key.c textsw_put_key.c textsw_filter.c \
		textsw_attr.c textsw_dbx.c textsw_tool.c textsw_scroll.c
CFILESLIB=	$(ESCFILESLIB) $(EVCFILESLIB) $(MISCCFILESLIB) $(TEXTCFILESLIB)

CFILESALL=$(CFILESLIB)
HDRSALL=$(HDRSPUBLIC) $(HDRSPRIVATE)
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile

OBJFILES=	textsw_once.o textsw_input.o textsw_edit.o textsw_menu.o \
		textsw_display.o textsw_event.o textsw_find.o textsw_file.o \
		textsw_selection.o textsw_seln_svc.o textsw_again.o \
		textsw_get_key.o textsw_put_key.o textsw_filter.o \
		textsw_attr.o textsw_dbx.o textsw_tool.o textsw_scroll.o \
		ps_impl.o \
		ev_edit.o ev_display.o ev_op_bdry.o finger_table.o ev_once.o \
		ev_attr.o ei_plain_text.o es_file.o es_mem.o es_util.o \
		es_attr.o ei_attr.o copy_file.o
GPROFFILESALL= ${OBJFILES:.o=.pg}
LIBFILE=	libtext.a
GPROFLIBFILE=	libtext_p.a
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
	
#	FIXED so don't have to do a CFLAGS_NOT_ROI_DATA: textsw_menu.c,
#	textsw_seln_svc.c, textsw_input.c.

# monad.c monad.h and monad_impl.h have all been moved to the attic.

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
es_file.o: es_file.c \
	entity_stream.h

es_mem.o: es_mem.c \
	entity_stream.h

es_util.o: es_util.c \
	primal.h \
	entity_stream.h

es_attr.o: es_attr.c \
	${INCLUDE}/sunwindow/attr.h \
	primal.h \
	entity_stream.h

ps_impl.o: ps_impl.c \
	primal.h \
	ps_impl.h \
	entity_stream.h \
	finger_table.h

ev_display.o: ev_display.c \
	primal.h \
	ev_impl.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	${INCLUDE}/sunwindow/attr.h \
	finger_table.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h

ev_op_bdry.o: ev_op_bdry.c \
	primal.h \
	ev_impl.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	${INCLUDE}/sunwindow/attr.h \
	finger_table.h

ev_edit.o: ev_edit.c \
	primal.h \
	ev_impl.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	${INCLUDE}/sunwindow/attr.h \
	finger_table.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h

ev_once.o: ev_once.c \
	primal.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool_struct.h \
	ev_impl.h \
	entity_view.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h

ev_attr.o: ev_attr.c \
	primal.h \
	ev_impl.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	${INCLUDE}/sunwindow/attr.h \
	finger_table.h

copy_file.o: copy_file.c

ei_plain_text.o: ei_plain_text.c \
	primal.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool_struct.h \
	entity_view.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h

ei_attr.o: ei_attr.c \
	primal.h \
	entity_interpreter.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/attr.h \
	entity_stream.h

finger_table.o: finger_table.c \
	primal.h \
	finger_table.h \
	entity_stream.h

textsw_once.o: textsw_once.c \
	primal.h \
	textsw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/sunwindow/attr.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_notify.h \
	${INCLUDE}/sunwindow/defaults.h \
	${INCLUDE}/sunwindow/sun.h

textsw_input.o: textsw_input.c \
	primal.h \
	textsw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/sunwindow/attr.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sundev/kbd.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/win_struct.h

textsw_edit.o: textsw_edit.c \
	primal.h \
	textsw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/sunwindow/attr.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h

textsw_menu.o: textsw_menu.c \
	primal.h \
	textsw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/sunwindow/attr.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/suntool/fullscreen.h \
	${INCLUDE}/suntool/menu.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/suntool/image.h

textsw_display.o: textsw_display.c \
	primal.h \
	textsw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/sunwindow/attr.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_notify.h

textsw_event.o: textsw_event.c \
	primal.h \
	textsw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/sunwindow/attr.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h

textsw_find.o: textsw_find.c \
	primal.h \
	textsw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/sunwindow/attr.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h

textsw_file.o: textsw_file.c \
	primal.h \
	textsw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/sunwindow/attr.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/suntool/expand_name.h

textsw_selection.o: textsw_selection.c \
	primal.h \
	textsw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/sunwindow/attr.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h

textsw_seln_svc.o: textsw_seln_svc.c \
	primal.h \
	textsw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/sunwindow/attr.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h

textsw_again.o: textsw_again.c \
	primal.h \
	textsw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/sunwindow/attr.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h

textsw_get_key.o: textsw_get_key.c \
	primal.h \
	textsw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/sunwindow/attr.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h

textsw_put_key.o: textsw_put_key.c \
	primal.h \
	textsw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/sunwindow/attr.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h

textsw_filter.o: textsw_filter.c \
	primal.h \
	textsw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/sunwindow/attr.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/filter.h \
	${INCLUDE}/sunwindow/io_stream.h \
	${INCLUDE}/sunwindow/sun.h

textsw_attr.o: textsw_attr.c \
	primal.h \
	textsw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/sunwindow/attr.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/defaults.h \
	${INCLUDE}/sunwindow/sun.h

textsw_dbx.o: textsw_dbx.c \
	primal.h \
	textsw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/sunwindow/attr.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h

textsw_tool.o: textsw_tool.c \
	primal.h \
	textsw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/sunwindow/attr.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_struct.h

textsw_scroll.o: textsw_scroll.c \
	primal.h \
	textsw_impl.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/sunwindow/attr.h \
	entity_view.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	entity_stream.h \
	entity_interpreter.h \
	finger_table.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h



# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# SEE HOW make depend WORKS FOR FURTHER INFO
