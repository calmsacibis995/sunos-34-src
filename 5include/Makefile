#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 4.3 83/07/10
#
# Doing a make install builds /usr/5include
#
# The ``rm -rf''s used below are safe because rm doesn't
# follow symbolic links.
#
DESTDIR=
SUBDIRS=sys
STD=	assert.h ctype.h \
	mon.h pwd.h \
	stdio.h time.h \
	ustat.h
LINKS=	termio.h

all: ${STD}

install: mkdir ${STD}
	rm -rf ${DESTDIR}/usr/5include/sys
	mkdir ${DESTDIR}/usr/5include/sys
	/etc/chown bin ${DESTDIR}/usr/5include/sys
	chmod 755 ${DESTDIR}/usr/5include/sys
	(cd sys; for j in *.h; do \
		install -c -m 644 $$j ${DESTDIR}/usr/5include/sys/$$j; \
	done)
	-for i in ${STD}; do \
		install -c -m 644 $$i ${DESTDIR}/usr/5include/$$i; \
	done
	(cd /usr/src/5lib/libcurses; ${MAKE} DESTDIR=${DESTDIR} install_h)
	-for i in ${LINKS}; do \
		rm -f ${DESTDIR}/usr/5include/$$i; \
		ln -s sys/$$i ${DESTDIR}/usr/5include/$$i; \
	done

mkdir:
	-mkdir ${DESTDIR}/usr/5include && \
		/etc/chown bin ${DESTDIR}/usr/5include && \
		chmod 755 ${DESTDIR}/usr/5include

clean:
