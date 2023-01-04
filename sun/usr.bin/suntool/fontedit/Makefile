#
# @(#)Makefile 1.3 87/01/14 SMI
#
# Makefile for fontedit - a vfont font editor.

CFLAGS = -O
OFILES = fted_main.o button_build.o button_char.o button_scroll.o\
         edit_display.o  edit_pad.o init.o input.o fted_draw.o \
	 line.o echo.o write_font.o util.o \
	 slider.o button.o undo.o patches.o

#every .o except for the one defining main/fontedit_main	 
NO_MAIN_OBJS= button_build.o button_char.o button_scroll.o\
         edit_display.o  edit_pad.o init.o input.o fted_draw.o \
	 line.o echo.o write_font.o util.o \
	 slider.o button.o undo.o patches.o

CFILES = fted_main.c button_build.c button_char.c button_scroll.c\
         edit_display.c edit_pad.c init.c input.c fted_draw.c patches.c \
	 line.c echo.c write_font.c util.c slider.c button.c undo.c
GPROFFILESALL= ${CFILES:.c=.pg}
HFILES = fontedit.h externs.h buttons.h edit.h patches.h
SRC    = $(HFILES) $(CFILES)
GPROF_MERGE_LIB= libtoolmerge_p.a

EXTERN_OFILES = 
AWK=	/bin/awk
TOUCH=	touch -cf
DESTDIR=
CPUFLAG=
PROFILE=
LDFLAGS=
CTAGS=	/usr/ucb/ctags -w
LIBS=	-lsuntool -lsunwindow -lpixrect
LIBDEPS= /usr/lib/libsuntool.a /usr/lib/libsunwindow.a /usr/lib/libpixrect.a
LINTLIBS= 	/usr/lib/lint/llib_lpixrect.ln   \
		/usr/lib/lint/llib_lsunwindow.ln \
		/usr/lib/lint/llib_lsuntool.ln

LIBFILE= libfontedit.a
.SUFFIXES: .pg

CFLAGS_NOT_ROI_DATA= $(CFLAGS) $(DBX) $(CFLAGS_LOCAL) $(CPUFLAG)

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

BASETOOLS_BIN= suntools
OTHERTOOLS_BIN= othertools

${LIBFILE}:	${OFILES}
	ar uv ${LIBFILE} $?
	ranlib ${LIBFILE}

gprof:: ${GPROFFILESALL}
	ar uvl ../${GPROF_MERGE_LIB} ${GPROFFILESALL}

standalone: $(NO_MAIN_OBJS) fted_main.c $(LIBDEPS)
	cc -DSTANDALONE $(CFLAGS) $(NO_MAIN_OBJS) $(CPUFLAG) fted_main.c \
	$(EXTERN_OFILES) $(LIBS) -o fontedit
	
clean:
	rm -f *.o core fontedit errs *.pg

install: ${LIBFILE}
	rm -f ${DESTDIR}/usr/bin/fontedit
	ln -s ${OTHERTOOLS_BIN} ${DESTDIR}/usr/bin/fontedit
	
$(SRC):
	sccs get $@;

srcget: $(SRC)

tags:	$(CFILES)
	${CTAGS} $(CFILES);

print:	$(SRC)
	imprint -2 -L -O -R $(SRC)

lint:
	lint $(CFLAGS) $(CFILES) $(LINTLIBS)
depend:
	grep '^#include' ${CFILES} | grep -v '<' | \
	      sed 's/:[^"]*"\([^"]*\)".*/: \1/' | \
	      sed 's/\.c/.o/' | sed 's,../[a-z]*/,,' | \
	awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' > makedep
#	echo '$$r makedep' >>eddep
	echo '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep
	
# DO NOT DELETE THIS LINE

fted_main.o: other_hs.h fontedit.h externs.h button.h slider.h edit.h
button_build.o: other_hs.h fontedit.h externs.h button.h slider.h edit.h
button_char.o: other_hs.h fontedit.h externs.h button.h slider.h edit.h
button_scroll.o: other_hs.h fontedit.h externs.h button.h slider.h edit.h
button_scroll.o: patches.h
edit_display.o: other_hs.h fontedit.h externs.h button.h slider.h edit.h
edit_display.o: patches.h
edit_pad.o: other_hs.h fontedit.h externs.h button.h slider.h edit.h
init.o: other_hs.h fontedit.h externs.h
input.o: other_hs.h fontedit.h externs.h button.h action_table.h
fted_draw.o: other_hs.h fontedit.h externs.h button.h slider.h patches.h edit.h
patches.o: patches.h
echo.o: other_hs.h fontedit.h externs.h button.h slider.h edit.h
write_font.o: other_hs.h fontedit.h externs.h
util.o: other_hs.h fontedit.h externs.h
slider.o: other_hs.h fontedit.h externs.h button.h slider.h edit.h
button.o: button.h
undo.o: other_hs.h fontedit.h externs.h button.h slider.h edit.h
