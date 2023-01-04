#ifndef lint
static	char sccsid[] = "@(#)dbx_nfs.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * This file is optionally brought in by including a
 * "psuedo-device dbx" line in the config file.  It is
 * compiled using the "-g" flag to generate structure
 * information which is used by dbx with the -k flag.
 */

#include "../h/param.h"
#include "../h/errno.h"
#include "../h/time.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/pathname.h"
#include "../h/uio.h"
#include "../h/socket.h"
#include "../netinet/in.h"

#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"

#include "../nfs/nfs.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/rnode.h"
