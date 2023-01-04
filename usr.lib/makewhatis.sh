#!/bin/sh -
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)makewhatis.sh 1.1 86/09/25 SMI; from UCB 5.1 6/6/85
#
:
MANDIR=${1-/usr/man}
rm -f /tmp/whatis1.$$ /tmp/whatis2.$$
cd $MANDIR
MANDIR=`pwd`
for i in man1 man2 man3 man4 man5 man6 man7 man8 mann manl
do
	if [ -d $i ]; then
		cd $i
		/usr/lib/getNAME *.*
		cd $MANDIR
	fi
done >/tmp/whatis1.$$
sed  </tmp/whatis1.$$ >/tmp/whatis2.$$ \
	-e 's/\\-/-/' \
	-e 's/\\\*-/-/' \
	-e 's/ VAX-11//' \
	-e 's/\\f[PRIB0123]//g' \
	-e 's/\\s[-+0-9]*//g' \
	-e 's/.TH [^ ]* \([^ 	]*\).*	\([^-]*\)/\2(\1)	/' \
	-e 's/	 /	/g'
/usr/ucb/expand -24,28,32,36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96,100 \
	/tmp/whatis2.$$ | sort | /usr/ucb/unexpand -a > whatis
chmod 644 whatis
rm -f /tmp/whatis1.$$ /tmp/whatis2.$$
