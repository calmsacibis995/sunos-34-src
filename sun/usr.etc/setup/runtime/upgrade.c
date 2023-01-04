#ifndef lint
static	char sccsid[] = "@(#)upgrade.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include <netdb.h>
#include "setup_runtime.h"
#include "install.h"

typedef struct	hostent 	*Hostent;
typedef union	Internet 	Internet;

union	Internet {
	int		inetnum;	/* Internet number */
	unsigned char	bytes[4];	/* The four constituent bytes */
	struct	{
		int	netnum:   8;
		int	hostnum: 24;
	} classa;			/* Class A network number */
	struct	{
		int	netnum:   8;
		int	dummy:	  8;
		int	hostnum: 16;
	} classb;			/* Class B network number */
	struct	{
		int	netnum:   8;
		int	dummy:	 16;
		int	hostnum:  8;
	} classc;			/* Class C network number */
};

/*
 * Boundaries for network number classes.
 */
#define CLASSAMIN         0
#define CLASSAMAX       127
#define CLASSBMIN       128
#define CLASSBMAX       191

static	char		domainname_cmd[] = "/bin/domainname";
static	char		hostname_cmd[]   = "/bin/hostname";
static	char		hostname_eq[]    = "hostname=";
static	char		pub_name[]       = "0";

static	Client		find_client();
static	Client		add_client();
static	Soft_partition	add_softpart();
static	Boolean 	userline();
static	Boolean 	etherline();
static	char		*find_line();

/*
 * Initialization routine for "setup -u" the upgrade option.
 * Upgrade means that informmation pertaining to clients, disk
 * partitions, host numbers, etc. will be obtained from places
 * such as /etc/nd.local and the yellow pages.
 */
upgrade_init(ws)
Workstation	ws;
{
	Controller	cont;
	Disk		disk;
	Hard_partition	root;
	char		*rootname;

	/*
	 * Consider the root parition to be the first partition 
	 * on the first unit of the first controller.
	 */
	cont = (Controller) setup_get(workstation, WS_CONTROLLER, 0);
	disk = (Disk) setup_get(cont, CONTROLLER_DISK, 0);
	root = (Hard_partition) setup_get(disk, DISK_HARD_PARTITION, 0);
	rootname = setup_get(root, HARD_NAME);
	mount_root(ws, rootname);
	my_info(ws);
	parse_nd(ws);
	parse_ethers(ws);
	get_netinfo(ws);
	read_setup_info(ws);
	unmount_root(ws, rootname);
}

/*
 * Get info about the workstation we are installing on.
 * Look for the hostname, host number, network number and domainname.
 */
static
my_info(ws)
Workstation	ws;
{
	setup_set(ws,
	    WS_TYPE,		WORKSTATION_STANDALONE,
	    PARAM_AUTOHOST,	false,
	    0);
	get_hostname(ws);
	get_domain(ws);
	get_mounts(ws);
}

/*
 * Find the machines host name.
 * Here we must poke around a bit.
 * It may be defined in /etc/rc.local or /etc/rc.boot.
 * Look in /etc/rc.local for a line like
 *	/bin/hostname name
 * Look in /etc/rc.boot for a line like
 *	hostname=name
 */
static
get_hostname(ws)
Workstation		ws;
{
	FILE		*fp;
	char		file[BUF];
	char		line[BUF];
	char		host[BUF];

	sprintf(file, "%s%s", ROOT_DIR, RCLOCAL);
	fp = fopen(file, "r");
	if (fp != NULL) {
		if (find_line(fp, line, sizeof(line), hostname_cmd) != NULL) {
			sscanf(line, "%*s %s", host);
			add_host(ws, host);
			fclose(fp);
			return;
		}
		fclose(fp);
	}

	sprintf(file, "%s%s", ROOT_DIR, RCBOOT);
	fp = fopen(file, "r"); 
	if (fp != NULL) { 
		if (find_line(fp, line, sizeof(line), hostname_eq) != NULL) {
			sscanf(line, "%*[^=]=%s", host);
			add_host(ws, host);
			fclose(fp);
			return;
		}
		fclose(fp);
	}
}

/*
 * Tell the middle-end about the host info.
 */
static
add_host(ws, host)
Workstation	ws;
char		*host;
{
	Internet	inet;
	Hostent 	hostent;
	char		netnum[20];
	char		hostnum[20];

	hostent = gethostbyname(host);
	if (hostent != NULL) {
		inet.inetnum = *(int *) hostent->h_addr;
		get_nethost(&inet, netnum, hostnum);
		setup_set(ws,
		    WS_NETWORK, 	netnum,
		    WS_HOST_NUMBER,	hostnum,
		    0);
	}
	setup_set(ws,
	    WS_NAME,		host,
	    0);
}

