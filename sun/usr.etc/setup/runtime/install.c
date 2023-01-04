#ifndef lint
static	char sccsid[] = "@(#)install.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <setjmp.h>
#include "setup_runtime.h"
#include "install.h"
#include <sys/stat.h>
#include <sys/wait.h>

enum	type_of_install {
	FIRST_TIME,
	UPGRADE,
	REENTRANT,
};
enum	type_of_install	install_type();


static	int		dflag;		/* Debugging flag */
static	Mount_list	mount_list;	/* List of mounted file systems */
static	jmp_buf	jmpbuf;

static	Arch_served	servers_arch();
static	Boolean		is_on_network();
static	Mount_list	add_mount();
char			*index();
char			*mktemp();

/*
 * Do the install.
 * The front-end has collected all the information
 * and the middle-end has blessed it as being acceptable.
 * Now is the time to actually do something with it.
 */
setup_execute(ws)
Workstation ws;
{
	Arch_served	serv_arch;

	message(ws, "\n");
	runtime_message(SETUP_BEGINSTALL);
	message(ws, setup_msgbuf);
	serv_arch = servers_arch(ws);
	get_tape(ws);
	if (setjmp(jmpbuf) != 0) {
		return;			/* Return here on error conditions */
	}
	if (everything_ok(ws, serv_arch) == false) {
		return;
	}
	if (geteuid() != 0) {
		runtime_message(SETUP_NOT_SUPERUSER);
		message(ws, setup_msgbuf);
		return;
	}
	umask(022);
	label_disks(ws);
	init_tapetoc(ws, serv_arch);
	mount_pts(ws, serv_arch);
	switch (install_type()) {
	case FIRST_TIME:
		install_first_time(ws, serv_arch);
		break;
	case UPGRADE:
		install_upgrade(ws, serv_arch);
		break;
	case REENTRANT:
		install_reentrant(ws, serv_arch);
		break;
	}
	message(ws, "Installation complete");
	sync();
	sync();				/* superstition */
}

/*
 * First time installation.
 * This means installing from a tape.
 * If the workstations is a server, then build an
 * /etc/nd.local file, initialize the 'pub' area and
 * initialize each of the clients.
 */
static
install_first_time(ws, serv_arch)
Workstation 	ws;
Arch_served	serv_arch;
{
	xtr_root(ws, serv_arch);
	add_to_hosts(ws);
	hostname(ws);
	domainname(ws);
	if (isserver(ws)) {
		install_server(ws, serv_arch);
	} else {
		xtr_standpub(ws, serv_arch);
		xtr_standalone(ws, serv_arch);
		xtr_sys(ws, serv_arch);
		install_optsoftware(ws, serv_arch);
		yellow_pages(ws);
		sendmail(ws);
	}
}

/*
 * Upgrade installation.
 * We have an existing system and wish to upgrade it 
 * and its clients to a new release.
 */
static
install_upgrade(ws)
Workstation ws;
{
	message(ws, "Installing an upgrade is not implemented yet.");
}

/*
 * Reentrant installation.
 * We have an existing system and wish to make some changes are additions.
 */
static
install_reentrant(ws)
Workstation ws;
{
	message(ws, "Installing re-entrant changes is not implemented yet.");
}

/*
 * Determine all the mount points and create an /etc/fstab
 * and an /etc/exports file.  Do a mkfs on
 * each file system and mount them under /setup.root.
 */
