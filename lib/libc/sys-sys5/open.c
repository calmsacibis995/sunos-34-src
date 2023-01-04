#ifndef lint
static	char sccsid[] = "@(#)open.c 1.1 86/09/24 SMI";
#endif

/*
	open -- system call emulation for 4.2BSD

	last edit:	15-Dec-1983	D A Gwyn
*/

#include	<errno.h>
#include	<fcntl.h>

#define FIONBIO 0x8004667e		/* 4.2BSD _ioctl() request */

extern int	_open(), _ioctl();
extern int	errno;

/*VARARGS2*/
int
open( path, oflag, mode )		/* returns fildes, or -1 */
	register char	*path;		/* pathname of file */
	register int	oflag;		/* flag bits, see open(2) */
	int		mode;		/* O_CREAT protection mode */
	{
	register int	fd;		/* file descriptor */

	if ((fd = _open( path, oflag, mode )) < 0)
		return fd;

	/* 4.2BSD emulation of O_NDELAY requires non-blocking I/O: */

	if ( (oflag & O_NDELAY) != 0 )
		{
		static int	on = 1; /* stupid syscall design */

		(void)_ioctl( fd, FIONBIO, (char *)&on );
		}

	return fd;
	}
