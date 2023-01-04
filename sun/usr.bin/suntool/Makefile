#
# @(#)Makefile 1.10 87/04/20 SMI
#

# CFILES_USER_MINUS are CFILES_USER minus toolmerge.c
CFILES_USER_MINUS= suntools.c suntools_menu.c selection_svc.c \
	textedit.c view_surface.c gfxtool.c shelltool.c clock.c \
	toolplaces.c overview.c tektool.c canvas_demo.c cursor_demo.c \
	jumpdemo.c spheresdemo.c bouncedemo.c framedemo.c \
	perfmeter.c meter.c clockhands.c clockhands.rom.c \
	swin.c get_view_surface.c switcher.c 

# CFILES_USER are C files distributed to the user
CFILES_USER= toolmerge.c ${CFILES_USER_MINUS}

TOOL_FILES= basetools.h othertools.h
# Moved to the attic: selections.c mousetool.c 

# CFILES_NO_DISTRIBUTE are non-distributed C files
CFILES_NO_DISTRIBUTE= perfmon.c lockscreen.c lockscreen_default.c \
		adjacentscreens.c rect_util.c

GPROFFILESALL= ${CFILES_USER_MINUS:.c=.pg} ${CFILES_NO_DISTRIBUTE:.c=.pg}
.SUFFIXES: .pg
MAILTOOL_DIR= ./mailtool
OFILES_MAILTOOL= ${MAILTOOL_DIR}/main.o ${MAILTOOL_DIR}/tool.o \
		${MAILTOOL_DIR}/selection.o ${MAILTOOL_DIR}/cmds.o \
		${MAILTOOL_DIR}/mail.o ${MAILTOOL_DIR}/subr.o \
		${MAILTOOL_DIR}/vars.o
# (all mailtool C files are non-distributed)
OFILES_MAILTOOL_INSTALL= main.o tool.o selection.o cmds.o mail.o \
			 subr.o vars.o

AR_OBJS=	selection_svc.o \
			textedit.o \
			shelltool.o \
			gfxtool.o \
			view_surface.o

BASETOOLS_OBJS=		selection_svc.o \
			textedit.o \
			shelltool.o \
			gfxtool.o \
			view_surface.o \
			overview.o \
			perfmon.o \
	 		suntools_menu.o suntools.o \
			clock.o clockhands.o clockhands.rom.o \
			perfmeter.o meter.o \
			switcher.o 


OTHERTOOLS_OBJS=	cursor_demo.o \
			framedemo.o \
			spheresdemo.o \
			canvas_demo.o \
			jumpdemo.o \
			bouncedemo.o \
			lockscreen.o \
			lockscreen_default.o \
			tektool.o \
			swin.o \
			adjacentscreens.o \
			toolplaces.o

BASETOOLS_CMDS=		selection_svc \
			textedit \
			shelltool \
			gfxtool \
			overview \
			perfmon \
			clock \
			switcher \
			perfmeter

# BASE_INLIB and OTHERS_INLIB = cmds to be installed in ${DESTDIR}/usr/lib
BASE_INLIB= view_surface
OTHERS_INLIB=

# SINGLE_FILE_BASETOOLS_CMDS consist of cmds that only has one source file
# Need this variable basically for standalone compilation syntex shortcut

SINGLE_FILE_BASETOOLS_CMDS= selection_svc \
			textedit \
			shelltool \
			gfxtool \
			view_surface \
			overview \
			switcher \
			perfmon 

DEMO_CMDS=		cursor_demo \
			framedemo \
			spheresdemo \
			canvas_demo \
			jumpdemo \
			bouncedemo

OTHERS=			lockscreen \
			lockscreen_default \
			swin \
			adjacentscreens \
			tektool \
			toolplaces									
OTHERTOOLS_CMDS=${DEMO_CMDS} ${OTHERS}

OBJFILES=${BASETOOLS_OBJS} ${OTHERTOOLS_OBJS} 


CFILESALL=$(CFILES_USER) $(CFILES_NO_DISTRIBUTE) 
HDRSALL= meter.h clockhands.h
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile

INCLUDE=/usr/include
AWK=	/bin/awk
TOUCH=	touch -cf
CTAGS=	/usr/ucb/ctags -w
CHOWN=	/etc/chown -f
DESTDIR=
CPUFLAG= 
PROFILE=

DBX=
CFLAGS=	-O -I. -I$(INCLUDE)
CFLAGS_LOCAL=

CFLAGS_NOT_ROI_DATA= $(CFLAGS) $(DBX) $(CFLAGS_LOCAL) $(CPUFLAG)

LDFLAGS= ${DBX}

