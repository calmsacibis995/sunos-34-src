#ifndef lint
static	char sccsid[] = "@(#)write.c 1.1 86/09/24 SMI";
#endif

/*
	write -- system call emulation for 4.2BSD

	last edit:	01-Jul-1983	D A Gwyn

	The only reason for this layer is to support O_NDELAY mode.
*/

#include	<errno.h>

extern int	errno;
extern int	_write();		/* actual system call */

int
write( fildes, buf, nbyte )
	int	fildes;
	char	*buf;
	int	nbyte;
	{
	register int	serrno = errno; /* save errno */
	register int	nwritten;

	if ( (nwritten = _write( fildes, buf, nbyte )) >= 0
	  || errno != EWOULDBLOCK
	   )
		return nwritten;

	/* O_NDELAY set and write would block: */

	errno = serrno; 		/* restore errno */
	return 0;
	}
