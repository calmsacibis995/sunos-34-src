#
# @(#)Makefile 1.1 86/09/25 SMI
#

#	Build core library

CFLAGS = -O -fsingle -fswitch
OBJS =  segdefaults.o auxiliary.o pdfio.o errors.o init_termin.o \

	setprimatt.o inqprimatt.o view_surface.o textmark.o inqtextent.o \
	outprim2.o outprim3.o inprims.o inputctrl.o  inqinput.o scribe.o \
	polygon2.o polygon3.o rasterprim.o rasterfileio.o sfont.o \
	cmapshared.o zbuffer.o winzbuffer.o \

	segments.o credelseg.o segatt3.o setsegatt.o inqsegatt.o segdraw.o \
	inquiry.o repaint.o batch.o setmatrix.o newframe.o \
	savesegment.o \

	xformclip2.o xformclip3.o outputclip.o xformattrib.o ndctowld.o \
	round.o view_trans.o matinv.o cg1dd.o cg2dd.o cg1subs.o pixwindd.o \
	pixwinsubs.o cgpixwindd.o cgpixwinsubs.o gp1dd.o gp1pixwindd.o \
	bw1dd.o bw1subs.o bw2dd.o cg4dd.o\
	bw2subs.o rawddsubs.o windowddsubs.o winsig.o inputdds.o \
	devecho.o get_view_surface.o \
	gp1_attr.o gp1_prims.o gp1_control.o gp1_segdraw.o gp1subs.o \
	sqrttable.o

HDRS =	bw1dd.h colorshader.h fbio.h langnames.h bw2dd.h coretypes.h \
	h pixwindd.h cgdd.h corevars.h gp1_pwpr.h usercore.h \
	cgpixwindd.h gp1pixwindd.h gp1cmds.h


libcore.a: ${OBJS}
	ar uv libcore.a $?
	ranlib libcore.a

clean:
	rm -f *.o libcore.a

install:	libcore.a usercore.h
	install -c -m 644 libcore.a ${DESTDIR}/usr/lib
	install -c -m 644 usercore.h ${DESTDIR}/usr/include
	ranlib ${DESTDIR}/usr/lib/libcore.a

lint:
	lint ${CFLAGS} *.c
