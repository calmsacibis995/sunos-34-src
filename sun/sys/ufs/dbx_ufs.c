#ifndef lint
static	char sccsid[] = "@(#)dbx_ufs.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * This file is optionally brought in by including a
 * "psuedo-device dbx" line in the config file.  It is
 * compiled using the "-g" flag to generate structure
 * information which is used by dbx with the -k flag.
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"

#include "../ufs/fs.h"
#include "../ufs/fsdir.h"
#include "../ufs/inode.h"
#include "../ufs/mount.h"
#ifdef QUOTA
#include "../ufs/quota.h"
#endif QUOTA
