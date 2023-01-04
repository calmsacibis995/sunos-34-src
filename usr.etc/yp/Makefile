#
# @(#)Makefile 1.1 86/09/25 (C) 1985 SMI
#
SRCS = makedbm.c stdhosts.c yppush.c ypset.c ypxfr.c yppoll.c 
SUBDIR = revnetgroup
CFLAGS = -O
TOUCH =	touch -cf
SCRIPT= ypinit ypxfr_1perday ypxfr_2perday ypxfr_1perhour
LIBS = -ldbm

all: makedbm yppush stdhosts ypset ypxfr yppoll 

makedbm: makedbm.o
	cc $(CFLAGS) makedbm.o -o makedbm $(LIBS)

yppush: yppush.o
	cc $(CFLAGS) yppush.o -o yppush

ypset: ypset.o
	cc $(CFLAGS) ypset.o -o ypset

ypxfr: ypxfr.o
	cc $(CFLAGS) ypxfr.o -o ypxfr $(LIBS)

yppoll: yppoll.o
	cc $(CFLAGS) yppoll.o -o yppoll

stdhosts: stdhosts.o
	cc $(CFLAGS) stdhosts.o -o stdhosts

install: all
	-mkdir ${DESTDIR}/usr/etc/yp && chown bin ${DESTDIR}/usr/etc/yp && \
		chmod 755 ${DESTDIR}/usr/etc/yp
	install -s makedbm $(DESTDIR)/usr/etc/yp
	install -s yppush $(DESTDIR)/usr/etc/yp
	install -s ypset $(DESTDIR)/usr/etc/yp
	install -s ypxfr $(DESTDIR)/usr/etc/yp
	install -s yppoll $(DESTDIR)/usr/etc/yp
	install -s stdhosts $(DESTDIR)/usr/etc/yp
	for i in ${SCRIPT}; do (install -c $$i.sh ${DESTDIR}/usr/etc/yp/$$i); done
	for i in $(SUBDIR); do \
		(cd $$i; make $(MFLAGS) DESTDIR=$(DESTDIR) install); done
	install -c make.script ${DESTDIR}/usr/etc/yp/Makefile
	rm -rf ${DESTDIR}/etc/yp
	ln -s /usr/etc/yp ${DESTDIR}/etc/yp

clean:
	rm -f *.o makedbm yppush stdhosts ypset ypxfr yppoll
	for i in $(SUBDIR); do (cd $$i; make $(MFLAGS) clean); done

lint:
	lint $(SRCS)

print:
	pr $(SRCS) | lpr

depend:
	cp /dev/null x.c
	for i in $(SRCS) ; do \
		(/bin/grep '^#[ 	]*include' x.c $$i | sed \
			-e '/\.\.\/h/d' \
			-e '/\.\.\/ufs/d' \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.c/\.o/' >>makedep); done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep x.c
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it

makedbm.o: /usr/include/dbm.h
makedbm.o: /usr/include/stdio.h
makedbm.o: /usr/include/sys/file.h
makedbm.o: /usr/include/sys/param.h
makedbm.o: /usr/include/sys/stat.h
stdhosts.o: /usr/include/stdio.h
yppush.o: /usr/include/dbm.h
yppush.o: /usr/include/stdio.h
yppush.o: /usr/include/sys/time.h
yppush.o: /usr/include/ctype.h
yppush.o: /usr/include/netdb.h
yppush.o: /usr/include/rpc/rpc.h
yppush.o: /usr/include/sys/socket.h
yppush.o: /usr/include/rpcsvc/ypclnt.h
yppush.o: /usr/include/rpcsvc/yp_prot.h
makedbm.o: /usr/include/dbm.h
makedbm.o: /usr/include/stdio.h
makedbm.o: /usr/include/sys/file.h
makedbm.o: /usr/include/sys/param.h
makedbm.o: /usr/include/sys/stat.h
stdhosts.o: /usr/include/stdio.h
yppush.o: /usr/include/dbm.h
yppush.o: /usr/include/stdio.h
yppush.o: /usr/include/sys/time.h
yppush.o: /usr/include/ctype.h
yppush.o: /usr/include/netdb.h
yppush.o: /usr/include/rpc/rpc.h
yppush.o: /usr/include/sys/socket.h
yppush.o: /usr/include/rpcsvc/ypclnt.h
yppush.o: /usr/include/rpcsvc/yp_prot.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
