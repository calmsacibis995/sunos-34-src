#
# @(#)makefile 1.1 86/09/24 SMI; from S5R2 1.3	(1.9	12/28/82)
#
# There is currently no provision for local mods (reorder) or
# user mods ($TERMCAP).  These will be coming eventually.
#
# PARTS=	header \
# 	adds annarbor beehive cdc concept datamedia dec diablo general \
# 	hardcopy hazeltine heath homebrew hp ibm lsi microterm misc \
# 	pc perkinelmer print special \
# 	tektronix teleray teletype televideo ti visual \
# 	trailer

PARTS=	header *.ti trailer
COMPILE=../screen/tic

install: $(PARTS)
	TERMINFO=$(DESTDIR)/usr/5lib/terminfo $(COMPILE) $?;
	make terminfo.src
	touch install

terminfo.src:	$(PARTS)
	cat $(PARTS) > terminfo.src

all:	terminfo.src
	TERMINFO=$(DESTDIR)/usr/5lib/terminfo $(COMPILE) -v terminfo.src;
	touch install

clean:
	-rm -f terminfo.src install
	-rm -f errs

VGRIND=/usr/ucb/vgrind
vgrind:
	${VGRIND} -n -h "Terminal Information Data Base" $(PARTS)
