#
# @(#)Makefile 1.5 87/02/25 SMI
#

CFLAGS= -O -fswitch -fsingle

LTOOL= -lsuntool
LSUNWINDOW= -lsunwindow
LPIXRECT= -lpixrect
LTOOL= -lsuntool
LIBS= ${LTOOL} ${LSUNWINDOW} ${LPIXRECT} -lm

GFXLIB_CFILES= AppWrdLst.c DelObj.c NewObj.c PutObjVal.c SizeObj.c bitmap.c \
	gpbuf.c matrix.c pix.c vp.c vpoval.c
GFXLIB_HFILES= bitmap.h bitmap_int.h gpbuf.h gpbuf_int.h obj.h obj_int.h pix.h \
	types.h vp.h vp_int.h shader.h

GFXLIB= gfxlib.a
GFXLIBOBJ= AppWrdLst.o DelObj.o NewObj.o PutObjVal.o SizeObj.o bitmap.o \
	gpbuf.o matrix.o pix.o vp.o vpoval.o

DEMOS= rotcube box boxvp shader
SHADER_CFILES= shader_main.c shader_panel.c shader_canvas.c shader_draw.c \
	shader_color.c shader_drag.c shader_file.c shader_light.c \
	shader_matinv.c
OTHER_CFILES= rotcube.c box.c boxvp.c

ALL_CFILES= $(GFXLIB_CFILES) $(SHADER_CFILES) $(OTHER_CFILES)

all: ${GFXLIB} ${DEMOS}

${GFXLIB}: ${GFXLIBOBJ}
	ar rv ${GFXLIB} $?
	ranlib ${GFXLIB}

rotcube: rotcube.o ${GFXLIB}
	cc -o rotcube rotcube.o ${GFXLIB} ${LIBS} ${CFLAGS}

rotcube.o: rotcube.c
	cc -c rotcube.c ${CFLAGS}

box: box.o ${GFXLIB}
	cc -o box box.o ${GFXLIB} ${LIBS} ${CFLAGS}

box.o: box.c
	cc -c box.c ${CFLAGS}

boxvp: boxvp.o ${GFXLIB}
	cc -o boxvp boxvp.o ${GFXLIB} ${LIBS} ${CFLAGS}

boxvp.o: boxvp.c
	cc -c boxvp.c ${CFLAGS}

shader: shader.h shader.icon shader_main.o shader_panel.o \
	shader_canvas.o shader_draw.o shader_color.o shader_drag.o \
	shader_file.o shader_light.o shader_matinv.o ${GFXLIB}
	cc -o shader shader_main.o shader_panel.o shader_canvas.o \
	shader_draw.o shader_color.o shader_drag.o shader_file.o \
	shader_light.o shader_matinv.o ${GFXLIB} ${LIBS} ${CFLAGS}

shader.icon:
	$(GET) $(GFLAGS) -Gshader.icon SCCS/s.shader.icon

install: all
	-mkdir ${DESTDIR}/usr/demo/GP1/VIEWPORT
	-chown bin ${DESTDIR}/usr/demo/GP1/VIEWPORT
	-chmod 755 ${DESTDIR}/usr/demo/GP1/VIEWPORT
	-for i in ${ALL_CFILES}; do \
		cp $$i ${DESTDIR}/usr/demo/GP1/VIEWPORT && \
		compact ${DESTDIR}/usr/demo/GP1/VIEWPORT/$$i; \
		done
	-for i in ${GFXLIB_HFILES}; do \
		cp $$i ${DESTDIR}/usr/demo/GP1/VIEWPORT; \
		done
	-cp shader.icon ${DESTDIR}/usr/demo/GP1/VIEWPORT
	-sccs get README.release
	-cp README.release ${DESTDIR}/usr/demo/GP1/VIEWPORT/README
	-sccs get Makefile.release
	-cp Makefile.release ${DESTDIR}/usr/demo/GP1/VIEWPORT/Makefile

clean:
	-rm -f *.o
	-for i in ${GFXLIB}; do \
		rm -f $$i; \
		done
	-for i in ${DEMOS}; do \
		rm -f $$i; \
		done
