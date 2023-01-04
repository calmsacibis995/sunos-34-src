#! /bin/sh
# @(#)fasthalt.sh 1.1 86/09/24 SMI; from UCB 4.2
cp /dev/null /fastboot
/etc/halt $*
