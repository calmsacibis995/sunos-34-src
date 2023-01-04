#
# @(#)Makefile 1.1 86/09/25 SMI
#
# Copyright (c) 1986 by Sun Microsystems, Inc.
#

.SUFFIXES: .s .il

CPP 	= /lib/cpp

SOURCES	 = common.s fswitch.s fsoft.s fsky.s f68881.s ffpa.s

OBJECTS	= fswitch.il fsoft.il fsky.il f68881.il ffpa.il

all:	$(OBJECTS)

fswitch.il:	common.s fswitch.s
	$(CPP) -P -DP=V common.s | cat fswitch.s - > fswitch.tmp
	mv fswitch.tmp fswitch.il

fsoft.il:	common.s fsoft.s
	$(CPP) -P -DP=F common.s | cat fsoft.s - > fsoft.tmp
	mv fsoft.tmp fsoft.il

fsky.il:	common.s fsky.s
	$(CPP) -P -DP=S common.s | cat fsky.s - > fsky.tmp
	mv fsky.tmp fsky.il

f68881.il:	common.s f68881.s
	$(CPP) -P -DP=M common.s | cat f68881.s - > f68881.tmp
	mv f68881.tmp f68881.il

ffpa.il:	common.s ffpa.s
	$(CPP) -P -DP=W common.s | cat ffpa.s - > ffpa.tmp
	mv ffpa.tmp ffpa.il

clean:
	rm -f $(OBJECTS)

install:	$(OBJECTS)
	for i in $(OBJECTS) ; do cp $$i $(DESTDIR)/usr/lib/$$i ; done ;
