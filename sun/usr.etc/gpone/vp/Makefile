#
# @(#)Makefile 1.1 86/09/25 SMI
#

CPP=	/lib/cpp
UFILES= main.vp.u pixrect.vp.u ropnull.vp.u ropfb.vp.u attributes.vp.u \
        recip.vp.u cgi.vp.u 2Dfp.vp.u clpvec2.vp.u xfpolygon2.vp.u 3Dfp.vp.u \
	clpvec3.vp.u xfpolygon3.vp.u polyscan.vp.u polyscan3.vp.u \
	gbuffer.vp.u multiply.vp.u tex.vp.u roptex.vp.u

HDRS=	vp.h 

all:	gp1cg2.1024.vp.o gp1cg2.1152.vp.o

gp1cg2.1024.vp.o: ${HDRS} ${UFILES} vpas
	rm -f gp1cg2.1024.vp.list gp1cg2.1024.vp.o
	$(CPP) -DSWIDTH=1024 main.vp.u | vpas gp1cg2.1024.vp.o > gp1cg2.1024.vp.list

gp1cg2.1152.vp.o: ${HDRS} ${UFILES} vpas
	rm -f gp1cg2.1152.vp.list gp1cg2.1152.vp.o
	$(CPP) -DSWIDTH=1152  main.vp.u | vpas gp1cg2.1152.vp.o > gp1cg2.1152.vp.list

clean:
	rm -f core gp1cg2.1024.vp.list gp1cg2.1152.vp.list
	rm -f gp1cg2.1024.vp.o gp1cg2.1152.vp.o

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
main.vp.u: vp.h
main.vp.u: pixrect.vp.u
main.vp.u: attributes.vp.u
main.vp.u: recip.vp.u
main.vp.u: cgi.vp.u
main.vp.u: 2Dfp.vp.u
main.vp.u: 3Dfp.vp.u
main.vp.u: polyscan.vp.u
main.vp.u: gbuffer.vp.u
pixrect.vp.u: ropnull.vp.u
pixrect.vp.u: ropfb.vp.u
2Dfp.vp.u: clpvec2.vp.u
2Dfp.vp.u: xfpolygon2.vp.u
3Dfp.vp.u: clpvec3.vp.u
3Dfp.vp.u: xfpolygon3.vp.u
polyscan.vp.u: polyscan3.vp.u
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