static
mount_pts(ws, serv_arch)
Workstation	ws;
Arch_served	serv_arch;
{
	FILE		*tmpfp;
	FILE		*fstab;
	FILE		*exports;
	char		dir[BUF];
	char		dev[BUF];
	char		cmd[BUF];
	char		filename[BUF];
	char		type[20];
	char		options[20];
	char		*tmpfile;
	int		passno;
	int		freq;

	tmpfile = mktemp("/tmp/setup.XXXXXX");
	collect_mountpts(ws, tmpfile);
	sprintf(cmd, "sort +1 -o %s %s", tmpfile, tmpfile);
	mysystem(ws, cmd);

	tmpfp = fopen(tmpfile, "r");
	if (tmpfp == NULL) {
		error(ws, SETUP_EOPEN_FAILED, tmpfile, "reading");
	}
	fstab = fopen("/tmp/fstab", "w");
	if (fstab == NULL) {
		error(ws, SETUP_EOPEN_FAILED, "/tmp/fstab", "writing");
	}
	exports = fopen("/tmp/exports", "w");
	if (exports == NULL) {
		error(ws, SETUP_EOPEN_FAILED, "/tmp/exports", "writing");
	}
	while (fscanf(tmpfp, "%s %s %s %s %d %d", 
	    dev, dir, type, options, &freq, &passno) != EOF) {
		if (dir[0] == '/') {
			linkdir(ws, serv_arch, dir);
			fprintf(fstab, "%s %s %s %s %d %d\n", 
			    dev, dir, type, options, freq, passno);
			fprintf(exports, "%s\n", dir);
			if (streq(dir, "/")) {
				strcpy(filename, ROOT_DIR);
			} else {
				sprintf(filename, "%s%s", ROOT_DIR, dir);
				mount_list = add_mount(mount_list, dev);
			}
			mymount(ws, dev, filename);
		}
	}
	fclose(tmpfp);
	fclose(fstab);
	fclose(exports);
	unlink(tmpfile);
	sprintf(cmd, "mkdir %s/etc", ROOT_DIR);
	mysystem(ws, cmd);
	sprintf(cmd, "mv /tmp/fstab /tmp/exports %s/etc", ROOT_DIR);
	mysystem(ws, cmd);
}

/*
 * List of architecure-dependent directories.
 */
static	char	*arch_dirs[] = {
	"/usr",
	"/lib",
	"/pub",
	"/private",
	NULL
};

/*
 * Check to see if a directory pathname begins with an architecture
 * dependent name.  If so substitute the architecture dependent name.
 * This applies to server's only.
 */
static
linkdir(ws, serv_arch, dir)
Workstation	ws;
Arch_served	serv_arch;
char		*dir;
{
	char		savec;
	char		buf[BUF];
	char		archstr[BUF];
	char		*archname;
	char		*slash;
	char		**dpp;

	if (!isserver(ws)) {
		return;
	}
	archname = get_archname(serv_arch, archstr);
	slash = index(&dir[1], '/');
	if (slash != NULL) {
		savec = slash[0];
		slash[0] = '\0';
	}
	for (dpp = arch_dirs; *dpp != NULL; dpp++) {
		if (streq(dir, *dpp)) {
			if (slash != NULL) {
				sprintf(buf, "%s.%s/%s", 
				    dir, archname, &slash[1]);
			} else {
				sprintf(buf, "%s.%s", dir, archname);
			}
			strcpy(dir, buf);
			return;
		}
	}
	if (slash != NULL) {
		slash[0] = savec;
	}
}

/*
 * Mount a partition on a directory.
 * Make any directories that may be missing.
 */
static
mymount(ws, dev, dir)
Workstation	ws;
char		*dev;
char		*dir;
{
	struct	stat	statb;
	char		cmd[BUF];
	char		savec;
	char		*slash;

	slash = index(&dir[1], '/');
	while (slash != NULL) {
		savec = slash[0];
		slash[0] = '\0';
		if (stat(dir, &statb) == -1) {
			sprintf(cmd, "mkdir %s", dir);
			mysystem(ws, cmd);
		}
		slash[0] = savec;
		slash = index(&slash[1], '/');
	}
	if (stat(dir, &statb) == -1) {
		sprintf(cmd, "mkdir %s", dir);
		mysystem(ws, cmd);
	}
	sprintf(cmd, "/etc/mount %s %s", dev, dir);
	mysystem(ws, cmd);
}

/*
 * Collect all the mount points and put them in a temporary file.
 * Do a mkfs on each partition here.  It is done here because this
 * is where we have the info.
 */
static
collect_mountpts(ws, tmpfile)
Workstation	ws;
char		*tmpfile;
{
	Controller	cont;
	Disk		disk;
	Hard_type	hard_type;
	Hard_partition	hard;
	Hard_reserved_type res_type;
	Boolean		preserve;
	FILE		*tmpfp;
	char		*mnt_pt;
	int		passno;
	int		swapno;
	int		cix, dix, hix;

	tmpfp = fopen(tmpfile, "w");
	if (tmpfp == NULL) {
		error(ws, SETUP_EOPEN_FAILED, tmpfile, "writing");
	}
	passno = 1;
	preserve = (Boolean) setup_get(ws, WS_PRESERVED);
	SETUP_FOREACH_OBJECT(ws, WS_CONTROLLER, cix, cont) {
		SETUP_FOREACH_OBJECT(cont, CONTROLLER_DISK, dix, disk) {
			SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, hix, hard) {
				hard_type = (Hard_type) 
				    setup_get(hard, HARD_TYPE);
				if (hard_type == HARD_SWAP) {
					if (swapno > 0) {
						fprintf(tmpfp, "/dev/%s nodir swap ro 0 0\n",
						    setup_get(hard, HARD_NAME));
					}
					swapno++;
				}
				if (hard_type != HARD_UNIX) {
					continue;
				}
				mnt_pt = setup_get(hard, HARD_MOUNT_PT);
				if (mnt_pt == NULL || mnt_pt[0] == '\0') {
					continue;
				}
				fprintf(tmpfp, "/dev/%s %s 4.2 rw 1 %d\n", 
				    setup_get(hard, HARD_NAME), mnt_pt, passno);
				passno++;
				res_type = (Hard_reserved_type)
				    setup_get(hard,HARD_RESERVED_TYPE);
				if (preserve && 
				    res_type == HARD_RESERVED_NONE) {
					continue;
				}
				message(ws, "Making a file system for `%s'",
				    mnt_pt);
				newfs(ws, hard);
			} SETUP_END_FOREACH
		} SETUP_END_FOREACH
	} SETUP_END_FOREACH
	fclose(tmpfp);
}

/*
 * Add a device to the list of mounted file systems.
 * The list is kept in reverse order to make it easy
 * to umount everything.
 */
static	Mount_list
add_mount(mount_list, dev)
Mount_list	mount_list;
char		*dev;
{
	Mount_list	ml;

	ml = new(struct mount_list);
	ml->next = mount_list;
	ml->dev = strdup(dev);
	return(ml);
}

/*
 * Run the xtr shell script to extract the root files system.
 */
static
xtr_root(ws, arch)
Workstation	ws;
Arch_served	arch;
{
	Hard_partition	hard;
	char		cmd[BUF];
	char		archstr[BUF];
	char		*archname;
	int		nfile;

	archname = get_archname(arch, archstr);
	nfile = get_tapeup(ws, archname, "root");
	hard = (Hard_partition) setup_get(ws, WS_SERVED_ROOTHARD);
	message(ws, "Extracting the root files");
	sprintf(cmd, "%s %s/xtr_root %s %d %d",
	    get_remote(ws),
	    SETUP_DIR,
	    get_tapedev(),
	    nfile - 1,
	    get_blocksize());
	mysystem(ws, cmd);
	make_devices(ws);
}

/*
 * Make disk and tape devices in the new /dev directory.
 * Build one MAKEDEV command to do the trick.
 */
static
make_devices(ws)
Workstation	ws;
{
	Controller	cont;
	Disk		disk;
	char		cmd[BUF];
	int		cix;
	int		dix;

	cmd[0] = '\0';
	sprintf(cmd, "(cd %s/dev; MAKEDEV %s", ROOT_DIR, get_tapename());
	SETUP_FOREACH_OBJECT(ws, WS_CONTROLLER, cix, cont) {
		SETUP_FOREACH_OBJECT(cont, CONTROLLER_DISK, dix, disk) {
			strcat(cmd, " ");
			strcat(cmd, setup_get(disk, DISK_NAME));
		} SETUP_END_FOREACH
	} SETUP_END_FOREACH
	strcat(cmd, ")");
	mysystem(ws, cmd);
}

/*
 * Add entries to the /etc/hosts file
 * One entry for the server and each client
 */
static
add_to_hosts(ws)
Workstation ws;
{
	FILE		*hostsfp;
	Client		client;
	Yp_type		yp_type;
	char		file[BUF];
	char		*network;
	int		ix;

	message(ws, "Updating %s", HOSTS);
	if (!dflag) {
		sprintf(file, "%s%s", ROOT_DIR, HOSTS);
		hostsfp = fopen(file, "w");
		if (hostsfp == NULL) {
			error(ws, SETUP_EOPEN_FAILED, file, "writing");
		}
	} else {
		hostsfp = stdout;
		fprintf(hostsfp, "---- %s ----\n", file);
	}
	fprintf(hostsfp, "#\n");
	fprintf(hostsfp, "# If the yellow pages is running, this file is only consulted when booting\n");
	fprintf(hostsfp, "#\n");
	fprintf(hostsfp, "# These lines added by the Sun Setup Program from server %s\n",
	    setup_get(ws, WS_NAME));
	fprintf(hostsfp, "#\n");
	network = setup_get(ws, WS_NETWORK);
	if (is_on_network(ws)) {
		fprintf(hostsfp, "%s.%s\t%s loghost\n", 
		    network,
		    setup_get(ws, WS_HOST_NUMBER), 
		    setup_get(ws, WS_NAME));
	}
	if (isserver(ws)) {
		SETUP_FOREACH_OBJECT(ws, WS_CLIENT, ix, client)		
			fprintf(hostsfp, "%s.%s\t%s\n", 
			    network,
			    setup_get(client, CLIENT_HOST_NUMBER),
			    setup_get(client, CLIENT_NAME));
		SETUP_END_FOREACH
	} else if (istapeless(ws)) {
		fprintf(hostsfp, "%s\t%s\n", 
		    setup_get(ws, WS_HOST_INTERNET_NUMBER),
		    setup_get(ws, WS_TAPE_SERVER));
		
	}
	yp_type = (Yp_type) setup_get(ws, WS_YPTYPE);
	if (yp_type == YP_CLIENT || yp_type == YP_SLAVE_SERVER) {
		fprintf(hostsfp, "%s\t%s\n", 
		    setup_get(ws, WS_YPMASTER_INTERNET),
		    setup_get(ws, WS_YPMASTER_NAME));
	}
	if (network[0] != '\0') {
		fprintf(hostsfp, "127.0.0.1\tlocalhost\n");
	} else {
		fprintf(hostsfp, "127.0.0.1\tlocalhost loghost\n");
	}
	fprintf(hostsfp, "#\n");
	fprintf(hostsfp, "# End of lines added by the Sun Setup Program\n");
	fprintf(hostsfp, "#\n");
	if (!dflag) {
		fclose(hostsfp);
	}
}

/*
 * Fix the hostname in /etc/rc.boot
 */
static
hostname(ws)
Workstation ws;
{
	char		cmd[BUF];
	char		*hostname;

	hostname = (char *) setup_get(ws, WS_NAME);
	sprintf(cmd, "%s/fix_hostname %s/etc %s", 
	    SETUP_DIR, ROOT_DIR, hostname);
	mysystem(ws, cmd);
}

/*
 * Fix the domainname in /etc/rc.local
 */
static
domainname(ws)
Workstation ws;
{
	char		cmd[BUF];
	char		*domain_name;

	domain_name = get_domainname(ws);
	sprintf(cmd, "%s/fix_domainname %s/etc %s", 
	    SETUP_DIR, ROOT_DIR, domain_name);
	mysystem(ws, cmd);
}

/*
 * Extract the files that go into 'pub' on a server and put them on
 * the root.
 */
static
xtr_standpub(ws, myarch)
Workstation     ws;
Arch_served     myarch;
{
	char		cmd[BUF];
	char		archstr[BUF];
	char		*archname;
	int		nfile;

	archname = get_archname(myarch, archstr);
	nfile = get_tapeup(ws, archname, "pub");
	message(ws, "Extracting more root files");
	sprintf(cmd, "%s %s/xtr_standpub %s %d %d",
	    get_remote(ws),
	    SETUP_DIR,
	    get_tapedev(),
	    nfile - 1,
	    get_blocksize());
	mysystem(ws, cmd);
}

/*
 * Initialize the root partition for a standalone user.
 * This is the easy case.
 */
static
xtr_standalone(ws, myarch)
Workstation	ws;
Arch_served	myarch;
{
	char		cmd[BUF];
	char		archstr[BUF];
	char		*archname;
	int		nfile;

	archname = get_archname(myarch, archstr);
	nfile = get_tapeup(ws, archname, "usr");
	message(ws, "Extracting the usr files");
	sprintf(cmd, "%s %s/xtr_standalone %s %d %d",
	    get_remote(ws),
	    SETUP_DIR,
	    get_tapedev(),
	    nfile - 1,
	    get_blocksize());
	mysystem(ws, cmd);
}

/*
 * Issue an /etc/ifconfig command.
 * Set the host name first.
 */
ifconfig(ws)
Workstation	ws;
{
	char		cmd[BUF];
	char		*dev;
	char		*ether_str;
	char		*host;
	int		ether_ix;

	host = setup_get(ws, WS_NAME);
	sethostname(host, strlen(host));
	ether_ix = (int) setup_get(ws, WS_ETHERTYPE);
	ether_str = setup_get(ws, SETUP_CHOICE_STRING, CONFIG_ETHERTYPE, 
	    ether_ix);
	dev = get_device_abbrev(ether_str);
	sprintf(cmd, "/etc/ifconfig %s %s -trailers up", dev, host);
	mysystem(ws, cmd);
}

