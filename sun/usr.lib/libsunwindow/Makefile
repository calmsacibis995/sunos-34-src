#
#
# @(#)Makefile 1.8 87/01/14 SMI
#

OBJS= \
	pw_access.o win_input.o pw_rop.o pw_text.o \
	pw_copy.o pw_util.o pw_read.o \
	rect.o rectlist.o rect_util.o \
	win_cursor.o win_geometry.o win_global.o \
	win_misc.o win_screen.o win_getscr.o win_blanket.o win_treeop.o \
	pw_vector.o pw_get.o pw_put.o pw_batchrop.o pw_rotcmap.o \
	win_enum.o win_screenadj.o pw_polypoint.o\
	win_environ.o win_ttyenv.o setenv.o cmdline.o getlogindir.o \
	pw_cms.o win_cms.o pw_minlock.o pw_batch.o \
	pw_polygon2.o pw_traprop.o pw_polyline.o pw_curve.o pw_line.o \
	pw_shared.o pw_test_set.o pw_dbl.o\
	cursor.o attr.o attr_cu.o attr_copy.o attr_util.o win_central.o \
	win_axe_ind.o win_enum_ind.o win_is_ind.o win_set_ind.o win_bell.o
CFILESALL= \
	pw_access.c pw_cms.c pw_copy.c pw_get.c pw_put.c pw_read.c pw_text.c \
	pw_util.c pw_vector.c pw_rop.c pw_batchrop.c win_cms.c pw_minlock.c \
	pw_line.c pw_dbl.c\
	rect.c rectlist.c rect_util.c pw_rotcmap.c pw_batch.c \
	win_cursor.c win_environ.c win_ttyenv.c setenv.c \
	cmdline.c getlogindir.c \
	win_geometry.c win_global.c win_getscr.c win_blanket.c \
	win_enum.c win_screenadj.c pw_polypoint.c\
	win_input.c win_misc.c win_screen.c win_treeop.c \
	pw_polygon2.c pw_traprop.c pw_polyline.c pw_curve.c \
	pw_shared.c \
	cursor.c attr.c attr_cu.c attr_copy.c attr_util.c win_central.c \
	win_axe_ind.c win_enum_ind.c win_is_ind.c win_set_ind.c win_bell.c
GPROFFILESALL=${CFILESALL:.c=.pg} pw_test_set.pg
SFILESALL= \
	pw_test_set.s
HDRSPUBLIC= \
	attr.h cms.h cms_grays.h cms_mono.h cms_rainbow.h cms_rgb.h \
	pixwin.h pw_util.h rect.h rectlist.h sun.h win_cursor.h win_enum.h \
	win_environ.h win_input.h win_ioctl.h win_notify.h \
	win_screen.h win_struct.h window_hs.h win_lock.h pw_dblbuf.h
HDRSPRIVATE= \
	chain.h cursor_impl.h win_impl.h

HDRSALL=${HDRSPUBLIC} ${HDRSPRIVATE}
SRCSALL=$(CFILESALL) $(SFILESALL) $(HDRSALL) Makefile

SUBDIRS=defaults filter io_stream notify string_utils
INCLUDE=/usr/include

AWK=	/bin/awk
TOUCH=	touch -c
CTAGS=	/usr/ucb/ctags -w
DESTDIR=
CPUFLAG=

DBX=
CFLAGS=	-O -I$(INCLUDE)
CFLAGS_LOCAL=

CFLAGS_NOT_ROI_DATA= $(CFLAGS) $(DBX) $(CFLAGS_LOCAL) $(CPUFLAG)
GPROFLIBFILE= libsunwindow_p.a
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
.s.pg:
	as -o $*.pg $<

# Warning: always do "make" or "make all"
# do NOT do "make libsunwindow.a"
all: libsunwindow.tmp.a
	rm -f libsunwindow.a
	cp libsunwindow.tmp.a libsunwindow.a
	ranlib libsunwindow.a

gprof:: ${GPROFFILESALL}
	ar uvl ${GPROFLIBFILE} ${GPROFFILESALL}
	ranlib ${GPROFLIBFILE}

