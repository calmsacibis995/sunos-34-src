#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 4.3 83/07/05
#
# Machine dependent routines for the VAX are located here
#
ALL=	gen net stdio sys

vaxlib vaxlib_p: ${ALL}
	-mkdir tmp
	for i in ${ALL}; do (cd tmp; ar x ../$$i/$${i}lib); done
	ar cr vaxlib `ls tmp/*.o | sort -t/ +1`
	rm -f tmp/*
	for i in ${ALL}; do (cd tmp; ar x ../$$i/$${i}lib_p); done
	ar cr vaxlib_p `ls tmp/*.o | sort -t/ +1`
	rm -rf tmp

${ALL}:	FRC
	cd $@; make ${MFLAGS}

FRC:

clean:
	for i in ${ALL}; do (cd $$i; make ${MFLAGS} clean); done
	rm -f tmp/*.o;
	-rmdir tmp
