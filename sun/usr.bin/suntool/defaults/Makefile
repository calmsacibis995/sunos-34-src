#
# @(#)Makefile 1.4 87/04/20 SMI
#
CPUFLAG= 
DESTDIR=
LIBS=	-lsuntool -lsunwindow -lpixrect

INCLUDE=/usr/include
PRODLIBDIR=/usr/lib
PRODINCLUDEDIR=/usr/include/suntool

DBX=
CFLAGS=	-O -I$(INCLUDE)
CFLAGS_LOCAL=

CFLAGS_NOT_ROI_DATA= $(CFLAGS) $(DBX) $(CFLAGS_LOCAL) $(CPUFLAG)

LDFLAGS= ${DBX}

BASE_LINKS= 

OTHERS_LINKS= defaults_to_indentpro indentpro_to_defaults scrolldefaults \
		defaultsedit stty_from_defaults \
		mailrc_to_defaults defaults_to_mailrc defaults_from_input \
		input_from_defaults
HDRS=		
TABLE_SRCS=	klunker_tables.c no_sunview_tables.c sun1_tables.c	\
		sun2+_tables.c
CFILES= 	defaultsedit.c stty_from_defaults.c  \
		defaults_from_input.c input_from_defaults.c $(TABLE_SRCS) \
		scrolldefaults.c m2d_main.c m2d_defaults.c m2d_vars.c \
		d2m_main.c m2d_execute.c \
		defaults_to_indentpro.c indentpro_to_defaults.c
TABLE_OBJS=	keytables.o klunker_tables.o no_sunview_tables.o	\
		sun1_tables.o sun2+_tables.o
OBJFILES=	defaultsedit.o stty_from_defaults.o \
		defaults_from_input.o input_from_defaults.o $(TABLE_OBJS) \
		scrolldefaults.o m2d_main.o m2d_defaults.o m2d_vars.o \
		d2m_main.o  m2d_execute.o  \
		defaults_to_indentpro.o indentpro_to_defaults.o

GPROFFILESALL= defaultsedit.pg stty_from_defaults.pg  \
		defaults_from_input.pg input_from_defaults.pg \
		klunker_tables.pg no_sunview_tables.pg sun1_tables.pg      \
		sun2+_tables.pg d2m_main.pg m2d_execute.pg \
		scrolldefaults.pg m2d_main.pg m2d_defaults.pg m2d_vars.pg \
		defaults_to_indentpro.pg indentpro_to_defaults.pg keytables.o

CFILESALL=$(CFILES)
HDRSALL=$(HDRS)
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile
 
DEFAULTSFILES=  *.d
AWK=	/bin/awk
LINT=   /usr/bin/lint
BASETOOLS_BIN= suntools
OTHERTOOLS_BIN= othertools
LIBFILE=	libdefaults.a
GPROF_MERGE_LIB= libtoolmerge_p.a
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

standalone:	${LINKS}

defaultsedit:       defaultsedit.c
	${CC} ${CFLAGS_NOT_ROI_DATA} -o defaultsedit -DSTANDALONE \
	defaultsedit.c ${LIBS}

stty_from_defaults: stty_from_defaults.c    
	cc ${CFLAGS_NOT_ROI_DATA} -o stty_from_defaults \
	stty_from_defaults.c ${LIBS}  

scrolldefaults: scrolldefaults.c
	cc ${CFLAGS_NOT_ROI_DATA} -o scrolldefaults scrolldefaults.c ${LIBS}

mailrc_to_defaults: m2d_main.c m2d_execute.o m2d_vars.o m2d_defaults.o
	cc ${CFLAGS_NOT_ROI_DATA}  -o mailrc_to_defaults \
	m2d_main.c m2d_execute.o m2d_vars.o m2d_defaults.o ${LIBS}

defaults_to_mailrc: d2m_main.c m2d_execute.o m2d_vars.o m2d_defaults.o
	cc ${CFLAGS_NOT_ROI_DATA} d2m_main.c m2d_execute.o m2d_vars.o \
	m2d_defaults.o -o defaults_to_mailrc $(LIBS)

defaults_from_input: defaults_from_input.c
	cc ${CFLAGS_NOT_ROI_DATA} -o defaults_from_input \
	defaults_from_input.c

input_from_defaults: input_from_defaults.c ${TABLE_OBJS}
	cc ${CFLAGS_NOT_ROI_DATA} -o input_from_defaults \
	input_from_defaults.c ${TABLE_OBJS}
	
