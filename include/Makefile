#
# @(#)Makefile 1.6 87/03/18 SMI; from UCB 4.3 83/07/10
#
# Doing a make install builds /usr/include
#
# Define SHARED to indicate whether you want
# symbolic links to the system source (``symlinks''),
# or a separate copy (``copies'').
# (latter useful in environments where it's
# not possible to keep /sys publicly readable)
#
# The ``rm -rf''s used below are safe because rm doesn't
# follow symbolic links.
#
DESTDIR=
SUBDIRS=arpa pascal protocols
SUNVIEW_SUBDIRS=sunwindow suntool pixrect
STD=	a.out.h ar.h assert.h colorbuf.h ctype.h curses.h \
	dbm.h disktab.h dumprestor.h errno.h \
	fstab.h ftw.h grp.h lastlog.h \
	malloc.h math.h memory.h mntent.h mp.h \
	nan.h ndbm.h netdb.h nlist.h prof.h pwd.h \
	ranlib.h rasterfile.h regexp.h \
	search.h setjmp.h sgtty.h stab.h stdio.h string.h strings.h \
	struct.h sysexits.h syslog.h time.h \
	unistd.h utmp.h values.h varargs.h vfont.h
LINKS=	fcntl.h signal.h syscall.h
MACHINES=vax sun
NETDIRS=net netimp netinet netpup
SYSDIRS=${NETDIRS} nfs ufs debug
RPCDIRS=rpc yp des
SYS=/usr/src/sun/sys
SHARED=	copies
MONFILES= bootparam.h eeprom.h keyboard.h s2addrs.h led.h sunmon.h sunromvec.h

all: ${STD}

install: mkdir ${SHARED} ${STD}
	for i in ${STD}; do \
		install -c -m 644 $$i ${DESTDIR}/usr/include/$$i; \
	done
	for i in ${SUBDIRS} ${SUNVIEW_SUBDIRS}; do \
		if [ ! -d ${DESTDIR}/usr/include/$$i ]; \
		then \
			mkdir ${DESTDIR}/usr/include/$$i; \
			chown bin  ${DESTDIR}/usr/include/$$i; \
			chmod 755 ${DESTDIR}/usr/include/$$i; \
		fi; \
	done
	for i in ${SUBDIRS}; do \
		(cd $$i; for j in *.h; do \
			install -c -m 644 $$j ${DESTDIR}/usr/include/$$i/$$j; \
		done); \
	done
	for i in ${SUNVIEW_SUBDIRS}; do \
		cd /usr/src/sun/usr.lib/lib$$i; \
		make ${MFLAGS} DESTDIR=${DESTDIR} install_h; \
	done
	for i in ${RPCDIRS}; do \
		cd /usr/src/lib/libc/$$i; \
		make ${MFLAGS} DESTDIR=${DESTDIR} install; \
	done
	for i in ${LINKS}; do \
		rm -f ${DESTDIR}/usr/include/$$i; \
		ln -s sys/$$i ${DESTDIR}/usr/include/$$i; \
	done
	for i in ${MACHINES}; do \
		if $$i; then \
			rm -f ${DESTDIR}/usr/include/machine; \
			ln -s ./`arch` ${DESTDIR}/usr/include/machine; \
			rm -f ${DESTDIR}/usr/include/stand; \
			ln -s ./$${i}stand ${DESTDIR}/usr/include/stand; \
		fi; done
	rm -f ${DESTDIR}/usr/include/frame.h
	ln -s machine/frame.h ${DESTDIR}/usr/include/frame.h

mkdir:
	-mkdir ${DESTDIR}/usr/include && chown bin ${DESTDIR}/usr/include && \
		chmod 755 ${DESTDIR}/usr/include

symlinks:
	-for i in ${SYSDIRS}; do \
		rm -rf ${DESTDIR}/usr/include/$$i; \
		ln -s ${SYS}/$$i ${DESTDIR}/usr/include/$$i; \
	done
	-cd ${SYS}; for j in ${MACHINES}; do \
		if $$j; then \
			for i in $${j}*; do \
				rm -rf ${DESTDIR}/usr/include/$$i; \
				ln -s ${SYS}/$$i ${DESTDIR}/usr/include/$$i; \
			done; \
			rm -f ${DESTDIR}/usr/include/$${j}window; \
			rm -f ${DESTDIR}/usr/include/$${j}windowdev; \
		fi; \
	done
	rm -rf ${DESTDIR}/usr/include/sys
	ln -s ${SYS}/h ${DESTDIR}/usr/include/sys
	rm -rf ${DESTDIR}/usr/include/mon
	ln -s /usr/src/sun/mon/h ${DESTDIR}/usr/include/mon

copies:
	-for i in ${SYSDIRS}; do \
		rm -rf ${DESTDIR}/usr/include/$$i; \
		cd ${SYS}; \
		tar cf - $$i/*.h | (cd ${DESTDIR}/usr/include; tar xpf -); \
		chown bin ${DESTDIR}/usr/include/$$i; \
		chmod 755 ${DESTDIR}/usr/include/$$i; \
	done
	-cd ${SYS}; for j in ${MACHINES}; do \
		if $$j; then \
			for i in $${j}*; do \
				rm -rf ${DESTDIR}/usr/include/$$i; \
				tar cf - $$i/*.h | \
				    (cd ${DESTDIR}/usr/include; tar xpf -); \
				chown bin ${DESTDIR}/usr/include/$$i; \
				chmod 755 ${DESTDIR}/usr/include/$$i; \
			done; \
		fi; \
	done
	rm -rf ${DESTDIR}/usr/include/sys;
	mkdir ${DESTDIR}/usr/include/sys; 
	chown bin ${DESTDIR}/usr/include/sys;
	chmod 755 ${DESTDIR}/usr/include/sys;
	(cd ${SYS}/h; tar cf - *.h | (cd ${DESTDIR}/usr/include/sys; tar xpf -))
	rm -rf ${DESTDIR}/usr/include/mon;
	mkdir ${DESTDIR}/usr/include/mon; 
	chown bin ${DESTDIR}/usr/include/mon;
	chmod 755 ${DESTDIR}/usr/include/mon;
	(cd /usr/src/sun/mon/h; tar cf - ${MONFILES} | \
		(cd ${DESTDIR}/usr/include/mon; tar xpf -))
	-for i in `cat ${SYS}/conf/EXCEPTIONS`; do \
		rm -rf ${DESTDIR}/usr/include/$$i; done

clean:
