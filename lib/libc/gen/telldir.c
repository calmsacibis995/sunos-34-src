#ifndef lint
static	char sccsid[] = "@(#)telldir.c 1.1 86/09/24 SMI";
#endif

#include <sys/param.h>
#include <sys/dir.h>

/*
 * return a pointer into a directory
 */
long
telldir(dirp)
	register DIR *dirp;
{

	return((dirp->dd_bbase * dirp->dd_bsize) + dirp->dd_entno);
}
