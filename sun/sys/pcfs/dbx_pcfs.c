#ifndef lint
static	char sccsid[] = "@(#)dbx_pcfs.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * This file is optionally brought in by including a
 * "psuedo-device dbx" line in the config file.  It is
 * compiled using the "-g" flag to generate structure
 * information which is used by dbx with the -k flag.
 */

#include "../h/param.h"
#include "../h/buf.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"

#include "../pcfs/pc_dir.h"
#include "../pcfs/pc_fs.h"
#include "../pcfs/pc_label.h"
#include "../pcfs/pc_node.h"
