
#ifndef lint
static	char sccsid[] = "@(#)hardware.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"

#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <sys/stat.h>

#define	MAXUNITS	4	/* maximum number of units per cntlr */

char	*filename = "/tmp/setuphardware.XXXXXX";

char *
get_hardware_config(ws)
Workstation	ws;
{
	FILE	*fp;

	mktemp(filename);
	if ((fp = fopen(filename, "w")) == NULL) {
		runtime_error("Unable to open %s.", filename);
	}

	get_disk_info(ws, fp);

	fclose(fp);
	return (filename);
}


get_disk_info(ws, fp)
Workstation	ws;
FILE		*fp;
{
	int	n;
	char	*cntlr;
	int	i;

	SETUP_FOREACH_CHOICE(ws, CONFIG_DISKTYPE, n, cntlr)
		for (i = 0; i < MAXUNITS; i++) {
			dkinfo(fp, get_device_abbrev(cntlr), i);
		}
	SETUP_END_FOREACH
}


get_ethernet_info(ws)
Workstation	ws;
{
	int	n;
	char	*string;
	int	count;
	char	*array[CONFIG_ITEMS];

	count = 0;
        SETUP_FOREACH_CHOICE(ws, CONFIG_ETHERTYPE, n, string) {
		if (legal_ethernet_if(n)) {
			array[count++] = string;
		}
        } SETUP_END_FOREACH

	for (n = 0; n < count; n++) {
		setup_set(ws, 
			SETUP_CHOICE_STRING, ord(CONFIG_ETHERTYPE), n, array[n], 
			0);
	}
	setup_set(ws, 
		SETUP_CHOICE_STRING, ord(CONFIG_ETHERTYPE), n, NULL, 
		0);

	/*
	 * if we have only 1 ethernet I/F, then go ahead and set it as the
	 * ethernet I/F (count == 2 because "NONE" will always be present)
	 */
	if (count == 2) {
		setup_set(ws,
			WS_ETHERTYPE, 1,
			0);
	}
	
}


legal_ethernet_if(etype)
int     etype;
{
	char	*ether_str;
	char	*dev;
        int	s;
        char	buf[BUFSIZ];
        struct ifconf ifc;
        struct ifreq ifreq, *ifr;
        int	len;
	int	found = 0;

        ether_str = setup_get(workstation, 
	    SETUP_CHOICE_STRING, CONFIG_ETHERTYPE, etype);
	if (streq(ether_str, "None")) {
		return(TRUE);
	}
        if ((dev = get_device_abbrev(ether_str)) == NULL) {
		return(FALSE);
	}
 
        if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("get_myaddress: socket");
            exit(1);
        }
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = buf;
        if (ioctl(s, SIOCGIFCONF, (char *)&ifc) < 0) {
                perror("get_myaddress: ioctl (get interface configuration)");
                exit(1);
        }
        ifr = ifc.ifc_req;
        for (len = ifc.ifc_len; len; len -= sizeof ifreq) {
                ifreq = *ifr;
		if (streq(ifreq.ifr_name, dev)) {
			found = 1;
			break;
		}
		ifr++;
        }
	close(s);

	if (found) {
		return(TRUE);
	} else {
		return(FALSE);
	}
}


dkinfo(fp, cntlr, unit)
FILE	*fp;
char	*cntlr;
int	unit;
{
	char		partition;
	char 		device[64];
	char 		cmd[64];
	int 		fd;
	struct stat	statb;
	struct dk_info inf;
	struct dk_map	p;
	struct dk_geom	g;
	static int 	old_cntlr_addr = -1;
	static int 	old_unit_number = -1;

	sprintf(device, "/dev/r%s%dc", cntlr, unit);
	if ((fd = open(device, 0)) < 0) {
		return;
	}
	close(fd);
        for (partition = 'a'; partition < 'i'; partition++) {

		sprintf(device, "/dev/r%s%d%c", cntlr, unit, partition);
		if ((fd = open(device, 0)) < 0) {
			continue;
		}
		if (ioctl(fd, DKIOCINFO, &inf) != 0) {
			runtime_error("Unable to DKIOCINFO disk %s.\n", device);
		}
		if (ioctl(fd, DKIOCGPART, &p) != 0) {
			runtime_error("Unable to DKIOCGPART disk %s.\n",device);
		}
		if (ioctl(fd, DKIOCGGEOM, &g) != 0) {
			runtime_error("Unable to DKIOCGGEOM disk %s.\n",device);
		}
		close(fd);

		if (p.dkl_nblk == 0) {
			continue;
		}

		if (inf.dki_ctlr != old_cntlr_addr) {
			fprintf(fp, "%% disk_controller\n");
                        fprintf(fp, "cntlr=%s addr=%x\n", cntlr, inf.dki_ctlr);
			old_cntlr_addr = inf.dki_ctlr;
			old_unit_number = -1;
		}

		if (inf.dki_unit != old_unit_number) {
			fprintf(fp, "%% disk\n");
			fprintf(fp,
                            "cntlr=%s unit=%d cyls=%d heads=%d secs/track=%d\n",
                            cntlr, unit, g.dkg_ncyl, g.dkg_nhead, g.dkg_nsect);
			old_unit_number = inf.dki_unit;
		}

		fprintf(fp, "partition=%c start_cyl=%d sectors=%d\n", 
			partition, p.dkl_cylno, p.dkl_nblk);

	}

}

