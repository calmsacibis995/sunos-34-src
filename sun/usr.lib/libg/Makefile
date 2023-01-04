#
# @(#)Makefile 1.1 86/09/25 SMI
#

libg.a:	libg.s
	as libg.s -o libg.a

install: libg.a
	install -c -m 644 libg.a ${DESTDIR}/usr/lib

clean:
	rm -f libg.a
