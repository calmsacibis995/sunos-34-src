#ifndef lint
static	char sccsid[] = "@(#)initialize.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include "setup_runtime.h"
#include <nlist.h>

static  char    config_file_name[] = "setup.config"; 
static  char    card_file_name[] = "setup.cards"; 

static void	fake_hardware();


Workstation
mid_init(argc, argv, msg_proc)
int	argc;
char	*argv[];
Voidfunc msg_proc;
{
	int		hardware_file = FALSE;
	int		upgrade = FALSE;
	char		*hardware_file_name;
	char		*get_hardware_config();

	Mflag = FALSE;
	for (; argc > 1; argv++, argc--) {
                switch (argv[1][1]) {
                case 'f':
                        hardware_file = TRUE;
                        argv++; argc--;
                        hardware_file_name = argv[1];
			break;

		case 'u':
			upgrade = TRUE;
                        break;

		case 'M':
			Mflag = TRUE;
                        break;
                }
        }

	suppress_console();
	
	workstation = setup_create(WORKSTATION, 0);
	setup_set(workstation, 
	    SETUP_MESSAGE_PROC, msg_proc,
	    SETUP_UPGRADE,	upgrade,
	    0);

	
	read_file(workstation, config_file_name);

	if (hardware_file == FALSE) {
		hardware_file_name = get_hardware_config(workstation);
	}
	read_file(workstation, hardware_file_name);
        get_ethernet_info(workstation);

	setup_reserved(workstation);

	read_card_file(workstation, card_file_name);

	return(workstation);
}


setup_reserved(ws)
Workstation	ws;
{
	Controller	cont;
	Disk		disk;
	Hard_partition	hp;
	int		rounding;

	/*
	 * if there aren't any controllers then exit with an error,
	 * there are too many dependencies in setup on there being at
	 * least one disk on the workstation
	 */
	cont = setup_get(ws, WS_CONTROLLER, 0);
	if (cont == NULL) {
		runtime_error("No disks detected on this machine.");
	}
	disk = setup_get(cont, CONTROLLER_DISK, 0);

	/*
	 * Set the default root parition to the 'a' partition
	 * of the first unit of the first controller.
	 */
	hp = (Hard_partition) setup_get(disk, DISK_HARD_PARTITION, 0);
	setup_set(workstation, 
	    WS_SERVED_ROOTHARD, hp, 
	    0);

	/*
	 * force rounding off for root and first swap because setup
	 * cannot change their sizes and we might round up
	 */
	rounding = (int)setup_get(disk, DISK_PARAM_CYLROUNDING);
	setup_set(disk,
		DISK_PARAM_CYLROUNDING, FALSE,
		0);
	hard_partition_initialization(HARD_RESERVED_ROOT, disk);
	hard_partition_initialization(HARD_RESERVED_SWAP, disk);
	setup_set(disk,
		DISK_PARAM_CYLROUNDING, rounding,
		0);

}

/*
 * Save some state for the next time that setup is invoked in
 * the "upgrade" mode.
 *
 * The call to save_client_cpu_types() overwrites the "/etc/setup.info"
 * file.  All subsequent calls append to the file.
 */
mid_cleanup()
{
	save_client_cpu_types(workstation);
	save_oswg(workstation);
}

/*
 * Supress all console messages.
 * This is overkill and not quite the right solution, but is the
 * best we can do for now.  The problem is that console messages
 * mess up the screen and the user has no way of asking setup
 * to repaint the screen.
 */
static
suppress_console()
{
	struct	nlist	nl[2];
	int		fd;
	int		nonzero = 1;

	nl[0].n_name = "_noprintf";
	nl[0].n_value = 0;
	nl[1].n_name = "";
	nlist("/vmunix", nl);
	if (nl[0].n_value == 0) {
		return;
	}
	fd = open("/dev/kmem", 1);
	if (fd != -1) {
		lseek(fd, nl[0].n_value, 0);
		write(fd, (char *) &nonzero, sizeof(nonzero));
		close(fd);
	}
}
