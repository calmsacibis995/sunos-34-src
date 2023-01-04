#
# @(#)Makefile 1.1 86/09/24 SMI; from S5R2 1.9
#

VERSION= 3.2ALPHA

MIP = mip
LINT = lint
LINT1 = ./lint1
LFLAGS = $(LDFLAGS)
#  -DBUG4 is necessary to turn off pcc debugging tools (these tools cannot
#   be used with the lint shell script since their output conflicts)
CFLAGS = -O -DLINT -I$(MIP) -I. -DBUG4 -DFLEXNAMES
FRC =
MFILES = macdefs $(MIP)/manifest $(MIP)/mfile1

#object files for the first and second passes
OFILES1 = trees.o pftn.o scan.o comm1.o messages.o optim.o xdefs.o cgram.o\
	lerror.o msgbuf.o lint.o yyerror.o
OFILES2 = lpass2.o lerror2.o messages.o msgbuf2.o

#source files
LINTLIBS=llib-lc.ln llib-lm.ln llib-port.ln
LLSRCS=	llib-lc llib-lm llib-port
SRCS = $(MFILES) lerror.h lmanifest lpass2.h $(MIP)/manifest\
	$(MIP)/messages.h $(MIP)/common\
	$(MIP)/trees.c $(MIP)/pftn.c $(MIP)/scan.c $(MIP)/comm1.c\
	$(MIP)/messages.c $(MIP)/optim.c $(MIP)/xdefs.c $(MIP)/cgram.y\
	lerror.c msgbuf.c lint.c $(MIP)/yyerror.c\
	lpass2.c lerror2.c msgbuf2.c ${LLSRCS} lint.sh Makefile

all:	lint1 lint2 ${LINTLIBS} lint.sh

#makes for the first pass of lint
lint1:	$(OFILES1) $(FRC)
	$(CC) $(LFLAGS) -o lint1 $(OFILES1)

trees.o:	$(MIP)/messages.h $(MFILES) $(MIP)/trees.c
			$(CC) $(CFLAGS) -c  $(MIP)/trees.c
pftn.o:		$(MIP)/messages.h $(MFILES) $(MIP)/pftn.c
			$(CC) $(CFLAGS) -c  $(MIP)/pftn.c
scan.o:		$(MIP)/messages.h $(MFILES) $(MIP)/scan.c
			$(CC) $(CFLAGS) -c  $(MIP)/scan.c
comm1.o:	$(MIP)/common $(MFILES)     $(MIP)/comm1.c
			$(CC) $(CFLAGS) -c  $(MIP)/comm1.c
messages.o:	$(MIP)/messages.h           $(MIP)/messages.c
			$(CC) $(CFLAGS) -c  $(MIP)/messages.c
optim.o:	$(MFILES)                   $(MIP)/optim.c
			$(CC) $(CFLAGS) -c  $(MIP)/optim.c
yyerror.o:	$(MFILES)                   $(MIP)/yyerror.c
			$(CC) $(CFLAGS) -c  $(MIP)/yyerror.c
xdefs.o:	$(MFILES)                   $(MIP)/xdefs.c
			$(CC) $(CFLAGS) -c  $(MIP)/xdefs.c
cgram.o:	$(MIP)/messages.h $(MFILES) 
cgram.c:	$(MIP)/cgram.y
		yacc $(MIP)/cgram.y ; mv y.tab.c cgram.c
lerror.o:	lerror.h $(MIP)/messages.h 
msgbuf.o:	lerror.h $(MIP)/messages.h
lint.o:		lerror.h lmanifest $(MIP)/messages.h $(MFILES)

#makes for the lint libraries
llib-lc.ln: llib-lc $(LINT1)
	/lib/cpp -E -C -Dlint -I/usr/5include llib-lc | $(LINT1) -L >llib-lc.ln

llib-lm.ln: llib-lm $(LINT1)
	/lib/cpp -E -C -Dlint -I/usr/5include llib-lm | $(LINT1) -L >llib-lm.ln

llib-port.ln: llib-port $(LINT1)
	/lib/cpp -E -C -Dlint -I/usr/5include llib-port | $(LINT1) -L >llib-port.ln

#makes for the second pass of lint
lint2:	$(OFILES2) $(FRC)
	$(CC) $(LFLAGS) -o lint2 $(OFILES2)
