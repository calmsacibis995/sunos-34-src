#ifndef lint
static	char sccsid[] = "@(#)data.c 1.1 86/09/24 SMI"; /* from UCB 4.2 82/10/05 */
#endif

#include <stdio.h>
#include <sys/param.h>

struct	_iobuf	_iob[_NFILE] ={
	{ 0, NULL, NULL, NULL, _IOREAD, 0},
	{ 0, NULL, NULL, NULL, _IOWRT, 1},
	{ 0, NULL, NULL, NULL, _IOWRT, 2},
};
/*
 * Ptr to end of buffers
 */
struct	_iobuf	*_lastbuf = { &_iob[_NFILE] };
