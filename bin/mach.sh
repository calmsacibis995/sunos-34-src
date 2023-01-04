#! /bin/sh
#     @(#)mach.sh 1.1 86/09/24 SMI

if [ -f /bin/mc68010 ] && /bin/mc68010; then
        echo "mc68010"
elif [ -f /bin/mc68020 ] && /bin/mc68020; then
        echo "mc68020"
fi
