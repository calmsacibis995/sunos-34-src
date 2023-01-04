
#ifndef lint
static	char sccsid[] = "@(#)read_config.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <ctype.h>
#include <strings.h>
#include "setup_runtime.h"


typedef struct  xsect   Sect;

struct  xsect   {
        char    	*ty_name;       /* name of the section */
        Config_type	type;           /* number */
};

Sect	config_classes[] = {
	{ "parameters",			CONFIG_PARAMETERS },
	{ "models",			CONFIG_MODEL },
	{ "types",			CONFIG_TYPE },
	{ "cpus",			CONFIG_CPU },
	{ "disktypes",			CONFIG_DISKTYPE },
	{ "tapetypes",			CONFIG_TAPETYPE },
	{ "ethertypes",			CONFIG_ETHERTYPE },
	{ "disk_controller",		CONFIG_DISKCONTROLLER },
	{ "disk",			CONFIG_DISK },
	{ "hard_partition_types",	CONFIG_HARD_PARTITION_TYPE },
	{ "hard_partition_reserved",	CONFIG_HARD_PARTITION_RESERVED },
	{ "soft_partition_types",	CONFIG_SOFT_PARTITION_TYPE },
	{ "disk_display_units",		CONFIG_DISK_DISPLAY_UNITS },
	{ "optional_software_group",	CONFIG_OSWG },
	{ "yp_type",			CONFIG_YP_TYPE },
	{ "tape_location",		CONFIG_TAPE_LOCATION },
	{ "mail_types",			CONFIG_MAIL_TYPES },
	{ 0,				CONFIG_LAST }
};

Config_type	findtype();


typedef struct  cntlr_xsect   Controller_sect;

struct  cntlr_xsect   {
        char    	*ty_name;       /* name of the section */
        Setup_setting	type;           /* number */
};

Controller_sect	disk_controller_names[] = {
	{ "sd",		SETUP_SCSI },
	{ "xy",		SETUP_XYLOGICS },
	{ 0,		SETUP_LAST_SETTING }
};

Setup_setting	find_controller_type();
Controller	current_disk_controller;


read_file(ws, filename)
Workstation	ws;
char		*filename;
{
	FILE		*fp;
	Config_type	t;
	int		c;
	int		round;;

	fp = fopen(filename, "r");
	if (fp == NULL) {
		runtime_error("Could not open %s.", filename);
	}
	while ((c = skipspace(fp)) != EOF) {
		if (c == '%') {
			fscanf(fp, "%s", scratch_buf);
			while ((c = getc(fp)) != '\n')
				;

			t = findtype(scratch_buf);

			switch(t) {

			case CONFIG_PARAMETERS:
				read_parameters(ws, fp);
				break;

			case CONFIG_MODEL:
			case CONFIG_TYPE:
			case CONFIG_CPU:
			case CONFIG_DISKTYPE:
			case CONFIG_TAPETYPE:
			case CONFIG_ETHERTYPE:
			case CONFIG_HARD_PARTITION_TYPE:
			case CONFIG_SOFT_PARTITION_TYPE:
			case CONFIG_DISK_DISPLAY_UNITS:
			case CONFIG_YP_TYPE:
			case CONFIG_TAPE_LOCATION:
			case CONFIG_MAIL_TYPES:
				read_items(ws, ord(t), fp);
				break;

			case CONFIG_HARD_PARTITION_RESERVED:
				read_hard_partition_init(ws, ord(t), fp);
				break;


			case CONFIG_DISKCONTROLLER:
				new_disk_controller(ws, fp);
				break;

			case CONFIG_DISK:
				new_disk(ws, fp);
				break;
				
			case CONFIG_OSWG:
				read_oswg(ws, fp);
				break;
				

			default:
				runtime_error("Unrecognized type `%s' in %s.",
					scratch_buf, filename);
			}
		}
	}
	fclose(fp);
}


/*
 * Found a "% type" line, determine the type
 */
static	Config_type
findtype(str)
char	*str;
{
	Sect	*tp;

	for (tp = config_classes; tp->ty_name != NULL; tp++) {
		if (strcmp(str, tp->ty_name) == 0) {
			return(tp->type);
		}
	}
	runtime_error("Unknown keyword type \"%s\".", str);
}


static Setup_setting
find_controller_type(str)
char    *str;
{
        Controller_sect    *tp;

        for (tp = disk_controller_names; tp->ty_name != NULL; tp++) {
                if (strcmp(str, tp->ty_name) == 0) {
                        return(tp->type);
                }
        }
        runtime_error("Unknown controller type \"%s\".", str);
}


static char *param_sscanf = "disk_display=%d autohost=%d first_host=%d default_oswg=0x%x network_num=%s";

read_parameters(ws, fp)
Workstation     ws;
FILE    *fp;
{
        int     c;
	int	overlap;
	int	rounding;
	int	disk_display;
	int	autohost;
	int	firsthost;
	int	default_oswg;
	char	netnum[20];

        for (;;) {
                c = skipspace(fp);
                if (c == EOF) {
                        return;
                }
                ungetc(c, fp);
                if (c == '%') {
                        return;
                }

                if (fgets(scratch_buf, sizeof(scratch_buf), fp) == NULL) {
                        return;
                }
                *(rindex(scratch_buf, '\n')) = NULL;

                sscanf(scratch_buf, param_sscanf, 
		    &disk_display, &autohost, &firsthost, &default_oswg,
		    netnum);
 
                setup_set(ws, 
                    PARAM_DISK_DISPLAY_UNITS, disk_display,
                    PARAM_AUTOHOST, autohost,
                    PARAM_FIRSTHOST, firsthost,
		    PARAM_DEFAULT_OSWG, default_oswg,
		    WS_NETWORK, netnum, 
		    0);
 
        }
}

	
read_items(ws, n, fp)
Workstation	ws;
int	n;
FILE	*fp;
{
	int	c;
	int	count;

	count = 0;
	for (;;) {
		c = skipspace(fp);
		if (c == EOF) {
			return;
		}
		ungetc(c, fp);
		if (c == '%') {
			return;
		}
	
		if (fgets(scratch_buf, sizeof(scratch_buf), fp) == NULL) {
			return;
		}
		*(rindex(scratch_buf, '\n')) = NULL;

		if (count >= CONFIG_ITEMS) {
			runtime_error("Too many items in config file.");
		}

		setup_set(ws, SETUP_CHOICE_STRING, n, count++, scratch_buf, 0);

	}
}


read_hard_partition_init(ws, n, fp)
Workstation     ws;
int     n;
FILE    *fp;
{
        int     c;
        int     count;
	int	name[32];
	char	desc[256];
	char	mnt_pt[256];
	int	size;
 
        count = 0;
        for (;;) {
                c = skipspace(fp);
                if (c == EOF) {
                        return;
                }
                ungetc(c, fp);
                if (c == '%') {
                        return;
                }
         
                if (fgets(scratch_buf, sizeof(scratch_buf), fp) == NULL) {
                        return;
                }
                *(rindex(scratch_buf, '\n')) = NULL;
 
                sscanf(scratch_buf, "%[^,], %c %s %d %d", 
		    hard_reserved_info[count].name, 
		    &hard_reserved_info[count].partition, 
		    hard_reserved_info[count].mount_point, 
		    &hard_reserved_info[count].type, 
		    &hard_reserved_info[count].minimum_size);
		if (strcmp(hard_reserved_info[count].mount_point, "None") == 0) {
			strcpy(hard_reserved_info[count].mount_point, "");
		}
		count++;
 
        }
}


