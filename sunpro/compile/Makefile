# @(#)Makefile 1.2 87/01/08 SMI

all: compile

VROOT=../vroot
CPP_FLAGS = -I$(VROOT)
CPP=/lib/cpp
CFLAGS=  -O $(CPP_FLAGS)
LDFLAGS= 
LIBS =
SRCS = compile.c data.c run_pass.c setup.c
LINTS = $(SRCS:.c=.ln)
OBJS = $(SRCS:.c=.o)
LINTFLAGS = -x
INS = install

depend $(OBJS):	driver.h $(VROOT)/vroot.h $(VROOT)/report.h
$(VROOT)/vroot.h $(VROOT)/report.h $(VROOT)/vroot.a: FRC
	cd $(@D); $(MAKE) $(@F)

install:	compile
	$(INS) -s compile $(DESTDIR)/usr/sunpro/compile
	ln -s /lib/compile $(DESTDIR)/bin/cc
	ln -s /lib/compile $(DESTDIR)/usr/bin/f77
	ln -s /lib/compile $(DESTDIR)/usr/bin/lint
	ln -s /lib/compile $(DESTDIR)/usr/ucb/pc

data.o:	data.c
	$(CC) $(CFLAGS) -c data.c -R

compile:	$(OBJS) $(VROOT)/vroot.a
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) $(VROOT)/vroot.a

lint:	$(LINTS)
	lint $(LINTS) $(LINTFLAGS)

clean:
	rm -f $(OBJS) compile ocompile $(LINTS) *.BAK
FRC:

depend: $(SRCS) FRC
	rm -f makedep
	for i in $(SRCS); do \
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
compile.o: compile.c
compile.o: ./driver.h
compile.o: /usr/include/stdio.h
compile.o: /usr/include/sys/types.h
compile.o: /usr/include/sys/sysmacros.h
compile.o: /usr/include/sys/time.h
compile.o: /usr/include/time.h
compile.o: /usr/include/sys/resource.h
compile.o: ../vroot/vroot.h
compile.o: /usr/include/stdio.h
compile.o: ../vroot/report.h
compile.o: /usr/include/stdio.h
compile.o: /usr/include/sys/param.h
compile.o: /usr/include/machine/param.h
compile.o: /usr/include/signal.h
compile.o: /usr/include/sys/types.h
compile.o: /usr/include/sys/file.h
compile.o: /usr/include/sys/fcntl.h
compile.o: /usr/include/signal.h
compile.o: /usr/include/ctype.h
data.o: data.c
data.o: ./driver.h
data.o: /usr/include/stdio.h
data.o: /usr/include/sys/types.h
data.o: /usr/include/sys/sysmacros.h
data.o: /usr/include/sys/time.h
data.o: /usr/include/time.h
data.o: /usr/include/sys/resource.h
data.o: ../vroot/vroot.h
data.o: /usr/include/stdio.h
data.o: ../vroot/report.h
data.o: /usr/include/stdio.h
data.o: /usr/include/sys/param.h
data.o: /usr/include/machine/param.h
data.o: /usr/include/signal.h
data.o: /usr/include/sys/types.h
run_pass.o: run_pass.c
run_pass.o: ./driver.h
run_pass.o: /usr/include/stdio.h
run_pass.o: /usr/include/sys/types.h
run_pass.o: /usr/include/sys/sysmacros.h
run_pass.o: /usr/include/sys/time.h
run_pass.o: /usr/include/time.h
run_pass.o: /usr/include/sys/resource.h
run_pass.o: ../vroot/vroot.h
run_pass.o: /usr/include/stdio.h
run_pass.o: ../vroot/report.h
run_pass.o: /usr/include/stdio.h
run_pass.o: /usr/include/sys/param.h
run_pass.o: /usr/include/machine/param.h
run_pass.o: /usr/include/signal.h
run_pass.o: /usr/include/sys/types.h
run_pass.o: /usr/include/sys/file.h
run_pass.o: /usr/include/sys/fcntl.h
run_pass.o: /usr/include/sys/wait.h
run_pass.o: /usr/include/signal.h
run_pass.o: /usr/include/ctype.h
setup.o: setup.c
setup.o: ./driver.h
setup.o: /usr/include/stdio.h
setup.o: /usr/include/sys/types.h
setup.o: /usr/include/sys/sysmacros.h
setup.o: /usr/include/sys/time.h
setup.o: /usr/include/time.h
setup.o: /usr/include/sys/resource.h
setup.o: ../vroot/vroot.h
setup.o: /usr/include/stdio.h
setup.o: ../vroot/report.h
setup.o: /usr/include/stdio.h
setup.o: /usr/include/sys/param.h
setup.o: /usr/include/machine/param.h
setup.o: /usr/include/signal.h
setup.o: /usr/include/sys/types.h
setup.o: /usr/include/errno.h
setup.o: /usr/include/sys/errno.h
setup.o: /usr/include/sys/stat.h
setup.o: /usr/include/sys/file.h
setup.o: /usr/include/sys/fcntl.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
