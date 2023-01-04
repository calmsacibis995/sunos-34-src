#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 4.3 83/07/05
#
GENERIC=crt gen net stdio sys
BSD42=	stdio-4.2
SYS5=	stdio-sys5
ALL=	${GENERIC} ${BSD42} ${SYS5}
CPP=	/lib/cpp
MAKE=	make CC="${CC}" AS="${AS}" CPP="${CPP}" ${MFLAGS}

sunlib sunlib_p sunlibs5 sunlibs5_p: ${ALL}
	-mkdir tmp
	for i in ${GENERIC}; do (cd tmp; ar x ../$$i/$${i}lib); done
	for i in ${BSD42}; do (cd tmp; ar x ../$$i/$${i}lib); done
	ar cr sunlib `ls tmp/*.o | sort -t/ +1`
	rm -f tmp/*
	for i in ${GENERIC}; do (cd tmp; ar x ../$$i/$${i}lib_p); done
	for i in ${BSD42}; do (cd tmp; ar x ../$$i/$${i}lib_p); done
	ar cr sunlib_p `ls tmp/*.o | sort -t/ +1`
	rm -f tmp/*
	for i in ${GENERIC}; do (cd tmp; ar x ../$$i/$${i}libs5); done
	for i in ${SYS5}; do (cd tmp; ar x ../$$i/$${i}lib); done
	ar cr sunlibs5 `ls tmp/*.o | sort -t/ +1`
	rm -f tmp/*
	for i in ${GENERIC}; do (cd tmp; ar x ../$$i/$${i}libs5_p); done
	for i in ${SYS5}; do (cd tmp; ar x ../$$i/$${i}lib_p); done
	ar cr sunlibs5_p `ls tmp/*.o | sort -t/ +1`
	rm -rf tmp

${ALL}:	FRC
	cd $@; ${MAKE}

FRC:

clean:
	for i in ${ALL}; do (cd $$i; ${MAKE} clean); done
	rm -f tmp/*.o sunlib sunlib_p sunlibs5 sunlibs5_p
	-rmdir tmp

print:
	@ls -l | pr -f
	@pr -f Makefile *.h
	@for i in ${ALL}; do ls -l $$i | pr -f; pr -f $$i/Makefile $$i/*.h $$i/*.c $$i/*.s; done
