#
# @(#)Makefile 1.1 86/09/24 SMI; from S5R2 1.1 10/15/83
#
#	Curses Library High Level Makefile.
#	To compile libcurses & the tic database compiler normally, type:
#		"make all tic"
#	To compile a particular file with normal compilation type:
#		"make FILES='<particular .o files>"
#	If debugging is desired then type:
#		"make O=debug FILES='<particular .o files>"
#	If tracing is desired then type:
#		"make O=trace FILES='<particular .o files>"
#	If profiling is desired then type:
#		"make O=profile FILES='<particular .o files>"
#	To compile the show program type:
#		"make show"
#	To compile only the tic compiler type:
#		"make tic"
#	To compile the test program ti4 that uses MINICURSES type:
#		"make ti4"
#
ULIB	= $(DESTDIR)/usr/5lib
CC	= /usr/5bin/cc

all:
	cd screen ; make all CC=$(CC) LD=$(LD)
	@echo "The libcurses.a & tic have been made."

libcurses.a:
	cd screen ; make libcurses.a CC=$(CC) LD=$(LD)
	@echo "Libcurses.a has been made."

ti4:
	cd screen ; make ti4 CC=$(CC) LD=$(LD)
	@echo "The ti4 program has been made."
	
tic:
	cd screen ; make tic CC=$(CC) LD=$(LD)
	@echo "The tic compiler has been made."
	
show:
	cd screen ; make show CC=$(CC) LD=$(LD)
	@echo "The show program has been made."
	
install:
	cd screen ; make install CC=$(CC) LD=$(LD)
	-mkdir ${ULIB}/terminfo
	@echo It will take 20 minutes or more to generate ${ULIB}/terminfo/*/*.
	(cd terminfo ; make DESTDIR=${DESTDIR} all 2>&1 > errs ; echo "tic done" )

install_h:
	cd screen ; make install_h CC=$(CC) LD=$(LD)

clean:
	cd screen ; make clobber
	cd terminfo ; make clean
