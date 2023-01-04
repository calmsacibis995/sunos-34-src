#ifndef lint
static	char sccsid[] = "@(#)stubs.c 1.3 87/01/21 SMI";
#endif

/*
 * Stubs for routines that can't be configured
 * out with binary-only distribution.
 */
#include "../h/errno.h"
#include "../h/types.h"
#include "../h/socket.h"
#include "../h/protosw.h"
#include "../h/domain.h"
#include "../net/if.h"
#include "../ufs/quota.h"

#ifndef GENERIC
setconf() { }
#endif !GENERIC

#ifndef SYSACCT
sysacct() { return (ENODEV); }
acct() { }
#endif !SYSACCT

#ifndef QUOTA
void qtinit() { }
struct dquot *getinoquota() { return ((struct dquot *)0); }
int chkdq() { return (0); }
int chkiq() { return (0); }
void dqrele() { }
int closedq() { return (0); }
#endif !QUOTA

#include "nd.h"
#if NND == 0
struct	ifnet *ndif;
ndintr() { }
nd_chknewaddr() { }
ndopen() { return (ENXIO); }
#endif NND == 0

#include "ms.h"
#if NMS == 0
msintr() { }
#endif NMS == 0

#include "sky.h"
#if defined(sun2)
#if NSKY == 0
skysave() { }
skyrestore() { }
#endif NSKY == 0

#include "pi.h"
#if NPI == 0
short *piaddr, pilast;
piintr() {}
#endif NPI == 0
#endif defined(sun2)

#include "kb.h"
#if NKB == 0
kbdreset() {}
kbdsettrans() {}
#endif NKB == 0

#include "win.h"
#if (NWIN == 0) && defined(sun2)

#if NBWONE == 0
bw1_rop() {}
bw1_putcolormap() {}
#endif NBWONE == 0

#if NCGONE == 0
cg1_rop() {}
cg1_colormap() {}
#endif NCGONE == 0

#endif (NWIN == 0) && defined(sun2)

#include "gpone.h"
#if NGPONE == 0
gp1_kern_sync () {}
kernsyncrestart () {}
#endif NGPONE == 0

#include "zs.h"
#if NZS == 0
zslevel6intr() { panic("level 6 interrupt and no ZS device configured"); }
#endif NZS == 0

#include "ether.h"
#if NETHER == 0
int arpioctl() { return (ENOPROTOOPT); }
int localetheraddr() { return (0); }
#endif NETHER == 0

#ifndef NIT
struct ifnet *nit_ifwithaddr() { return ((struct ifnet *)0); }
nit_tap() {}
struct domain nitdomain =
	{ AF_NIT, "nit", (struct protosw *)0, (struct protosw *)0 };
#endif !NIT

#ifndef IPCSEMAPHORE
seminit() {}
semexit() {}
#endif !IPCSEMAPHORE

#ifndef IPCMESSAGE
msginit() {}
#endif !IPCMESSAGE

#ifndef IPCSHMEM
shmexec() {}
shmfork() {}
shmexit() {}
#endif !IPCSHMEM

#include "fpa.h"
#if NFPA == 0
int fpa_fork_context() { return (0); }
fpa_shutdown() {}
fpa_dorestore() {}
fpa_save() {}
fpa_restore() {}
#endif NFPA == 0
