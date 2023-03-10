#
# @(#)Makefile 1.1 86/09/24 SMI
#
# Hack or Quest Makefile.

# on some systems the termcap library is in -ltermcap
TERMLIB = -ltermlib

# make hack
GAME = hack
GAMEDIR = /usr/games/lib/hackdir
CFLAGS = -O
HACKOWNER = daemon
HACKCSRC = hack.Decl.c\
	hack.apply.c hack.bones.c hack.c hack.cmd.c hack.do.c\
	hack.do_name.c hack.do_wear.c hack.dog.c hack.eat.c hack.end.c\
	hack.engrave.c hack.fight.c hack.invent.c hack.ioctl.c\
	hack.lev.c hack.main.c hack.makemon.c hack.mhitu.c\
	hack.mklev.c hack.mkmaze.c hack.mkobj.c hack.mkshop.c\
	hack.mon.c hack.monst.c hack.o_init.c hack.objnam.c\
	hack.options.c hack.pager.c hack.potion.c hack.pri.c\
	hack.read.c hack.rip.c hack.rumors.c hack.save.c\
	hack.search.c hack.shk.c hack.shknam.c hack.steal.c\
	hack.termcap.c hack.timeout.c hack.topl.c\
	hack.track.c hack.trap.c hack.tty.c hack.unix.c\
	hack.u_init.c hack.vault.c\
	hack.wield.c hack.wizard.c hack.worm.c hack.worn.c hack.zap.c\
	hack.version.c rnd.c alloc.c

CSOURCES = $(HACKCSRC) makedefs.c

HSOURCES = hack.h hack.mfndpos.h config.h\
	def.edog.h def.eshk.h def.flag.h def.func_tab.h def.gold.h\
	def.mkroom.h\
	def.monst.h def.obj.h def.objclass.h def.objects.h\
	def.permonst.h def.rm.h def.trap.h def.wseg.h

SOURCES = $(CSOURCES) $(HSOURCES)

AUX = data help hh rumors

HOBJ = hack.Decl.o hack.apply.o hack.bones.o hack.o hack.cmd.o hack.do.o\
	hack.do_name.o hack.do_wear.o hack.dog.o hack.eat.o hack.end.o\
	hack.engrave.o hack.fight.o hack.invent.o hack.ioctl.o\
	hack.lev.o hack.main.o hack.makemon.o hack.mhitu.o hack.mklev.o\
	hack.mkmaze.o hack.mkobj.o hack.mkshop.o hack.mon.o\
	hack.monst.o hack.o_init.o hack.objnam.o hack.options.o\
	hack.pager.o hack.potion.o hack.pri.o\
	hack.read.o hack.rip.o hack.rumors.o hack.save.o\
	hack.search.o hack.shk.o hack.shknam.o hack.steal.o\
	hack.termcap.o hack.timeout.o hack.topl.o\
	hack.track.o hack.trap.o\
	hack.tty.o hack.unix.o hack.u_init.o hack.vault.o hack.wield.o\
	hack.wizard.o hack.worm.o hack.worn.o hack.zap.o\
	hack.version.o rnd.o alloc.o

$(GAME):	$(HOBJ)
	@echo "Loading ..."
	@ld -X -o $(GAME) /lib/crt0.o $(HOBJ) $(TERMLIB) -lc

clean:
	rm -rf $(HOBJ) $(GAME) hack.onames.h makedefs

makedefs:	makedefs.c
	cc -o makedefs makedefs.c


hack.onames.h:	makedefs def.objects.h
	makedefs > hack.onames.h

lint:
# lint cannot have -p here because (i) capitals are meaningful:
# [Ww]izard, (ii) identifiers may coincide in the first six places:
# doweararm() versus dowearring().
# _flsbuf comes from <stdio.h>, a bug in the system libraries.
	@echo lint -axbh -DLINT ...
	@lint -axbh -DLINT $(HACKCSRC) | sed '/_flsbuf/d'


diff:
	@- for i in $(SOURCES) $(AUX) ; do \
		cmp -s $$i $D/$$i || \
		( echo diff $D/$$i $$i ; diff $D/$$i $$i ; echo ) ; done

distribution: Makefile
	@- for i in READ_ME $(SOURCES) $(AUX) Makefile date.h hack.onames.h\
		; do \
		cmp -s $$i $D/$$i || \
		( echo cp $$i $D ; cp $$i $D ) ; done
# the distribution directory also contains the empty files
# perm and record, and the informative files Porting and Bugs.


install:
	-mkdir $(DESTDIR)/$(GAMEDIR)
	chown $(HACKOWNER) $(DESTDIR)/$(GAMEDIR)
	chmod 700 $(DESTDIR)/$(GAMEDIR)
	install -m 4711 -s -o $(HACKOWNER) hack $(DESTDIR)/usr/games/$(GAME)
	cp  $(AUX)  $(DESTDIR)/$(GAMEDIR)
	touch $(DESTDIR)/$(GAMEDIR)/perm
	touch $(DESTDIR)/$(GAMEDIR)/record
	cd $(DESTDIR)/$(GAMEDIR); chown $(HACKOWNER) *
	rm -f $(DESTDIR)/$(GAMEDIR)/bones*


depend:
# For the moment we are lazy and disregard /usr/include files because
# the sources contain them conditionally. Perhaps we should use cpp.
#		( /bin/grep '^#[ 	]*include' $$i | sed -n \
#			-e 's,<\(.*\)>,"/usr/include/\1",' \
#
	for i in ${CSOURCES}; do \
		( /bin/grep '^#[ 	]*include[ 	]*"' $$i | sed -n \
			-e 's/[^"]*"\([^"]*\)".*/\1/' \
			-e H -e '$$g' -e '$$s/\n/ /g' \
			-e '$$s/.*/'$$i': &/' -e '$$s/\.c:/.o:/p' \
			>> makedep); done
	for i in ${HSOURCES}; do \
		( /bin/grep '^#[ 	]*include[ 	]*"' $$i | sed -n \
			-e 's/[^"]*"\([^"]*\)".*/\1/' \
			-e H -e '$$g' -e '$$s/\n/ /g' \
			-e '$$s/.*/'$$i': &\
				touch '$$i/p \
			>> makedep); done
	@echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	@echo '$$r makedep' >>eddep
	@echo 'w' >>eddep
	@cp Makefile Makefile.bak
	ed - Makefile < eddep
	@rm -f eddep makedep
	@echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	@echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	@echo '# see make depend above' >> Makefile
	- diff Makefile Makefile.bak
	@rm -f Makefile.bak

# DO NOT DELETE THIS LINE

hack.Decl.o:  hack.h def.mkroom.h
hack.apply.o:  hack.h def.edog.h def.mkroom.h
hack.bones.o:  hack.h
hack.o:  hack.h
hack.cmd.o:  hack.h def.func_tab.h
hack.do.o:  hack.h
hack.do_name.o:  hack.h
hack.do_wear.o:  hack.h
hack.dog.o:  hack.h hack.mfndpos.h def.edog.h def.mkroom.h
hack.eat.o:  hack.h
hack.end.o:  hack.h
hack.engrave.o:  hack.h
hack.fight.o:  hack.h
hack.invent.o:  hack.h def.wseg.h
hack.ioctl.o:  config.h
hack.lev.o:  hack.h def.mkroom.h def.wseg.h
hack.main.o:  hack.h
hack.makemon.o:  hack.h
hack.mhitu.o:  hack.h
hack.mklev.o:  hack.h def.mkroom.h
hack.mkmaze.o:  hack.h def.mkroom.h
hack.mkobj.o:  hack.h
hack.mkshop.o:  hack.h def.mkroom.h def.eshk.h
hack.mon.o:  hack.h hack.mfndpos.h
hack.monst.o:  hack.h def.eshk.h
hack.o_init.o:  config.h def.objects.h hack.onames.h
hack.objnam.o:  hack.h
hack.options.o:  config.h hack.h
hack.pager.o:  hack.h
hack.potion.o:  hack.h
hack.pri.o:  hack.h def.wseg.h
hack.read.o:  hack.h
hack.rip.o:  hack.h
hack.rumors.o:  config.h
hack.save.o:  hack.h
hack.search.o:  hack.h
hack.shk.o:  hack.h hack.mfndpos.h def.mkroom.h def.eshk.h
hack.shknam.o:  hack.h
hack.steal.o:  hack.h
hack.termcap.o:  config.h def.flag.h
hack.timeout.o:  hack.h
hack.topl.o:  hack.h
hack.track.o:  hack.h
hack.trap.o:  hack.h def.mkroom.h
hack.tty.o:  hack.h
hack.unix.o:  hack.h def.mkroom.h
hack.u_init.o:  hack.h
hack.vault.o:  hack.h def.mkroom.h
hack.wield.o:  hack.h
hack.wizard.o:  hack.h
hack.worm.o:  hack.h def.wseg.h
hack.worn.o:  hack.h
hack.zap.o:  hack.h
hack.version.o:  date.h
hack.h:  config.h def.objclass.h def.monst.h def.gold.h def.trap.h def.obj.h def.flag.h def.rm.h def.permonst.h hack.onames.h
			-touch hack.h
def.objects.h:  config.h def.objclass.h
			touch def.objects.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
