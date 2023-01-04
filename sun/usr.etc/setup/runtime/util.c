
#ifndef lint
static	char sccsid[] = "@(#)util.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"
#include <ctype.h>

char *
sectors_to_units(hp, sectors)
Hard_partition	hp;
int		sectors;
{
	Disk_display_units	disk_units;
	double			value;
	Disk			disk;
	int			nheads;
	int			sec_track;
	static char		buffer[32];

	disk_units = (Disk_display_units)setup_get(workstation, 
			PARAM_DISK_DISPLAY_UNITS);

	switch(disk_units) {
	case MBYTES:
		/*
		 * Must insure that Megabytes are always rounded down.
		 */
		if (sectors == 0) {
			value = 0.0;
		} else {
			value = (double)sectors / 2048.0 - 0.005;
		}
		sprintf(buffer, "%10.2lf", value);
		break;
	case KBYTES:
		value = (double)sectors / 2.0;
		sprintf(buffer, "%10.1lf", value);
		break;
	case CYLS:
		disk = (Disk)setup_get(hp, HARD_DISK);
		nheads = (int)setup_get(disk, DISK_NHEADS);
		sec_track = (int)setup_get(disk, DISK_SEC_TRACK);
		sprintf(buffer, "%10d", (sectors / (nheads * sec_track)));
		break;
	case SECTORS:
		sprintf(buffer, "%10d", sectors);
		break;
	}

	return(buffer);

}

/*
 * Convert sectors to display units, left justify and 
 * include a unit specifier.
 */
char *
sectors_to_units_left(hp, sectors)
Hard_partition	hp;
int		sectors;
{
	Disk_display_units	disk_units;
	double			value;
	Disk			disk;
	int			nheads;
	int			sec_track;
	static char		buffer[32];

	disk_units = (Disk_display_units)setup_get(workstation, 
			PARAM_DISK_DISPLAY_UNITS);

	switch(disk_units) {
	case MBYTES:
		if (sectors == 0) {
			value = 0.0;
		} else {
			value = (double)sectors / 2048.0 - 0.005;
		}
		sprintf(buffer, "%-2.2lf M", value);
		break;
	case KBYTES:
		value = (double)sectors / 2.0;
		sprintf(buffer, "%-1.1lf K", value);
		break;
	case CYLS:
		disk = (Disk)setup_get(hp, HARD_DISK);
		nheads = (int)setup_get(disk, DISK_NHEADS);
		sec_track = (int)setup_get(disk, DISK_SEC_TRACK);
		sprintf(buffer, "%d C", (sectors / (nheads * sec_track)));
		break;
	case SECTORS:
		sprintf(buffer, "%d S", sectors);
		break;
	}

	return(buffer);

}

/*
 * Convert a size to sectors.  If the size is simply a number,
 * interpret it as the currently selected units.  The number
 * may have a unit specifier following it to over-ride the
 * currently selected units.
 */
units_to_sectors(hp, units)
Hard_partition	hp;
char		*units;
{
	Disk_display_units	disk_units;
	double			atof();
	char			*cp;
	Disk			disk;
	int			nheads;
	int			sec_track;

	for (cp = units; *cp && isspace(*cp); cp++)
		;
	while (*cp) {
		if (!isdigit(*cp) && *cp != '.') {
			while (*cp && isspace(*cp)) {
				cp++;
			}
			break;
		}
		cp++;
	}

	switch (*cp) {
	case 'M':
	case 'm':
		disk_units = MBYTES;
		break;
	case 'K':
	case 'k':
		disk_units = KBYTES;
		break;
	case 'C':
	case 'c':
		disk_units = CYLS;
		break;
	case 'S':
	case 's':
		disk_units = SECTORS;
		break;
	default:
		disk_units = (Disk_display_units)setup_get(workstation, 
				PARAM_DISK_DISPLAY_UNITS);
		break;
	}

	switch(disk_units) {
	case MBYTES:
		return((int)(atof(units) * 2048.0));
		break;
	case KBYTES:
		return((int)(atof(units) * 2.0));
		break;
	case CYLS:
		disk = (Disk)setup_get(hp, HARD_DISK);
		nheads = (int)setup_get(disk, DISK_NHEADS);
		sec_track = (int)setup_get(disk, DISK_SEC_TRACK);
		return(atoi(units) * (nheads * sec_track));
		break;
	case SECTORS:
		return(atoi(units));
		break;
	}

}


