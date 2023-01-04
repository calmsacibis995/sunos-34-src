#
# @(#)Makefile 1.1 86/09/25 SMI
#
DESTDIR=
ALL=	vfont font fontinfo

# should make font width tables in font/* from fonts in vfont/*
all:

install: all
	-for i in ${ALL}; do \
		mkdir ${DESTDIR}/usr/lib/$$i && \
		chown bin ${DESTDIR}/usr/lib/$$i && \
		chmod 755 ${DESTDIR}/usr/lib/$$i; \
		cp $$i/* ${DESTDIR}/usr/lib/$$i; \
	done

clean:
