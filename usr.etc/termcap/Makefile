#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 1.10 07/01/83
#
# reorder gives an editor command for most common terminals
# (in reverse order from n'th to 1'st most commonly used)
# to move them to the front of termcap
#
DESTDIR =

termcap: reorder termcap.src
	ex - termcap.src < reorder

install: termcap
	install -m 664 termcap ${DESTDIR}/usr/etc/termcap
	rm -fr ${DESTDIR}/usr/lib/tabset
	-mkdir ${DESTDIR}/usr/lib/tabset && \
		chown bin ${DESTDIR}/usr/lib/tabset && \
		chmod 755 ${DESTDIR}/usr/lib/tabset
	cp tabset/* ${DESTDIR}/usr/lib/tabset

clean:
	rm -f termcap
