
/*
 * test program to exercise the middle end
 */

#include <stdio.h>
#include <sys/types.h>
#include "setup_attr.h"

char	*strcpy();
#define strdup(src)     strcpy(malloc((unsigned) strlen(src) + 1), src)

static Workstation	ws; 
void	handle_err();

main(argc, argv)
int	argc;
char	*argv;
{
	ws = mid_init(argc, argv, handle_err);

	setup_set(ws, SETUP_CALLBACK, handle_err, 0);
	
	doit();

}


doit()
{
	Controller	cont;
	Disk		disk;
	Hard_partition	hp;	
	int		n;

	cont = (Controller)setup_get(ws, WS_CONTROLLER, 0);
	disk = (Disk)setup_get(cont, CONTROLLER_DISK, 0);

	SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, n, hp) {
		setup_set(hp, SETUP_CALLBACK, handle_err, 0);
	} SETUP_END_FOREACH

	fprintf(stderr, "\nDisk before any operations:\n");
	display_disk();

        setup_set(ws, WS_TYPE, WORKSTATION_STANDALONE, 0);
	fprintf(stderr, "Disk after becoming standalone:\n");
	display_disk();

	hp = (Hard_partition)setup_get(disk, DISK_HARD_PARTITION, 4);  /* 'e' */
	setup_set(hp, HARD_SIZE_STRING, "100", 0);
	fprintf(stderr, "Disk after adding 100 to partition 'e':\n");
	display_disk();

        setup_set(ws, WS_TYPE, WORKSTATION_SERVER, 0);
	fprintf(stderr, "Disk after becoming server:\n");
	display_disk();

	exit(100);

	hp = (Hard_partition)setup_get(disk, DISK_HARD_PARTITION, 7);  /* 'h' */
	setup_set(hp, HARD_SIZE, 100, 0);
	fprintf(stderr, "Disk after adding 100 to partition 'h':\n");
	display_disk();

	hp = (Hard_partition)setup_get(disk, DISK_HARD_PARTITION, 4);  /* 'e' */
	setup_set(hp, HARD_SIZE, 0, 0);
	fprintf(stderr, "Disk after setting to 0 partition 'e':\n");
	display_disk();

        setup_set(ws, WS_TYPE, WORKSTATION_SERVER, 0);
	fprintf(stderr, "Disk after becoming server:\n");
	display_disk();

        setup_set(ws, WS_ARCH_SERVED, 0x03, 0);
	fprintf(stderr, "Disk after adding new architecture:\n");
	display_disk();

        setup_set(ws, WS_ARCH_SERVED, 0x01, 0);
	fprintf(stderr, "Disk after removing new architecture:\n");
	display_disk();

        setup_set(ws, WS_TYPE, WORKSTATION_NONE, 0);
	fprintf(stderr, "Disk after becoming none:\n");
	display_disk();

        setup_set(ws, WS_TYPE, WORKSTATION_STANDALONE, 0);
	fprintf(stderr, "Disk after becoming standalone again:\n");
	display_disk();

}


void
handle_err(obj, attr, display_value, err_msg)
Opaque          obj;
Setup_attribute attr;
caddr_t         display_value;
char            *err_msg;
{
	if (err_msg)
		fprintf(stderr, "%s\n", err_msg);
}


display_disk()
{
        Controller      cont;
        Disk            disk;
        Hard_partition  hp;
        int             n;
	char		*offset_string;
	char		*size_string;

	cont = (Controller)setup_get(ws, WS_CONTROLLER, 0);
	disk = (Disk)setup_get(cont, CONTROLLER_DISK, 0);

	fprintf(stderr,
		"\n\tFloating = %d; Overlapping = %d; Cylrounding = %d\n\n",
		(int)setup_get(disk, DISK_PARAM_FLOATING),
		(int)setup_get(disk, DISK_PARAM_OVERLAPPING),
		(int)setup_get(disk, DISK_PARAM_CYLROUNDING));

	SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, n, hp) {
		offset_string = strdup((char *)setup_get(hp,HARD_OFFSET_STRING));
		size_string = strdup((char *)setup_get(hp, HARD_SIZE_STRING));
		fprintf(stderr, "\t%s: %s (%3d) %s    %-14s %1d  %1d",
			(char *)setup_get(hp, HARD_LETTER),
			offset_string, offset_to_cyls(hp), size_string,
			(char *)setup_get(hp, HARD_MOUNT_PT),
			(int)setup_get(hp, HARD_TYPE),
			(int)setup_get(hp, HARD_RESERVED_TYPE));
		if ((int)setup_get(hp, HARD_PARAM_FREEHOG)) {
			fprintf(stderr, " FREEHOG\n");
		} else {
			fprintf(stderr, "\n");
		}
			
	} SETUP_END_FOREACH

	fprintf(stderr, "\n");

}

