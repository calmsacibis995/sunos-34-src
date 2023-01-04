#
# @(#)Makefile 1.1 86/09/25 SMI
#

all: setup.sh rootmenu

install: setup.sh rootmenu
	install -c setup.sh ${DESTDIR}/usr/etc/setup
	install -c rootmenu ${DESTDIR}/usr/etc/setup.files/rootmenu

setup.sh: SCCS/s.setup.sh
	sccs get setup.sh

rootmenu: SCCS/s.rootmenu
	sccs get rootmenu


clean:
	rm -f a.out core *.s *.o

depend:
