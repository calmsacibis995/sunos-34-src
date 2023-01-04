IDENT='-DID="Rev Q"' -DVME -DXYBOOT -DXTBOOT -DSTBOOT -DSDBOOT -DS2FRAMEBUF -DS2COLOR -DNDBOOT -DMTBOOT -DMAPCMDS -DLOGO -DKEYBS2 -DKEYBOARD -DIEBOOT -DIDPROM -DFRAMEBUF -DFBBOTH -DBREAKKEY -DAUTOBOOT
#
# Makefile for Sun ROM Monitor
#
# @(#)Makefile.mon 1.8 84/11/29 Copyright (c) 1983 by Sun Microsystems, Inc.
#
# This Makefile is constructed from a configuration description.
# Most changes should be made there:
#	/usr/src/sun/mon/devl/conf/<config>
# after which you should do
#	sunconfig <config>
# Generic Makefile changes should be made in
#	/usr/src/sun/mon/devl/conf/Makefile
# after which sunconfig should be rerun for all config files.
#
# -N on loader gives old-style load module (not pageable or pagealigned)
LD=/bin/ld -N
# -j on assembler gives short external PC-relative jumps, JSR's.
AS=/bin/as -j
C2=/lib/c2

COPTS=
CFLAGS=-I../h -I. ${COPTS} ${IDENT} -DMC68000
AFLAGS=-c
LDFLAGS=-lpixrect -lc

RELOC=EF0000

OBJS=romvec.o version.o trap.o sunmon.o s2map.o diag.o banner.o \
	commands.o getidprom.o idprom.o space.o setbus.o usecmd.o \
	getline.o getnum.o ishex.o printf.o printhex.o busyio.o ldivt.o \
	keypress.o getkey.o ktab.s2.o config.o boot.o common.o probe.o \
	blts.o copy.o drawchr.o expand.o fill.o finit.o fwritestr.o \
	gallmash.o insdel.o mem_grab.o scutils.o sunlogo.o if_ie.o mt.o \
	nd.o xy.o sc.o sd.o st.o xt.o ttboot.o xxboot.o 

CFILES=../kernel/romvec.s ../kernel/version.c ../kernel/trap.s \
	../kernel/sunmon.c ../kernel/s2map.s ../diag/diag.s \
	../kernel/banner.c ../kernel/commands.c ../kernel/getidprom.s \
	../kernel/idprom.c ../kernel/space.s ../kernel/setbus.s \
	../kernel/usecmd.c ../kernel/getline.c ../kernel/getnum.c \
	../kernel/ishex.c ../kernel/printf.c ../kernel/printhex.c \
	../kernel/busyio.c ../kernel/ldivt.s ../keyboard/keypress.c \
	../keyboard/getkey.c ../keyboard/ktab.s2.c ../prom2/config.c \
	../prom2/boot.c ../prom2/common.c ../prom2/probe.s ../dpy/blts.s \
	../dpy/copy.c ../dpy/drawchr.c ../dpy/expand.c ../dpy/fill.c \
	../dpy/finit.c ../dpy/fwritestr.c ../dpy/gallmash.c \
	../dpy/insdel.c ../dpy/mem_grab.c ../dpy/scutils.c \
	../dpy/sunlogo.c ../../sys/sunstand/if_ie.c ../prom2/mt.c \
	../../sys/sunstand/nd.c ../prom2/xy.c ../../sys/sunstand/sc.c \
	../../sys/sunstand/sd.c ../../sys/sunstand/st.c \
	../../sys/sunstand/xt.c ../prom2/ttboot.c ../prom2/xxboot.c 