LIBS=	-lsuntool -lsunwindow -lpixrect
GPROF_LIBS= -lsuntool_p -lsunwindow_p -lpixrect

# BINS are the binaries that live in ${DESTDIR}/usr/bin
#BINS=	suntools lockscreen lockscreen_default toolplaces adjacentscreens \
	overview perfmon tektool swin
BASETOOLS_BIN=	suntools
OTHERTOOLS_BIN=	othertools
MERGE_LIB=	libtoolmerge.a
GPROF_MERGE_LIB= libtoolmerge_p.a

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIRS=defaults iconedit mailtool misc \
	fontedit traffic

#	Default compile DOES NOT uses -A-R flag.  This is because
#	a "main" program generally uses much more global data.
#.c.o:
#	cc $(CFLAGS) $(DBX) $(CFLAGS_LOCAL) -A-R -c $<
.c.o:
	cc $(CFLAGS_NOT_ROI_DATA) -c $<

.c.pg:
	if [ -f $*.o ]; then \
		mv $*.o $*.o.tmp; \
	fi
	cc ${CFLAGS_NOT_ROI_DATA} -pg -c $<
	-mv $*.o $*.pg
	if [ -f $*.o.tmp ]; then \
		mv $*.o.tmp $*.o; \
	fi
all:  ${MERGE_LIB} basetools.o othertools.o ${LIB}
	cc ${LDFLAGS} -o suntools basetools.o ${MERGE_LIB} ${LIBS} \
	-ltermcap -lm -lrpcsvc
	cc ${LDFLAGS} -o othertools othertools.o ${MERGE_LIB} ${LIBS} \
	-ltermcap -lm -lrpcsvc 

basetools:	basetools.o ${MERGE_LIB} ${LIB}
	cc ${LDFLAGS} -o suntools basetools.o ${MERGE_LIB} ${LIBS} \
	-ltermcap -lm -lrpcsvc

othertools:	othertools.o ${MERGE_LIB} ${LIB}
	cc ${LDFLAGS} -o othertools othertools.o ${MERGE_LIB} ${LIBS} \
	-ltermcap -lm -lrpcsvc

gprof:: ${GPROF_MERGE_LIB} basetools.pg othertools.pg
	ld -o suntools_p /lib/gcrt0.o basetools.pg \
	${GPROF_MERGE_LIB} ${GPROF_LIBS} -ltermcap -lm -lrpcsvc -lc
	ld -o othertools_p /lib/gcrt0.o othertools.pg \
	${GPROF_MERGE_LIB} ${GPROF_LIBS} -ltermcap -lm -lrpcsvc -lc


${GPROF_MERGE_LIB}:: ${GPROFFILESALL}

${GPROF_MERGE_LIB}::
	ar uvl ${GPROF_MERGE_LIB} ${GPROFFILESALL}

${GPROF_MERGE_LIB}:: FRC
	for i in ${SUBDIRS}; do \
	(	echo "Descending into directory: $$i"; \
		cd $$i; \
		make ${MFLAGS} \
			"CFLAGS=${CFLAGS}" "INCLUDE=${INCLUDE}"\
			"DBX=${DBX}" "LIBS=${LIBS}"  "CPUFLAG=${CPUFLAG}" gprof\
	); done
	ranlib ${GPROF_MERGE_LIB}

${MERGE_LIB}: libtoolmerge.tmp.a
	rm -f libtoolmerge.a
	cp libtoolmerge.tmp.a libtoolmerge.a
	ranlib libtoolmerge.a

libtoolmerge.tmp.a:: ${OBJFILES}
	ar uv libtoolmerge.tmp.a $?

