#! /bin/sh
#
#	@(#)ccat.sh 1.1 86/09/25 SMI; from UCB 4.1 83/02/11
#
for file in $*
do
	/usr/ucb/uncompact < $file
done
