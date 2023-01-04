#! /bin/sh
#
# @(#)ypxfr_1perday.sh 1.1 86/09/25 Copyr 1985 Sun Microsystems, Inc.  
#
# ypxfr_1perday.sh - Do daily yp map check/updates
#

# set -xv
/etc/yp/ypxfr group.byname
/etc/yp/ypxfr group.bygid 
/etc/yp/ypxfr protocols.byname
/etc/yp/ypxfr protocols.bynumber
/etc/yp/ypxfr networks.byname
/etc/yp/ypxfr networks.byaddr
/etc/yp/ypxfr services.byname
/etc/yp/ypxfr ypservers
