#! /bin/sh
#
#	@(#)install.sh 1.1 86/09/25 SMI; from UCB 4.7 9/7/85
#
cmd=/bin/cp
strip=""
chmod="/bin/chmod -f 755"
chown="/etc/chown -f root"
chgrp="/bin/chgrp -f staff"
while true ; do
	case $1 in
		-s )	strip="/bin/strip"
			shift
			;;
		-c )	cmd="/bin/cp"
			shift
			;;
		-m )	chmod="/bin/chmod -f $2"
			shift
			shift
			;;
		-o )	chown="/etc/chown -f $2"
			shift
			shift
			;;
		-g )	chgrp="/bin/chgrp -f $2"
			shift
			shift
			;;
		* )	break
			;;
	esac
done

if [ ! ${2-""} ]
then	echo "install: no destination specified"
	exit 1
fi
if [ ${3-""} ]
then	echo "install: too many files specified -> $*"
	exit 1
fi
if [ $1 = $2 -o $2 = . ]
then	echo "install: can't move $1 onto itself"
	exit 1
fi
if [ '!' -f $1 ]
then	echo "install: can't open $1"
	exit 1
fi
if [ -d $2 ]
then	file=$2/`basename $1`
else	file=$2
fi
/bin/rm -f $file
$cmd $1 $file
if [ $strip ]
then	$strip $file
fi
$chown $file
$chgrp $file
$chmod $file
