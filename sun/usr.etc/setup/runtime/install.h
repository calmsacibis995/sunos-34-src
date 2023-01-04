/*	@(#)install.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#define	BUF		256
#define SETUP_DIR	"/usr/etc/setup.files"	/* Shell scripts live here */
#define SETUP_LOG	"/tmp/setup.log"  	/* Log file for the scripts */
#define	SETUP_INFO	"/etc/setup.info"	/* State file for upgrades */
#define ROOT_DIR	"/setup.root"		/* Root-to-be mounted here */
#define	HOSTS		"/etc/hosts"
#define	FSTAB		"/etc/fstab"
#define	NDLOCAL		"/etc/nd.local"
#define	ETHERS		"/etc/ethers"
#define	RCLOCAL		"/etc/rc.local"
#define	RCBOOT		"/etc/rc.boot"
#define	CRONTAB		"/usr/lib/crontab"
#define	SENDMAIL_CF	"sendmail.cf"
#define	SENDMAIL_MAIN_CF	"sendmail.main.cf"
#define	SENDMAIL_SUBSIDIARY_CF	"sendmail.subsidiary.cf"
#define BS_1OVER2	20		/* Block size for 1/2" tapes */
#define BS_1OVER4	126		/* Block size for 1/4" tapes */

#define isserver(ws)	((Workstation_type) setup_get(ws, WS_TYPE) == \
				WORKSTATION_SERVER)
#define isstandalone(ws) ((Workstation_type) setup_get(ws, WS_TYPE) == \
				WORKSTATION_STANDALONE)
#define istapeless(ws)	((int) setup_get(ws, WS_TAPE_TYPE) == SETUP_TAPELESS)

typedef	struct	mount_list	*Mount_list;
struct	mount_list {
	char		*dev;		/* Name of mounted device */
	Mount_list	next;		/* Linked list */
};

Mount_list	get_mountlist();
Boolean 	everything_ok();
Boolean 	tapedev_ok();
Arch_type	archstr_to_type();
Hard_partition	find_hard();
char		*get_archname();
char		*get_tapeserver();
char		*get_tapecontroller();
char		*get_tapedev();
char		*get_tapename();
char		*get_remote();
char		*get_domainname();
