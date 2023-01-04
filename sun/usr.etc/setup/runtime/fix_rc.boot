#! /bin/sh
#
# @(#)fix_rc.boot 1.1 86/09/25 SMI; from UCB X.X XX/XX/XX
#
if [ $# -ne 2 ]
then
	echo "Usage: $0 dir archname"
	exit 1
fi
cd $1
ed - rc.boot <<END
/\/pub/s/pub/pub.$2/
w
q
END
