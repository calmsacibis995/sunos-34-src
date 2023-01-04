#
# Makefile for scripts written to upgrade systems running 3.0FCS to 3.2FCS
# and 3.2PILOT to 3.2FCS.
#
#       @(#)  Makefile 1.1 86/09/25
#
#       Copyright (c) 1986 by Sun Microsystems, Inc.
#
DESTDIR=

#
# Shell scripts that need only be installed and are never removed.
#
SCRIPTS=UPGRADE adm_tool merge_tool extracting verify_tapevol_arch opt_software\
	3.0crontab_3.2crontab 3.0group_3.2group 3.0protocols_3.2protocols \
	3.0rcboot_3.2rcboot 3.0rclocal_3.2rclocal 3.0rpc_3.2rpc \
	3.0servers_3.2servers 3.0services_3.2services 3.0ttys_3.2ttys \
	pilotprotocols_3.2protocols pilotrcboot_3.2rcboot \
	pilotrclocal_3.2rclocal pilotrpc_3.2rpc pilotservices_3.2services

Dummy:

install: ${SCRIPTS}
	-mkdir ${DESTDIR}/usr/etc/upgrade && \
		chown bin ${DESTDIR}/usr/etc/upgrade && \
                chmod 755 ${DESTDIR}/usr/etc/upgrade
	for i in ${SCRIPTS}; do \
		(install -c $$i ${DESTDIR}/usr/etc/upgrade/$$i); done

clean:

FRC:
