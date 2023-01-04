#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 3.11 07/09/83
#
#
#	Makefile for sendmail base directory
#

ALL=	src/sendmail cf/main.cf cf/subsidiary.cf lib/sendmail.hf lib/aliases
DIRS=	$(DESTDIR)/usr/spool/mqueue $(DESTDIR)/usr/spool/mail
SENDMAIL=$(DESTDIR)/usr/lib/sendmail

all:
	cd src; m4 Makefile.m4 > Makefile; make ${MFLAGS} all
	cd aux; m4 Makefile.m4 > Makefile; make ${MFLAGS} all
	cd cf;  make ${MFLAGS} all

install: all $(ALL) $(DIRS)
	install -c -s -m 4551	src/sendmail	$(SENDMAIL)
	install -c -m 444	lib/sendmail.hf	$(SENDMAIL).hf
#	install -c -m 660	/dev/null	$(SENDMAIL).fc
	install -c -m 444	cf/subsidiary.cf $(SENDMAIL).subsidiary.cf
	install -c -m 444	cf/main.cf	$(SENDMAIL).main.cf
	install -c -m 644	cf/subsidiary.cf $(SENDMAIL).cf
	install -c -m 666	lib/aliases	$(DESTDIR)/usr/lib/aliases
	install -c -s -m 755	aux/vacation	$(DESTDIR)/usr/ucb/vacation
	install -c -s -m 755	aux/mconnect	$(DESTDIR)/usr/etc/mconnect
	install -c -s -m 755	aux/mailstats	$(DESTDIR)/usr/etc/mailstats
	rm -f					$(DESTDIR)/usr/ucb/newaliases
	ln -s		/usr/lib/sendmail	$(DESTDIR)/usr/ucb/newaliases
	$(DESTDIR)/usr/ucb/newaliases -oA$(DESTDIR)/usr/lib/aliases

$(DESTDIR)/usr/spool/mqueue:
	-mkdir $(DESTDIR)/usr/spool/mqueue
	chmod 770 $(DESTDIR)/usr/spool/mqueue

$(DESTDIR)/usr/spool/mail:
	-mkdir $(DESTDIR)/usr/spool/mail
	chmod 777 $(DESTDIR)/usr/spool/mail

clean:
	cd src; make ${MFLAGS} clean
	cd cf;  make ${MFLAGS} clean
	cd aux; make ${MFLAGS} clean
