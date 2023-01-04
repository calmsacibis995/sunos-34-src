#ifndef lint
static	char sccsid[] = "@(#)read.c 1.1 86/09/24 SMI";
#endif

/*
	read -- system call emulation for 4.2BSD

	last edit:	16-Jun-1983	D A Gwyn

	The only reason for this layer is to support O_NDELAY mode.
*/

#include	<errno.h>

extern int	errno;
extern int	_read();		/* actual system call */

int
read( fildes, buf, nbyte )
	int	fildes;
	char	*buf;
	int	nbyte;
	{
	register int	serrno = errno; /* save errno */
	register int	nread;

	if ( (nread = _read( fildes, buf, nbyte )) >= 0
	  || errno != EWOULDBLOCK
	   )
		return nread;

	/* O_NDELAY set and read would block: */

	errno = serrno; 		/* restore errno */
	return 0;
	}
