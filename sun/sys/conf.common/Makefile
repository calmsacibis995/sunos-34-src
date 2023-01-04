#
# @(#)Makefile 1.5 86/12/12 SMI
#
# This makefile creates the distributed kernel objects and
# configuration files.

MASTER=GENERIC
DESTDIR=
HDIRS=	h net netinet netimp netpup nfs rpc specfs sun sun2 sun3 \
	sundev sunif ufs
FILES= \
	sun/conf.c sun/stubs.c sun/swapgeneric.c sun/symbols.sort \
	sun/symbols.raw sun/README sys/init_sysent.c sys/tty_conf.c \
	netinet/in_proto.c netpup/pup_proto.c sys/uipc_proto.c \
	sun2/swapgeneric.c sun3/swapgeneric.c sundev/sc_conf.c \
	sundev/mti_conf.c nfs/dbx_nfs.c pcfs/dbx_pcfs.c rpc/dbx_rpc.c \
	sun2/dbx_machdep.c sun3/dbx_machdep.c sundev/dbx_sundev.c \
	sunif/dbx_sunif.c sys/dbx_sys.c ufs/dbx_ufs.c mon/sunromvec.h \
	sys/vfs_conf.c sys/init_dbx.c sundev/xy_conf.c \
	debug/debug.h
HFILES= \
	conf/${MASTER} conf/README conf/RELEASE conf/devices.`arch` \
	conf/files conf/newvers.sh conf/param.c

all:	
	@echo make install only!

install:
	cd ../${MASTER};make install DESTDIR=${DESTDIR}
	-rm -fr ${DESTDIR}/usr/sys/OBJ
	-mkdir ${DESTDIR}/usr ${DESTDIR}/usr/sys
	-mkdir ${DESTDIR}/usr/sys/OBJ ${DESTDIR}/usr/sys/conf
	-ln -s usr/sys ${DESTDIR}/sys
	(cd ../${MASTER};tar cf - *.[ho])|(cd ${DESTDIR}/usr/sys/OBJ;tar xpf -)
	(cd ${DESTDIR}/usr/sys/OBJ; chmod 444 *.[ho])
	cat ../conf/files.`arch` >${DESTDIR}/usr/sys/conf/files.`arch`
	echo "sun/stubs.c	standard" >>${DESTDIR}/usr/sys/conf/files.`arch`
	cp ../conf/makefile.obj.`arch` ${DESTDIR}/usr/sys/conf/makefile.`arch`
	-for i in ${HDIRS}; do \
		mkdir ${DESTDIR}/usr/sys/$$i; \
		(cd ../$$i;tar cf - *.h)|(cd ${DESTDIR}/usr/sys/$$i;tar xpf -) \
		done
	-mkdir ${DESTDIR}/usr/sys/sys
	(cd ..; tar cf - ${FILES}) | (cd ${DESTDIR}/usr/sys; tar xpf -)
	(cd ..; tar chf - ${HFILES} `cat conf/MACHINES`) | \
		(cd ${DESTDIR}/usr/sys; tar xpf -)
	-rm -f ${DESTDIR}/usr/sys/machine
	-ln -s `arch` ${DESTDIR}/usr/sys/machine
	-for i in `cat ../conf/EXCEPTIONS`; do rm -rf ${DESTDIR}/usr/sys/$$i; done
	-(cd ${DESTDIR}/usr/sys; chown root * */* 2>/dev/null)

clean:
	@echo make install only!