offset_to_cyls(hp)
Hard_partition	hp;
{
	Disk	disk;
	int	heads;
	int	sectors_track;
	int	sectors_cyl;
	int	offset;

	disk = (Disk)setup_get(hp, HARD_DISK);
	heads		= (int)setup_get(disk, DISK_NHEADS);
	sectors_track	= (int)setup_get(disk, DISK_SEC_TRACK);

	sectors_cyl = heads * sectors_track;

	offset = (int)setup_get(hp, HARD_OFFSET);
	offset = round_to_cyls(disk, offset);

	return(offset / sectors_cyl);
}


/*
 * round up to cylinder boundary
 */
round_to_cyls(disk, sectors)
Disk	disk;
int	sectors;
{
	int	heads;
	int	sectors_track;
	int	sectors_cyl;

	heads		= (int)setup_get(disk, DISK_NHEADS);
	sectors_track	= (int)setup_get(disk, DISK_SEC_TRACK);
	sectors_cyl = heads * sectors_track;

	if ((sectors % sectors_cyl) == 0) {
		return(sectors);
	} else {
		return(sectors + (sectors_cyl - (sectors % sectors_cyl)));
	}
}


/*
 * round down to cylinder boundary
 */
round_down_to_cyls(disk, sectors)
Disk	disk;
int	sectors;
{
	int	heads;
	int	sectors_track;
	int	sectors_cyl;

	heads		= (int)setup_get(disk, DISK_NHEADS);
	sectors_track	= (int)setup_get(disk, DISK_SEC_TRACK);
	sectors_cyl = heads * sectors_track;

	if ((sectors % sectors_cyl) == 0) {
		return(sectors);
	} else {
		return(sectors - (sectors % sectors_cyl));
	}
}


display_units_changed()
{
        Controller      controller;
        Disk            disk;
        Hard_partition  hp;
        int		cont_index;
	int		disk_index;
	int		hp_index;
	int		n;

	SETUP_FOREACH_OBJECT(workstation, WS_CONTROLLER, 
	    cont_index, controller) {
		SETUP_FOREACH_OBJECT(controller, CONTROLLER_DISK, 
		    disk_index, disk) {
			callback(disk, DISK_SIZE_STRING_LEFT, FALSE);
			callback(disk, DISK_FREE_SPACE_STRING_LEFT, FALSE);
			SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION,
			    hp_index, hp) {
				callback(hp, HARD_OFFSET_STRING, FALSE);
				callback(hp, HARD_OFFSET_STRING_LEFT, FALSE);
				callback(hp, HARD_SIZE_STRING, FALSE);
				callback(hp, HARD_SIZE_STRING_LEFT, FALSE);
				callback(hp, HARD_MIN_SIZE_IN_UNITS, FALSE);
				callback(hp, HARD_MAX_SIZE_IN_UNITS, FALSE);
			} SETUP_END_FOREACH
		} SETUP_END_FOREACH
	} SETUP_END_FOREACH
}


delete_array_entry(obj, num_attr, array_attr, elem)
Setup_object	*obj;
Setup_attribute	num_attr;
Setup_attribute	array_attr;
int		elem;
{
	int	count;
	int	i;
	Opaque	array_obj;
 
        count = ((int)setup_get(obj, num_attr) - 1);
        setup_set(obj, 
		num_attr, count, 
		0);
 
        for (i = (elem + 1); i <= count; i++) {
                array_obj = (Opaque)setup_get(obj, array_attr, i);
                setup_set(obj, 
			array_attr, (i - 1), array_obj, 
			0);
        }
	setup_set(obj, 
		array_attr, count, 0,
		0);
}


setup_reboot(ws)
Workstation	ws;
{
	if (geteuid() != 0) {
		message(ws, "You must be super-user to reboot");
		return;
	}
	sync();
	sync();
	reboot(0);
}
