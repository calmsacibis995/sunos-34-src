#! /bin/sh
#
# @(#)ypxfr_1perhour.sh 1.1 86/09/25 Copyr 1985 Sun Microsystems, Inc.  
#
# ypxfr_1perhour.sh - Do hourly yp map check/updates
#

# set -xv
/etc/yp/ypxfr passwd.byname
/etc/yp/ypxfr passwd.byuid 
