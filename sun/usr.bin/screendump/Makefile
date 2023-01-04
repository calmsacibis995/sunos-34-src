#
# @(#)Makefile 1.4 87/02/23 SMI
#

# makefile for screendump, screenload and other raster file utilities

DESTDIR=
BINDIR= /usr/bin

MERGE= screenmerge
BINMERGE= screendump
LINKS= screenload rasfilter8to1 rastrepl clear_colormap
CMDS= $(BINMERGE) $(LINKS)

OBJ= screendump.o screenload.o rasfilter8to1.o rastrepl.o clear_colormap.o
SRC= screendump.c screenload.c rasfilter8to1.c rastrepl.c clear_colormap.c

UTIL= screenutil.o
LIBS= ../../usr.lib/libpixrect/libpixrect.a

RM= rm -f
CHOWN= chown -f
OWNER= bin
LINT= lint

# compilation flags
DEBUG=
INC= 
OPTIM= -O
READONLY= -R
CFLAGS_NOREADONLY = $(OPTIM) $(DEBUG) $(INC)
CFLAGS_NOMERGE = $(OPTIM) $(READONLY) $(DEBUG) $(INC)
CFLAGS= -DMERGE $(CFLAGS_NOMERGE)

all: $(MERGE)

$(MERGE): screenmerge.o $(OBJ) $(UTIL) $(LIBS)
	$(CC) $(CFLAGS) screenmerge.o $(OBJ) $(UTIL) $(LIBS) -o $@

$(CMDS): $(SRC) $(UTIL) $(LIBS)
	$(CC) $(CFLAGS_NOMERGE) $@.c $(UTIL) $(LIBS) -o $@

screenutil.o: screenutil.c
	$(CC) $(CFLAGS_NOREADONLY) -c screenutil.c

install: $(MERGE)
	-set +e ; \
	install -c -s -m 755 -o $(OWNER) \
		$(MERGE) $(DESTDIR)$(BINDIR)/$(BINMERGE) ; \
	for i in $(LINKS); do \
		$(RM) $(DESTDIR)$(BINDIR)/$$i; \
		ln -s $(BINMERGE) $(DESTDIR)$(BINDIR)/$$i ; \
	done 

clean:
	rm -f a.out core errs* $(MERGE) $(CMDS) *.o

# dependencies
clear_colormap.o:	screendump.h
rasfilter8to1.o:	screendump.h
rastrepl.o:		screendump.h
screendump.o:		screendump.h
screenload.o:		screendump.h