libtoolmerge.tmp.a::
	-for i in ${SUBDIRS}; do \
	(	echo "Descending into directory: $$i"; \
		cd $$i; \
		make ${MFLAGS} \
		     "CFLAGS=${CFLAGS}" "INCLUDE=${INCLUDE}"\
		     "DBX=${DBX}" "LIBS=${LIBS}"  "CPUFLAG=${CPUFLAG}"\
	); done
	-mkdir tmp
	-rm -rf tmp/*
	for i in ${SUBDIRS}; do \
		(cd tmp; ar x ../$$i/lib$${i}.a; rm -f __.SYMDEF); done;
	ar uv libtoolmerge.tmp.a tmp/*
	-rm -rf tmp

basetools.o:	toolmerge.c basetools.h
	cc $(CFLAGS_NOT_ROI_DATA) -c -DTOOLSLIST=\"basetools.h\" toolmerge.c
	mv toolmerge.o basetools.o
	
othertools.o:	toolmerge.c othertools.h
	cc $(CFLAGS_NOT_ROI_DATA) -c -DTOOLSLIST=\"othertools.h\" toolmerge.c
	mv toolmerge.o othertools.o
	

basetools.pg:	toolmerge.c basetools.h
	cc $(CFLAGS_NOT_ROI_DATA) -c -pg -DTOOLSLIST=\"basetools.h\" toolmerge.c
	mv toolmerge.o basetools.pg
	
othertools.pg:	toolmerge.c othertools.h
	cc $(CFLAGS_NOT_ROI_DATA) -c -pg -DTOOLSLIST=\"othertools.h\" toolmerge.c
	mv toolmerge.o othertools.pg
	
# This is for building all the objs without linking; Useful when
# building the libraries in parallel with the executables.
objsall: ${OBJFILES}
	-for i in ${SUBDIRS}; do \
	(	echo "Descending into directory: $$i"; \
		cd $$i; \
		make ${MFLAGS} "CFLAGS=${CFLAGS}" "DBX=${DBX}"  "CPUFLAG=${CPUFLAG}" objsall \
	); done

# All std tools can run with -A-R flag:
${AR_OBJS}:
	cc $(CFLAGS_NOT_ROI_DATA) -A-R -c $*.c

standalone: ${BASETOOLS_CMDS} ${OTHERTOOLS_CMDS} ${BASE_INLIB} ${OTHERS_INLIB} suntools

standalone: 
	-for i in ${SUBDIRS}; do \
	(	echo "Descending into directory: $$i"; \
		cd $$i; \
		make ${MFLAGS} \
		     "CFLAGS=${CFLAGS}" "INCLUDE=${INCLUDE}"\
		     "DBX=${DBX}" "LIBS=${LIBS}"  "CPUFLAG=${CPUFLAG}" standalone\
	); done
	
${SINGLE_FILE_BASETOOLS_CMDS}:  $$@.c
	cc ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE -O $? -o $@ ${LIBS} 
	
${OTHERTOOLS_CMDS}:  $$@.c
	cc ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE -O $? -o $@ ${LIBS}
	
suntools: suntools.c suntools_menu.o
	cc ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE -O suntools.c \
	suntools_menu.o -o suntools ${LIBS}

clock:	 clock.c clockhands.o clockhands.rom.o 
	cc ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE -O clock.c \
	clockhands.o clockhands.rom.o  -o clock ${LIBS}
	
perfmeter: perfmeter.c meter.o
	cc ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE -O perfmeter.c \
	meter.o  -o perfmeter ${LIBS} -ltermcap -lm -rpcsvc
	
FRC:
# non-existent so always younger

fast_install: install_bins install_libs 
	-for i in ${SUBDIRS} fixedwidthfonts; do \
		(cd $$i; make ${MFLAGS} BASETOOLS_BIN=${BASETOOLS_BIN} OTHERTOOLS_BIN=${OTHERTOOLS_BIN} DESTDIR=${DESTDIR} install); done

install: install_dirs install_bins install_libs install_demos \
		install_sources install_images install_tekfonts
	-for i in ${SUBDIRS} fixedwidthfonts; do \
		(cd $$i; make ${MFLAGS} BASETOOLS_BIN=${BASETOOLS_BIN} OTHERTOOLS_BIN=${OTHERTOOLS_BIN} DESTDIR=${DESTDIR} install); done

# 	Note that when creating the directories, if you are not 
# 	  running as superuser, the ${CHOWN} will not succeed.
install_dirs:
	-for i in '' /src /src/sun /src/sun/suntool /src/sun/suntool/examples \
	/src/sun/suntool/mailtool /src/sun/suntool/iconedit \
		/bin /include /include/images /demo; do \
		mkdir ${DESTDIR}/usr$$i && \
		chmod 755 ${DESTDIR}/usr$$i && \
		${CHOWN} bin ${DESTDIR}/usr$$i; \
	done
	-for i in '' /fonts /fonts/fixedwidthfonts /fonts/tekfonts; do \
		mkdir ${DESTDIR}/usr/lib$$i && \
		chmod 755 ${DESTDIR}/usr/lib$$i && \
		${CHOWN} bin ${DESTDIR}/usr/lib$$i; \
	done

install_bins: 
	-for i in ${BASETOOLS_BIN} ${OTHERTOOLS_BIN}; do \
		install -s $$i ${DESTDIR}/usr/bin; \
	done 
	-for i in ${BASETOOLS_CMDS} ttytool mailtool clock perfmeter clocktool coretool; do \
		rm ${DESTDIR}/usr/bin/$$i; \
		ln -s ${BASETOOLS_BIN} ${DESTDIR}/usr/bin/$$i; \
	done
	-for i in ${OTHERS}; do \
		rm ${DESTDIR}/usr/bin/$$i; \
		ln -s ${OTHERTOOLS_BIN} ${DESTDIR}/usr/bin/$$i; \
	done
	-for i in ${BASE_INLIB}; do \
		rm ${DESTDIR}/usr/lib/$$i; \
		ln -s ../bin/${BASETOOLS_BIN} ${DESTDIR}/usr/lib/$$i; \
	done
	-for i in ${OTHERS_INLIB}; do \
		rm ${DESTDIR}/usr/lib/$$i; \
		ln -s ../bin/${OTHERTOOLS_BIN} ${DESTDIR}/usr/lib/$$i; \
	done

install_libs:
	-install ${MERGE_LIB} ${DESTDIR}/usr/lib
	-ranlib ${DESTDIR}/usr/lib/${MERGE_LIB}
	-install ${GPROF_MERGE_LIB} ${DESTDIR}/usr/lib
	-ranlib ${DESTDIR}/usr/lib/${GPROF_MERGE_LIB}
	-cd lib; \
	for i in rootmenu suntools textswrc; do \
		install -s -m 644 $$i ${DESTDIR}/usr/lib/.$$i; \
		rm ${DESTDIR}/usr/lib/$$i; \
		ln -s .$$i ${DESTDIR}/usr/lib/$$i; \
	done

install_demos:
	-for i in ${DEMO_CMDS}; do \
		rm ${DESTDIR}/usr/demo/$$i; \
		ln -s ../bin/${OTHERTOOLS_BIN} ${DESTDIR}/usr/demo/$$i; \
	done

# note: libcore/get_view_surface nonexistant in PE world.
install_sources:
	sccs get -p Makefile.customer > ${DESTDIR}/usr/src/sun/suntool/Makefile;
	-for i in $(CFILES_USER) $(HDRSALL) $(TOOL_FILES) ; do \
		install -m 644 $$i ${DESTDIR}/usr/src/sun/suntool; \
	done
	-install -m 644 /usr/src/sun/usr.lib/libcore/get_view_surface.c \
		${DESTDIR}/usr/src/sun/suntool/get_view_surface.c;
	-cd mailtool; \
	for i in $(OFILES_MAILTOOL_INSTALL); do \
		install -s -m 644 $$i ${DESTDIR}/usr/src/sun/suntool/mailtool; \
	done
#	    The mailtool object files are distributed so that customers can do
#	    toolmerges; sources are not distributed because AT&T owns them.
	-for i in iconedit examples; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install_sources); \
	done


install_images:
	-cd images; \
	for i in *.*; do \
		install -m 644 $$i ${DESTDIR}/usr/include/images; \
	done
	-cd ${DESTDIR}/usr/include/images; \
	rm -f clocktool.icon; \
	rm -f clocktool.rom.icon; \
	ln -s clock.icon clocktool.icon; \
	ln -s clock.rom.icon clocktool.rom.icon

install_tekfonts:
	-cd tekfonts; \
	for i in tekfont[0-9]; do \
		install $$i ${DESTDIR}/usr/lib/fonts/tekfonts; \
	done

tar:
	echo You will want to have an empty directory that you are taring to!
	tar fc - . | rsh ${DESTMACH} \
		"cd ${DESTDIR}/usr/src/sun/suntool;tar xpBf -"

clean:
	-for i in ${BASETOOLS_BIN} ${OTHERTOOLS_BIN}; do \
		mv $$i $$i-; \
	done
	rm -f *.o core $(CMDS) *.pg
	for i in ${SUBDIRS}; do (cd $$i; make ${MFLAGS} clean); done

tags:	$(CFILESALL)
	${CTAGS} $(CFILESALL);

print:	$(SRCSALL)
	lpr -p -Pimagen $(SRCSALL)

lint:
	lint $(CFLAGS) $(CFILESALL) /usr/lib/lint/llib_lpixrect.ln \
		/usr/lib/lint/llib_lsunwindow.ln \
		/usr/lib/lint/llib_lsuntool.ln

depend:
	make_depend -I${INCLUDE} ${CFILESALL}

#include Make.depend

# DO NOT DELETE THIS LINE -- make depend uses it
toolmerge.o: toolmerge.c

suntools.o: suntools.c \
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
	${INCLUDE}/sunwindow/defaults.h \
	${INCLUDE}/sunwindow/sun.h \
	${INCLUDE}/suntool/menu.h \
	${INCLUDE}/suntool/wmgr.h \
	${INCLUDE}/suntool/selection.h \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/icon_load.h

suntools_menu.o: suntools_menu.c \
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
	${INCLUDE}/suntool/wmgr.h \
	${INCLUDE}/suntool/icon_load.h

selection_svc.o: selection_svc.c \
	${INCLUDE}/sunwindow/notify.h

textedit.o: textedit.c \
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
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/suntool/text.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/images/textedit.icon

view_surface.o: view_surface.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/emptysw.h \
	${INCLUDE}/images/core_eye.icon

gfxtool.o: gfxtool.c \
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
	${INCLUDE}/suntool/ttysw.h \
	${INCLUDE}/suntool/ttytlsw.h \
	${INCLUDE}/suntool/emptysw.h \
	${INCLUDE}/images/gfxtool.icon

shelltool.o: shelltool.c \
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
	${INCLUDE}/suntool/tty.h \
	${INCLUDE}/images/terminal.icon

cmdtool.o: cmdtool.c \
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
	${INCLUDE}/suntool/ttysw.h \
	${INCLUDE}/suntool/ttytlsw.h \
	${INCLUDE}/suntool/textsw.h \
	${INCLUDE}/suntool/window.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/images/cmdtool.icon

clock.o: clock.c \
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
	${INCLUDE}/suntool/msgsw.h \
	clockhands.h \
	${INCLUDE}/images/clock.icon \
	${INCLUDE}/images/clock.rom.icon

toolplaces.o: toolplaces.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/cms_mono.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/suntool/wmgr.h

overview.o: overview.c \
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
	${INCLUDE}/images/overview.icon

tektool.o: tektool.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/teksw.h \
	${INCLUDE}/images/tektool.icon

canvas_demo.o: canvas_demo.c \
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
	${INCLUDE}/suntool/canvas.h \
	${INCLUDE}/suntool/scrollbar.h \
	${INCLUDE}/images/canvas_demo.icon

cursor_demo.o: cursor_demo.c \
	${INCLUDE}/sunwindow/cms_rainbow.h \
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
	${INCLUDE}/images/k1.pr \
	${INCLUDE}/images/k2.pr \
	${INCLUDE}/images/k3.pr \
	${INCLUDE}/images/k4.pr \
	${INCLUDE}/images/k5.pr \
	${INCLUDE}/images/down_arrow.pr \
	${INCLUDE}/images/off.pr \
	${INCLUDE}/images/on.pr \
	${INCLUDE}/images/cursor_demo.icon

jumpdemo.o: jumpdemo.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/cms_rgb.h \
	${INCLUDE}/suntool/gfxsw.h

spheresdemo.o: spheresdemo.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/cms_rainbow.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/suntool/gfxsw.h

bouncedemo.o: bouncedemo.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/suntool/gfxsw.h

framedemo.o: framedemo.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/suntool/gfxsw.h

perfmeter.o: perfmeter.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_environ.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	${INCLUDE}/suntool/menu.h \
	${INCLUDE}/suntool/walkmenu.h \
	${INCLUDE}/suntool/wmgr.h \
	meter.h

meter.o: meter.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/suntool/icon.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/suntool/tool.h \
	${INCLUDE}/suntool/tool_struct.h \
	meter.h \
	clockhands.h \
	${INCLUDE}/images/rspeedometer.icon \
	${INCLUDE}/images/speedometer.icon \
	${INCLUDE}/images/dead.icon

clockhands.o: clockhands.c \
	clockhands.h

clockhands.rom.o: clockhands.rom.c \
	clockhands.h

demo_merge.o: demo_merge.c

misc_merge.o: misc_merge.c

swin.o: swin.c \
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
	${INCLUDE}/sunwindow/win_notify.h

perfmon.o: perfmon.c \
	${INCLUDE}/sundev/mbvar.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/suntool/gfxsw.h

lockscreen.o: lockscreen.c \
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
	${INCLUDE}/images/lockscreen.icon \
	${INCLUDE}/images/jump1.icon \
	${INCLUDE}/images/jump2.icon \
	${INCLUDE}/images/jump3.icon \
	${INCLUDE}/images/jump4.icon \
	${INCLUDE}/images/jump5.icon \
	${INCLUDE}/images/jump6.icon

lockscreen_default.o: lockscreen_default.c \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/suntool/tool_hs.h \
	${INCLUDE}/sunwindow/window_hs.h \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/pw_util.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_environ.h \
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
	${INCLUDE}/suntool/gfxsw.h \
	${INCLUDE}/images/sun30.icon

adjacentscreens.o: adjacentscreens.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_struct.h



# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# SEE HOW make depend WORKS FOR FURTHER INFO
