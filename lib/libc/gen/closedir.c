#ifndef lint
static	char sccsid[] = "@(#)closedir.c 1.1 86/09/24 SMI";
#endif

#include <sys/param.h>
#include <sys/dir.h>

/*
 * close a directory.
 */
void
closedir(dirp)
	register DIR *dirp;
{
	extern void free();
	extern int close();

	(void) close(dirp->dd_fd);
	dirp->dd_fd = -1;
	dirp->dd_loc = 0;
	free(dirp->dd_buf);
	free((char *)dirp);
}