msgbuf2.o:	lerror.h 
lpass2.o:	lerror.h lmanifest lpass2.h $(MIP)/manifest
lerror2.o:	lerror.h lmanifest lpass2.h $(MIP)/manifest $(MIP)/messages.h

#miscellaneous utilities

# to reinstall libraries after clean has been used, try :
#	make inslibs LINT1=/usr/5lib/lint/lint1
#  (this uses the installed version of lint pass 1)
inslibs:
	for i in llib-*; do install -c -m 644 $$i ${DESTDIR}/usr/5lib/lint; done
insdirs:
	-mkdir ${DESTDIR}/usr/5lib/lint && chown bin ${DESTDIR}/usr/5lib/lint && \
		chmod 755 ${DESTDIR}/usr/5lib/lint 
install: all insdirs inslibs
	install -s lint1 ${DESTDIR}/usr/5lib/lint/lint1
	install -s lint2 ${DESTDIR}/usr/5lib/lint/lint2
	install -c lint.sh ${DESTDIR}/usr/5bin/lint

lintall:
	$(LINT) -DLINT  -I. -I$(MIP)  $(MIP)/xdefs.c $(MIP)/scan.c \
	$(MIP)/pftn.c $(MIP)/trees.c $(MIP)/optim.c lint.c $(MIP)/messages.c \
	$(MIP)/comm1.c $(MIP)/yyerror.c msgbuf.c lerror.c -b cgram.c 
	$(LINT) -DLINT  -I$(MIP) -I. lpass2.c lerror2.c msgbuf2.c \
	$(MIP)/messages.c
cflowall:
	cflow -DLINT  -I. -I$(MIP)  $(MIP)/xdefs.c $(MIP)/scan.c \
	$(MIP)/pftn.c $(MIP)/trees.c $(MIP)/optim.c lint.c $(MIP)/messages.c \
	$(MIP)/comm1.c $(MIP)/yyerror.c msgbuf.c lerror.c -b cgram.c 
	$(LINT) -DLINT  -I$(MIP) -I. lpass2.c lerror2.c msgbuf2.c \
	$(MIP)/messages.c
sources:
	for i in ${SRCS} ; do ( sccs get -G$$i $$i )  done;
version: sources
	rm -f $@
	echo '#!/bin/csh' >> $@
	echo '# xWx (Sun) xGx' | sed 's/x/%/g'  >> $@
	echo '# Usage:' >> $@
	echo '#	version diffs' >> $@
	echo '# produces a list of diffs since the files in this' >> $@
	echo '# directory were checked in for release ${VERSION}' >> $@
	echo '#	version get' >> $@
	echo '# retrieves the versions of the files in this' >> $@
	echo '# directory used to build release ${VERSION}' >> $@
	echo '#' >> $@
	echo 'if ($$#argv != 1) then' >> $@
	echo '    echo usage: version "{diffs|get}"' >> $@
	echo '    exit(1)' >> $@
	echo 'else' >> $@
	echo '    set cmd = $$argv[1]' >> $@
	echo '    if ($$cmd != "diffs" && $$cmd != "get") then' >> $@
	echo '	echo usage: version "{diffs|get}"' >> $@
	echo '	exit(1)' >> $@
	echo '    endif' >> $@
	echo 'endif' >> $@
	echo 'set list = ( \' >> $@
	for i in ${SRCS}; do \
	    (/usr/ucb/what  $$i | grep "^	" | \
		awk '{ printf("    %s ", $$2) }' >> $@ ;\
		    echo $$i '\' >> $@ ) \
	done
	echo '    0 )' >> $@
	echo 'while ( $$list[1] != 0 )' >> $@
	echo '    set vers = $$list[1]' >> $@
	echo '    shift list' >> $@
	echo '    set file = $$list[1]' >> $@
	echo '    shift list' >> $@
	echo '    if ($$cmd == "get") then' >> $@
	echo '	sccs get -r$$vers -G$$file $$file' >> $@
	echo '    else' >> $@
	echo '	sccs diffs -r$$vers $$file' >> $@
	echo '    endif' >> $@
	echo 'end' >> $@
	chmod +x $@

clean:
	rm -f *.o cgram.c
	rm -f lint1 lint2 ${LINTLIBS}
FRC:
