#ifndef lint
static	char sccsid[] = "@(#)install_server.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <ctype.h>
#include "setup_runtime.h"
#include "install.h"
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/mbuf.h>
#include <netinet/if_ether.h>
#include <net/nit.h>

static	int		get_pubunit();
static	char		*build_fstab();
static	char		*three_com_nd();
static	char		*three_com_fstab();
static	Boolean		three_com_weirdness();
	char		*mktemp();

/*
 * Install a server.
 * Create an nd.local file and run nd to active the nd partitions.
 * Initialize the usr area and then, for each architecture, 
 * extract pub, extract usr and initialize each of the clients
 * of the same architecture.
 */
install_server(ws, serv_arch)
Workstation	ws;
Arch_served	serv_arch;
{
	Arch_served	arch;
	int		ix;
	char		archstr[BUF];

	add_to_ndlocal(ws, serv_arch);
	fix_rcboot(ws, serv_arch);
	SETUP_FOREACH_OBJECT(ws, WS_ARCH_SERVED_ARRAY, ix, arch) {
		message(ws,"Beginning the installation for the %s architecture",
		    get_archname(arch, archstr));
		xtr_rootarch(ws, arch);
		xtr_clients(ws, serv_arch, arch);
		xtr_usrarch(ws, arch);
		xtr_sys(ws, arch);
		install_optsoftware(ws, arch);
	} SETUP_END_FOREACH
	tftpboot(ws, serv_arch);
	xtr_symlinks(ws, serv_arch);
	yellow_pages(ws);
	sendmail(ws);
	shadow_pub_sh(ws, serv_arch);
}

/*
 * Add the partition lines to /etc/nd.local
 * Add the public partitions first, followed by the user
 * and swap partitions for each user.
 */
static
add_to_ndlocal(ws, serv_arch)
Workstation 	ws;
Arch_served	serv_arch;
{
	Client		client;
	Hard_partition	hard;
	Soft_partition	soft;
	Soft_partition	client_soft[2];
	Soft_type	soft_type;
	FILE		*ndfp;
	char		harddev[BUF];
	char		cmd[BUF];
	char		file[BUF];
	char		*name;
	int		ndln;
	int		ndl;
	int		unit;
	int		ix;
	int		i;

	message(ws, "Updating %s", NDLOCAL);
	sprintf(file, "%s%s", ROOT_DIR, NDLOCAL);
	ndfp = fopen(file, "w");
	if (ndfp == NULL) {
		error(ws, SETUP_EOPEN_FAILED, file, "writing");
	}
	fprintf(ndfp, "#\n");
	fprintf(ndfp, "# These lines added by the Sun Setup Program\n");
	fprintf(ndfp, "#\n");
	fprintf(ndfp, "clear\n");
	fprintf(ndfp, "version 1\n");
	pub_nd(ws, ndfp);
	ndln = 0;
	SETUP_FOREACH_OBJECT(ws, WS_CLIENT, ix, client) {
		client_soft[0] = (Soft_partition)
		    setup_get(client, CLIENT_ROOT_PARTITION);
		client_soft[1] = (Soft_partition)
		    setup_get(client, CLIENT_SWAP_PARTITION);
		for (i = 0; i < 2; i++) {
			soft = client_soft[i];
			hard = (Hard_partition) 
			    setup_get(soft, SOFT_HARD_PARTITION);
			sprintf(harddev, "/dev/%s", setup_get(hard, HARD_NAME));
			soft_type = (Soft_type) setup_get(soft, SOFT_TYPE);
			switch (soft_type) {
			case SOFT_ROOT:
				name = setup_get(soft, SOFT_CLIENT_NAME);
				ndl = ndln++;
				unit = 0;
				break;
			case SOFT_SWAP:
				name = setup_get(soft, SOFT_CLIENT_NAME);
				ndl = -1;
				unit = 1;
				break;
			}
			setup_set(soft, 
			    SOFT_NDL, ndl, 
			    0);
			fprintf(ndfp, "user %s %d %s %d %d %d %s\n",
			    name, unit, harddev,
			    (int) setup_get(soft, SOFT_OFFSET),
			    (int) setup_get(soft, SOFT_SIZE),
			    ndl, three_com_nd(serv_arch, client));
			if (ndl != -1) {
				sprintf(cmd, "(cd /dev; /dev/MAKEDEV ndl%d)",
				    ndl);
				mysystem(ws, cmd);
				sprintf(cmd, "(cd %s/dev; ./MAKEDEV ndl%d)",
				    ROOT_DIR, ndl);
				mysystem(ws, cmd);
			}
		}
	} SETUP_END_FOREACH
	fprintf(ndfp, "son\n");
	fprintf(ndfp, "#\n");
	fprintf(ndfp, "# End of lines added by the Sun Setup Program\n");
	fprintf(ndfp, "#\n");
	fclose(ndfp);
	add_ether_lines(ws);
	/*
	 * Move a copy of the new /etc/hosts file into /etc
	 * for the duration of the ifconfig and nd commands.
	 */
	sprintf(cmd, "cp %s %s.orignal", HOSTS, HOSTS);
	mysystem(ws, cmd);
	sprintf(cmd, "cp %s%s %s", ROOT_DIR, HOSTS, HOSTS);
	mysystem(ws, cmd);
	sprintf(cmd, "cp %s%s %s", ROOT_DIR, ETHERS, ETHERS);
	mysystem(ws, cmd);
	ifconfig(ws);
	sprintf(cmd, "/etc/nd - < %s%s", ROOT_DIR, NDLOCAL);
	mysystem(ws, cmd);
	sprintf(cmd, "cp %s.orignal %s", HOSTS, HOSTS);
	mysystem(ws, cmd);
}

/*
 * Put the lines into /etc/nd.local for the pub partitions.
 * Each pub for each architecture occupies all of a hard partition.
 * Get the hard partition for the pub for each architecture and map 
 * an nd pub partition over it.
 */
static
pub_nd(ws, fp)
Workstation	ws;
FILE		*fp;
{
	Arch_served	arch;
	Hard_partition	pub;
	Soft_partition	soft;
	Arch_type	arch_type;
	int		next_ndp;
	int		ndp;
	int		size;
	int		ix;

	next_ndp = 1;
	SETUP_FOREACH_OBJECT(ws, WS_ARCH_SERVED_ARRAY, ix, arch) {
		arch_type = (Arch_type) setup_get(arch, ARCH_SERVED_TYPE);
		if (arch_type == MC68010) {
			ndp = 0;
		} else {
			ndp = next_ndp;
			next_ndp++;
		}
		pub = (Hard_partition) setup_get(arch, ARCH_SERVED_PUBHARD);
		size = (int) setup_get(pub, HARD_SIZE);
		fprintf(fp, "user 0 %d /dev/%s 0 %d -1\n",
		    ndp, setup_get(pub, HARD_NAME), size);
		soft = (Soft_partition) setup_create(SOFT_PARTITION,
		    SOFT_HARD_PARTITION,pub,
		    0);
		setup_set(soft,
		    SOFT_OFFSET,	0,
		    SOFT_SIZE,		size,
		    SOFT_NDL,		ndp,
		    0);
		setup_set(pub, 
		    HARD_SOFT_PARTITION, SETUP_APPEND, soft,
		    0);
	} SETUP_END_FOREACH
}

/*
 * Add the ethernet address to /etc/ethers.
 */
static
add_ether_lines(ws)
Workstation 	ws;
{
	FILE		*ethersfp;
	Client		client;
	char		file[BUF];
	int		ix;

	message(ws, "Updating %s", ETHERS);
	sprintf(file, "%s%s", ROOT_DIR, ETHERS);
	ethersfp = fopen(file, "w");
	if (ethersfp == NULL) {
		error(ws, SETUP_EOPEN_FAILED, file, "writing");
	}
	fprintf(ethersfp, "#\n");
	fprintf(ethersfp, "# These lines added by the Sun Setup Program\n");
	fprintf(ethersfp, "#\n");
	get_etheraddr(ws, ethersfp);
	SETUP_FOREACH_OBJECT(ws, WS_CLIENT, ix, client)		
		fprintf(ethersfp, "%s %s\n", 
		    setup_get(client, CLIENT_E_ADDR),
		    setup_get(client, CLIENT_NAME));
	SETUP_END_FOREACH
	fprintf(ethersfp, "#\n");
	fprintf(ethersfp, "# End of lines added by the Sun Setup Program\n");
	fprintf(ethersfp, "#\n");
	fclose(ethersfp);
}

/*
 * Get the ethernet address of the machine being installed.
 */
static struct ifreq	ifr;

static
get_etheraddr(ws, fp)
Workstation	ws;
FILE		*fp;
{
	int			ether_ix;
	char			*ether_str;
	char			*dev;
	int			s; 
	struct ether_addr	*eap;

	ether_ix = (int) setup_get(ws, WS_ETHERTYPE);
	ether_str = setup_get(ws, SETUP_CHOICE_STRING, CONFIG_ETHERTYPE, 
	    ether_ix);
	dev = get_device_abbrev(ether_str);

	if ((s = socket (AF_NIT, SOCK_RAW, NITPROTO_RAW)) < 0) {
		runtime_message(SETUP_EETHERADDR, "could not open socket");
		message(ws, setup_msgbuf);
	    	return;
	}
        strncpy(ifr.ifr_name, dev, sizeof ifr.ifr_name);
        if (ioctl (s, SIOCGIFADDR, (caddr_t) &ifr) < 0) {
		runtime_message(SETUP_EETHERADDR, "ioctl (SIOCGIFADDR)");
		message(ws, setup_msgbuf);
		close(s);
	    	return;
        }

	eap = (struct ether_addr *) &ifr.ifr_addr.sa_data[0];
	fprintf(fp, "%x:%x:%x:%x:%x:%x\t%s\n", 
	    eap->ether_addr_octet[0], eap->ether_addr_octet[1],
	    eap->ether_addr_octet[2], eap->ether_addr_octet[3],
	    eap->ether_addr_octet[4], eap->ether_addr_octet[5],
	    setup_get(ws, WS_NAME));

	close(s);
}

/*
 * Move the appropriate files to pub and setup the symbolic links
 * The real work is done in a shell file.
 */
static
xtr_usrarch(ws, arch)
Workstation 	ws;
Arch_served	arch;
{
	Hard_partition	pub;
	char		cmd[BUF];
	char		archstr[BUF];
	char		*archname;
	int		nfile;

	archname = get_archname(arch, archstr);
	pub = (Hard_partition) setup_get(arch, ARCH_SERVED_PUBHARD);
	nfile = get_tapeup(ws, archname, "usr");
	message(ws, "Extracting the `usr' files");
	sprintf(cmd, "%s %s/xtr_usrarch %s %s %s %s %d %d",
	    get_remote(ws),
	    SETUP_DIR,
	    setup_get(ws, WS_NAME),
	    setup_get(pub, HARD_NAME),
	    archname,
	    get_tapedev(),
	    nfile - 1,
	    get_blocksize());
	mysystem(ws, cmd);
}
		
/*
 * Extract the architecture-dependent files that go on the root
 * partition.
 */
static
xtr_rootarch(ws, arch)
Workstation 	ws;
Arch_served	arch;
{
	char		cmd[BUF];
	char		archstr[BUF];
	char		*archname;
	int		nfile;

	archname = get_archname(arch, archstr);
	nfile = get_tapeup(ws, archname, "pub");
	message(ws, "Extracting the public files");
	sprintf(cmd, "%s %s/xtr_rootarch %s %s %d %d",
	    get_remote(ws),
	    SETUP_DIR,
	    archname, 
	    get_tapedev(),
	    nfile - 1,
	    get_blocksize());
	mysystem(ws, cmd);
}

/*
 * Create symbolic links to the directories of the appropriate
 * architecture.  Also copy a few files between pub and the
 * server's root.
 */
static
xtr_symlinks(ws, arch)
Workstation	ws;
Arch_served	arch;
{
	char		cmd[BUF];
	char		archstr[BUF];
	char		*archname;

	archname = get_archname(arch, archstr);
	sprintf(cmd, "%s/xtr_symlinks %s", SETUP_DIR, archname);
	mysystem(ws, cmd);
}