new_disk_controller(ws, fp)
Workstation	ws;
FILE		*fp;
{
	int	c;
	char	cntlr[16];
	int	addr;

	for (;;) {
		c = skipspace(fp);
		if (c == EOF) {
			return;
		}
		ungetc(c, fp);
		if (c == '%') {
			return;
		}
	
		if (fgets(scratch_buf, sizeof(scratch_buf), fp) == NULL) {
			return;
		}
		*(rindex(scratch_buf, '\n')) = NULL;

		sscanf(scratch_buf, "cntlr=%s addr=%d", cntlr, &addr);

		current_disk_controller = (Controller)setup_create(CONTROLLER,
                    CONTROLLER_NAME, cntlr,
                    CONTROLLER_TYPE, find_controller_type(cntlr),
                    0);

		setup_set(ws,
		    WS_CONTROLLER,    
		    SETUP_APPEND,   current_disk_controller,
                    0);

	}

}


static char *disk_sscanf = "cntlr=%s unit=%d cyls=%d heads=%d secs/track=%d";

new_disk(ws, fp)
Workstation	ws;
FILE		*fp;
{
	Disk		disk = NULL;
	Hard_partition	hp;
	int		c;
	char		cntlr[16];
	int		unit;
	char		partition;
	int		heads;
	int		start_cyl;
	int		sectors;
	int		cyls;
	int		sec_track;
	int		total_sectors;
	char		hpbuf[8];
	char		part;
	char		last_partition;

	for (;;) {
		c = skipspace(fp);
		if (c == EOF) {
			create_empty_partitions(disk, 'i', last_partition);
			setup_set(disk, 
				DISK_PARAM_CYLROUNDING, TRUE,
				DISK_PARAM_OVERLAPPING, FALSE,
				DISK_PARAM_FLOATING, TRUE, 
			0);
			return;
		}
		ungetc(c, fp);
		if (c == '%') {
			create_empty_partitions(disk, 'i', last_partition);
			setup_set(disk, 
				DISK_PARAM_CYLROUNDING, TRUE,
				DISK_PARAM_OVERLAPPING, FALSE,
				DISK_PARAM_FLOATING, TRUE, 
			0);
			return;
		}
		if (fgets(scratch_buf, sizeof(scratch_buf), fp) == NULL) {
			create_empty_partitions(disk, 'i', last_partition);
			setup_set(disk, 
				DISK_PARAM_CYLROUNDING, TRUE,
				DISK_PARAM_OVERLAPPING, FALSE,
				DISK_PARAM_FLOATING, TRUE, 
			0);
			return;
		}
		*(rindex(scratch_buf, '\n')) = NULL;

		if (disk == NULL) {

			sscanf(scratch_buf, disk_sscanf, 
				cntlr, &unit, &cyls, &heads, &sec_track);

			total_sectors = cyls * heads * sec_track;

			sprintf(scratch_buf, "%s%d", cntlr, unit);
			disk = (Disk) setup_create(DISK,
				DISK_NAME, scratch_buf,
				DISK_TYPE, find_controller_type(cntlr),
				DISK_NCYLS, cyls,
				DISK_NHEADS, heads,
				DISK_SEC_TRACK, sec_track,
				DISK_SIZE, total_sectors,
				DISK_FREE_SPACE, total_sectors,
				DISK_PARAM_OVERLAPPING, TRUE,
				0);

		    	setup_set(current_disk_controller,
		        	CONTROLLER_DISK, 
				SETUP_APPEND, disk,
		        	0);
	
			last_partition = 'a';

			continue;

		}

		sscanf(scratch_buf, "partition=%c start_cyl=%d sectors=%d", 
			&partition, &start_cyl, &sectors);

		create_empty_partitions(disk, partition, last_partition);
		last_partition = partition + 1;

		/*
		 * HARD_DISK must come first in attribute list because it
		 * is used when changing other attributes, HARD_OFFSET must
		 * come before HARD_SIZE
		 */
		sprintf(hpbuf, "%c", partition);
		hp = setup_create(HARD_PARTITION,
		      HARD_DISK,		disk,
		      HARD_OFFSET,		(start_cyl * heads * sec_track),
		      HARD_SIZE,                sectors,
                      HARD_LETTER,              hpbuf,
		      HARD_RESERVED_TYPE,	HARD_RESERVED_NONE,
                      0);

		if (partition == 'c') {
			setup_set(hp, 
			        HARD_OFFSET, 0,
			        HARD_SIZE, (cyls * heads * sec_track),
				HARD_RESERVED_TYPE, HARD_RESERVED_ENTIREDISK, 
				0);
		}

		setup_set(disk,
		    DISK_HARD_PARTITION,    partition - 'a',   hp,
		    0);

	}
}


