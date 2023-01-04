#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 1.13 03/29/86
#

VERSION= 3.2DEV3

M=/usr/src/lib/mip
LOAD1=
LOAD2=
CFLAGS=-O -DFLEXNAMES
LINTLIBS=llib-lc.ln llib-lcore.ln llib-lcurses.ln llib-lm.ln llib-lmp.ln \
	llib-lplot.ln llib-lsunwindow.ln llib-lsuntool.ln llib-ltermcap.ln
LLSRCS=	llib-lc llib-lcore llib-lcurses llib-lm llib-lmp llib-lplot \
	llib-lsuntool llib-lsunwindow llib-ltermcap
SRCS=	$M/pftn.c $M/cgram.y $M/common.c $M/cpass1.h $M/mip.h \
	$M/optim.c $M/scan.c $M/trees.c $M/xdefs.c $M/yyerror.c \
	machdep.h lmanifest lint.c hash.c ${LLSRCS} Makefile

all:	lpass1 lpass2 ${LINTLIBS}

lpass1: cgram.o xdefs.o scan.o comm1.o pftn.o trees.o optim.o lint.o hash.o yyerror.o
	${CC} cgram.o xdefs.o scan.o comm1.o pftn.o trees.o optim.o \
		lint.o hash.o yyerror.o -o lpass1

trees.o: $M/mip.h machdep.h $M/cpass1.h $M/trees.c
	${CC} -c ${CFLAGS} -I$M -I. $M/trees.c

optim.o: $M/mip.h machdep.h $M/cpass1.h $M/optim.c
	${CC} -c ${CFLAGS} -I$M -I. $M/optim.c

pftn.o: $M/mip.h machdep.h $M/cpass1.h $M/pftn.c
	${CC} -c ${CFLAGS} -I$M -I. $M/pftn.c

yyerror.o: $M/mip.h machdep.h $M/cpass1.h $M/yyerror.c
	${CC} -c ${CFLAGS} -I$M -I. $M/yyerror.c

lint.o: $M/mip.h machdep.h $M/cpass1.h lmanifest
	${CC} -c ${CFLAGS} -I$M -I. lint.c

scan.o: $M/mip.h machdep.h $M/cpass1.h $M/scan.c
	${CC} -c ${CFLAGS} -I$M -I. $M/scan.c

xdefs.o: $M/mip.h $M/cpass1.h machdep.h $M/xdefs.c
	${CC} -c ${CFLAGS} -I$M -I. $M/xdefs.c

cgram.o: $M/mip.h $M/cpass1.h machdep.h cgram.c
	${CC} -c ${CFLAGS} -I$M -I. cgram.c

cgram.c: $M/cgram.y
	yacc $M/cgram.y
	mv y.tab.c cgram.c

comm1.o: $M/mip.h $M/cpass1.h $M/common.c machdep.h
	rm -f comm1.c
	ln -s $M/common.c comm1.c
	${CC} -c ${CFLAGS} -DPASS1 -I$M -I. comm1.c

llib-lm.ln: llib-lm lpass1
	-(/lib/cpp -C -Dlint llib-lm | ./lpass1 -uv > llib-lm.ln )

llib-lmp.ln: llib-lmp lpass1
	-(/lib/cpp -C -Dlint llib-lmp | ./lpass1 -uv > llib-lmp.ln )

llib-lplot.ln: llib-lplot lpass1
	-(/lib/cpp -C -Dlint llib-lplot | ./lpass1 -uv > llib-lplot.ln )

llib-ltermcap.ln: llib-ltermcap lpass1
	-(/lib/cpp -C -Dlint llib-ltermcap | ./lpass1 -uv > llib-ltermcap.ln )

llib-lc.ln: llib-lc lpass1
	-(/lib/cpp -C -Dlint llib-lc | ./lpass1 -v > llib-lc.ln )

llib-lcurses.ln: llib-lcurses lpass1
	-(/lib/cpp -C -Dlint llib-lcurses | ./lpass1 -v > llib-lcurses.ln )

llib-lcore.ln: llib-lcore lpass1
	-(/lib/cpp -C -Dlint llib-lcore | ./lpass1 -v > llib-lcore.ln )

llib-lsunwindow.ln: llib-lsunwindow lpass1
	-(/lib/cpp -C -Dlint llib-lsunwindow | ./lpass1 -v > llib-lsunwindow.ln)

llib-lsuntool.ln: llib-lsuntool lpass1
	-(/lib/cpp -C -Dlint llib-lsuntool | ./lpass1 -v > llib-lsuntool.ln )

lpass2: lpass2.o hash.o
	${CC} lpass2.o hash.o -o lpass2

lpass2.o: $M/mip.h lmanifest
	${CC} ${CFLAGS} -c -I$M -I. lpass2.c
	
lintall:
	lint -hpv -I. -I$M  $M/cgram.c $M/xdefs.c $M/scan.c \
		$M/pftn.c $M/trees.c $M/optim.c lint.c

install: all
	-mkdir ${DESTDIR}/usr/lib/lint && chown bin ${DESTDIR}/usr/lib/lint && \
		chmod 755 ${DESTDIR}/usr/lib/lint 
	install -s lpass1 ${DESTDIR}/usr/lib/lint/lint1
	install -s lpass2 ${DESTDIR}/usr/lib/lint/lint2
	for i in ${LLSRCS} ${LINTLIBS}; do install -c -m 644 $$i ${DESTDIR}/usr/lib/lint; done
	-rm -f ${DESTDIR}/usr/lib/lint/llib-ltermlib*
	ln -s llib-ltermcap ${DESTDIR}/usr/lib/lint/llib-ltermlib
	ln -s llib-ltermcap.ln ${DESTDIR}/usr/lib/lint/llib-ltermlib.ln
	rm -f ${DESTDIR}/usr/bin/lint
	ln -s /lib/compile ${DESTDIR}/usr/bin/lint

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

shrink:
	rm -f *.o

clean: shrink
	rm -f lpass1 lpass2 cgram.c ${LINTLIBS}

tags:	${SRCS}
	ctags -w ${SRCS}