/*
 * Initialize the client disks
 * This includes doing a mkfs on the disk,
 * copying the files to the disk,
 * editing the local copy of /etc/hosts,
 * and making the proper devices for the client.
 * All the clients of a given architecture are done here at 
 * one time.  The first client is initialized from the tape
 * and each subsequent client is initialized from the first one.
 */
static
xtr_clients(ws, serv_arch, arch)
Workstation 	ws;
Arch_served	serv_arch;
Arch_served	arch;
{
	Client		client;
	Arch_type	client_arch;
	Arch_type	arch_type;
	Soft_partition	root_soft;
	Yp_type		yp_type;
	char		cmd[BUF];
	char		archstr[100];
	char		*archname;
	char		*client_name;
	char		*ws_name;
	char		*domain_name;
	char		*fstab;
	char		*yp_str;
	int		first_client;
	int		pub_unit;
	int		nfile;
	int		ndl;
	int		ix;
	
	first_client = -1;
	archname = get_archname(arch, archstr);
	nfile = get_tapeup(ws, archname, "client image");
	ws_name = setup_get(ws, WS_NAME);
	domain_name = get_domainname(ws);
	arch_type = (Arch_type) setup_get(arch, ARCH_SERVED_TYPE);
	pub_unit = get_pubunit(arch);
	yp_type = (Yp_type) setup_get(ws, WS_YPTYPE);
	yp_str = (yp_type == YP_NONE)? "no": "yes";
	SETUP_FOREACH_OBJECT(ws, WS_CLIENT, ix, client)	{
		client_arch = (Arch_type) setup_get(client, CLIENT_ARCH);
		if (client_arch != arch_type) {
			continue;
		}
		client_name = setup_get(client, CLIENT_NAME);
		message(ws, "Making a file system for client `%s'", 
		    client_name);
		root_mkfs(ws, client);
		root_soft = (Soft_partition) setup_get(client,
		    CLIENT_ROOT_PARTITION);
		ndl = (int) setup_get(root_soft, SOFT_NDL);
		message(ws, "Initializing the root file system for client `%s'",
		    client_name);
		if (first_client == -1) {
			sprintf(cmd, "%s %s/xtr_client /dev/ndl%d %s %d %d",
			    get_remote(ws),
			    SETUP_DIR, 
			    ndl, 
			    get_tapedev(), 
			    nfile - 1, 
			    get_blocksize());
			mysystem(ws, cmd);
			first_client = ndl;
		} else {
			sprintf(cmd, "%s/copy_client /dev/ndl%d /dev/ndl%d",
			    SETUP_DIR,
			    first_client,
			    ndl);
			    mysystem(ws, cmd);
		}
		
		/*
		 * Now fix the client's root partition.
	 	 * Create devices, links, etc.
		 */
		fstab = build_fstab(ws, serv_arch, client, arch);
		sprintf(cmd, "%s/fix_client /dev/ndl%d %d %s %s %s %s %s",
		    SETUP_DIR,
		    ndl, pub_unit, 
		    fstab, client_name, domain_name, archname, yp_str);
		mysystem(ws, cmd);
		unlink(fstab);
	} SETUP_END_FOREACH
}

/*
 * Build an /etc/fstab file for the clients.
 * The fstab file is a little bit different for each architecture.
 */