create_empty_partitions(disk, partition, last_partition)
Disk    disk;
char	partition;
char	last_partition;
{
        Hard_partition  hp;
	char	c;
	char	hpbuf[8];

	for (c = last_partition; c < partition; c++) {

		sprintf(hpbuf, "%c", c);

		/*
		 * HARD_DISK must come first in attribute list because it
		 * is used when changing other attributes, HARD_OFFSET must
		 * come before HARD_SIZE
		 */
		hp = setup_create(HARD_PARTITION,
		      HARD_DISK,		disk,
		      HARD_OFFSET,              0,
		      HARD_SIZE,                0,
		      HARD_NAME,                strdup(hpbuf),
		      HARD_LETTER,              hpbuf,
		      HARD_RESERVED_TYPE,	HARD_RESERVED_NONE,
		      0);
 
		setup_set(disk,
		    DISK_HARD_PARTITION,    c - 'a',   hp,
		    0);
	}
}


/*
 * Skip white space
 */
skipspace(fp)
FILE	*fp;
{
	int	c;

	do {
		c = getc(fp);
	} while (isspace(c) || iscmt(c, fp));
	return(c);
}


/*
 * Check for a comment and if its found skip to the end of the line
 */
iscmt(c, fp)
int	c;
FILE	*fp;
{
	if (c == '#') {
		while ((c = getc(fp)) != '\n')
			;
		return(TRUE);
	}
	return(FALSE);
}


char *
get_device_abbrev(device)
char	*device;
{
	char	*ptr1;
	char	*ptr2;

	if ((ptr1 = index(device, '(')) == 0) {
		return(NULL);
	}

	ptr2 = (char *)strdup(ptr1 + 1);
	*(rindex(ptr2, ')')) = NULL;
	return(ptr2);
}


read_oswg(ws, fp)
Workstation     ws;
FILE           *fp;
{
	int             c;
	char            desc[256];
	char            standdir[256];
	char            serverdir[256];
	int		MC68010usrsize;
	int		MC68020usrsize;
	Oswg            oswg;

	for (;;) {
		c = skipspace(fp);
		if (c == EOF) {
			return;
		}
		ungetc(c, fp);
		if (c == '%') {
			return;
		}
		if (fgets(scratch_buf, sizeof(scratch_buf), fp) == NULL) {
			return;
		}
		*(rindex(scratch_buf, '\n')) = NULL;

		sscanf(scratch_buf, 
		    "%[^,], MC68010=%d, MC68020=%d, %[^,], %[^,]", 
		    desc, &MC68010usrsize, &MC68020usrsize,
		    standdir, serverdir);

		oswg = (Oswg) setup_create(OSWG,
		    OSWG_NAME, desc,
		    OSWG_USRSIZE, ord(MC68010), MC68010usrsize,
		    OSWG_USRSIZE, ord(MC68020), MC68020usrsize,
		    OSWG_DIRECTORY, ord(WORKSTATION_STANDALONE), standdir,
		    OSWG_DIRECTORY, ord(WORKSTATION_SERVER), serverdir,
		    0);

		setup_set(ws, 
			WS_OSWG, 
			SETUP_APPEND, oswg, 
			0);

	}
}

