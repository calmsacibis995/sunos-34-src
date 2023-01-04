#
# @(#)Makefile 1.1 86/09/24 SMI; from S5R2 1.11
#
CFLAGS=-O 
LNAME = libPW.a

OBJECTS =	abspath.o anystr.o bal.o curdir.o fdfopen.o giveup.o \
	imatch.o index.o lockit.o logname.o move.o patoi.o \
	patol.o regcmp.o regex.o rename.o repeat.o repl.o satoi.o \
	setsig.o sname.o strend.o substr.o trnslat.o userdir.o \
	username.o verify.o any.o xalloc.o xcreat.o xlink.o \
	xopen.o xpipe.o xunlink.o xwrite.o xmsg.o \
	cat.o dname.o fatal.o clean.o userexit.o zero.o zeropad.o

all:	$(LNAME)

$(LNAME):	$(OBJECTS)
	-rm -f $(LNAME)
	ar r $(LNAME) $(OBJECTS)

install:	$(LNAME)
	install -c -m 644 $(LNAME) $(DESTDIR)/usr/5lib
	ranlib $(DESTDIR)/usr/5lib/$(LNAME)

clean:
	-rm -f $(OBJECTS)
	-rm -f $(LNAME)
