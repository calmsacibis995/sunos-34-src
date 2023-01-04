#ifndef lint
static	char sccsid[] = "@(#)freopen.c 1.1 86/09/24 SMI"; /* from UCB 4.2 81/03/09 */
#endif

#include <stdio.h>
#include <errno.h>
#include <sys/file.h>

FILE *
freopen(file, mode, iop)
	char *file;
	register char *mode;
	register FILE *iop;
{
	extern int errno;
	register f, rw, openflags;

	rw = mode[1] == '+';

	fclose(iop);
	openflags = 0;
	switch (*mode){
	case 'w':
		openflags = O_TRUNC;
		/* FALL THROUGH */
	case 'a':
		openflags |= O_CREAT|(rw?O_RDWR:O_WRONLY);
		break;
	case 'r':
		openflags = rw?O_RDWR:O_RDONLY;
		break;
	}
	f = open(file, openflags, 0666);
	if (f < 0)
		return (NULL);
	iop->_cnt = 0;
	iop->_file = f;
	switch (*mode){
	case 'r':
		iop->_flag |= rw?_IORW:_IOREAD;
		break;
	case 'a':
		lseek(f, 0L, 2);
		/* FALL THROUGH */
	case 'w':
		iop->_flag |= rw?_IORW:_IOWRT;
		break;
	}
	return (iop);
}
