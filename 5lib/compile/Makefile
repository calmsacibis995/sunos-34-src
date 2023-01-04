# @(#)Makefile 1.1 86/09/24 SMI

.SUFFIXES:
.SUFFIXES: .o .ln _plain.S .S .S~ .c .c~ .LINT .syscall .syscall~ .h .h~

all: compile

SRCDIR=../../lib/compile
CPPFLAGS = -I$(SRCDIR)
CFLAGS=  -O -DS5EMUL $(CPPFLAGS)
LDFLAGS= 
LINKS = cc f77 lint m2c pc
XLINKS = xcc xf77 xlint xm2c xpc
LIBS =
SRCS = compile.c data.c run_pass.c setenv.c setup.c vroot.c
LINTS = compile.ln data.ln run_pass.ln setenv.ln setup.ln vroot.ln
OBJS = compile.o data.o run_pass.o setenv.o setup.o vroot.o
LINTFLAGS = -x
INS = install

compile.o: $(SRCDIR)/driver.h $(SRCDIR)/vroot.h $(SRCDIR)/SYS.h \
	$(SRCDIR)/syscall.h $(SRCDIR)/compile.c
	$(CC) -c $(CFLAGS) $(SRCDIR)/compile.c

run_pass.o: $(SRCDIR)/driver.h $(SRCDIR)/vroot.h $(SRCDIR)/SYS.h \
	$(SRCDIR)/syscall.h $(SRCDIR)/run_pass.c
	$(CC) -c $(CFLAGS) $(SRCDIR)/run_pass.c

setenv.o: $(SRCDIR)/driver.h $(SRCDIR)/vroot.h $(SRCDIR)/SYS.h \
	$(SRCDIR)/syscall.h $(SRCDIR)/setenv.c
	$(CC) -c $(CFLAGS) $(SRCDIR)/setenv.c

setup.o: $(SRCDIR)/driver.h $(SRCDIR)/vroot.h $(SRCDIR)/SYS.h \
	$(SRCDIR)/syscall.h $(SRCDIR)/setup.c
	$(CC) -c $(CFLAGS) $(SRCDIR)/setup.c

vroot.o: $(SRCDIR)/driver.h $(SRCDIR)/vroot.h $(SRCDIR)/SYS.h \
	$(SRCDIR)/syscall.h $(SRCDIR)/vroot.c
	$(CC) -c $(CFLAGS) $(SRCDIR)/vroot.c

install:	compile
	$(INS) -s compile $(DESTDIR)/usr/5lib/compile

data.o: $(SRCDIR)/driver.h $(SRCDIR)/vroot.h $(SRCDIR)/SYS.h \
	$(SRCDIR)/syscall.h $(SRCDIR)/data.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/data.c -R

compile:	$(OBJS) $(SRCDIR)/syscall.a
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) $(SRCDIR)/syscall.a

clean:
	rm -f $(OBJS) compile ocompile $(LINTS) *.BAK

help:
	cc -help | llist -small -name cc
	f77 -help | llist -small -name f77
	m2c -help | llist -small -name m2c
	pc -help | llist -small -name pc
	lint -help | llist -small -name lint
	compile -help | llist -small -name compile

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