/*
 * Given an architecture, return the first word of its name.
 */
char	*
get_archname(arch, buf)
Arch_served	arch;
char		*buf;
{
	char		*archname;
	char		*blank;

	archname = setup_get(arch, ARCH_SERVED_NAME);
	strcpy(buf, archname);
	blank = index(buf, ' ');
	if (blank != NULL) {
		*blank = '\0';
	}
	return(buf);
}

/*
 * Get the server's architecture.
 */
static	Arch_served
servers_arch(ws)
Workstation	ws;
{
	Arch_served	arch;
	Arch_type	serv_type;
	Arch_type	type;
	int		ix;

	serv_type = (Arch_type) setup_get(ws, WS_ARCH);
	SETUP_FOREACH_OBJECT(ws, WS_ARCH_SERVED_ARRAY, ix, arch) {
		type = (Arch_type) setup_get(arch, ARCH_SERVED_TYPE);
		if (serv_type == type) {
			return(arch);
		}
	} SETUP_END_FOREACH
	error(ws, SETUP_ENO_SERVERARCH);
	/* NOTREACHED */
}


/*
 * Install the yellow pages as a server for the hosts data base.
 * There are four possibilities:
 *	1) Is not on a network or does not want yellow pages,
 *	2) Is a client of the yellow pages,
 *	3) Is a slave yellow pages server,
 *	4) Is a master yellow pages server.
 */
yellow_pages(ws)
Workstation	ws;
{
	Yp_type		yp_type;
	char		cmd[BUF];
	char		*yp_master;
	char		*host;
	char		*domain;

	if (!is_on_network(ws)) {	
		sprintf(cmd, "mv %s/etc/ypbind %s/etc/ypbind.orig",
		    ROOT_DIR, ROOT_DIR);
		mysystem(ws, cmd);
		return;
	}
	yp_type = (Yp_type) setup_get(ws, WS_YPTYPE);
	host = setup_get(ws, WS_NAME);
	sethostname(host, strlen(host));
	domain = get_domainname(ws);
	setdomainname(domain, strlen(domain));
	switch (yp_type) {
	case YP_MASTER_SERVER:
		message(ws, "Making `%s' a yellow pages master server",
		    setup_get(ws, WS_NAME));
		sprintf(cmd, "setup=yes /usr/etc/yp/ypinit -m");
		chroot_cmd(ws, cmd);
		break;

	case YP_SLAVE_SERVER:
		message(ws, "Making `%s' a yellow pages slave server",
		    setup_get(ws, WS_NAME));
		yp_master = setup_get(ws, WS_YPMASTER_NAME);
		sprintf(cmd, "setup=yes /usr/etc/yp/ypinit -s %s", yp_master);
		chroot_cmd(ws, cmd);
		yp_add_to_cron(ws);
		break;

	case YP_CLIENT:
		break;

	case YP_NONE:
		sprintf(cmd, "mv %s/etc/ypbind %s/etc/ypbind.orig",
		    ROOT_DIR, ROOT_DIR);
		mysystem(ws, cmd);
		break;
	}
}

/*
 * If the workstation is a yellow pages slave server,
 * then add some lines to /usr/lib/crontab.
 */
static
yp_add_to_cron(ws)
Workstation	ws;
{
	FILE		*fp;
	char		file[BUF];
	char		*hoststr;
	int		hostnum;
	int		minute;

	sprintf(file, "%s%s", ROOT_DIR, CRONTAB);
	fp = fopen(file, "a");
	if (fp == NULL) {
		return;
	}
	hoststr = setup_get(ws, WS_HOST_NUMBER);
	hostnum = atoi(hoststr);
	minute = hostnum % 60;
	fprintf(fp, "%d 0    * * * /etc/yp/ypxfr_1perday\n", minute);
	fprintf(fp, "%d 0,12 * * * /etc/yp/ypxfr_2perday\n", minute);
	fprintf(fp, "%d *    * * * /etc/yp/ypxfr_1perhour\n", minute);
	fclose(fp);
}

/*
 * Front-end to system().
 * Redirect all output from stdout and stderr to /dev/null.
 */
