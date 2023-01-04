#ifndef lint
static	char sccsid[] = "@(#)label.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"
#include <sys/ioctl.h>
#include <sun/dkio.h>
#include <sun/dklabel.h>
#include <sys/fcntl.h>

#define BUF	256

/*
 * This file deals with some low-level disk things.
 * It gets the disk geometry, labels a disk and does a mkfs.
 */

/*
 * Re-write the label on the disks.
 */
label_disks(ws)
Workstation	ws;
{
	Controller	cont;
	Disk		disk;
	int		dix;
	int		cix;

	message(ws, "Labelling the disks");
	SETUP_FOREACH_OBJECT(ws, WS_CONTROLLER, cix, cont) {
		SETUP_FOREACH_OBJECT(cont, CONTROLLER_DISK, dix, disk) {
			relabel(ws, disk);
		} SETUP_END_FOREACH
	} SETUP_END_FOREACH
}

/*
 * Relabel a disk.
 * First read the existing label.
 * Change it and write it back.  Then tell UNIX about each
 * of the changes as well.
 */
static
relabel(ws, disk)
Workstation	ws;
Disk		disk;
{
	Hard_partition	hard;
	char		device[BUF];
	char		*disk_name;
	char		*letter;
	struct	dk_label label;
	struct	dk_label newlabel;
	struct	dk_map	*mapp;
	struct	dk_map	savemap;
	long		seeksector;
	int		sector;
	int		fd;
	int		part_ix;
	int		r;
	int		hix;

	/*
	 * Open the raw 'c' partition and read the label.
	 */
	disk_name = setup_get(disk, DISK_NAME);
	sprintf(device, "/dev/r%sc", disk_name);
	fd = open(device, 0);
	if (fd == -1) {
		error(ws, SETUP_EOPEN_FAILED, device, "reading");
	}
	r = read(fd, (char *) &label, sizeof(label));
	if (r != sizeof(label)) {
		error(ws, SETUP_EREAD_LABEL, device);
	}
	close(fd);
	newlabel = label;

	/*
	 * For each hard parition, get its beginning cylinder number,
	 * and number of sectors.  Update the maps in the label.
	 * Along the way, inform UNIX about the change in the partition
	 * sizes.
	 */
	SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, hix, hard) {
		letter =  setup_get(hard, HARD_LETTER);
		part_ix = letter[0] - 'a';
		mapp = &newlabel.dkl_map[part_ix];
		mapp->dkl_cylno = (int) setup_get(hard, HARD_OFFSET_CYL_REAL);
		mapp->dkl_nblk = (int) setup_get(hard, HARD_SIZE_REAL);
		label_inform_unix(ws, disk_name, letter, mapp);
	} SETUP_END_FOREACH

	/*
	 * Checksum the label and write it back to disk.
	 * Write it only if it changed.
	 */
	mk_cksum(&newlabel);
	if (bcmp(&newlabel, &label, sizeof(label)) != 0) {
		
		/*
		 * Save the partition map for the 'c' partition and
		 * put in a dummy one that includes the alternate
		 * cylinders.  This makes it possible to write backup
		 * labels.
		 */
		part_ix = 'c' - 'a';
		mapp = &newlabel.dkl_map[part_ix];
		savemap = *mapp;
		mapp->dkl_cylno = 0;
		mapp->dkl_nblk = (label.dkl_ncyl + label.dkl_acyl) *
		    label.dkl_nhead * label.dkl_nsect;
		label_inform_unix(ws, disk_name, "c", mapp);

		/*
		 * Write the label with the dummy 'c' partition.
		 */
		mk_cksum(&newlabel);
		fd = open(device, 1);
		if (fd == -1) {
			error(ws, SETUP_EOPEN_FAILED, device, "writing");
		}
		lseek(fd, 0, 0);
		r = write(fd, (char *) &newlabel, sizeof(newlabel));
		if (r != sizeof(newlabel)) {
			error(ws, SETUP_EWRITE_LABEL, device);
		}
		close(fd);
		*mapp = savemap;
		mk_cksum(&newlabel);
		fd = open(device, 1);
		if (fd == -1) {
			error(ws, SETUP_EOPEN_FAILED, device, "writing");
		}
		/*
		 * Write the backup labels.
		 * They are written in every other sector at the end of
		 * the disk.
		 */
		for (sector = 1; sector < 11; sector += 2) {
			if (strncmp(disk_name, "sd", 2) == 0) {	/* Is a SCSI? */
				seeksector = 
				    (label.dkl_ncyl + label.dkl_acyl - 1) *
				    label.dkl_nhead * label.dkl_nsect +
				    1 * label.dkl_nsect + 
				    sector;
			} else {
				seeksector = 
				    (label.dkl_ncyl + label.dkl_acyl - 1) *
				    label.dkl_nhead * label.dkl_nsect +
				    (label.dkl_nhead - 1) * label.dkl_nsect + 
				    sector;
			}
			lseek(fd, seeksector*512, 0);
			r = write(fd, (char *) &newlabel, sizeof(newlabel));
			if (r != sizeof(newlabel)) {
				error(ws, SETUP_EWRITE_LABEL, device);
			}
		}

		/*
		 *  Write the non-dummy label back to the beginning of the disk.
		 */
		lseek(fd, 0, 0);
		r = write(fd, (char *) &newlabel, sizeof(newlabel));
		if (r != sizeof(newlabel)) {
			error(ws, SETUP_EWRITE_LABEL, device);
		}
		label_inform_unix(ws, disk_name, "c", &savemap);
		close(fd);
	}
}

