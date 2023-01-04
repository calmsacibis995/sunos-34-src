#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 4.3 83/05/23
#

CFLAGS= -O

all:	spell hlist hlista hlistb hstop spellin spellout words

install:	all
	-mkdir ${DESTDIR}/usr/dict && chown bin ${DESTDIR}/usr/dict && \
		chmod 755 ${DESTDIR}/usr/dict
	install -s spell $(DESTDIR)/usr/lib
	install -s spellin ${DESTDIR}/usr/bin
	install -s spellout ${DESTDIR}/usr/bin
	install -c -m 644 hlista $(DESTDIR)/usr/dict
	install -c -m 644 hlistb $(DESTDIR)/usr/dict
	install -c -m 644 hstop $(DESTDIR)/usr/dict
	install -c -m 644 words $(DESTDIR)/usr/dict
	install -c spell.sh $(DESTDIR)/usr/bin/spell

clean:	
	rm -f *.o spell spellin hlista hlistb hlist hstop spellout

spell: spell.o
	cc spell.o -o spell
spellin: spellin.o
	cc spellin.o -o spellin
spellout: spellout.o
	cc spellout.o -o spellout

hlist: words spellin
	spellin <words >hlist
hlista: american local hlist spellin
	(cat american local)|spellin hlist >hlista
hlistb: british local hlist spellin
	(cat british local)|spellin hlist >hlistb
hstop: stop spellin
	spellin <stop >hstop
