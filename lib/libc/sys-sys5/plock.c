#ifndef lint
static	char sccsid[] = "@(#)plock.c 1.1 86/09/24 SMI";
#endif

/*
	plock -- system call emulation for 4.2BSD and BRL PDP-11 UNIX

	last edit:	28-Aug-1983	D A Gwyn
*/

#include	<errno.h>

int
plock( op )
	int	op;			/* operation to be performed */
	{
	errno = EPERM;			/* correct unless super-user */
	return -1;
	}
