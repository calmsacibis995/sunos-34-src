#ifndef lint
static	char sccsid[] = "@(#)getpgrp.c 1.1 86/09/24 SMI";
#endif

/*
	getpgrp -- system call emulation for 4.2BSD

	last edit:	01-Jul-1983	D A Gwyn
*/

extern int	_getpgrp();

int
getpgrp()
	{
	return _getpgrp( 0 );		/* 0 means this process */
	}
