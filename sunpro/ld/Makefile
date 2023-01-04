#
# @(#)Makefile	1.1 (Sun) 1/8/87 11:52:44
#


VROOT=../vroot
CPP_FLAGS=-Dsun3 -DSUNPRO -I$(VROOT)
CPP=/lib/cpp
CFLAGS  = -O $(CPP_FLAGS)
OBJS    = ld.o incl.o
PROGRAM = ld
HDRS= $(VROOT)/vroot.h $(VROOT)/report.h

${PROGRAM}:	${OBJS} $(VROOT)/vroot.a
	${CC} -o ${PROGRAM} ${OBJS} $(VROOT)/vroot.a

$(VROOT)/vroot.a $(HDRS):	FRC
	cd $(@D); $(MAKE) $(@F)
FRC:

install:	${PROGRAM}
	install -s ${PROGRAM} ${DESTDIR}/usr/sunpro/${PROGRAM}

clean:	
	rm -f ${PROGRAM} ${OBJS}

depend: $(OBJS:.o=.c) $(HDRS)
	rm -f makedep
	for i in $(OBJS:.o=.c); do \
	    ( $(CPP) -M $(CPP_FLAGS) $$i >> makedep ); \
	done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile
	rm -f eddep makedep

# DO NOT DELETE THIS LINE -- make depend uses it
ld.o: ld.c
ld.o: ld.c
ld.o: /usr/include/sys/param.h
ld.o: /usr/include/machine/param.h
ld.o: /usr/include/signal.h
ld.o: /usr/include/sys/types.h
ld.o: /usr/include/sys/sysmacros.h
ld.o: /usr/include/signal.h
ld.o: /usr/include/stdio.h
ld.o: /usr/include/ctype.h
ld.o: /usr/include/ar.h
ld.o: /usr/include/a.out.h
ld.o: /usr/include/sys/exec.h
ld.o: /usr/include/ranlib.h
ld.o: /usr/include/sys/stat.h
ld.o: /usr/include/sys/file.h
ld.o: /usr/include/sys/fcntl.h
ld.o: /usr/include/stab.h
ld.o: /usr/include/sys/dir.h
ld.o: ../vroot/vroot.h
ld.o: /usr/include/stdio.h
ld.o: ../vroot/report.h
ld.o: /usr/include/stdio.h
incl.o: incl.c
incl.o: /usr/include/stdio.h
incl.o: /usr/include/a.out.h
incl.o: /usr/include/sys/exec.h
incl.o: /usr/include/stab.h
incl.o: /usr/include/ctype.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
