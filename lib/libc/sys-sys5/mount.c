#ifndef lint
static	char sccsid[] = "@(#)mount.c 1.1 86/09/24 SMI";
#endif

#include <sys/types.h>
#include <sys/mount.h>

int
mount(spec, dir, flags)
	char *spec;
	char *dir;
	int flags;
{
	struct ufs_args args;

	args.fspec = spec;
	return(_mount(MOUNT_UFS, dir, flags, (caddr_t) &args));
}