gprof:: FRC
	-for i in ${SUBDIRS}; do \
	(       echo "Descending into directory: $$i"; \
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

libsunwindow.tmp.a::	$(OBJS)
	ar uv libsunwindow.tmp.a $?

libsunwindow.tmp.a::	FRC
	-for i in ${SUBDIRS}; do \
	(	echo "Descending into directory: $$i"; \
		cd $$i; make ${MFLAGS} "CFLAGS=${CFLAGS}" "INCLUDE=${INCLUDE}"\
		"DBX=${DBX}" \
	); done
	-mkdir tmp
	-rm -rf tmp/*
	for i in ${SUBDIRS}; do \
		(cd tmp; ar x ../$$i/lib$${i}.a; rm -f __.SYMDEF); done;
	ar uv libsunwindow.tmp.a tmp/*
	-rm -rf tmp
# Remove each __.SYMDEF from tmp so that the ranlib of the parent .a will work

#	FIXED so don't have to do a CFLAGS_NOT_ROI_DATA: win_screen.c,
#	pw_cms.c

#	Might want to change operations vector in pw_access.c
pw_access.o:
	cc ${CFLAGS_NOT_ROI_DATA} -c pw_access.c

#	Can change pw_dontclipflag in pw_rop.c
pw_rop.o:
	cc ${CFLAGS_NOT_ROI_DATA} -c pw_rop.c

#	Can change pf_textop in pw_text.c
pw_text.o:
	cc ${CFLAGS_NOT_ROI_DATA} -c pw_text.c

pw_test_set.o: pw_test_set.s
	as -o pw_test_set.o ${CPUFLAG} pw_test_set.s
#	win_error is initialized to a default and then changed in
#	win_errorhandler in win_misc.c.
win_misc.o:
	cc ${CFLAGS_NOT_ROI_DATA} -c win_misc.c

# copy and strip when install
install: install_lint
	-mkdir ${DESTDIR}/usr/include/sunwindow && \
		chown bin ${DESTDIR}/usr/include/sunwindow && \
		chmod 755 ${DESTDIR}/usr/include/sunwindow
	install -c -m 644 libsunwindow.a ${DESTDIR}/usr/lib
	ranlib ${DESTDIR}/usr/lib/libsunwindow.a
	install -c -m 644 libsunwindow_p.a ${DESTDIR}/usr/lib
	ranlib ${DESTDIR}/usr/lib/libsunwindow_p.a
	make DESTDIR=${DESTDIR} install_h

install_lint: llib-lsunwindow.ln
	install -c -m 644 llib-lsunwindow ${DESTDIR}/usr/lib/lint
	install -c -m 644 llib-lsunwindow.ln ${DESTDIR}/usr/lib/lint

llib-lsunwindow.ln: llib-lsunwindow
	lint -I${INCLUDE} -Csunwindow llib-lsunwindow

llib-lsunwindow:
	@if [ ! -f llib-lsunwindow ]; then \
		sccs get llib-lsunwindow; \
	fi

install_h:
	-for i in ${SUBDIRS}; do (cd $$i; \
		make ${MFLAGS} DESTDIR=${DESTDIR} install_h); done
	for i in ${HDRSPUBLIC}; do \
		(install -c -m 644 $$i ${DESTDIR}/usr/include/sunwindow) done

clean:
	-for i in ${SUBDIRS}; do (cd $$i; make ${MFLAGS} clean); done
	rm -f *.o libsunwindow.tmp.a *.pg
	rm -f err* core *% *.BAK *.CKP

$(SRCSALL):
	sccs get $@;

srcsget: $(SRCSALL)

tags:	$(CFILESALL)
	${CTAGS} $(CFILESALL);

print:	$(SRCSALL)
	pr -f $(SRCSALL) | rsh krypton lpr -Pimagen -r

lint:
	lint $(CFLAGS) $(CFILESALL)

FRC:
# non-existent so always younger

depend:
	make_depend -I${INCLUDE} ${CFILESALL}

#include Make.depend


# DO NOT DELETE THIS LINE -- make depend uses it
pw_access.o: pw_access.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	pw_util.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/sunwindow/win_lock.h

pw_cms.o: pw_cms.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/cms_mono.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_screen.h

pw_copy.o: pw_copy.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	pw_util.h

pw_get.o: pw_get.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	pw_util.h

pw_put.o: pw_put.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	pw_util.h

pw_read.o: pw_read.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	pw_util.h

pw_text.o: pw_text.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	pw_util.h

pw_util.o: pw_util.c \
	rect.h \
	rectlist.h \
	cms.h \
	pixwin.h

pw_vector.o: pw_vector.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	pw_util.h

pw_rop.o: pw_rop.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	pw_util.h

pw_batchrop.o: pw_batchrop.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	pw_util.h

win_cms.o: win_cms.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_ioctl.h

pw_minlock.o: pw_minlock.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h

rect.o: rect.c \
	${INCLUDE}/sunwindow/rect.h

rectlist.o: rectlist.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h

rect_util.o: rect_util.c \
	${INCLUDE}/sunwindow/rect.h

pw_rotcmap.o: pw_rotcmap.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h

pw_batch.o: pw_batch.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	pw_util.h

win_cursor.o: win_cursor.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h

win_environ.o: win_environ.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/win_environ.h \
	${INCLUDE}/sunwindow/win_struct.h

win_ttyenv.o: win_ttyenv.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_environ.h

setenv.o: setenv.c

cmdline.o: cmdline.c

getlogindir.o: getlogindir.c

win_geometry.o: win_geometry.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/sunwindow/win_struct.h

win_global.o: win_global.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/sunwindow/win_struct.h

win_getscr.o: win_getscr.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/sunwindow/win_struct.h

win_blanket.o: win_blanket.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/sunwindow/win_struct.h

win_enum.o: win_enum.c \
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
	win_enum.h

win_screenadj.o: win_screenadj.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_struct.h

win_input.o: win_input.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/win_ioctl.h

win_misc.o: win_misc.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/sunwindow/win_struct.h

win_screen.o: win_screen.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/sunwindow/win_struct.h \
	${INCLUDE}/sunwindow/cms_mono.h

win_treeop.o: win_treeop.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/sunwindow/win_struct.h

pw_polygon2.o: pw_polygon2.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	pw_util.h

pw_traprop.o: pw_traprop.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	pw_util.h

pw_polyline.o: pw_polyline.c \
	${INCLUDE}/sys/types.h \
	${INCLUDE}/pixrect/pixrect.h \
	${INCLUDE}/pixrect/pr_line.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	pw_util.h

pw_polypoint.o: pw_polypoint.c \
	${INCLUDE}/sys/types.h \
	${INCLUDE}/pixrect/pixrect.h \
	${INCLUDE}/pixrect/pr_line.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	pw_util.h

pw_line.o: pw_line.c \
	${INCLUDE}/sys/types.h \
	${INCLUDE}/pixrect/pixrect.h \
	${INCLUDE}/pixrect/pr_line.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	pw_util.h

pw_curve.o: pw_curve.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	pw_util.h \
	chain.h

pw_shared.o: pw_shared.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/sunwindow/win_lock.h \
	cursor_impl.h

pw_dbl.o: pw_dblbuf.h pw_dbl.c

cursor.o: cursor.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h \
	cursor_impl.h

attr.o: attr.c \
	${INCLUDE}/sunwindow/attr.h attr_impl.h

attr_copy.o: attr_copy.c \
	${INCLUDE}/sunwindow/attr.h

win_central.o: win_central.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_notify.h \
	win_impl.h

win_axe_ind.o: win_axe_ind.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/sunwindow/win_struct.h

win_enum_ind.o: win_enum_ind.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/sunwindow/win_struct.h

win_is_ind.o: win_is_ind.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/sunwindow/win_struct.h

win_set_ind.o: win_set_ind.c \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/win_screen.h \
	${INCLUDE}/sunwindow/win_input.h \
	${INCLUDE}/sundev/vuid_event.h \
	${INCLUDE}/sunwindow/win_ioctl.h \
	${INCLUDE}/sunwindow/win_struct.h

win_bell.o: win_bell.c \
	${INCLUDE}/sundev/kbd.h \
	${INCLUDE}/sundev/kbio.h \
	${INCLUDE}/sunwindow/defaults.h \
	${INCLUDE}/sunwindow/sun.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h \
	${INCLUDE}/sunwindow/cms.h \
	${INCLUDE}/sunwindow/pixwin.h \
	${INCLUDE}/sunwindow/win_screen.h



# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# SEE HOW make depend WORKS FOR FURTHER INFO