/*
 * Find the machine's domain name.
 * Look in /etc/rc.local for a line like
 *	/bin/domainname name
 */
static
get_domain(ws)
Workstation	ws;
{
	FILE		*fp;
	char		file[BUF];
	char		line[BUF];
	char		domain[BUF];

	sprintf(file, "%s%s", ROOT_DIR, RCLOCAL);
	fp = fopen(file, "r");
	if (fp != NULL) {
		if (find_line(fp, line, sizeof(line), domainname_cmd) != NULL) {
			sscanf(line, "%*s %s", domain);
			setup_set(ws,
			    WS_DOMAIN,		domain,
			    0);
			fclose(fp);
			return;
		}
		fclose(fp);
	}
}

/*
 * Given a file pointer read each line and see if it begins
 * with the given string.
 */
static	char	*
find_line(fp, line, linesize, matchstr)
FILE		*fp;
char		*line;
int		linesize;
char		*matchstr;
{
	while (fgets(line, linesize, fp) != NULL) {
		if (strncmp(line, matchstr, strlen(matchstr)) == 0) {
			return(line);
		}
	}
	return(NULL);
}

/*
 * Read the nd.local file to build a table of clients and
 * partitions.  People are encouraged to comment out lines
 * that are valid but unused.  So here we look at commented
 * lines as well.
 */
static
parse_nd(ws)
Workstation	ws;
{
	FILE		*ndfp;
	char		line[BUF];
	char		file[BUF];
	char		*cp;

	sprintf(file, "%s%s", ROOT_DIR, NDLOCAL);
	ndfp = fopen(file, "r");
	if (ndfp == NULL) {
		return;
	}
	while (fgets(line, sizeof(line), ndfp) != NULL) {
		cp = line;
		while (*cp != '\0' && (isspace(*cp) || *cp == '#')) {
			cp++;
		}
		if (userline(ws, cp)) {
			continue;
		}
		if (etherline(ws, cp)) {
			continue;
		}
	}
	fclose(ndfp);
}

/*
 * See if the line is an nd.local "user" line.
 * If so, parse it and build the appropriate data structures.
 */
static	Boolean
userline(ws, line)
Workstation	ws;
char		*line;
{
	Client		client;
	char		dev[100];
	char		name[100];
	char		keyword[100];
	int		offset;
	int		length;
	int		ndl;
	int		unit;

	if (sscanf(line, "%s %s %d %s %d %d %d", 
	    keyword, name, &unit, dev, &offset, &length, &ndl) != 7) {
		return(false);
	}
	if (!streq(keyword, "user")) {
		return(false);
	}
	setup_set(ws,
	    WS_TYPE,	WORKSTATION_SERVER,
	    0);
	if (!streq(name, pub_name)) {
		client = add_client(ws, name);
		if (unit == 0) {
			add_softpart(ws, client, dev, 
			    CLIENT_ROOT_PARTITION_INDEX, ndl);
			setup_set(client, 
			    CLIENT_ROOT_SIZE, length, 
			    0);
		} else if (unix == 1) {
			add_softpart(ws, client, dev, 
			    CLIENT_SWAP_PARTITION_INDEX, ndl);
			setup_set(client, 
			    CLIENT_SWAP_SIZE, length, 
			    0);
		}
	}
	return(true);
}

/*
 * See if the line is an nd.local "ether" line.
 * Assume that the "ether" line comes after the lines for
 * the partitions.  This means that the client has already
 * been entered to the middle end.
 */ 
static	Boolean
etherline(ws, line)
Workstation	ws;
char		*line;
{
	Client		client;
	char		keyword[100];
	char		name[100];
	char		eaddr[100];

	if (sscanf(line, "%s %s %s", keyword, name, eaddr) != 3) {
		return(false);
	}
	if (!streq(keyword, "ether")) {
		return(false);
	}
	set_eaddr(ws, name, eaddr);
	return(true);
}

/*
 * Set a client's ethernet address.
 */
static
set_eaddr(ws, name, eaddr)
Workstation	ws;
char		*name;
char		*eaddr;
{
	Client		client;

	client = find_client(ws, name);
	if (client != NULL) {
		setup_set(client,
		    CLIENT_E_ADDR,	eaddr,
		    0);
	}
}

/*
 * Look in the /etc/ethers file for ethernet addresses.
 */
