#ifndef lint
static	char sccsid[] = "@(#)opendir.c 1.1 86/09/24 SMI";
#endif

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <errno.h>

/*
 * open a directory.
 */
DIR *
opendir(name)
	char *name;
{
	register DIR *dirp;
	register int fd;
	struct stat sb;
	extern int errno;
	extern char *malloc();
	extern int open(), close(), fstat();

	if ((fd = open(name, 0)) == -1)
		return (NULL);
	if (fstat(fd, &sb) == -1) {
		(void) close(fd);
		return (NULL);
	}
	if ((sb.st_mode & S_IFMT) != S_IFDIR) {
		errno = ENOTDIR;
		(void) close(fd);
		return (NULL);
	}
	if (((dirp = (DIR *)malloc(sizeof(DIR))) == NULL) ||
	    ((dirp->dd_buf = malloc((unsigned)sb.st_blksize)) == NULL)) {
		if (dirp)
			free(dirp);
		(void) close(fd);
		return (NULL);
	}
	dirp->dd_bsize = sb.st_blksize;
	dirp->dd_bbase = 0;
	dirp->dd_entno = 0;
	dirp->dd_fd = fd;
	dirp->dd_loc = 0;
	return (dirp);
}
