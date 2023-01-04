#! /bin/sh

#
# @(#)setup.sh 1.1 86/09/25 SMI
#
#  Copyright (c) 1985 by Sun Microsystems, Inc.
#

HOME=/; export HOME
PATH=/bin:/usr/bin:/etc:/usr/etc:/usr/ucb
SETUPDIR=/usr/etc/setup.files
SHELL=/bin/sh; export SHELL
WINDOW_WALKMENU=TRUE; export WINDOW_WALKMENU

cd ${SETUPDIR}

INTRO="Enter the appropriate number response for the following questions."

Q1="
Are you running Setup to:
	1) Install a major Sun Unix release
	2) Upgrade between major Sun Unix releases
	3) Demonstrate Setup
>> "

Q2="
Will you be running Setup from a:
	1) Sun bit mapped display device
	2) cursor addressable terminal (TTY)
>> "

TERMCHOICES="
Select your terminal type:
	1) Televideo 925
	2) Wyse Model 50
	3) Sun Workstation
	4) Other
>> "

TERMTYPE="
Enter the terminal type (the terminal type must be in /etc/termcap):
>> "

echo
echo "${INTRO}"

while true; do
	echo -n "${Q1}"
	read ANSWER
	case "${ANSWER}" in
	1)
		SETUPARGS=""
		MODE="INSTALL"
		break
		;;
	2)
		SETUPARGS="-u"
		MODE="UPGRADE"
		break
		;;
	3)
		SETUPARGS="-f setuphardware.file"
		MODE="DEMO"
		break
		;;
	esac
done

if [ -f "/.MINIROOT" ]; then
	/etc/portmap &
	SETUPARGS="${SETUPARGS} -M"
else
	if [ "${MODE}" != "DEMO" ]; then
		if [ "`whoami`" != "root" ]; then
			echo "Setup must be run as root (super-user)."
			exit 1
		fi
	fi
fi

while true; do
	echo -n "${Q2}"
	read ANSWER
	case "${ANSWER}" in
	1)
		if [ ! -f ${SETUPDIR}/setup.window ]; then
			echo "Could not find setup window interface executable."
			exit 1
		fi
		if [ -z "${WINDOW_PARENT}" ]; then
			ROOTMENU=${SETUPDIR}/rootmenu
			export ROOTMENU
			TMPFILE=/tmp/setuptmp.$$
			echo "${SETUPDIR}/setup.window ${SETUPARGS}" > ${TMPFILE}
			/usr/bin/suntools -s ${TMPFILE}
			rm -f ${TMPFILE}
		else
			${SETUPDIR}/setup.window ${SETUPARGS}
		fi
		break
		;;
	2)
		while true; do
			echo -n "${TERMCHOICES}"
			read ANSWER
			case "${ANSWER}" in
			1)
				eval `tset -Q -s 925`
				break
				;;
			2)
				eval `tset -Q -s wyse`
				break
				;;
			3)
				eval `tset -Q -s sun`
				break
				;;
			4)
				while true; do
					echo -n "${TERMTYPE}"
					read ANSWER
					eval `tset -Q -s ${ANSWER}`
					if [ "${TERM}" != "unknown" ]; then
						break
					fi
				done
				break
				;;
			esac
		done
		${SETUPDIR}/setup.tty ${SETUPARGS}
		break
		;;
	esac
done

exit 0