static	char	*
build_fstab(ws, serv_arch, client, arch)
Workstation	ws;
Arch_served	serv_arch;
Client		client;
Arch_served	arch;
{
	FILE		*fp;
	char		client_archstr[BUF];
	char		server_archstr[BUF];
	char		*client_archname;
	char		*server_archname;
	char		*server_name;
	char		*fstab;
	int		pub_unit;
	static	char	tmpfile[BUF];

	strcpy(tmpfile, "/tmp/fstab.XXXXXXX");
	fstab = mktemp(tmpfile);
	fp = fopen(fstab, "w");
	if (fp == NULL) {
		error(ws, SETUP_EOPEN_FAILED, fstab, "writing");
	}
	server_name = setup_get(ws, WS_NAME);
	client_archname = get_archname(arch, client_archstr);
	server_archname = get_archname(serv_arch, server_archstr);
	pub_unit = get_pubunit(arch);
	fprintf(fp, "/dev/nd0 / 4.2 rw 1 1\n");
	fprintf(fp, "/dev/ndp%d /pub 4.2 ro 0 0\n", pub_unit);
	fprintf(fp, "%s:/usr.%s /usr nfs ro,hard%s 0 0\n",
	    server_name, client_archname,
	    three_com_fstab(serv_arch, client));
	fprintf(fp, "%s:/usr/%s /usr/%s nfs rw,hard%s 0 0\n",
	    server_name, server_name, server_name,
	    three_com_fstab(serv_arch, client));
	fclose(fp);
	return(fstab);
}

/*
 * Given an architecture, return its pub partitions unit number.
 */
static	int
get_pubunit(arch)
Arch_served	arch;
{
	Hard_partition	pub;
	Soft_partition	pub_soft;
	int		pub_unit;

	pub = (Hard_partition) setup_get(arch, ARCH_SERVED_PUBHARD);
	pub_soft = (Soft_partition) setup_get(pub, HARD_SOFT_PARTITION, 0);
	pub_unit = (int) setup_get(pub_soft, SOFT_NDL);
	return(pub_unit);
}

/*
 * For TFTP booting we need to make a link between the users
 * Internet address and the correct boot program in directory
 * /tftpboot.
 *
 * Currently, the tftpboot directory is in /pub for each architecture
 * that is being served.  Create /tftpboot and move the contents
 * of each /pub.arch/tftpboot directory to /tftpboot.
 *
 * Then create the links to the appropriate files.
 */
static
tftpboot(ws, serv_arch)
Workstation	ws;
Arch_served	serv_arch;
{
	Arch_served	arch;
	Arch_type	arch_type;
	Arch_type	client_arch;
	Client		client;
	char		cmd[BUF];
	char		archstr[BUF];
	char		*archname;
	int		aix;
	int		cix;

	sprintf(cmd, "mkdir %s/tftpboot", ROOT_DIR);
	mysystem(ws, cmd);
	SETUP_FOREACH_OBJECT(ws, WS_ARCH_SERVED_ARRAY, aix, arch) {
		arch_type = (Arch_type) setup_get(arch, ARCH_SERVED_TYPE);
		archname = get_archname(arch, archstr);
		/* must handle symbolic links too */
		sprintf(cmd, "tar cf - -C %s/pub.%s/tftpboot . |\
		   (cd %s/tftpboot; tar xf - )", ROOT_DIR, archname, ROOT_DIR);
		mysystem(ws, cmd);
		sprintf(cmd, "rm -rf %s/pub.%s/tftpboot", ROOT_DIR, archname);
		mysystem(ws, cmd);
		SETUP_FOREACH_OBJECT(ws, WS_CLIENT, cix, client) {
			client_arch = (Arch_type) setup_get(client,CLIENT_ARCH);
			if (client_arch != arch_type) {
				continue;
			}
			tftplink(ws, client, arch);
		} SETUP_END_FOREACH
	} SETUP_END_FOREACH
}

/*
 * For a given client, create a link between a file named from
 * the client's internet number, and one of several tftpboot
 * files.
 */
static
tftplink(ws, client, arch)
Workstation	ws;
Client		client;
Arch_served	arch;
{
	Arch_type	arch_type;
	char		oldfile[BUF];
	char		newfile[BUF];
	char		internet_str[BUF];
	char		*network;
	char		*hostnum;
	char		*arch_str;
	int		inetnum;
	int		pubunit;

	network = setup_get(ws, WS_NETWORK);
	hostnum = setup_get(client, CLIENT_HOST_NUMBER);
	arch_type = (Arch_type) setup_get(arch, ARCH_SERVED_TYPE);
	sprintf(internet_str, "%s.%s", network, hostnum);
	inetnum = inet_addr(internet_str);
	pubunit = get_pubunit(arch);
	switch (arch_type) {
	case MC68010:
		arch_str = "sun2";
		break;
	case MC68020:
		arch_str = "sun3";
		break;
	}
	sprintf(oldfile, "ndboot.%s.pub%d", arch_str, pubunit);
	sprintf(newfile, "%s/tftpboot/%X", ROOT_DIR, inetnum);
	symlink(oldfile, newfile);
}