/*
 * Inform UNIX about a hard partition's size.
 */
static
label_inform_unix(ws, disk_name, letter, mapp)
Workstation	ws;
char		*disk_name;
char		*letter;
struct	dk_map	*mapp;
{
	char		device[BUF];
	int		part_fd;

	sprintf(device, "/dev/r%s%s", disk_name, letter);
	part_fd = open(device, O_NDELAY);
	if (part_fd == -1) {
		error(ws, SETUP_EOPEN_FAILED, device, "reading");
	} else {
		ioctl(part_fd, DKIOCSPART, mapp);
		close(part_fd);
	}
}

/*
 * Checksum the label and put the checksum into the label.
 * Stolen from diag.
 */
mk_cksum(l)
struct dk_label *l;
{
        short   *sp;
        short   sum;
        short   count;
 
        sum = 0;
        count = sizeof(struct dk_label)/sizeof(short) - 1;
        sp = (short *)l;
        while(count--)
                sum ^= *sp++;
        l->dkl_cksum = sum;
}

/*
 * Do a mkfs on a hard partition.
 */
newfs(ws, hard)
Workstation	ws;
Hard_partition	hard;
{
	char		*hard_name;
	char		cmd[BUF];

	hard_name = (char *) setup_get(hard, HARD_NAME);
	sprintf(cmd, "/etc/newfs /dev/r%s", hard_name);
	mysystem(ws, cmd);
}

/*
 * Do a mkfs on the root partition of a client.
 * The root partition is an nd partition.
 */
root_mkfs(ws, client)
Workstation	ws;
Client		client;
{
	Soft_partition	soft;
	Hard_partition	hard;
	Disk		disk;
	char		cmd[BUF];

	soft = (Soft_partition) setup_get(client, CLIENT_ROOT_PARTITION);
	hard = (Hard_partition) setup_get(soft, SOFT_HARD_PARTITION);
	disk = (Disk) setup_get(hard, HARD_DISK);
	sprintf(cmd, "/etc/mkfs /dev/rndl%d %d %d %d 8192 1024",
	    setup_get(soft, SOFT_NDL),
	    setup_get(soft, SOFT_SIZE),
	    setup_get(disk, DISK_SEC_TRACK),
	    setup_get(disk, DISK_NHEADS));
	mysystem(ws, cmd);
}
