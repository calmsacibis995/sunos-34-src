#
# @(#)Makefile 1.1 86/09/25 SMI
#
CMDS = hello goodbye status cursor restore_cursor status_icon \
	status_image status_lock sketch sketch_menu status_size \
	hello_own canvasflash canvasinput
CFILES = hello.c goodbye.c status.c cursor.c restore_cursor.c status_icon.c \
	status_image.c status_lock.c sketch.c sketch_menu.c status_size.c \
	hello_own.c canvasflash.c canvasinput.c
LIBS = -lsuntool -lsunwindow -lpixrect
CFLAGS = -g
LDFLAGS = -g
TUTORIALDIR =usr/src/sun/suntool/tutorial

# Have empty 'all' so that don't make cmds when parent does a sub make.
# This is because it wouldn't make in the source tree before begin installed.
all:

cmds: $(CMDS)

hello: hello.o
	cc $(LDFLAGS) hello.o -o hello $(LIBS)
goodbye: goodbye.o
	cc $(LDFLAGS) goodbye.o -o goodbye $(LIBS)
status: status.o 
	cc $(LDFLAGS) status.o -o status $(LIBS)
cursor: cursor.o 
	cc $(LDFLAGS) cursor.o -o cursor $(LIBS)
restore_cursor: restore_cursor.o 
	cc $(LDFLAGS) restore_cursor.o -o restore_cursor $(LIBS)
status_icon: status_icon.o 
	cc $(LDFLAGS) status_icon.o -o status_icon $(LIBS)
status_image: status_image.o 
	cc $(LDFLAGS) status_image.o -o status_image $(LIBS)
status_lock: status_lock.o 
	cc $(LDFLAGS) status_lock.o -o status_lock $(LIBS)
sketch: sketch.o
	cc $(LDFLAGS) sketch.o -o sketch $(LIBS)
sketch_menu: sketch_menu.o
	cc $(LDFLAGS) sketch_menu.o -o sketch_menu $(LIBS)
status_size: status_size.o 
	cc $(LDFLAGS) status_size.o -o status_size $(LIBS)
hello_own: hello_own.o
	cc $(LDFLAGS) hello_own.o -o hello_own $(LIBS)
canvasflash: canvasflash.o
	cc $(LDFLAGS) canvasflash.o -o canvasflash $(LIBS)
canvasinput: canvasinput.o
	cc $(LDFLAGS) canvasinput.o -o canvasinput $(LIBS)

clean:
	-rm *.o core $(CMDS)

install: FRC
	-mkdir ${DESTDIR}/${TUTORIALDIR}
	chown bin ${DESTDIR}/${TUTORIALDIR}
	chmod 755 ${DESTDIR}/${TUTORIALDIR}
	-cp $(CFILES) Makefile ${DESTDIR}/${TUTORIALDIR}

FRC:


