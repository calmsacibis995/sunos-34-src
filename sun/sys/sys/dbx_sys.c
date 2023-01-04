#ifndef lint
static	char sccsid[] = "@(#)dbx_sys.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * This file is optionally brought in by including a
 * "psuedo-device dbx" line in the config file.  It is
 * compiled using the "-g" flag to generate structure
 * information which is used by dbx with the -k flag.
 */

#include "../h/param.h"

#include "../h/acct.h"
#include "../h/buf.h"
#include "../h/callout.h"
#include "../h/clist.h"
#include "../h/cmap.h"
#include "../h/conf.h"
#include "../h/core.h"
#include "../h/des.h"
#include "../h/dir.h"
#include "../h/dkbad.h"
#include "../h/dnlc.h"
#include "../h/domain.h"
#include "../h/file.h"
#include "../h/gprof.h"
#include "../h/ioctl.h"
#include "../h/map.h"
#include "../h/mbuf.h"
#include "../h/msgbuf.h"
#include "../h/mtio.h"
#include "../h/pathname.h"
#include "../h/protosw.h"
#include "../h/ptrace.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/stat.h"
#include "../h/systm.h"
#include "../h/text.h"
#include "../h/time.h"
#include "../h/timeb.h"
#include "../h/times.h"
#include "../h/tty.h"
#include "../h/ttychars.h"
#include "../h/ttydev.h"
#include "../h/uio.h"
#include "../h/un.h"
#include "../h/unpcb.h"
#include "../h/vfs.h"
#include "../h/vm.h"
#include "../h/vnode.h"
#include "../h/vtimes.h"
#include "../h/wait.h"