static
parse_ethers(ws)
Workstation	ws;
{
	FILE		*ethersfp;
	char		file[BUF];
	char		line[BUF];
	char		eaddr[BUF];
	char		client_name[BUF];
	int		dummy;
	int		n;

	sprintf(file, "%s%s", ROOT_DIR, ETHERS);
	ethersfp = fopen(file, "r");
	if (ethersfp == NULL) {
		return;
	}
	while (fgets(line, sizeof(line), ethersfp) != NULL) {
		n = sscanf(line, "%s %s", eaddr, client_name);
		if (n != 2) {
			continue;
		}
		n = sscanf(eaddr, "%x:%x:%x:%x:%x:%x", &dummy, &dummy, &dummy,
		    &dummy, &dummy, &dummy);
		if (n != 6) {
			continue;
		}
		set_eaddr(ws, client_name, eaddr);
	}
	fclose(ethersfp);
}

/*
 * Add a name to the list of clients.
 * Check if the client has already been installed.
 */
static	Client
add_client(ws, name)
Workstation 	ws;
char		*name;
{
	Client	client;

	client = find_client(ws, name);
	if (client == NULL) {
		client = (Client) setup_create(CLIENT, 
		    CLIENT_NAME, 	name,
		    0);
		setup_set(ws, 
		    WS_CLIENT, SETUP_APPEND,	client,
		    0);
	}
	return(client);
}

/*
 * General purpose find routine.
 * Given a parent and a attribute, its search each element of
 * that attribute for a given child attribute.  The child
 * attribute must be a string and is compared with an arg
 * that is passed in.
 */
static	Client
find_client(ws, name)
Workstation	ws;
char		*name;
{
	Client		client;
	char		*client_name;
	int		ix;

	SETUP_FOREACH_OBJECT(ws, WS_CLIENT, ix, client) {
		client_name = setup_get(client, CLIENT_NAME);
		if (streq(name, client_name)) {
			return(client);
		}
	} SETUP_END_FOREACH
	return(NULL);
}


/*
 * Add a soft partition to a hard partition.
 */
static	Soft_partition
add_softpart(ws, client, dev, attr, ndl)
Workstation	ws;
Client		client;
char		*dev;
Setup_attribute	attr;
int		ndl;
{
	Disk		disk;
	Hard_type	hard_type;
	Hard_partition	hard;
	Hard_partition	ndhard;
	Soft_partition	soft;
	Boolean		floating;
	int		ix;

	hard = find_hard(ws, dev);
	if (hard == NULL) {
		return(NULL);
	}

	/*
	 * Get the type of the the hard partition this soft partition
	 * is to go on.  If the hard partition is FREE, make it into
	 * an ND partition.  If it is some other kind of non-ND partition,
	 * then try to place the soft partition on the first ND partition.
	 */
	hard_type = (Hard_type) setup_get(hard, HARD_TYPE);
	switch (hard_type) {
	case HARD_FREE:
		setup_set(hard,
		    HARD_TYPE,	HARD_ND,
		    0);
		disk = (Disk) setup_get(hard, HARD_DISK);
		floating = (Boolean) setup_get(disk, DISK_PARAM_FLOATING);
		if (!floating) {
			setup_set(disk, 
			    DISK_PARAM_FLOATING, true,
			    0);
		}
		break;
	case HARD_ND:
		break;
	default:
		hard = (Hard_partition) setup_get(ws, WS_ND_PARTITION, 0);
		break;
	}

	if (attr == CLIENT_ROOT_PARTITION_INDEX) {
		soft = (Soft_partition) setup_get(client,CLIENT_ROOT_PARTITION);
	} else {
		soft = (Soft_partition) setup_get(client,CLIENT_SWAP_PARTITION);
	}

	SETUP_FOREACH_OBJECT(ws, WS_ND_PARTITION, ix, ndhard) {
		if (ndhard == hard) {
			setup_set(client, attr, ix, 0);
			break;
		}
	} SETUP_END_FOREACH

	setup_set(soft,
	    SOFT_NDL,		ndl,
	    0);
		
	return(soft);
}

/*
 * Get the internet number for each of the clients.
 */