all: sunmon
	what ../[a-z]*/* >WhatModules
# The above remembers what the version # of each module was at the time
# this sunmon was built.  This is needed since the SCCS id's don't compile
# into the PROM, they are just comments in the source.

sunmon: Makefile ${OBJS} 
	@echo loading sunmon
	@rm -f sunmon
	$(LD) -o sunmon -T $(RELOC) ${OBJS} ${LDFLAGS}
	size sunmon

# assym.h contains assembler defns of structure members.  It's made by
# genassym.c, which just printf's their values.  make depend keeps it all OK.
# structconst.h contains hex values of constant structs like page map entries.
# This hack is used to get around C's refusal to initialize local structs
# and inability to define structure constants.
genassym.o: ../kernel/genassym.c Makefile
	${CC} -c ${CFLAGS} ../kernel/genassym.c
assym.h: genassym.o
	${CC} ${CFLAGS} genassym.o; ./a.out >assym.h; rm -f a.out
structconst.h: assym.h
	grep PME_ assym.h >structconst.h

install: sunmon
	;

clean:
	rm -f *.o *.s *.h *.lst errs linterrs lint.foo foo1 foo2 grindlist
	rm -f sunmon

lint:
	ls ${CFILES} >lint.foo
	lint -hbxnz -I. ${COPTS} ${IDENT} `fgrep -v .s <lint.foo`
	rm lint.foo

depend:
	@echo 'making dependencies ...'
	@egrep '^#include' ../kernel/genassym.c ${CFILES} | \
	      egrep -v '<' | \
	      sed 's/:[^"]*"\([^"]*\)".*/: \1/' | \
	      sed 's/\.[cs]/.o/' | \
	      sed 's,^\([.0-9a-z/]*/\)\([^:]*\): *,\2: \1,' | \
	      sed 's,: [.0-9a-z/]*Makefile,: Makefile,' | \
	      sed 's,: [.0-9a-z/]*assym\.h,: assym.h,' | \
	      sed 's,: [.0-9a-z/]*structconst\.h,: structconst.h,' | \
	      sed 's,/[^ /.]*/\.\./,/,' >depend.tmp
# The above is:
#	 egrep:	get all #include lines from all files.
#	 egrep: exclude system includes, just use "....." includes
#	 sed:   : #include "foobar.h"  ==>  : foobar.h
#		get rid of extraneous gunk
#	 sed:   file.c: foobar.h       ==>  file.o: foobar.h
#		the dependency is on the .o, not the .c or .s
#	 sed:   ../dir/name.o: ../h/bar.h   ==>  name.c: ../dir/../h/bar.h
#		the include file is relative to the source file's dir
#	 sed:   name.o: ../dir/Makefile	    ==>  name.o: Makefile
#		We know that 'Makefile' is in current dir, not source dir.
#	 sed:   name.o: ../dir/assym.h	    ==>  name.o: assym.h
#		We know that 'assym.h' is in current dir, not source dir.
#	 sed:   name.o: ../dir/structconst.h	    ==>  name.o: structconst.h
#		We know that 'structconst.h' is in current dir, not source dir.
#	 sed:   /name/../     ==>   /
#		clean up the cases like ../kernel/../h  to ../h
#        See, that wasn't too bad, was it?
	@awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' <depend.tmp  > makedep
# The above just merges lines of dependencies for the same module until
# they would exceed 78 characters.  Just for prettiness.
	@echo '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
	@echo '$$r makedep' >>eddep
	@echo 'w' >>eddep
	@ed - Makefile < eddep
	@rm eddep makedep depend.tmp
	@echo '... done'

tags:
	/usr/ucb/ctags ${CFILES}

