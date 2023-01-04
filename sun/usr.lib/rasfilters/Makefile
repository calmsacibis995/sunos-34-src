#
# @(#)Makefile 1.3 87/01/08 SMI
#
# makefile for /usr/lib/rasfilters
#

# definitions
SOURCES = convert.c
FILTERS = convert.0 convert.2

DESTDIR =
FILTERDIR = /usr/lib/rasfilters

CHOWN= chown -f
OWNER= bin

LIBS= ../libpixrect/libpixrect.a

DEBUG=
INC= 
OPTIM= -O
CFLAGS = $(OPTIM) $(DEBUG) $(INC)


all: $(FILTERS)

convert.0: convert.c $(LIBS)
	$(CC) $(CFLAGS) -DMY_RAS_TYPE=0 convert.c $(LIBS) -o $@

convert.2: convert.c $(LIBS)
	$(CC) $(CFLAGS) -DMY_RAS_TYPE=2 convert.c $(LIBS) -o $@

$(DESTDIR)$(FILTERDIR):
	-set +e ; \
	if [ ! -d $@ ] ; then \
		mkdir $@ ; chmod 755 $@ ; \
	fi ; \
	$(CHOWN) $(OWNER) $@

install: $(DESTDIR)$(FILTERDIR) $(FILTERS)
	-set +e ; \
	for i in $(FILTERS); do \
		install -c -s -m 755 -o $(OWNER) \
			$$i $(DESTDIR)$(FILTERDIR)/$$i; \
	done ; \
	install -c -m 644 -o $(OWNER) \
		convert.c $(DESTDIR)$(FILTERDIR)/convert.2.c

clean:
	rm -f a.out core errs *.o $(FILTERS)

tags:
	ctags -w *.h *.c
