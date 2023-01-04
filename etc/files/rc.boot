#
#	@(#)rc.boot 1.1 86/09/24 SMI
#
# Executed once at boot time
#
# Note that all "echo" commands are in parentheses.  This is done
# because all commands that redirect the output to "/dev/console"
# must be done in a child of the main shell, so that the main shell
# does not open a terminal and get its process group set.  Since
# "echo" is a builtin command, redirection for it will be done
# in the main shell unless the command is run in a subshell.
#
hostname=noname
#
# It is important to fsck the root filesystem here to prevent
# spurious panics after system crashes.
#
error=0
if [ $1x = singleuserx ]
then
	(echo "Singleuser boot -- fsck not done")		>/dev/console
else
	if [ -r /fastboot ]
	then
		(echo "Fast boot ... skipping disk checks")	>/dev/console
	else
		/etc/fsck -p >/dev/console 2>&1
		case $? in
		0)
			;;
		4)
			(echo "Root fixed - rebooting.")	>/dev/console
			/etc/reboot -q -n
			;;
		8)
			(echo "Reboot failed...help!")		>/dev/console
			error=1
			;;
		12)
			(echo "Reboot interrupted.")		>/dev/console
			error=1
			;;
		*)
			(echo "Unknown error in reboot fsck.")	>/dev/console
			error=1
			;;
		esac
	fi
fi
> /etc/mtab
/etc/mount -f /
/etc/mount /pub
/bin/hostname $hostname
/etc/ifconfig ec0 $hostname -trailers up
/etc/ifconfig ie0 $hostname -trailers up
/etc/ifconfig le0 $hostname -trailers up
/etc/umount -at nfs
sync
#
# exit with error status if there were any fsck errors above
#
exit $error
