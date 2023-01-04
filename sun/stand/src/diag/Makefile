#
# @(#)Makefile 1.21 84/12/22 SMI
#
DESTDIR=/
CFLAGS=	-O -D`arch`
LFLAGS=	-D`arch`
OFILES=	stack.o diag.o label.o ip.o xy.o prf.o machdep.o sd.o probe.o xd.o\
	def.o defutil.o nft.o xyxd.o
CFILES=	diag.c label.c ip.c xy.c prf.c machdep.c sd.c xd.c def.c defutil.c\
	nft.c xyxd.c
SFILES= stack.s probe.s
SRCS=	diag.h $(CFILES) $(SFILES)
TOUCH=	touch -cf

all:	diag

diag:	$(OFILES)
	ld -T 4000 -o diag $(OFILES) -lc 

tags:	$(CFILES)
	ctags $(CFILES)

lint:
	lint -ubxn $(LFLAGS) $(CFILES) > lint.errs 2>&1

clean:
	rm -f diag *.o *errs tags

install:	diag
	install -s diag ${DESTDIR}/stand

print:
	pr $(SRCS) | lpr

depend:
	cp /dev/null x.c
	for i in $(SRCS) ; do \
		(/bin/grep '^#[ 	]*include' x.c $$i | sed \
			-e '/\.\.\//d' \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.c/\.o/' >>makedep); done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep x.c
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

diag.h:
	$(TOUCH) diag.h

# DO NOT DELETE THIS LINE -- make depend uses it

diag.h: /usr/include/machine/param.h
diag.h: /usr/include/mon/sunromvec.h
diag.h: /usr/include/setjmp.h
diag.o: diag.h
diag.o: /usr/include/sys/types.h
diag.o: /usr/include/sun/dkio.h
diag.o: /usr/include/sys/dkbad.h
label.o: /usr/include/sys/types.h
label.o: diag.h
label.o: /usr/include/sun/dklabel.h
ip.o: diag.h
xy.o: diag.h
xy.o: /usr/include/sys/types.h
xy.o: /usr/include/sundev/xyreg.h
xy.o: /usr/include/sundev/xycreg.h
xy.o: xyxd.h
prf.o: diag.h
prf.o: /usr/include/sys/types.h
machdep.o: diag.h
machdep.o: /usr/include/sys/types.h
machdep.o: /usr/include/sys/vmmac.h
machdep.o: /usr/include/sun2/mmu.h
machdep.o: /usr/include/sun2/cpu.h
machdep.o: /usr/include/sun2/pte.h
machdep.o: /usr/include/sun3/mmu.h
machdep.o: /usr/include/sun3/cpu.h
machdep.o: /usr/include/sun3/pte.h
machdep.o: /usr/include/sun3/clock.h
machdep.o: /usr/include/sun3/interreg.h
sd.o: diag.h
sd.o: /usr/include/sys/types.h
sd.o: /usr/include/sys/dkbad.h
sd.o: /usr/include/sys/buf.h
sd.o: /usr/include/sun/dklabel.h
sd.o: /usr/include/sun/dkio.h
sd.o: /usr/include/sundev/screg.h
sd.o: /usr/include/sundev/sireg.h
sd.o: /usr/include/sundev/scsi.h
xd.o: diag.h
xd.o: /usr/include/sys/types.h
xd.o: /usr/include/sundev/xdreg.h
xd.o: /usr/include/sundev/xdcreg.h
xd.o: xyxd.h
def.o: diag.h
def.o: /usr/include/sys/types.h
def.o: /usr/include/sun/dkio.h
def.o: /usr/include/sys/dkbad.h
def.o: def.h
defutil.o: diag.h
defutil.o: /usr/include/sys/types.h
defutil.o: def.h
nft.o: diag.h
nft.o: /usr/include/sys/types.h
nft.o: /usr/include/sun/dkio.h
nft.o: /usr/include/sys/dkbad.h
nft.o: def.h
nft.o: xyxd.h
xyxd.o: diag.h
xyxd.o: /usr/include/sys/types.h
xyxd.o: /usr/include/sundev/xyreg.h
xyxd.o: /usr/include/sundev/xycreg.h
xyxd.o: /usr/include/sundev/xdreg.h
xyxd.o: /usr/include/sundev/xdcreg.h
xyxd.o: xyxd.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
