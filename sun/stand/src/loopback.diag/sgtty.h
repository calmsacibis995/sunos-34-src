/*      @(#)sgtty.h 1.1 86/09/25 SMI; from UCB 4.1 83/05/03	*/

/*
 * Structure for stty and gtty system calls.
 */

#ifndef	_IOCTL_
#include <sys/ioctl.h>
#endif

#ifndef _SGTTYB_
#define	_SGTTYB_
struct sgttyb {
	char	sg_ispeed;		/* input speed */
	char	sg_ospeed;		/* output speed */
	char	sg_erase;		/* erase character */
	char	sg_kill;		/* kill character */
	short	sg_flags;		/* mode flags */
};
#endif
