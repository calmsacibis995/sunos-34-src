#! /bin/sh
#
#	@(#)calendar.sh 1.1 86/09/25 SMI; from UCB 4.5 84/02/14
#
PATH=/bin:/usr/bin:
tmp=/tmp/cal0$$
trap "rm -f $tmp /tmp/cal1$$ /tmp/cal2$$"
trap exit 1 2 13 15
/usr/lib/calendar >$tmp
case $# in
0)
	trap "rm -f $tmp ; exit" 0 1 2 13 15
	if [ -f calendar ]; then
		/lib/cpp calendar | egrep -f $tmp
	else
		echo $0: calendar not found
	fi;;
*)
	trap "rm -f $tmp /tmp/cal1$$ /tmp/cal2$$; exit" 0 1 2 13 15
	/bin/echo -n "Subject: Calendar for " > /tmp/cal1$$
	date | sed -e "s/ [0-9]*:.*//" >> /tmp/cal1$$
	sed '
		s/\([^:]*\):.*:\(.*\):[^:]*$/y=\2 z=\1/
	' /etc/passwd \
	| while read x
	do
		eval $x
		if test -r $y/calendar
		then
			(/lib/cpp $y/calendar | egrep -f $tmp) 2>/dev/null > /tmp/cal2$$
			if test -s /tmp/cal2$$
			then
				cat /tmp/cal1$$ /tmp/cal2$$ | /bin/mail $z
			fi
		fi
	done
esac