defaults_to_indentpro: defaults_to_indentpro.c
	cc ${CFLAGS_NOT_ROI_DATA} -o defaults_to_indentpro \
	defaults_to_indentpro.c

indentpro_to_defaults: indentpro_to_defaults.c
	cc ${CFLAGS_NOT_ROI_DATA} -o indentpro_to_defaults \
	indentpro_to_defaults.c

keytables.o:
	ln -s  /usr/sys/OBJ/keytables.o .
clean:	
	-rm -f core errs *.o *% *.BAK *.pg

install:	install_d FRC
	-for i in ${BASE_LINKS}; do \
		rm ${DESTDIR}/usr/bin/$$i; \
		ln -s ${BASETOOLS_BIN} ${DESTDIR}/usr/bin/$$i; \
	done
	-for i in ${OTHERS_LINKS}; do \
		rm ${DESTDIR}/usr/bin/$$i; \
		ln -s ${OTHERTOOLS_BIN} ${DESTDIR}/usr/bin/$$i; \
	done

install_d:	FRC
	-mkdir ${DESTDIR}/usr/lib/defaults
	-cd lib; \
	for i in *.d ; do \
		install -m 644 $$i ${DESTDIR}/usr/lib/defaults; \
	done     

## install_h:	FRC
## 	-for i in ${HFILES}; do \
## 		cmp -s $$i ${DESTDIR}${PRODINCLUDEDIR}/$$i || \
## 		install $$i ${DESTDIR}${PRODINCLUDEDIR}/$$i; \
## 	done

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
defaultsedit.o: defaultsedit.c \
	${INCLUDE}/sunwindow/sun.h \
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
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/suntool/menu.h \
	${INCLUDE}/sunwindow/defaults.h \
	${INCLUDE}/images/defaultsedit.icon

stty_from_defaults.o: stty_from_defaults.c \
	${INCLUDE}/sunwindow/defaults.h \
	${INCLUDE}/sunwindow/sun.h

defaults_from_input.o: defaults_from_input.c \
	${INCLUDE}/sundev/kbd.h \
	${INCLUDE}/sundev/kbio.h \
	${INCLUDE}/sundev/msio.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h

input_from_defaults.o: input_from_defaults.c \
	setkey_tables.h \
	${INCLUDE}/sundev/kbd.h \
	${INCLUDE}/sundev/kbio.h \
	${INCLUDE}/sundev/msio.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h

klunker_tables.o: klunker_tables.c \
	setkey_tables.h \
	${INCLUDE}/sundev/kbd.h \
	${INCLUDE}/sundev/kbio.h \
	${INCLUDE}/sundev/msio.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h

no_sunview_tables.o: no_sunview_tables.c \
	setkey_tables.h \
	${INCLUDE}/sundev/kbd.h \
	${INCLUDE}/sundev/kbio.h \
	${INCLUDE}/sundev/msio.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h

sun1_tables.o: sun1_tables.c \
	setkey_tables.h \
	${INCLUDE}/sundev/kbd.h \
	${INCLUDE}/sundev/kbio.h \
	${INCLUDE}/sundev/msio.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h

sun2+_tables.o: sun2+_tables.c \
	setkey_tables.h \
	${INCLUDE}/sundev/kbd.h \
	${INCLUDE}/sundev/kbio.h \
	${INCLUDE}/sundev/msio.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h

scrolldefaults.o: scrolldefaults.c \
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
	${INCLUDE}/suntool/panel.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/sunwindow/defaults.h \
	${INCLUDE}/sunwindow/sun.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/images/scrollbar_demo.icon

m2d_main.o: m2d_main.c \
	m2d_def.h

m2d_defaults.o: m2d_defaults.c \
	${INCLUDE}/sunwindow/defaults.h \
	${INCLUDE}/sunwindow/sun.h

m2d_vars.o: m2d_vars.c \
	m2d_def.h

d2m_main.o: d2m_main.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/defaults.h \
	${INCLUDE}/sunwindow/sun.h \
	m2d_def.h

m2d_execute.o: m2d_execute.c \
	m2d_def.h

defaults_to_indentpro.o: defaults_to_indentpro.c \
	${INCLUDE}/sunwindow/string_utils.h \
	${INCLUDE}/sunwindow/sun.h \
	${INCLUDE}/sunwindow/defaults.h

indentpro_to_defaults.o: indentpro_to_defaults.c \
	${INCLUDE}/sunwindow/string_utils.h \
	${INCLUDE}/sunwindow/sun.h \
	${INCLUDE}/sunwindow/defaults.h



# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# SEE HOW make depend WORKS FOR FURTHER INFO
