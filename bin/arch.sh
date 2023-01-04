#! /bin/sh
#     @(#)arch.sh 1.1 86/09/24 SMI

if [ -f /bin/sun2 ] && /bin/sun2; then
	echo "sun2"
elif [ -f /bin/sun3 ] && /bin/sun3; then
	echo "sun3"
fi
