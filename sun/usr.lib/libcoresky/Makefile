#
# @(#)Makefile 1.1 /86/09/25 SMI
#

#	Build core library for Sky board

CFLAGS = -O -fsky -fsingle -I../libcore

libcoresky.a: segdefaults.o auxiliary.o pdfio.o errors.o init_termin.o \

	setprimatt.o inqprimatt.o view_surface.o textmark.o inqtextent.o \
	outprim2.o outprim3.o inprims.o inputctrl.o  inqinput.o scribe.o \
	polygon2.o polygon3.o rasterprim.o rasterfileio.o sfont.o \
	cmapshared.o zbuffer.o winzbuffer.o \

	segments.o credelseg.o segatt3.o setsegatt.o inqsegatt.o segdraw.o \
	inquiry.o repaint.o batch.o setmatrix.o newframe.o savesegment.o \

	xformclip2.o xformclip3.o outputclip.o xformattrib.o ndctowld.o \
	round.o view_trans.o matinv.o cg1dd.o cg1subs.o cg2dd.o pixwindd.o \
	pixwinsubs.o cgpixwindd.o cgpixwinsubs.o  bw1dd.o bw1subs.o bw2dd.o \
	bw2subs.o rawddsubs.o windowddsubs.o winsig.o inputdds.o \
	devecho.o get_view_surface.o cg4dd.o\
	sqrttable.o gp1dd.o gp1pixwindd.o gp1_attr.o gp1_prims.o gp1_control.o \
	gp1_segdraw.o gp1subs.o
	ar ru libcoresky.a $?
	ranlib libcoresky.a

clean:
	rm -f *.o libcoresky.a

install:	libcoresky.a
	install -c -m 644 libcoresky.a ${DESTDIR}/usr/lib
	ranlib ${DESTDIR}/usr/lib/libcoresky.a

segdefaults.o:	../libcore/segdefaults.c
	cc -c $(CFLAGS) ../libcore/segdefaults.c

auxiliary.o:	../libcore/auxiliary.c
	cc -c $(CFLAGS) ../libcore/auxiliary.c

pdfio.o:	../libcore/pdfio.c
	cc -c $(CFLAGS) ../libcore/pdfio.c

errors.o:	../libcore/errors.c
	cc -c $(CFLAGS) ../libcore/errors.c

init_termin.o:	../libcore/init_termin.c
	cc -c $(CFLAGS) ../libcore/init_termin.c

setprimatt.o:	../libcore/setprimatt.c
	cc -c $(CFLAGS) ../libcore/setprimatt.c

inqprimatt.o:	../libcore/inqprimatt.c
	cc -c $(CFLAGS) ../libcore/inqprimatt.c

view_surface.o:	../libcore/view_surface.c
	cc -c $(CFLAGS) ../libcore/view_surface.c

textmark.o:	../libcore/textmark.c
	cc -c $(CFLAGS) ../libcore/textmark.c

inqtextent.o:	../libcore/inqtextent.c
	cc -c $(CFLAGS) ../libcore/inqtextent.c

outprim2.o:	../libcore/outprim2.c
	cc -c $(CFLAGS) ../libcore/outprim2.c

outprim3.o:	../libcore/outprim3.c
	cc -c $(CFLAGS) ../libcore/outprim3.c

inprims.o:	../libcore/inprims.c
	cc -c $(CFLAGS) ../libcore/inprims.c

inputctrl.o:	../libcore/inputctrl.c
	cc -c $(CFLAGS) ../libcore/inputctrl.c

inqinput.o:	../libcore/inqinput.c
	cc -c $(CFLAGS) ../libcore/inqinput.c

scribe.o:	../libcore/scribe.c
	cc -c $(CFLAGS) ../libcore/scribe.c

polygon2.o:	../libcore/polygon2.c
	cc -c $(CFLAGS) ../libcore/polygon2.c

polygon3.o:	../libcore/polygon3.c
	cc -c $(CFLAGS) ../libcore/polygon3.c

rasterprim.o:	../libcore/rasterprim.c
	cc -c $(CFLAGS) ../libcore/rasterprim.c

rasterfileio.o:	../libcore/rasterfileio.c
	cc -c $(CFLAGS) ../libcore/rasterfileio.c

sfont.o:	../libcore/sfont.c
	cc -c $(CFLAGS) ../libcore/sfont.c

cmapshared.o:	../libcore/cmapshared.c
	cc -c $(CFLAGS) ../libcore/cmapshared.c

zbuffer.o:	../libcore/zbuffer.c
	cc -c $(CFLAGS) ../libcore/zbuffer.c

winzbuffer.o:	../libcore/winzbuffer.c
	cc -c $(CFLAGS) ../libcore/winzbuffer.c

segments.o:	../libcore/segments.c
	cc -c $(CFLAGS) ../libcore/segments.c

credelseg.o:	../libcore/credelseg.c
	cc -c $(CFLAGS) ../libcore/credelseg.c

segatt3.o:	../libcore/segatt3.c
	cc -c $(CFLAGS) ../libcore/segatt3.c

setsegatt.o:	../libcore/setsegatt.c
	cc -c $(CFLAGS) ../libcore/setsegatt.c