print:
	../conf/pr.sh `basename \`pwd\`` Makefile $(CFILES) ../h/*.h | lpr

grindlist: ../conf/files
	ls `grep -v '^#' ../conf/files | colrm 30 | sed 's,^,../,' ` | sort -t/ +2 >grindlist

grindc: grindlist
	igrind ${GRINDOPTS} `grep '\.c$$' grindlist`

grinds: grindlist
	igrind ${GRINDOPTS} -n `pwd`/Makefile `grep -v '\.c$$' grindlist`

grindh: assym.h
	igrind ${GRINDOPTS} `pwd`/assym.h `pwd`/structconst.h ../h/*.h

grind: grindh grindc grinds
	@echo

romvec.o: ../kernel/romvec.s
	cp ../kernel/romvec.s romvec.c
	${CC} -E -DASM ${CFLAGS} romvec.c >romvec.i
	${AS} ${LFLAG} romvec.i -o romvec.o
	rm -f romvec.c romvec.i
version.o: ../kernel/version.c
	${CC} -S ${CFLAGS} ../kernel/version.c
	mv version.s version.p
	${C2} version.p version.s
	${AS} ${LFLAG} version.s -o version.o
	rm -f version.p
trap.o: ../kernel/trap.s
	cp ../kernel/trap.s trap.c
	${CC} -E -DASM ${CFLAGS} trap.c >trap.i
	${AS} ${LFLAG} trap.i -o trap.o
	rm -f trap.c trap.i
sunmon.o: ../kernel/sunmon.c
	${CC} -S ${CFLAGS} ../kernel/sunmon.c
	mv sunmon.s sunmon.p
	${C2} sunmon.p sunmon.s
	${AS} ${LFLAG} sunmon.s -o sunmon.o
	rm -f sunmon.p
s2map.o: ../kernel/s2map.s
	cp ../kernel/s2map.s s2map.c
	${CC} -E -DASM ${CFLAGS} s2map.c >s2map.i
	${AS} ${LFLAG} s2map.i -o s2map.o
	rm -f s2map.c s2map.i
diag.o: ../diag/diag.s
	cp ../diag/diag.s diag.c
	${CC} -E -DASM ${CFLAGS} diag.c >diag.i
	${AS} ${LFLAG} diag.i -o diag.o
	rm -f diag.c diag.i
banner.o: ../kernel/banner.c
	${CC} -S ${CFLAGS} ../kernel/banner.c
	mv banner.s banner.p
	${C2} banner.p banner.s
	${AS} ${LFLAG} banner.s -o banner.o
	rm -f banner.p
commands.o: ../kernel/commands.c
	${CC} -S ${CFLAGS} ../kernel/commands.c
	mv commands.s commands.p
	${C2} commands.p commands.s
	${AS} ${LFLAG} commands.s -o commands.o
	rm -f commands.p
getidprom.o: ../kernel/getidprom.s
	cp ../kernel/getidprom.s getidprom.c
	${CC} -E -DASM ${CFLAGS} getidprom.c >getidprom.i
	${AS} ${LFLAG} getidprom.i -o getidprom.o
	rm -f getidprom.c getidprom.i
idprom.o: ../kernel/idprom.c
	${CC} -S ${CFLAGS} ../kernel/idprom.c
	mv idprom.s idprom.p
	${C2} idprom.p idprom.s
	${AS} ${LFLAG} idprom.s -o idprom.o
	rm -f idprom.p
space.o: ../kernel/space.s
	cp ../kernel/space.s space.c
	${CC} -E -DASM ${CFLAGS} space.c >space.i
	${AS} ${LFLAG} space.i -o space.o
	rm -f space.c space.i
setbus.o: ../kernel/setbus.s
	cp ../kernel/setbus.s setbus.c
	${CC} -E -DASM ${CFLAGS} setbus.c >setbus.i
	${AS} ${LFLAG} setbus.i -o setbus.o
	rm -f setbus.c setbus.i
usecmd.o: ../kernel/usecmd.c
	${CC} -S ${CFLAGS} ../kernel/usecmd.c
	mv usecmd.s usecmd.p
	${C2} usecmd.p usecmd.s
	${AS} ${LFLAG} usecmd.s -o usecmd.o
	rm -f usecmd.p
getline.o: ../kernel/getline.c
	${CC} -S ${CFLAGS} ../kernel/getline.c
	mv getline.s getline.p
	${C2} getline.p getline.s
	${AS} ${LFLAG} getline.s -o getline.o
	rm -f getline.p
getnum.o: ../kernel/getnum.c
	${CC} -S ${CFLAGS} ../kernel/getnum.c
	mv getnum.s getnum.p
	${C2} getnum.p getnum.s
	${AS} ${LFLAG} getnum.s -o getnum.o
	rm -f getnum.p
ishex.o: ../kernel/ishex.c
	${CC} -S ${CFLAGS} ../kernel/ishex.c
	mv ishex.s ishex.p
	${C2} ishex.p ishex.s
	${AS} ${LFLAG} ishex.s -o ishex.o
	rm -f ishex.p
printf.o: ../kernel/printf.c
	${CC} -S ${CFLAGS} ../kernel/printf.c
	mv printf.s printf.p
	${C2} printf.p printf.s
	${AS} ${LFLAG} printf.s -o printf.o
	rm -f printf.p
printhex.o: ../kernel/printhex.c
	${CC} -S ${CFLAGS} ../kernel/printhex.c
	mv printhex.s printhex.p
	${C2} printhex.p printhex.s
	${AS} ${LFLAG} printhex.s -o printhex.o
	rm -f printhex.p
busyio.o: ../kernel/busyio.c
	${CC} -S ${CFLAGS} ../kernel/busyio.c
	mv busyio.s busyio.p
	${C2} busyio.p busyio.s
	${AS} ${LFLAG} busyio.s -o busyio.o
	rm -f busyio.p
ldivt.o: ../kernel/ldivt.s
	cp ../kernel/ldivt.s ldivt.c
	${CC} -E -DASM ${CFLAGS} ldivt.c >ldivt.i
	${AS} ${LFLAG} ldivt.i -o ldivt.o
	rm -f ldivt.c ldivt.i
keypress.o: ../keyboard/keypress.c
	${CC} -S ${CFLAGS} ../keyboard/keypress.c
	mv keypress.s keypress.p
	${C2} keypress.p keypress.s
	${AS} ${LFLAG} keypress.s -o keypress.o
	rm -f keypress.p
getkey.o: ../keyboard/getkey.c
	${CC} -S ${CFLAGS} ../keyboard/getkey.c
	mv getkey.s getkey.p
	${C2} getkey.p getkey.s
	${AS} ${LFLAG} getkey.s -o getkey.o
	rm -f getkey.p
ktab.s2.o: ../keyboard/ktab.s2.c
	${CC} -S ${CFLAGS} ../keyboard/ktab.s2.c
	mv ktab.s2.s ktab.s2.p
	${C2} ktab.s2.p ktab.s2.s
	${AS} ${LFLAG} ktab.s2.s -o ktab.s2.o
	rm -f ktab.s2.p
config.o: ../prom2/config.c
	${CC} -S ${CFLAGS} ../prom2/config.c
	mv config.s config.p
	${C2} config.p config.s
	${AS} ${LFLAG} config.s -o config.o
	rm -f config.p
boot.o: ../prom2/boot.c
	${CC} -S ${CFLAGS} ../prom2/boot.c
	mv boot.s boot.p
	${C2} boot.p boot.s
	${AS} ${LFLAG} boot.s -o boot.o
	rm -f boot.p
common.o: ../prom2/common.c
	${CC} -S ${CFLAGS} ../prom2/common.c
	mv common.s common.p
	${C2} common.p common.s
	${AS} ${LFLAG} common.s -o common.o
	rm -f common.p
probe.o: ../prom2/probe.s
	cp ../prom2/probe.s probe.c
	${CC} -E -DASM ${CFLAGS} probe.c >probe.i
	${AS} ${LFLAG} probe.i -o probe.o
	rm -f probe.c probe.i
blts.o: ../dpy/blts.s
	cp ../dpy/blts.s blts.c
	${CC} -E -DASM ${CFLAGS} blts.c >blts.i
	${AS} ${LFLAG} blts.i -o blts.o
	rm -f blts.c blts.i
copy.o: ../dpy/copy.c
	${CC} -S ${CFLAGS} ../dpy/copy.c
	mv copy.s copy.p
	${C2} copy.p copy.s
	${AS} ${LFLAG} copy.s -o copy.o
	rm -f copy.p
drawchr.o: ../dpy/drawchr.c
	${CC} -S ${CFLAGS} ../dpy/drawchr.c
	mv drawchr.s drawchr.p
	${C2} drawchr.p drawchr.s
	${AS} ${LFLAG} drawchr.s -o drawchr.o
	rm -f drawchr.p
expand.o: ../dpy/expand.c
	${CC} -S ${CFLAGS} ../dpy/expand.c
	mv expand.s expand.p
	${C2} expand.p expand.s
	${AS} ${LFLAG} expand.s -o expand.o
	rm -f expand.p
fill.o: ../dpy/fill.c
	${CC} -S ${CFLAGS} ../dpy/fill.c
	mv fill.s fill.p
	${C2} fill.p fill.s
	${AS} ${LFLAG} fill.s -o fill.o
	rm -f fill.p
finit.o: ../dpy/finit.c
	${CC} -S ${CFLAGS} ../dpy/finit.c
	mv finit.s finit.p
	${C2} finit.p finit.s
	${AS} ${LFLAG} finit.s -o finit.o
	rm -f finit.p
fwritestr.o: ../dpy/fwritestr.c
	${CC} -S ${CFLAGS} ../dpy/fwritestr.c
	mv fwritestr.s fwritestr.p
	${C2} fwritestr.p fwritestr.s
	${AS} ${LFLAG} fwritestr.s -o fwritestr.o
	rm -f fwritestr.p
gallmash.o: ../dpy/gallmash.c
	${CC} -S ${CFLAGS} ../dpy/gallmash.c
	mv gallmash.s gallmash.p
	${C2} gallmash.p gallmash.s
	${AS} ${LFLAG} gallmash.s -o gallmash.o
	rm -f gallmash.p
insdel.o: ../dpy/insdel.c
	${CC} -S ${CFLAGS} ../dpy/insdel.c
	mv insdel.s insdel.p
	${C2} insdel.p insdel.s
	${AS} ${LFLAG} insdel.s -o insdel.o
	rm -f insdel.p
mem_grab.o: ../dpy/mem_grab.c
	${CC} -S ${CFLAGS} ../dpy/mem_grab.c
	mv mem_grab.s mem_grab.p
	${C2} mem_grab.p mem_grab.s
	${AS} ${LFLAG} mem_grab.s -o mem_grab.o
	rm -f mem_grab.p
scutils.o: ../dpy/scutils.c
	${CC} -S ${CFLAGS} ../dpy/scutils.c
	mv scutils.s scutils.p
	${C2} scutils.p scutils.s
	${AS} ${LFLAG} scutils.s -o scutils.o
	rm -f scutils.p
sunlogo.o: ../dpy/sunlogo.c
	${CC} -S ${CFLAGS} ../dpy/sunlogo.c
	mv sunlogo.s sunlogo.p
	${C2} sunlogo.p sunlogo.s
	${AS} ${LFLAG} sunlogo.s -o sunlogo.o
	rm -f sunlogo.p
if_ie.o: ../../sys/sunstand/if_ie.c
	${CC} -S ${CFLAGS} ../../sys/sunstand/if_ie.c
	mv if_ie.s if_ie.p
	${C2} if_ie.p if_ie.s
	${AS} ${LFLAG} if_ie.s -o if_ie.o
	rm -f if_ie.p
mt.o: ../prom2/mt.c
	${CC} -S ${CFLAGS} ../prom2/mt.c
	mv mt.s mt.p
	${C2} mt.p mt.s
	${AS} ${LFLAG} mt.s -o mt.o
	rm -f mt.p
nd.o: ../../sys/sunstand/nd.c
	${CC} -S ${CFLAGS} ../../sys/sunstand/nd.c
	mv nd.s nd.p
	${C2} nd.p nd.s
	${AS} ${LFLAG} nd.s -o nd.o
	rm -f nd.p
xy.o: ../prom2/xy.c
	${CC} -S ${CFLAGS} ../prom2/xy.c
	mv xy.s xy.p
	${C2} xy.p xy.s
	${AS} ${LFLAG} xy.s -o xy.o
	rm -f xy.p
sc.o: ../../sys/sunstand/sc.c
	${CC} -S ${CFLAGS} ../../sys/sunstand/sc.c
	mv sc.s sc.p
	${C2} sc.p sc.s
	${AS} ${LFLAG} sc.s -o sc.o
	rm -f sc.p
sd.o: ../../sys/sunstand/sd.c
	${CC} -S ${CFLAGS} ../../sys/sunstand/sd.c
	mv sd.s sd.p
	${C2} sd.p sd.s
	${AS} ${LFLAG} sd.s -o sd.o
	rm -f sd.p
st.o: ../../sys/sunstand/st.c
	${CC} -S ${CFLAGS} ../../sys/sunstand/st.c
	mv st.s st.p
	${C2} st.p st.s
	${AS} ${LFLAG} st.s -o st.o
	rm -f st.p
xt.o: ../../sys/sunstand/xt.c
	${CC} -S ${CFLAGS} ../../sys/sunstand/xt.c
	mv xt.s xt.p
	${C2} xt.p xt.s
	${AS} ${LFLAG} xt.s -o xt.o
	rm -f xt.p
ttboot.o: ../prom2/ttboot.c
	${CC} -S ${CFLAGS} ../prom2/ttboot.c
	mv ttboot.s ttboot.p
	${C2} ttboot.p ttboot.s
	${AS} ${LFLAG} ttboot.s -o ttboot.o
	rm -f ttboot.p
xxboot.o: ../prom2/xxboot.c
	${CC} -S ${CFLAGS} ../prom2/xxboot.c
	mv xxboot.s xxboot.p
	${C2} xxboot.p xxboot.s
	${AS} ${LFLAG} xxboot.s -o xxboot.o
	rm -f xxboot.p

# DO NOT DELETE THIS LINE -- make depend uses it

genassym.o: ../h/s2addrs.h ../h/s2map.h ../h/sunmon.h ../h/globram.h
genassym.o: ../h/am9513.h ../h/suntimer.h ../h/enable.h ../h/m68vectors.h
genassym.o: ../h/zsreg.h ../h/sunromvec.h ../h/diag.h ../h/dpy.h ../h/am8068.h
genassym.o: ../h/montrap.h ../../usr.lib/libpixrect/memreg.h
genassym.o: ../../usr.lib/libpixrect/cg2reg.h
romvec.o: assym.h ../../sys/sun/fbio.h
version.o: Makefile
trap.o: assym.h ../h/s2led.h
sunmon.o: ../h/sunmon.h ../h/s2map.h ../h/s2addrs.h ../h/s2misc.h ../h/enable.h
sunmon.o: ../h/globram.h ../h/zsreg.h ../h/suntimer.h ../h/am9513.h
sunmon.o: ../h/keyboard.h ../h/sunromvec.h ../h/m68vectors.h ../h/framebuf.h
sunmon.o: ../h/pginit.h ../h/montrap.h ../h/dpy.h ../h/video.h ../h/s2led.h
sunmon.o: structconst.h
s2map.o: assym.h
diag.o: assym.h ../h/s2led.h
banner.o: ../h/sunmon.h ../h/globram.h ../h/diag.h ../h/idprom.h ../h/s2addrs.h
banner.o: ../h/video.h ../../sys/sun/fbio.h
commands.o: ../h/sunmon.h ../h/globram.h ../h/m68vectors.h ../h/statreg.h
commands.o: ../h/am9513.h ../h/s2addrs.h ../h/s2map.h ../h/buserr.h
commands.o: ../h/s2misc.h ../h/pginit.h ../h/montrap.h ../h/s2led.h
commands.o: ../h/keyboard.h
getidprom.o: assym.h
idprom.o: ../h/idprom.h
setbus.o: assym.h
usecmd.o: ../h/sunmon.h ../h/globram.h ../h/s2addrs.h ../h/s2misc.h
usecmd.o: ../h/zsreg.h
getline.o: ../h/sunmon.h ../h/globram.h
busyio.o: ../h/sasun.h ../h/zsreg.h ../h/sunmon.h ../h/globram.h
keypress.o: ../h/sunmon.h ../h/globram.h ../h/keyboard.h ../h/asyncbuf.h
keypress.o: ../h/s2addrs.h ../h/s2misc.h
getkey.o: ../h/s2addrs.h ../h/keyboard.h ../h/asyncbuf.h ../h/globram.h
ktab.o2.c: ../h/keyboard.h
config.o: ../h/bootparam.h ../h/s2addrs.h ../h/globram.h ../h/diag.h
boot.o: Makefile ../h/bootparam.h ../h/sunromvec.h
common.o: ../h/sasun.h ../h/globram.h
copy.o: ../h/globram.h ../h/dpy.h ../h/framebuf.h
drawchr.o: ../h/globram.h ../h/dpy.h ../h/framebuf.h
fill.o: ../h/globram.h ../h/dpy.h ../h/framebuf.h
finit.o: ../h/s2addrs.h ../h/globram.h ../h/dpy.h ../../sys/sun/fbio.h
finit.o: ../h/video.h ../h/framebuf.h ../h/pginit.h ../h/s2map.h
fwritestr.o: ../h/globram.h ../h/dpy.h ../h/keyboard.h ../h/video.h
fwritestr.o: ../h/s2addrs.h ../h/framebuf.h
insdel.o: ../h/globram.h ../h/dpy.h
scutils.o: structconst.h ../h/s2addrs.h ../h/s2map.h ../h/pginit.h
scutils.o: ../../usr.lib/libpixrect/memreg.h ../../usr.lib/libpixrect/cg2reg.h
sunlogo.o: ../h/globram.h ../h/dpy.h ../h/framebuf.h
if_ie.o: ../../sys/sunstand/saio.h ../../sys/sunstand/sasun.h
if_ie.o: ../../sys/h/socket.h ../../sys/net/if.h ../../sys/netinet/in.h
if_ie.o: ../../sys/netinet/if_ether.h ../../sys/sunif/if_iereg.h
if_ie.o: ../../sys/sunif/if_mie.h ../../sys/sunif/if_obie.h
if_ie.o: ../../sys/mon/idprom.h
mt.o: ../h/sunmon.h ../h/sasun.h ../h/bootparam.h ../h/msg.h
nd.o: ../../sys/sunstand/saio.h ../../sys/h/socket.h ../../sys/net/if.h
nd.o: ../../sys/netinet/in.h ../../sys/netinet/if_ether.h
nd.o: ../../sys/netinet/in_systm.h ../../sys/netinet/ip.h ../../sys/sun/ndio.h
nd.o: ../../sys/mon/sunromvec.h ../../sys/mon/s2addrs.h ../../sys/mon/idprom.h
xy.o: ../h/sasun.h ../h/bootparam.h ../h/msg.h
sc.o: ../../sys/h/types.h ../../sys/sun/param.h ../../sys/sundev/screg.h
sd.o: ../../sys/sunstand/saio.h ../../sys/sunstand/sasun.h
sd.o: ../../sys/sun/dklabel.h ../../sys/sun/dkio.h ../../sys/sundev/screg.h
st.o: ../../sys/sunstand/saio.h ../../sys/sunstand/sasun.h
st.o: ../../sys/sun/dklabel.h ../../sys/sun/dkio.h ../../sys/sundev/screg.h
st.o: ../../sys/sundev/streg.h
xt.o: ../../sys/sunstand/saio.h ../../sys/sunstand/sasun.h
xt.o: ../../sys/sundev/xycreg.h ../../sys/sundev/xtreg.h
ttboot.o: ../../sys/sunstand/saio.h ../../sys/sunstand/sasun.h ../h/sasun.h
xxboot.o: ../../sys/sunstand/saio.h ../../sys/sunstand/sasun.h ../h/sasun.h
