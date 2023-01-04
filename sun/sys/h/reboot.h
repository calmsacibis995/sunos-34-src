/*	@(#)reboot.h 1.1 86/09/25 SMI; from UCB 4.3 82/10/31	*/

/*
 * Arguments to reboot system call and flags to init.
 *
 * On the VAX, these are passed to boot program in r11,
 * and on to init.
 *
 * On the Sun, these are parsed from the boot command line
 * and used to construct the argument list for init.
 */
#define	RB_AUTOBOOT	0	/* flags for system auto-booting itself */

#define	RB_ASKNAME	0x01	/* ask for file name to reboot from */
#define	RB_SINGLE	0x02	/* reboot to single user only */
#define	RB_NOSYNC	0x04	/* dont sync before reboot */
#define	RB_HALT		0x08	/* don't reboot, just halt */
#define	RB_INITNAME	0x10	/* name given for /etc/init */
#define	RB_NOBOOTRC	0x20	/* don't run /etc/rc.boot */
#define	RB_DEBUG	0x40	/* being run under debugger */

#define	RB_PANIC	0	/* reboot due to panic */
#define	RB_BOOT		1	/* reboot due to boot() */