mysystem(ws, cmd)
Workstation	ws;
char		*cmd;
{
	FILE		*fp;
	char		buf[BUF];
	int		r;
	union	wait	w;
	static	Boolean	first = true;

	if (first) {
		unlink(SETUP_LOG);
		first = false;
	}
	if (dflag) {
		message(ws, "mysystem: %s", cmd);
		r = 0;
	} else {
		fp = fopen(SETUP_LOG, "a");
		if (fp != NULL) {
			fprintf(fp, "%s\n", cmd);
			fclose(fp);
		}
		close_on_exec();
		if (index(cmd, '>')) {
			sprintf(buf, "%s 2>> %s", cmd, SETUP_LOG);
		} else {
			sprintf(buf, "%s >> %s 2>&1", cmd, SETUP_LOG);
		}
		w.w_status = system(buf);
		if (WIFSIGNALED(w)) {
			message(ws, "Command `%s' killed by signal: %d",
			    cmd, w.w_termsig);
		} else if (WIFEXITED(w) && w.w_retcode != 0) {
			message(ws, "Command `%s' failed with exit status %d",
			    cmd, w.w_retcode);
		}
	}
	return(r);
}

/*
 * Stub - must be filled in later.
 */
static
enum type_of_install
install_type()
{
	return(FIRST_TIME);
}

/*
 * Send a message to the screen via the message proc.
 */
message(ws, str, a, b, c, d, e, f)
Workstation	ws;
char		*str;
{
	char		buf[BUF];
	Voidfunc	message_proc;

	sprintf(&buf[0], str, a, b, c, d, e, f);
	message_proc = (Voidfunc) setup_get(ws, SETUP_MESSAGE_PROC);
	message_proc(buf);
}

/*
 * If you have two many open file descriptors, the Bourne
 * shell gets very confused.  Since we may be a window program
 * here, the number of open file descriptors may be high.
 * Here we set all but stdin, stdout, and stderr to be closed on exec.
 */
close_on_exec()
{
	int 		i;
	int		n;

	n = getdtablesize();
	for (i = 3; i < n; i++) {
		ioctl(i, FIOCLEX, 0);
	}
}

/*
 * See if this workstation is on the network.
 */
static	Boolean
is_on_network(ws)
Workstation	ws;
{
	char		*ether_str;
	int		ether_ix;

	ether_ix = (int) setup_get(ws, WS_ETHERTYPE);
	ether_str = setup_get(ws, SETUP_CHOICE_STRING, CONFIG_ETHERTYPE, 
	    ether_ix);
	if (streq(ether_str, "None")) {
		return(false);
	} else {
		return(true);
	}
}

/*
 * Fatal error.
 * Print a message, then print a "fatal error" message and
 * then longjmp back to where we can return.
 */
error(ws, setup_errno, a, b, c, d, e)
Workstation	ws;
Setup_errno	setup_errno;
{
	char		buf[BUF];

	runtime_message(setup_errno, a, b, c, d, e);
	message(ws, setup_msgbuf);
	runtime_message(SETUP_EINSTALL_ABORT);
	message(ws, setup_msgbuf);
	longjmp(jmpbuf);
}

/*
 * Return a pointer to the mount list.
 */
Mount_list
get_mountlist()
{
	return(mount_list);
}

/*
 * Get the domainname.  If none, return "noname".
 */
char	*
get_domainname(ws)
Workstation	ws;
{
	char		*domain_name;

	domain_name = setup_get(ws, WS_DOMAIN);
	if (domain_name[0] == '\0') {
		domain_name = "noname";
	}
	return(domain_name);
}

/*
 * Issued a command that will run relative to the eventual root (/setup.root).
 * Use the chroot system call to fake out the commands.
 */
chroot_cmd(ws, cmd)
Workstation	ws;
char		*cmd;
{
	char		newcmd[BUF];

	sprintf(newcmd, "%s/chroot %s \"%s\"", SETUP_DIR, ROOT_DIR, cmd);
	mysystem(ws, newcmd);
}

/*
 * Install the correct sendmail configuration file.
 */
sendmail(ws)
Workstation	ws;
{
	Mail_type	mail_type;
	char		newfile[BUF];
	char		*oldfile;

	mail_type = (Mail_type) setup_get(ws, WS_MAILTYPE);
	switch (mail_type) {
	case MAIL_SERVER:
		oldfile = SENDMAIL_MAIN_CF;
		break;
	case MAIL_CLIENT:
		oldfile = SENDMAIL_SUBSIDIARY_CF;
		break;
	}
	sprintf(newfile, "%susr/lib/%s", ROOT_DIR, SENDMAIL_CF);
	unlink(newfile);
	symlink(oldfile, newfile);
}
