#
# @(#)Makefile 1.3 87/01/07 SMI
#

HEADERFILES= iconedit.h

CFILES= iconedit_canvas.c iconedit_main.c iconedit_mpr.c iconedit_panel.c \
	iconedit_browse.c

OBJFILES= iconedit_canvas.o iconedit_main.o iconedit_mpr.o iconedit_panel.o \
	  iconedit_browse.o

GPROFFILESALL= ${CFILES:.c=.pg}

NO_MAIN_OBJS= iconedit_canvas.o iconedit_browse.o iconedit_mpr.o \
		iconedit_panel.o
CFILESALL=	$(CFILES)
HDRSALL=	$(HEADERFILES)
SRCSALL=	$(CFILESALL) $(HDRSALL) Makefile
GPROF_MERGE_LIB= libtoolmerge_p.a

INCLUDE=/usr/include
AWK=	/bin/awk
CTAGS=	/usr/ucb/ctags -w
DESTDIR=
CPUFLAG=

DBX=
CFLAGS=	-O -I$(INCLUDE)
CFLAGS_LOCAL=

CFLAGS_NOT_ROI_DATA= $(CFLAGS) $(DBX) $(CFLAGS_LOCAL) $(CPUFLAG)

LDFLAGS= ${DBX}

LIBS=	-lsuntool -lsunwindow -lpixrect

LIBFILE= libiconedit.a

BASETOOLS_BIN= suntools
OTHERTOOLS_BIN= othertools
.SUFFIXES: .pg

#Note: not yet converted to -A-R
#.c.o:
#	cc $(CFLAGS) $(DBX) $(CFLAGS_LOCAL) -A-R -c $<
.c.o:
	cc ${CFLAGS_NOT_ROI_DATA} -c $<

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

objsall: ${OBJFILES}

gprof::  ${GPROFFILESALL}
	ar uvl ../${GPROF_MERGE_LIB} ${GPROFFILESALL}

standalone: ${NO_MAIN_OBJS} iconedit_main.c ${LIBS} 
	cc ${CFLAGS_NOT_ROI_DATA} -o iconedit -DSTANDALONE iconedit_main.c \
	${NO_MAIN_OBJS} ${LIBS} -lm

clean:
	rm -f *.o core iconedit *.pg

install:         
	-for i in iconedit; do \
		rm -f ${DESTDIR}/usr/bin/iconedit; \
		ln -s ${OTHERTOOLS_BIN} ${DESTDIR}/usr/bin/iconedit; \
	done 

install_sources:
	-for i in ${SRCSALL}; do \
		install -m 644 $$i ${DESTDIR}/usr/src/sun/suntool/iconedit; \
	done

tags:	$(CFILESALL)
	${CTAGS} $(CFILESALL);

print:	$(SRCSALL)
	lpr -p -Pimagen $(SRCSALL)

lint:
	lint $(CFLAGS) $(CFILESALL) /usr/lib/lint/llib_lpixrect.ln \
		/usr/lib/lint/llib_lsunwindow.ln \
		/usr/lib/lint/llib_lsuntool.ln

FRC:
# non-existent so always younger

depend:
	make_depend -I${INCLUDE} ${CFILESALL}

#include Make.depend


# DO NOT DELETE THIS LINE -- make depend uses it
iconedit_canvas.o: iconedit_canvas.c \
	iconedit.h \
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
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/suntool/frame.h \
	${INCLUDE}/suntool/panel.h \
	${INCLUDE}/suntool/canvas.h

iconedit_main.o: iconedit_main.c \
	iconedit.h \
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
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/suntool/frame.h \
	${INCLUDE}/suntool/panel.h \
	${INCLUDE}/suntool/canvas.h \
	${INCLUDE}/images/iconedit.icon

iconedit_mpr.o: iconedit_mpr.c \
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
	${INCLUDE}/images/hglass.cursor \
	${INCLUDE}/images/confirm.pr \
	${INCLUDE}/images/confirm_left.pr \
	${INCLUDE}/images/confirm_middle.pr \
	${INCLUDE}/images/confirm_right.pr \
	${INCLUDE}/images/tri_right.pr \
	${INCLUDE}/images/tri_left.pr \
	${INCLUDE}/images/tri_down.pr \
	${INCLUDE}/images/tri_up.pr \
	${INCLUDE}/images/grid.pr \
	${INCLUDE}/images/painting_hand.pr \
	${INCLUDE}/images/point.pr \
	${INCLUDE}/images/line.pr \
	${INCLUDE}/images/box.pr \
	${INCLUDE}/images/circle.pr \
	${INCLUDE}/images/abc.pr \
	${INCLUDE}/images/blank.pr \
	${INCLUDE}/images/screenr7.pr \
	${INCLUDE}/images/screenr11.pr \
	${INCLUDE}/images/screenr12.pr \
	${INCLUDE}/images/screenb12.pr \
	${INCLUDE}/images/screenr14.pr \
	${INCLUDE}/images/screenb14.pr \
	${INCLUDE}/images/cmrr14.pr \
	${INCLUDE}/images/cmrb14.pr \
	${INCLUDE}/images/gallantr19.pr \
	${INCLUDE}/images/circle_black.pr \
	${INCLUDE}/images/circle_75.pr \
	${INCLUDE}/images/circle_50.pr \
	${INCLUDE}/images/circle_root.pr \
	${INCLUDE}/images/circle_25.pr \
	${INCLUDE}/images/circle_0.pr \
	${INCLUDE}/images/square_75.pr \
	${INCLUDE}/images/square_50.pr \
	${INCLUDE}/images/square_root.pr \
	${INCLUDE}/images/square_25.pr \
	${INCLUDE}/images/square_0.pr \
	${INCLUDE}/images/none.cursor \
	${INCLUDE}/images/white.pr \
	${INCLUDE}/images/gr_25.cursor \
	${INCLUDE}/images/gr_root.cursor \
	${INCLUDE}/images/gr_50.cursor \
	${INCLUDE}/images/gr_75.cursor \
	${INCLUDE}/images/black.cursor

iconedit_panel.o: iconedit_panel.c \
	iconedit.h \
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
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/suntool/frame.h \
	${INCLUDE}/suntool/panel.h \
	${INCLUDE}/suntool/canvas.h \
	${INCLUDE}/suntool/expand_name.h \
	${INCLUDE}/suntool/icon_load.h \
	${INCLUDE}/suntool/fullscreen.h



# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# SEE HOW make depend WORKS FOR FURTHER INFO