inqsegatt.o:	../libcore/inqsegatt.c
	cc -c $(CFLAGS) ../libcore/inqsegatt.c

segdraw.o:	../libcore/segdraw.c
	cc -c $(CFLAGS) ../libcore/segdraw.c

inquiry.o:	../libcore/inquiry.c
	cc -c $(CFLAGS) ../libcore/inquiry.c

repaint.o:	../libcore/repaint.c
	cc -c $(CFLAGS) ../libcore/repaint.c

batch.o:	../libcore/batch.c
	cc -c $(CFLAGS) ../libcore/batch.c

setmatrix.o:	../libcore/setmatrix.c
	cc -c $(CFLAGS) ../libcore/setmatrix.c

newframe.o:	../libcore/newframe.c
	cc -c $(CFLAGS) ../libcore/newframe.c

savesegment.o:	../libcore/savesegment.c
	cc -c $(CFLAGS) ../libcore/savesegment.c

xformclip2.o:	../libcore/xformclip2.c
	cc -c $(CFLAGS) ../libcore/xformclip2.c

xformclip3.o:	../libcore/xformclip3.c
	cc -c $(CFLAGS) ../libcore/xformclip3.c

outputclip.o:	../libcore/outputclip.c
	cc -c $(CFLAGS) ../libcore/outputclip.c

xformattrib.o:	../libcore/xformattrib.c
	cc -c $(CFLAGS) ../libcore/xformattrib.c

ndctowld.o:	../libcore/ndctowld.c
	cc -c $(CFLAGS) ../libcore/ndctowld.c

round.o:	../libcore/round.s
	cc -c $(CFLAGS) ../libcore/round.s

view_trans.o:	../libcore/view_trans.c
	cc -c $(CFLAGS) ../libcore/view_trans.c

matinv.o:	../libcore/matinv.c
	cc -c $(CFLAGS) ../libcore/matinv.c

cg1dd.o:	../libcore/cg1dd.c
	cc -c $(CFLAGS) ../libcore/cg1dd.c

cg1subs.o:	../libcore/cg1subs.c
	cc -c $(CFLAGS) ../libcore/cg1subs.c

cg2dd.o:	../libcore/cg2dd.c
	cc -c $(CFLAGS) ../libcore/cg2dd.c
 
cg4dd.o:	../libcore/cg4dd.c
	cc -c $(CFLAGS) ../libcore/cg4dd.c
 
gp1dd.o:      ../libcore/gp1dd.c
	cc -c $(CFLAGS) ../libcore/gp1dd.c
 
gp1pixwindd.o:        ../libcore/gp1pixwindd.c
	cc -c $(CFLAGS) ../libcore/gp1pixwindd.c
 
gp1_attr.o:   ../libcore/gp1_attr.c
	cc -c $(CFLAGS) ../libcore/gp1_attr.c
 
gp1_prims.o:  ../libcore/gp1_prims.c
	cc -c $(CFLAGS) ../libcore/gp1_prims.c
 
gp1_control.o:        ../libcore/gp1_control.c
	cc -c $(CFLAGS) ../libcore/gp1_control.c
 
gp1_segdraw.o:        ../libcore/gp1_segdraw.c
	cc -c $(CFLAGS) ../libcore/gp1_segdraw.c
 
gp1subs.o:    ../libcore/gp1subs.c
	cc -c $(CFLAGS) ../libcore/gp1subs.c

pixwindd.o:	../libcore/pixwindd.c
	cc -c $(CFLAGS) ../libcore/pixwindd.c

pixwinsubs.o:	../libcore/pixwinsubs.c
	cc -c $(CFLAGS) ../libcore/pixwinsubs.c

cgpixwindd.o:	../libcore/cgpixwindd.c
	cc -c $(CFLAGS) ../libcore/cgpixwindd.c

cgpixwinsubs.o:	../libcore/cgpixwinsubs.c
	cc -c $(CFLAGS) ../libcore/cgpixwinsubs.c

bw1dd.o:	../libcore/bw1dd.c
	cc -c $(CFLAGS) ../libcore/bw1dd.c

bw1subs.o:	../libcore/bw1subs.c
	cc -c $(CFLAGS) ../libcore/bw1subs.c

bw2dd.o:	../libcore/bw2dd.c
	cc -c $(CFLAGS) ../libcore/bw2dd.c

bw2subs.o:	../libcore/bw2subs.c
	cc -c $(CFLAGS) ../libcore/bw2subs.c

rawddsubs.o:	../libcore/rawddsubs.c
	cc -c $(CFLAGS) ../libcore/rawddsubs.c

sqrttable.o:	../libcore/sqrttable.c
	cc -c $(CFLAGS) ../libcore/sqrttable.c

windowddsubs.o:	../libcore/windowddsubs.c
	cc -c $(CFLAGS) ../libcore/windowddsubs.c

winsig.o:	../libcore/winsig.c
	cc -c $(CFLAGS) ../libcore/winsig.c

inputdds.o:	../libcore/inputdds.c
	cc -c $(CFLAGS) ../libcore/inputdds.c

devecho.o:	../libcore/devecho.c
	cc -c $(CFLAGS) ../libcore/devecho.c

get_view_surface.o:	../libcore/get_view_surface.c
	cc -c $(CFLAGS) ../libcore/get_view_surface.c
