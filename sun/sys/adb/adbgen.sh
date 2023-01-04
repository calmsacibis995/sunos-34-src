#! /bin/sh
#
# @(#)adbgen.sh 1.1 86/09/25 SMI
#
case $1 in
-*)
	flag=$1
	shift;;
esac
for file in $*
do
	if [ `expr $file : '.*\.adb` -eq 0 ]
	then
		echo File $file invalid.
		exit 1
	fi
	if [ $# -gt 1 ]
	then
		echo $file:
	fi
	file=`basename $file .adb`
	if adbgen1 $flag < $file.adb > $file.adb.c
	then
		if cc -w -o $file.run $file.adb.c adbsub.o
		then
			$file.run | adbgen3 | adbgen4 > $file
			rm -f $file.run $file.adb.c $file.adb.o
		else
			rm -f $file.run $file.adb.c $file.adb.o
			echo compile failed
			exit 1
		fi
	else
		rm -f $file.adb.c
		echo adbgen1 failed
		exit 1
	fi
done
