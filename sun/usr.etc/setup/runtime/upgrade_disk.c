#ifndef lint
static	char sccsid[] = "@(#)upgrade_disk.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"
#include "install.h"

static  int             hp_count;

static  Hard_partition  find_hard_attribute();
static  Boolean         special_partition();
static  Boolean         compare_hard_dev();
static  Boolean         compare_hard_res_type();
static  Boolean         compare_hard_count();

/*
 * Mount the root partition.
 */
mount_root(ws, root)
Workstation	ws;
char	*root;
{
	char		cmd[BUF];

	sprintf(cmd, "mkdir %s", ROOT_DIR);
	mysystem(ws, cmd);
	sprintf(cmd, "/etc/mount /dev/%s %s", root, ROOT_DIR);
	mysystem(ws, cmd);
}

/*
 * Unmount the root partition
 */
unmount_root(ws, root)
Workstation	ws;
char		*root;
{
	char		cmd[BUF];

	sprintf(cmd, "/etc/umount /dev/%s", root);
	mysystem(ws, cmd);
	sprintf(cmd, "rm -fr %s", ROOT_DIR);
	mysystem(ws, cmd);
}

/*
 * Read the /etc/fstab file and get the mount points
 */
get_mounts(ws)
Workstation	ws;
{
	Hard_partition	hard;
	FILE		*fp;
	char		file[BUF];
	char		line[BUF];
	char		dir[BUF];
	char		dev[BUF];
	char		type[BUF];

	sprintf(file, "%s%s", ROOT_DIR, FSTAB);
	fp = fopen(file, "r");
	if (fp != NULL) {
		while (fgets(line, sizeof(line), fp) != NULL) {
			if (index(line, ':') != NULL) {
				continue;
			}
			sscanf(line, "%s %s %s", dev, dir, type);
			if (streq(type, "swap")) {
				hard = find_hard(ws, dev);
				if (hard != NULL) {
					setup_set(hard, 
					    HARD_TYPE,		HARD_SWAP,
					    0);
				}
			} else if (streq(type, "4.2")) {
				if (special_partition(ws, dev, dir)) {
					continue;
				}
				hard = find_hard(ws, dev);
				if (hard != NULL) {
					setup_set(hard, 
					    HARD_TYPE,		HARD_UNIX,
					    HARD_MOUNT_PT, 	dir, 
					    0);
				}
			}
		}
		fclose(fp);
	}
}

/*
 * Given a full device name, find the hard partition handle for it.
 */
Hard_partition
find_hard(ws, dev)
Workstation	ws;
char		*dev;
{
	char		*hard_name;

	hard_name = rindex(dev, '/');
	hard_name++;
	return(find_hard_attribute(ws, compare_hard_dev, hard_name));
}

/*
 * Compare a hard partition's name with the given arg.
 */
static	Boolean
compare_hard_dev(hard, data)
Hard_partition	hard;
char		*data;
{
	char		*name;

	name = setup_get(hard, HARD_NAME);
	return((Boolean) streq(name, data));
}

/*
 * Find a hard partition with a given reserved type.
 */
static	Hard_partition
find_hard_reserved(ws, res_type)
Workstation	ws;
Hard_reserved_type	res_type;
{
	return(find_hard_attribute(ws, compare_hard_res_type, res_type));
}

/*
 * Compare a hard partition's name with the given arg.
 */
static	Boolean
compare_hard_res_type(hard, res_type)
Hard_partition	hard;
Hard_reserved_type	res_type;
{
	Hard_reserved_type	hard_res_type;

	hard_res_type = (Hard_reserved_type)setup_get(hard, HARD_RESERVED_TYPE);
	return((Boolean) (hard_res_type == res_type));
}

/*
 * Given a hard partition find it index amongst all the hard
 * partitions.
 */
static
find_hard_index(ws, hard)
Workstation	ws;
Hard_partition	hard;
{
	hp_count = 0;
	find_hard_attribute(ws, compare_hard_count, (char *) hard);
	return(hp_count);
}

/*
 * Comparison routine the increments the global hp_count while
 * searching for a given hard partition.
 */