static
get_netinfo(ws)
Workstation	ws;
{
	Client		client;
	Hostent 	hostent;
	Internet	inet;
	char		cmd[BUF];
	char		inetbuf[100];
	char		*client_name;
	char		hostnum[20];
	char		netnum[20];
	int		ix;

	sprintf(cmd, "cp %s%s %s", ROOT_DIR, HOSTS, HOSTS);
	mysystem(ws, cmd);
	SETUP_FOREACH_OBJECT(ws, WS_CLIENT, ix, client) {
		client_name = (char *) setup_get(client, CLIENT_NAME);
		hostent = gethostbyname(client_name);
		if (hostent != NULL) {
			inet.inetnum = *(int *) hostent->h_addr;
			sprintf(inetbuf, "%u.%u.%u.%u", 
			    inet.bytes[0], inet.bytes[1], 
			    inet.bytes[2], inet.bytes[3]);
			get_nethost(&inet, netnum, hostnum);
			setup_set(client, 
			    CLIENT_HOST_NUMBER,	hostnum,
			    0);
		} else {
			message(ws,"Could not find an Internet number for client `%s'\n",
			    client_name);
		}
	} SETUP_END_FOREACH
}

/*
 * Given an internet number return the network number and
 * the host number as strings.
 * Depending upong the class of the network, the network
 * number is the first 1, 2, or 3 bytes of the internet
 * number and the host number is the remaining bytes.
 */
static
get_nethost(inetp, net, host)
Internet	*inetp;
char		*net;
char		*host;
{
	int	hibyte;

	hibyte = inetp->bytes[0];
	if (hibyte >= CLASSAMIN && hibyte <= CLASSAMAX) {
		sprintf(net, "%u", inetp->bytes[0]);
		sprintf(host, "%d", inetp->classa.hostnum);
	} else if (hibyte >= CLASSBMIN && hibyte <= CLASSBMAX) { 
		sprintf(net, "%u.%u", inetp->bytes[0], inetp->bytes[1]);
		sprintf(host, "%d", inetp->classb.hostnum);
	} else {
		sprintf(net, "%u.%u.%u", inetp->bytes[0], inetp->bytes[1],
		    inetp->bytes[2]);
		sprintf(host, "%d", inetp->classc.hostnum);
	}
}

/*
 * Get the cpu types of the clients.
 * Set both the cpu type for the client and set the arch served
 * for the server.
 */
read_client_cpu_types(ws, fp)
Workstation	ws;
FILE		*fp;
{
	Client		client;
	Arch_type	arch_type;
	Boolean		client_3com;
	char		line[BUF];
	char		client_name[BUF];
	char		archstr[BUF];
	char		three_com[BUF];
	int		c;

	for (;;) {
		c = skipspace(fp);
		if (c == EOF) {
			break;
		}
		ungetc(c, fp);
		if (c == '%') {
			break;
		}
		if (fgets(line, sizeof(line), fp) == NULL) {
			break;
		}
		sscanf(line, "%s %s %s", client_name, archstr, three_com);
		arch_type = archstr_to_type(archstr);
		upgrade_arch_served(ws, arch_type);
		client = find_client(ws, client_name);
		if (client != NULL) {
			if (streq(three_com, "3com")) {
				client_3com = true;
			} else {
				client_3com = false;
			}
			setup_set(client,
			    CLIENT_ARCH,	arch_type,
			    CLIENT_3COM_INTERFACE, client_3com,
			    0);
		}
	}
}

/*
 * Save the clients names and cpu types in the file /etc/setup.info.
 * This information will be used by setup on a subsequent "upgrade"
 * install.
 */
save_client_cpu_types(ws)
Workstation	ws;
{
	Client		client;
	Arch_type	client_arch;
	Boolean		client_3com;
	FILE		*fp;
	char		file[BUF];
	char		*client_name;
	char		*cpustr;
	char		*three_com;
	int		ix;

	sprintf(file, "%s%s", ROOT_DIR, SETUP_INFO);
	fp = fopen(file, "w");
	if (fp == NULL) {
		runtime_message(SETUP_EOPEN_FAILED, file, "writing");
		message(ws, setup_msgbuf);
		return;
	}
	fprintf(fp, "%% Client_cpu_types\n");
	SETUP_FOREACH_OBJECT(ws, WS_CLIENT, ix, client) {
		client_name = (char *) setup_get(client, CLIENT_NAME);
		client_arch = (Arch_type) setup_get(client, CLIENT_ARCH);
		client_3com = (Boolean) setup_get(client,CLIENT_3COM_INTERFACE);
		switch(client_arch) {
		case MC68010:
			cpustr = "MC68010";
			break;
		case MC68020:
			cpustr = "MC68020";
			break;
		}
		if (client_3com) {
			three_com = "3com";
		} else {
			three_com = "no3com";
		}
		fprintf(fp, "%s %s %s\n", client_name, cpustr, three_com);
	} SETUP_END_FOREACH
	fclose(fp);
}
