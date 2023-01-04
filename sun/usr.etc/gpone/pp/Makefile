#
# @(#)Makefile  1.1  86/09/25
#

CPP = /lib/cpp
HDRS = pp.h s1152.pp.h s1024.pp.h macros.pp.h

UFILES = pp.u mul.pp.u mulchip.pp.u m1152.pp.u m1024.pp.u div.pp.u \
	ack.pp.u zoom.pp.u init.pp.u ropnull.pp.u ropfb.pp.u vector.pp.u \
	octant.pp.u stclip.pp.u colormap.pp.u gbuffer.pp.u shade.pp.u \
	delay.pp.u texvector.pp.u texvec.pp.u setpat.pp.u offset.pp.u \
	fatvec.pp.u polyline.pp.u xfline.pp.u xflineinit.pp.u \
	setlineatts.pp.u roptex.pp.u clipoff.pp.u

all:	gp1cg2.1024.pp.o gp1cg2.1152.pp.o

gp1cg2.1024.pp.o: ${HDRS} ${UFILES} ppas
	rm -f gp1cg2.1024.pp.list gp1cg2.1024.pp.o
	$(CPP) -DSWIDTH=1024 pp.u | ppas gp1cg2.1024.pp.o > gp1cg2.1024.pp.list

gp1cg2.1152.pp.o: ${HDRS} ${UFILES} ppas
	rm -f gp1cg2.1152.pp.list gp1cg2.1152.pp.o
	$(CPP) -DSWIDTH=1152 pp.u | ppas gp1cg2.1152.pp.o > gp1cg2.1152.pp.list

clean:
	rm -f core gp1cg2.1024.pp.list gp1cg2.1152.pp.list
	rm -f gp1cg2.1024.pp.o gp1cg2.1152.pp.o

spotless:
	rm -f junk* tmp* temp* *.BAK *.CKP .emacs_* a.out
	make clean

install:
	echo 'Do nothing here'

depend:
	cat </dev/null >xxx.u
	for i in ${UFILES}; do \
		(/bin/grep '^#[ 	]*include' xxx.u $$i | sed \
			-e '/\.\.\/h/d' \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.c//' >>makedep); done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep xxx.u
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it
# DEPENDENCIES MUST END AT END OF FILE
pp.u: pp.h
pp.u: mulchip.pp.u
pp.u: mul.pp.u
pp.u: div.pp.u
pp.u: ack.pp.u
pp.u: zoom.pp.u
pp.u: init.pp.u
pp.u: ropnull.pp.u
pp.u: ropfb.pp.u
pp.u: vector.pp.u
pp.u: stclip.pp.u
pp.u: octant.pp.u
pp.u: colormap.pp.u
pp.u: gbuffer.pp.u
pp.u: shade.pp.u
pp.u: delay.pp.u
mul.pp.u: m1152.pp.u
mul.pp.u: m1024.pp.u
mulchip.pp.u: m1152.pp.u
mulchip.pp.u: m1024.pp.u
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
