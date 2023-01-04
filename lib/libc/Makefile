#
# @(#)Makefile 1.2 86/10/04 SMI; from UCB 4.6 83/07/02
#
# The system has two monitoring schemes, utilizing either ``prof''
# or ``gprof''. DFLMON determines the default profiler; mon.o
# specifies that ``prof'' is the default, gmon.o specifies that
# ``gprof'' is the default.
#
# Machine dependent routines are located in a subtree which parallels
# the top directories.  This subtree is identified by the machine name.
#
# Compatibility routines are kept in directories with a prefixing
# ``compat'' (so they all sort together).
# 
# The C run-time startup code is always machine dependent and expected
# to be located in ${MACHINE}csu
#
DESTDIR=
MACHINES=vax sun
INSTALL=install -m 644
GENERIC=compat gen inet net rpc yp stdio sys 
BSD42=	compat-4.1 gen-4.2 stdio-4.2 sys-4.2
SYS5=	compat-sys5 gen-sys5 stdio-sys5 sys-sys5
DEBUG=	gen
ALL=	${GENERIC} ${BSD42} ${SYS5}
CPP=	/lib/cpp
MAKE=	make CC="${CC}" AS="${AS}" CPP="${CPP}" ${MFLAGS}
CRYPTDIR= gen 

all: csu libc.a libc_p.a libcs5.a libcs5_p.a

libc.a libc_p.a libcs5.a libcs5_p.a: ${ALL} ${MACHINES}
	-mkdir tmp
	for i in ${MACHINES}; do \
		if $$i; then M=$$i; fi; done; \
	for i in ${GENERIC}; do (cd tmp; ar x ../$$i/$${i}lib); done; \
	for i in ${BSD42}; do (cd tmp; ar x ../$$i/$${i}lib); done; \
	for i in $${M}; do (cd tmp; ar x ../$$i/$${i}lib); done; \
	ls tmp/*.o | sort -t/ +1 > t1; \
	lorder `cat t1` | sed 's/^ *//' | sort | comm -23 - order.out | \
		cat order.in - | tsort >objlist; \
	ar cr libc.a `cat objlist` $${M}csu/mon.o; \
	rm -f objlist t1 tmp/*; \
	for i in ${GENERIC}; do (cd tmp; ar x ../$$i/$${i}lib_p); done; \
	for i in ${BSD42}; do (cd tmp; ar x ../$$i/$${i}lib_p); done; \
	for i in $${M}; do (cd tmp; ar x ../$$i/$${i}lib_p); done; \
	ls tmp/*.o | sort -t/ +1 > t1; \
	lorder `cat t1` | sed 's/^ *//' | sort | comm -23 - order.out | \
		cat order.in - | tsort >objlist; \
	ar cr libc_p.a `cat objlist` $${M}csu/mon.o; \
	rm -f objlist t1 tmp/*; \
	for i in ${GENERIC}; do (cd tmp; ar x ../$$i/$${i}libs5); done; \
	for i in ${SYS5}; do (cd tmp; ar x ../$$i/$${i}lib); done; \
	for i in $${M}; do (cd tmp; ar x ../$$i/$${i}libs5); done; \
	ls tmp/*.o | sort -t/ +1 > t1; \
	lorder `cat t1` | sed 's/^ *//' | sort | comm -23 - order.out | \
		cat order.in - | tsort >objlist; \
	ar cr libcs5.a `cat objlist` $${M}csu/mon.o; \
	rm -f objlist t1 tmp/*; \
	for i in ${GENERIC}; do (cd tmp; ar x ../$$i/$${i}libs5_p); done; \
	for i in ${SYS5}; do (cd tmp; ar x ../$$i/$${i}lib_p); done; \
	for i in $${M}; do (cd tmp; ar x ../$$i/$${i}libs5_p); done; \
	ls tmp/*.o | sort -t/ +1 > t1; \
	lorder `cat t1` | sed 's/^ *//' | sort | comm -23 - order.out | \
		cat order.in - | tsort >objlist; \
	ar cr libcs5_p.a `cat objlist` $${M}csu/mon.o; \
	rm -rf objlist t1 tmp

${MACHINES}: FRC
	if $@; then cd $@; ${MAKE}; fi

csu: FRC
	for i in ${MACHINES}; do \
		if $$i; then cd $${i}csu; ${MAKE}; fi; done

${ALL}: FRC
	cd $@; ${MAKE}

FRC:

international: 
	for i in ${CRYPTDIR}; do (cd $$i; ${MAKE} international); done
	ranlib ${DESTDIR}/lib/libc.a
	ranlib ${DESTDIR}/usr/lib/libc_p.a
	ranlib ${DESTDIR}/usr/5lib/libc.a
	ranlib ${DESTDIR}/usr/5lib/libc_p.a

clean:
	-for i in ${ALL}; do (cd $$i; ${MAKE} clean); done
	-for i in ${MACHINES}; do \
		if $$i; then \
			(cd $${i}csu; ${MAKE} clean;); \
			(cd $$i; ${MAKE} clean;); \
		fi \
	done
	-for i in ${DEBUG}; do \
		(cd $$i/debug; ${MAKE} clean;) \
	done
	rm -rf t1 tmp libc.a libc_p.a libcs5.a libcs5_p.a

install: all 
	${INSTALL} libc.a ${DESTDIR}/lib/libc.a
	ranlib ${DESTDIR}/lib/libc.a
	${INSTALL} libc_p.a ${DESTDIR}/usr/lib/libc_p.a
	ranlib ${DESTDIR}/usr/lib/libc_p.a
	${INSTALL} libcs5.a ${DESTDIR}/usr/5lib/libc.a
	ranlib ${DESTDIR}/usr/5lib/libc.a
	${INSTALL} libcs5_p.a ${DESTDIR}/usr/5lib/libc_p.a
	ranlib ${DESTDIR}/usr/5lib/libc_p.a
	for i in ${MACHINES}; do \
		if $$i; then \
			(cd $${i}csu; ${MAKE} DESTDIR=${DESTDIR} install;) \
		fi \
	done
	for i in ${DEBUG}; do \
		(cd $$i/debug; ${MAKE} DESTDIR=${DESTDIR} install;) \
	done
