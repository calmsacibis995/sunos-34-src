#
# @(#)Makefile.m4 1.1 86/09/25 SMI; from UCB 3.6 02/18/83
#
#
#  Makefile for assorted programs related (perhaps distantly) to Sendmail.
#
include(../md/config.m4)dnl

ALL=	mailstats	mconnect   vacation 	Makefile
SRCS=	mailstats.c	mconnect.c vacation.c

LIBS=	m4LIBS
DBMLIB=	-ldbm
CONVTIME=../src/convtime.o
DESTDIR=

CHOWN=	-echo chown
CHMOD=	chmod
O=	-O
COPTS=
CCONFIG=-I../`include' -I../src m4CONFIG
CFLAGS=	$O $(COPTS) $(CCONFIG)
AR=	-ar
ARFLAGS=rvu
LINT=	lint
XREF=	ctags -x
CP=	cp
MV=	mv
INSTALL=install -c -s
M4=	m4
TOUCH=	touch
ABORT=	false

GET=	sccs get
DELTA=	sccs delta
WHAT=	sccs what
PRT=	sccs prt
REL=

ROOT=	root
OBJMODE=755

all: $(ALL)

Makefile: Makefile.m4
	m4 Makefile.m4 >Makefile

mailstats: mailstats.o
	cc $(COPTS) -o $@ $@.o

logger: logger.o
	cc $(COPTS) -o $@ $@.o $(LIBS)

mconnect: mconnect.o
	cc $(COPTS) -o $@ $@.o

praliases: praliases.o
	cc $(COPTS) -o $@ $@.o

vacation: vacation.o
	cc $(COPTS) $(DBMLIB) -o $@ $@.o $(CONVTIME)

sources: $(SRCS)

$(SRCS):
	$(GET) $(REL) SCCS/s.$@

clean:
	rm -f $(ALL) core a.out make.out lint.out errs
	rm -f *.o ,*
