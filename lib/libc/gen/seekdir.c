#ifndef lint
static	char sccsid[] = "@(#)seekdir.c 1.1 86/09/24 SMI";
#endif

#include <sys/param.h>
#include <sys/dir.h>

/*
 * seek to an entry in a directory.
 * Only values returned by "telldir" should be passed to seekdir.
 */
void
seekdir(dirp, tell)
	register DIR *dirp;
	register long tell;
{
	register struct direct *dp;
	register long entno;
	long base;
	long curloc;
	extern long lseek();

	curloc = telldir(dirp);
	if (curloc == tell)
		return;
	base = tell / dirp->dd_bsize;
	entno = tell % dirp->dd_bsize;
	(void) lseek(dirp->dd_fd, base, 0);
	dirp->dd_loc = 0;
	dirp->dd_entno = 0;
	while (dirp->dd_entno < entno) {
		dp = readdir(dirp);
		if (dp == NULL)
			return;
	}
}