static Boolean
compare_hard_count(hard, hard1)
Hard_partition	hard;
Hard_partition	hard1;
{
	if (hard == hard1) {
		return(true);
	}
	hp_count++;
	return(false);
}

/*
 * Generic routine for finding a hard partition with a specific
 * attribute such as a certain device name or a certain reserved type.
 * This routine goes thru each hard partition and calls a comparison
 * routine to find out if it is the right one.
 */
static	Hard_partition
find_hard_attribute(ws, ismatch, arg)
Workstation	ws;
Boolean		(*ismatch)();
char		*arg;
{
	Controller	cont;
	Disk		disk;
	Hard_partition	hard;
	int		cix, dix, hix;

	SETUP_FOREACH_OBJECT(ws, WS_CONTROLLER, cix, cont) {
		SETUP_FOREACH_OBJECT(cont, CONTROLLER_DISK, dix, disk) {
			SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, hix, hard) {
				if (ismatch(hard, arg)) {
					return(hard);
				}
			} SETUP_END_FOREACH
		} SETUP_END_FOREACH
	} SETUP_END_FOREACH
	return(NULL);
}

/*
 * Special/reserved partition information.
 */
struct	Hard_special	{
	char		*dir;			/* Special directory name */
	Workstation_type   ws_type;		/* Implies standalone/server */
	Hard_reserved_type hard_res_type;	/* Reserved type of partition */
	Arch_type	   arch_type;		/* Arch served by this part */
};
typedef	struct	Hard_special	*Hard_special;

struct	Hard_special 	hard_special_array[] = {
	"/usr",	WORKSTATION_STANDALONE, HARD_RESERVED_STANDALONE_USR, MC68010,
	"/usr.MC68010", WORKSTATION_SERVER,     HARD_RESERVED_USR010, MC68010,
	"/pub.MC68010", WORKSTATION_SERVER,     HARD_RESERVED_PUB010, MC68010,
	"/usr.MC68020", WORKSTATION_SERVER,     HARD_RESERVED_USR020, MC68020,
	"/pub.MC68020", WORKSTATION_SERVER,     HARD_RESERVED_PUB020, MC68020,
	NULL,
};

/*
 * Must handle the special partitions here.
 * Determine if this is a special partition.  The directory
 * name implies whether it is special and in turn impiles whether
 * the machine is a standalone or file server.
 * Then find the hard parition that this special partition is 
 * currently assigned to.  If that parition differs from the
 * "dev" argument, "move" the special partition.
 */
static 	Boolean
special_partition(ws, dev, dir)
Workstation	ws;
char		*dev;
char		*dir;
{
	Hard_partition	hard_to;
	Hard_partition	hard_from;
	Hard_special	hsp;
	char		*hard_name;
	char		full_hard_name[BUF];
	int		hp_index;

	if (streq(dir, "/")) {
		return(true);		/* Root partition */
	}

	for (hsp = hard_special_array; hsp->dir != NULL; hsp++) {
		if (streq(dir, hsp->dir)) {
			break;
		}
	}
	if (hsp->dir == NULL) {
		return(false);
	}
	setup_set(ws, 
	   WS_TYPE,	hsp->ws_type, 
	   0);
	if (hsp->ws_type == WORKSTATION_SERVER) {
		upgrade_arch_served(ws, hsp->arch_type);
	}
	hard_from = find_hard_reserved(ws, hsp->hard_res_type);
	if (hard_to != NULL) {
		hard_name = setup_get(hard_from, HARD_NAME);
		sprintf(full_hard_name, "/dev/%s", hard_name);
		if (!streq(full_hard_name, dev)) {

			/*
			 * Must try to move the special partition.
			 */
			hard_to = find_hard(ws, dev);
			hp_index = find_hard_index(ws, hard_to);
			setup_set(ws, 
			    WS_MOVE_HARD_PART, hard_from, hp_index,
			    0);
		}
	}
	return(true);
}

/*
 * Tell the middle end to "serve" another architecture.
 */
upgrade_arch_served(ws, arch_type)
Workstation	ws;
Arch_type	arch_type;
{
	int		map;

	map = (int) setup_get(ws, WS_ARCH_SERVED);
	map |= (1 << (int) arch_type);
	setup_set(ws, 
	    WS_ARCH_SERVED,	map,
	    0);
}
