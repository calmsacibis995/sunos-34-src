#ifndef lint
static	char sccsid[] = "@(#)setpgrp.c 1.1 86/09/24 SMI";
#endif

/*
	setpgrp -- system call emulation for 4.2BSD

	last edit:	01-Jul-1983	D A Gwyn
*/

extern int	_setpgrp(), getpid();

int
setpgrp()
	{
	register int	pid = getpid();

	(void)_setpgrp( 0, pid );	/* 0 means current process */
	return pid;
	}
