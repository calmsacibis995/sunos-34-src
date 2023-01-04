#
# @(#)Makefile 1.5 87/04/17 SMI; Remotely from S5R2 1.7
#

CPP=/lib/cpp
LDFLAGS =
INSDIR = $(DESTDIR)/bin
LIBS = ../vroot/vroot.a
CPP_FLAGS=-I../vroot
CFLAGS = -O $(CPP_FLAGS)

SOURCES = ar.c doname.c dosys.c files.c macro.c main.c misc.c read.c state.c
OBJECTS =  $(SOURCES:.c=.o)

all:  make

make:  default.mk $(OBJECTS) ../vroot/vroot.a
	$(CC) -o make $(LDFLAGS) $(OBJECTS) $(LIBS)

$(OBJECTS):  defs.h funny.h ../vroot/report.h ../vroot/vroot.h

../vroot/vroot.a ../vroot/report.h ../vroot/vroot.h:	FRC
	cd $(@D); $(MAKE) $(@F)

install: all default.mk make.1
	install -c -s make $(DESTDIR)/usr/sunpro
	cp make.1 $(DESTDIR)/usr/sunpro/make.1
	cp default.mk $(DESTDIR)/usr/sunpro/default.mk
make.1 default.mk:
	sccs get $@

clean:
	-rm -f $(OBJECTS) a.out core errs make

tags:	FRC
	ctags *.[ch]

FRC:

lint:	$(SOURCES:.c=.ln)
	lint $(SOURCES:.c=.ln)

.c.ln:
	lint -i $(TARGET_ARCH) -I../vroot $(@:.ln=.c)

depend: $(SOURCES)
	rm -f makedep
	for i in $(SOURCES); do \
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
# DEPENDENCIES MUST END AT END OF FILE
ar.o: ar.c
ar.o: ./defs.h
ar.o: /usr/include/stdio.h
ar.o: /usr/include/sys/param.h
ar.o: /usr/include/machine/param.h
ar.o: /usr/include/signal.h
ar.o: /usr/include/sys/types.h
ar.o: /usr/include/sys/sysmacros.h
ar.o: /usr/include/sys/time.h
ar.o: /usr/include/time.h
ar.o: ../vroot/vroot.h
ar.o: /usr/include/stdio.h
ar.o: /usr/include/ar.h
ar.o: /usr/include/fcntl.h
ar.o: /usr/include/ranlib.h
ar.o: /usr/include/ctype.h
doname.o: doname.c
doname.o: ./defs.h
doname.o: /usr/include/stdio.h
doname.o: /usr/include/sys/param.h
doname.o: /usr/include/machine/param.h
doname.o: /usr/include/signal.h
doname.o: /usr/include/sys/types.h
doname.o: /usr/include/sys/sysmacros.h
doname.o: /usr/include/sys/time.h
doname.o: /usr/include/time.h
doname.o: ../vroot/vroot.h
doname.o: /usr/include/stdio.h
doname.o: ./funny.h
doname.o: /usr/include/sys/file.h
doname.o: /usr/include/sys/fcntl.h
doname.o: /usr/include/ctype.h
doname.o: /usr/include/values.h
dosys.o: dosys.c
dosys.o: ./defs.h
dosys.o: /usr/include/stdio.h
dosys.o: /usr/include/sys/param.h
dosys.o: /usr/include/machine/param.h
dosys.o: /usr/include/signal.h
dosys.o: /usr/include/sys/types.h
dosys.o: /usr/include/sys/sysmacros.h
dosys.o: /usr/include/sys/time.h
dosys.o: /usr/include/time.h
dosys.o: ../vroot/vroot.h
dosys.o: /usr/include/stdio.h
dosys.o: /usr/include/sys/wait.h
dosys.o: /usr/include/sys/signal.h
dosys.o: /usr/include/errno.h
dosys.o: /usr/include/sys/errno.h
dosys.o: /usr/include/ctype.h
files.o: files.c
files.o: ./defs.h
files.o: /usr/include/stdio.h
files.o: /usr/include/sys/param.h
files.o: /usr/include/machine/param.h
files.o: /usr/include/signal.h
files.o: /usr/include/sys/types.h
files.o: /usr/include/sys/sysmacros.h
files.o: /usr/include/sys/time.h
files.o: /usr/include/time.h
files.o: ../vroot/vroot.h
files.o: /usr/include/stdio.h
files.o: /usr/include/sys/stat.h
files.o: /usr/include/sys/file.h
files.o: /usr/include/sys/fcntl.h
files.o: /usr/include/sys/dir.h
files.o: /usr/include/errno.h
files.o: /usr/include/sys/errno.h
files.o: /usr/include/ctype.h
macro.o: macro.c
macro.o: ./defs.h
macro.o: /usr/include/stdio.h
macro.o: /usr/include/sys/param.h
macro.o: /usr/include/machine/param.h
macro.o: /usr/include/signal.h
macro.o: /usr/include/sys/types.h
macro.o: /usr/include/sys/sysmacros.h
macro.o: /usr/include/sys/time.h
macro.o: /usr/include/time.h
macro.o: ../vroot/vroot.h
macro.o: /usr/include/stdio.h
macro.o: /usr/include/ctype.h
main.o: main.c
main.o: ./defs.h
main.o: /usr/include/stdio.h
main.o: /usr/include/sys/param.h
main.o: /usr/include/machine/param.h
main.o: /usr/include/signal.h
main.o: /usr/include/sys/types.h
main.o: /usr/include/sys/sysmacros.h
main.o: /usr/include/sys/time.h
main.o: /usr/include/time.h
main.o: ../vroot/vroot.h
main.o: /usr/include/stdio.h
main.o: ../vroot/report.h
main.o: /usr/include/stdio.h
main.o: /usr/include/signal.h
main.o: /usr/include/values.h
misc.o: misc.c
misc.o: ./defs.h
misc.o: /usr/include/stdio.h
misc.o: /usr/include/sys/param.h
misc.o: /usr/include/machine/param.h
misc.o: /usr/include/signal.h
misc.o: /usr/include/sys/types.h
misc.o: /usr/include/sys/sysmacros.h
misc.o: /usr/include/sys/time.h
misc.o: /usr/include/time.h
misc.o: ../vroot/vroot.h
misc.o: /usr/include/stdio.h
misc.o: ./funny.h
read.o: read.c
read.o: ./defs.h
read.o: /usr/include/stdio.h
read.o: /usr/include/sys/param.h
read.o: /usr/include/machine/param.h
read.o: /usr/include/signal.h
read.o: /usr/include/sys/types.h
read.o: /usr/include/sys/sysmacros.h
read.o: /usr/include/sys/time.h
read.o: /usr/include/time.h
read.o: ../vroot/vroot.h
read.o: /usr/include/stdio.h
read.o: /usr/include/ctype.h
read.o: /usr/include/fcntl.h
state.o: state.c
state.o: ./defs.h
state.o: /usr/include/stdio.h
state.o: /usr/include/sys/param.h
state.o: /usr/include/machine/param.h
state.o: /usr/include/signal.h
state.o: /usr/include/sys/types.h
state.o: /usr/include/sys/sysmacros.h
state.o: /usr/include/sys/time.h
state.o: /usr/include/time.h
state.o: ../vroot/vroot.h
state.o: /usr/include/stdio.h
state.o: /usr/include/setjmp.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