/*
 * Here for some real slimy-ness.
 * /bin is a symlink to /pub/bin and /pub is a mounted file system.
 * When coming up single user, /pub is not mounted before a copy
 * of the shell is needed to get into the single user state.  Therefore,
 * in the /pub directory on the root partition a 'bin' directory
 * is created and a copy of the shell is put there.
 * That is what is being accomplished here.
 *
 * How to do it is another matter.  At this point, the entire
 * heirarchy is mounted under ROOT_DIR.  We will copy
 * /ROOT_DIR/bin/sh to /tmp and then umount all the filesystems
 * but the root.  Then we will mkdir /ROOT_DIR/pub/bin and copy /tmp/sh
 * to /ROOT_DIR/pub/bin.
 */
static
shadow_pub_sh(ws, serv_arch)
Workstation	ws;
Arch_served	serv_arch;
{
	Mount_list	ml;
	char		cmd[BUF];
	char		archstr[BUF];
	char		*archname;

	sprintf(cmd, "cp %s/bin/sh /tmp", ROOT_DIR);
	mysystem(ws, cmd);
	for (ml = get_mountlist(); ml != NULL; ml = ml->next) {
		sprintf(cmd, "/etc/umount %s", ml->dev);
		mysystem(ws, cmd);
	}
	archname = get_archname(serv_arch, archstr);
	sprintf(cmd, "mkdir %s/pub.%s/bin", ROOT_DIR, archname);
	mysystem(ws, cmd);
	sprintf(cmd, "cp /tmp/sh %s/pub.%s/bin", ROOT_DIR, archname);
	mysystem(ws, cmd);
}

/*
 * Edit the /etc/rc.boot file to change the mount command that
 * mounts the pub partition for the server's architecture.
 */
static
fix_rcboot(ws, serv_arch)
Workstation	ws;
Arch_served	serv_arch;
{
	char		cmd[BUF];
	char		archstr[BUF];
	char		*archname;

	archname = get_archname(serv_arch, archstr);
	sprintf(cmd, "%s/fix_rc.boot %s/etc %s", 
	    SETUP_DIR, 
	    ROOT_DIR, archname);
	mysystem(ws, cmd);
}

/*
 * If we have a 68010 client with a 3-com ethernet board
 * being served by a 68020 server, then there is a funny 
 * extra arg in the nd.local file that has the value "2".
 */
static	char	*
three_com_nd(serv_arch, client)
Arch_served	serv_arch;
Client		client;
{
	if (three_com_weirdness(serv_arch, client)) {
		return("2");
	}
	return("");
}

/*
 * If we have a 68010 client with a 3-com ethernet board
 * being served by a 68020 server, then there is an extra
 * parameter in the fstab file that set the read size to 2k.
 */
static	char	*
three_com_fstab(serv_arch, client)
Arch_served	serv_arch;
Client		client;
{
	if (three_com_weirdness(serv_arch, client)) {
		return(",rsize=2048");
	}
	return("");
}

/*
 * Answer the question: Is this a 68010 client with a 3-com ethernet
 * board being served by a 68020 server.
 */
static	Boolean
three_com_weirdness(serv_arch, client)
Arch_served	serv_arch;
Client		client;
{
	Arch_type	serv_arch_type;
	Arch_type	client_arch_type;
	Boolean		three_com;

	serv_arch_type = (Arch_type) setup_get(serv_arch, ARCH_SERVED_TYPE);
	client_arch_type = (Arch_type) setup_get(client, CLIENT_ARCH);
	three_com = (Boolean) setup_get(client, CLIENT_3COM_INTERFACE);
	if (serv_arch_type == MC68020 && client_arch_type == MC68010 &&
	    three_com == true) {
		return(true);
	}
	return(false);
}
