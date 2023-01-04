# @(#)Makefile 1.3 86/12/01 SMI

.SUFFIXES:
.SUFFIXES: .o .ln _plain.S .S .S~ .c .c~ .LINT .syscall .syscall~ .h .h~

all: compile

CPPFLAGS = -I.
CFLAGS=  -O
LDFLAGS= 
LINKS = cc f77 lint m2c pc
XLINKS = xcc xf77 xlint xm2c xpc
LIBS =
SRCS = compile.c data.c run_pass.c setenv.c setup.c vroot.c
LINTS = compile.ln data.ln run_pass.ln setenv.ln setup.ln vroot.ln
OBJS = compile.o data.o run_pass.o setenv.o setup.o vroot.o
LINTFLAGS = -x
INS = install
compile.o data.o run_pass.o setenv.o setup.o vroot.o:	driver.h vroot.h SYS.h

install:	compile
	$(INS) -s compile $(DESTDIR)/lib/compile
	ln -s /lib/compile $(DESTDIR)/bin/cc
	ln -s /lib/compile $(DESTDIR)/usr/bin/f77
	ln -s /lib/compile $(DESTDIR)/usr/ucb/pc
	ln -s /lib/compile $(DESTDIR)/usr/bin/lint

data.o:	data.c
	$(CC) $(CFLAGS) -c data.c -R

compile:	$(OBJS) syscall.a
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) syscall.a

lint:	$(LINTS)
	lint $(LINTS) $(LINTFLAGS)

clean:
	rm -f $(OBJS) compile ocompile $(LINTS) *.BAK
	rm -f $(OBJECT_SYSCALL) $(OBJECT_SYSCALLP) $(OBJECT_SYSCALL_PV) \
		$(OBJECT_SYSCALL_PLAIN) $(OBJECT_SYSCALL_THUNK)
	rm -f $(SYSCALL) $(SYSCALLP) $(SYSCALL_PV) $(SYSCALL_PLAIN) $(SYSCALL_THUNK)
	rm -f $(SYSCALL:.c=.LINT) $(SYSCALL:.c=.h)
	rm -f gen gen.o syscall.a

cleansource:	clean
	rm -f $(LINKS) $(XLINKS)
	-sccs delta `sccs tell` < /dev/null
	sccs clean

help:
	cc -help | llist -small -name cc
	f77 -help | llist -small -name f77
	m2c -help | llist -small -name m2c
	pc -help | llist -small -name pc
	lint -help | llist -small -name lint
	compile -help | llist -small -name compile

edit:
	sccs edit $(SRCS) driver.h Makefile
save:
	sccs delget `sccs tell`

links:
	rm -f $(LINKS)
	-ln -s compile cc
	-ln -s compile f77
	-ln -s compile lint
	-ln -s compile m2c
	-ln -s compile pc
xlinks:
	rm -f $(XLINKS)
	-ln -s compile xcc
	-ln -s compile xf77
	-ln -s compile xlint
	-ln -s compile xm2c
	-ln -s compile xpc

SYSCALL = access.c execve.c open.c truncate.c rename.c
OBJECT_SYSCALL = access.o execve.o open.o truncate.o rename.o
SYSCALLP = accessp.c execvep.c openp.c truncatep.c renamep.c
OBJECT_SYSCALLP = accessp.o execvep.o openp.o truncatep.o renamep.o
SYSCALL_PV = access_pv.c execve_pv.c open_pv.c truncate_pv.c rename_pv.c
OBJECT_SYSCALL_PV = access_pv.o execve_pv.o open_pv.o truncate_pv.o rename_pv.o
SYSCALL_PLAIN = access_plain.S execve_plain.S open_plain.S truncate_plain.S rename_plain.S
OBJECT_SYSCALL_PLAIN = access_plain.o execve_plain.o open_plain.o truncate_plain.o rename_plain.o
SYSCALL_THUNK = access_thunk.c execve_thunk.c open_thunk.c truncate_thunk.c
OBJECT_SYSCALL_THUNK = access_thunk.o execve_thunk.o open_thunk.o truncate_thunk.o

$(SYSCALL) $(SYSCALLP) $(SYSCALL_PV) $(SYSCALL_PLAIN) $(SYSCALL_THUNK): gen

gen:	gen.o
	$(CC) -O -o $@ gen.o

.c~.c .h~.h .syscall~.syscall:
	sccs get $@ -G$@ > /dev/null

.syscall.c .syscall.S .syscall_plain.S:
	gen $*.syscall $(GENARGS)
.S.o:
	/lib/cpp $(CPPFLAGS) $< $<.tmp
	$(AS) $<.tmp -o $@
	rm -f $<.tmp

syscall.a:	$(OBJECT_SYSCALL) $(OBJECT_SYSCALLP) $(OBJECT_SYSCALL_PV) \
		$(OBJECT_SYSCALL_PLAIN) $(OBJECT_SYSCALL_THUNK)
	ar r $@ $(OBJECT_SYSCALL) $(OBJECT_SYSCALLP) $(OBJECT_SYSCALL_PV) \
		$(OBJECT_SYSCALL_PLAIN) $(OBJECT_SYSCALL_THUNK)
	ranlib $@

access.c access.h access_plain.S access_pv.c access_thunk.c accessp.c: access.syscall
execve.c execve.h execve_plain.S execve_pv.c execve_thunk.c execvep.c: execve.syscall
open.c open.h open_plain.S open_pv.c open_thunk.c openp.c:  open.syscall
truncate.c truncate.h truncate_plain.S truncate_pv.c truncate_thunk.c truncatep.c: truncate.syscall
rename.c rename_plain.S rename_pv.c renamep.c:    rename.syscall

