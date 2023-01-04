#
# @(#)Makefile 1.4 87/01/27 SMI
#
CPUFLAG= 
DESTDIR=
LIBS=   -lsuntool -lsunwindow -lpixrect
        
INCLUDE=/usr/include
PRODLIBDIR=/usr/lib
PRODINCLUDEDIR=/usr/include/suntool

DBX=
CFLAGS= -O -I$(INCLUDE)
CFLAGS_LOCAL=
CFLAGS_NOT_ROI_DATA= $(CFLAGS) $(DBX) $(CFLAGS_LOCAL) $(CPUFLAG)


LDFLAGS= ${DBX}

HDRS=		glob.h tool.h
CFILES=  main.c tool.c selection.c cmds.c mail.c subr.c vars.c
OBJFILES=main.o tool.o selection.o cmds.o mail.o subr.o vars.o
GPROFFILESALL= ${CFILES:.c=.pg}
NO_MAIN_OBJS= tool.o selection.o cmds.o mail.o subr.o vars.o

CFILESALL=$(CFILES)
HDRSALL=$(HDRS)
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile
LIBFILE= libmailtool.a
BASETOOLS_BIN= suntools
OTHERTOOLS_BIN= othertools
GPROF_MERGE_LIB= libtoolmerge_p.a
 
AWK=	/bin/awk
LINT=   /usr/bin/lint
.SUFFIXES: .pg

.c.o:	
	cc ${CFLAGS_NOT_ROI_DATA} -A-R -c $<

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

gprof:: ${GPROFFILESALL}
	ar uvl ../${GPROF_MERGE_LIB} ${GPROFFILESALL}

standalone:       main.c ${NO_MAIN_OBJS} ${LIBS}
	${CC} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE -o mailtool main.c ${NO_MAIN_OBJS} ${LIBS}

mail.o:
	cc ${CFLAGS_NOT_ROI_DATA} -c mail.c

mail.pg:
	if [ -f mail.o ]; then \
		mv mail.o mail.o.tmp; \
	fi
	cc ${CFLAGS_NOT_ROI_DATA} -pg -c mail.c
	-mv mail.o mail.pg
	if [ -f mail.o.tmp ]; then \
		mv mail.o.tmp mail.o; \
	fi 

clean:	
	-rm -f core errs *.o *% *.BAK mailtool *.pg

install:	

install_h:	
	
lint:
	${LINT} -hb ${CFILES} ${LIBS}
        
tags:   ${CFILES}
	ctags -w ${CFILES}

FRC:
# non-existent so always younger

depend:
	make_depend -I${INCLUDE} ${CFILESALL}

#include Make.depend

# DO NOT DELETE THIS LINE -- make depend uses it
main.o: main.c \
	glob.h

tool.o: tool.c \
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
	${INCLUDE}/suntool/frame.h \
	${INCLUDE}/suntool/panel.h \
	${INCLUDE}/suntool/text.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/suntool/selection.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/suntool/menu.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/suntool/icon.h \
	glob.h \
	tool.h \
	${INCLUDE}/images/mail.icon \
	${INCLUDE}/images/nomail.icon \
	${INCLUDE}/images/dead.icon

selection.o: selection.c \
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
	${INCLUDE}/suntool/frame.h \
	${INCLUDE}/suntool/panel.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/suntool/selection.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	glob.h \
	tool.h

cmds.o: cmds.c \
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
	${INCLUDE}/sundev/kbd.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/suntool/frame.h \
	${INCLUDE}/suntool/panel.h \
	${INCLUDE}/suntool/text.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/suntool/scrollbar.h \
	glob.h \
	tool.h

mail.o: mail.c \
	glob.h

subr.o: subr.c

vars.o: vars.c \
	glob.h



# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# SEE HOW make depend WORKS FOR FURTHER INFO
