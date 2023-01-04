/*      @(#)nfs_subr.c 1.1 86/09/25 SMI      */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/buf.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/stat.h"
#include "../h/uio.h"
#include "../net/if.h"
#include "../netinet/in.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../nfs/nfs.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/rnode.h"
#include "../sun/consdev.h"

#ifdef NFSDEBUG
int nfsdebug = 2;
#endif

extern struct vnodeops nfs_vnodeops;
struct inode *iget();
struct rnode *rfind();

/*
 * Client side utilities
 */

/*
 * client side statistics
 */
struct {
	int	nclsleeps;		/* client handle waits */
	int	nclgets;		/* client handle gets */
	int	ncalls;			/* client requests */
	int	nbadcalls;		/* rpc failures */
	int	reqs[32];		/* count of each request */
} clstat;


#define MAXCLIENTS	6
struct chtab {
	int	ch_timesused;
	bool_t	ch_inuse;
	CLIENT	*ch_client;
} chtable[MAXCLIENTS];

int	clwanted = 0;

CLIENT *
clget(mi, cred)
	struct mntinfo *mi;
	struct ucred *cred;
{
	register struct chtab *ch;
	int retrans;

	/*
	 * If soft mount and server is down just try once
	 */
	if (!mi->mi_hard && mi->mi_down) {
		retrans = 1;
	} else {
		retrans = mi->mi_retrans;
	}

	/*
	 * Find an unused handle or create one if not at limit yet.
	 */
	for (;;) {
		clstat.nclgets++;
		for (ch = chtable; ch < &chtable[MAXCLIENTS]; ch++) {
			if (!ch->ch_inuse) {
				ch->ch_inuse = TRUE;
				if (ch->ch_client == NULL) {
					ch->ch_client =
					    clntkudp_create(&mi->mi_addr,
					    NFS_PROGRAM, NFS_VERSION,
					    retrans, cred);
				} else {
					clntkudp_init(ch->ch_client,
					    &mi->mi_addr, retrans, cred);
				}
				if (ch->ch_client == NULL) {
					panic("clget: null client");
				}
				ch->ch_timesused++;
				return (ch->ch_client);
			}
		}
		/*
		 * If we got here there are no available handles
		 */
		clwanted++;
		clstat.nclsleeps++;
		(void) sleep((caddr_t)chtable, PRIBIO);
	}
}

clfree(cl)
	CLIENT *cl;
{
	register struct chtab *ch;

	for (ch = chtable; ch < &chtable[MAXCLIENTS]; ch++) {
		if (ch->ch_client == cl) {
			ch->ch_inuse = FALSE;
			break;
		}
	}
	if (clwanted) {
		clwanted = 0;
		wakeup((caddr_t)chtable);
	}
}

#define	RPC_INTR	18
char *rfsnames[] = {
	"null", "getattr", "setattr", "unused", "lookup", "readlink", "read",
	"unused", "write", "create", "remove", "rename", "link", "symlink",
	"mkdir", "rmdir", "readdir", "fsstat" };

/*
 * Back off for retransmission timeout, MAXTIMO is in 10ths of a sec
 */
#define MAXTIMO	300
#define backoff(tim)	((((tim) << 2) > MAXTIMO) ? MAXTIMO : ((tim) << 2))

int
rfscall(mi, which, xdrargs, argsp, xdrres, resp, cred)
	register struct mntinfo *mi;
	int	 which;
	xdrproc_t xdrargs;
	caddr_t	argsp;
	xdrproc_t xdrres;
	caddr_t	resp;
	struct ucred *cred;
{
	CLIENT *client;
	register enum clnt_stat status;
	struct rpc_err rpcerr;
	struct timeval wait;
	struct ucred *newcred;
	int timeo;
	int user_told;
	bool_t tryagain;

#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "rfscall: %x, %d, %x, %x, %x, %x\n",
	    mi, which, xdrargs, argsp, xdrres, resp);
#endif
	clstat.ncalls++;
	clstat.reqs[which]++;

	rpcerr.re_errno = 0;
	newcred = NULL;
	timeo = mi->mi_timeo;
	user_told = 0;
retry:
	client = clget(mi, cred);

	/*
	 * If hard mounted fs, retry call forever unless hard error occurs
	 */
	do {
		tryagain = FALSE;

		wait.tv_sec = timeo / 10;
		wait.tv_usec = 100000 * (timeo % 10);
		status = CLNT_CALL(client, which, xdrargs, argsp,
		    xdrres, resp, wait);
		switch (status) {
		case RPC_SUCCESS:
			break;

		/*
		 * Unrecoverable errors: give up immediately
		 */
		case RPC_AUTHERROR:
		case RPC_CANTENCODEARGS:
		case RPC_CANTDECODERES:
		case RPC_VERSMISMATCH:
		case RPC_PROGVERSMISMATCH:
		case RPC_CANTDECODEARGS:
			break;

		default:
			if (mi->mi_hard) {
				if (mi->mi_int && interrupted()) {
					(int)status = RPC_INTR;
					rpcerr.re_status = RPC_SYSTEMERROR;
					rpcerr.re_errno = EINTR;
					tryagain = FALSE;
					break;
				} else {
					tryagain = TRUE;
					timeo = backoff(timeo);
					if (!mi->mi_printed) {
						mi->mi_printed = 1;
	printf("NFS server %s not responding still trying\n", mi->mi_hostname);
					}
					if (!user_told && u.u_ttyp &&
					    u.u_ttyd != consdev &&
					    u.u_ttyd != rconsdev) {
						user_told = 1;
	uprintf("NFS server %s not responding still trying\n", mi->mi_hostname);
					}
				}
			}
		}
	} while (tryagain);

	if (status != RPC_SUCCESS) {
		CLNT_GETERR(client, &rpcerr);
		clstat.nbadcalls++;
		mi->mi_down = 1;
		printf("NFS %s failed for server %s: %s\n", rfsnames[which],
			mi->mi_hostname, clnt_sperrno(status));
		if (u.u_ttyp && u.u_ttyd != consdev && u.u_ttyd != rconsdev) {
			uprintf("NFS %s failed for server %s: %s\n",
				rfsnames[which], mi->mi_hostname,
			  	clnt_sperrno(status)); 
		}
	} else if (resp && *(int *)resp == EACCES &&
	    newcred == NULL && cred->cr_uid == 0 && cred->cr_ruid != 0) {
		/*
		 * Boy is this a kludge!  If the reply status is EACCES
		 * it may be because we are root (no root net access).
		 * Check the real uid, if it isn't root make that
		 * the uid instead and retry the call.
		 */
		newcred = crdup(cred);
		cred = newcred;
		cred->cr_uid = cred->cr_ruid;
		clfree(client);
		goto retry;
	} else if (mi->mi_hard) {
		if (mi->mi_printed) {
			printf("NFS server %s ok\n", mi->mi_hostname);
			mi->mi_printed = 0;
		}
		if (user_told) {
			uprintf("NFS server %s ok\n", mi->mi_hostname);
		}
	} else {
		mi->mi_down = 0;
	}

	clfree(client);
#ifdef NFSDEBUG
	dprint(nfsdebug, 7, "rfscall: returning %d\n", rpcerr.re_errno);
#endif
	if (newcred) {
		crfree(newcred);
	}
	return (rpcerr.re_errno);
}

/*
 * Check if this process got an interrupt from the keyboard while sleeping
 */
int
interrupted() 
{
	int s, smask, intr;
	extern wakeup();
	struct proc *p = u.u_procp;

#define bit(a) 	(1<<(a-1))

	s = spl6();
	smask = p->p_sigmask;
	p->p_sigmask |= 
		~(bit(SIGHUP) | bit(SIGINT) | bit(SIGQUIT) | bit(SIGTERM));

	if (ISSIG(u.u_procp)) {
		intr = TRUE;
	} else {
		intr = FALSE;
	}
	p->p_sigmask = smask;
	(void) splx(s);

	return(intr);
}

nattr_to_vattr(na, vap)
	register struct nfsfattr *na;
	register struct vattr *vap;
{

	vap->va_type = (enum vtype)na->na_type;
	vap->va_mode = na->na_mode;
	vap->va_uid = na->na_uid;
	vap->va_gid = na->na_gid;
	vap->va_fsid = na->na_fsid;
	vap->va_nodeid = na->na_nodeid;
	vap->va_nlink = na->na_nlink;
	vap->va_size = na->na_size;
	vap->va_atime.tv_sec  = na->na_atime.tv_sec;
	vap->va_atime.tv_usec = na->na_atime.tv_usec;
	vap->va_mtime.tv_sec  = na->na_mtime.tv_sec;
	vap->va_mtime.tv_usec = na->na_mtime.tv_usec;
	vap->va_ctime.tv_sec  = na->na_ctime.tv_sec;
	vap->va_ctime.tv_usec = na->na_ctime.tv_usec;
	vap->va_rdev = na->na_rdev;
	vap->va_blocks = na->na_blocks;
	switch(na->na_type) {

	case NFBLK:
		vap->va_blocksize = BLKDEV_IOSIZE;
		break;

	case NFCHR:
		vap->va_blocksize = MAXBSIZE;
		break;

	default:
		vap->va_blocksize = na->na_blocksize;
		break;
	}
	/*
	 * This bit of ugliness is a *TEMPORARY* hack to preserve the
	 * over-the-wire protocols for named-pipe vnodes.  It remaps the
	 * special over-the-wire type to the VFIFO type. (see note in nfs.h)
	 *
	 * BUYER BEWARE:
	 *  If you are porting the NFS to a non-SUN server, you probably
	 *  don't want to include the following block of code.  The
	 *  over-the-wire special file types will be changing with the
	 *  NFS Protocol Revision.
	 */
	if (NA_ISFIFO(na)) {
		vap->va_type = VFIFO;
		vap->va_mode = (vap->va_mode & ~S_IFMT) | S_IFIFO;
		vap->va_rdev = 0;
		vap->va_blocksize = na->na_blocksize;
	}

}

vattr_to_sattr(vap, sa)
	register struct vattr *vap;
	register struct nfssattr *sa;
{
	sa->sa_mode = vap->va_mode;
	sa->sa_uid = vap->va_uid;
	sa->sa_gid = vap->va_gid;
	sa->sa_size = vap->va_size;
	sa->sa_atime.tv_sec  = vap->va_atime.tv_sec;
	sa->sa_atime.tv_usec = vap->va_atime.tv_usec;
	sa->sa_mtime.tv_sec  = vap->va_mtime.tv_sec;
	sa->sa_mtime.tv_usec = vap->va_mtime.tv_usec;
}

setdiropargs(da, nm, dvp)
	struct nfsdiropargs *da;
	char *nm;
	struct vnode *dvp;
{

	da->da_fhandle = *vtofh(dvp);
	da->da_name = nm;
}

struct rnode *rpfreelist = NULL;
int rreuse, rnew, ractive;

/*
 * return a vnode for the given fhandle.
 * If no rnode exists for this fhandle create one and put it
 * in a table hashed by fh_fsid and fs_fid.  If the rnode for
 * this fhandle is already in the table return it (ref count is
 * incremented by rfind.  The rnode will be flushed from the
 * table when nfs_inactive calls runsave.
 */
struct vnode *
makenfsnode(fh, attr, vfsp)
	fhandle_t *fh;
	struct nfsfattr *attr;
	struct vfs *vfsp;
{
	register struct rnode *rp;
	char newnode = 0;

	if ((rp = rfind(fh, vfsp)) == NULL) {
		if (rpfreelist) {
			rp = rpfreelist;
			rpfreelist = rp->r_next;
			rreuse++;
		} else {
			rp = (struct rnode *)kmem_alloc((u_int)sizeof(*rp));
			rnew++;
		}
		bzero((caddr_t)rp, sizeof(*rp));
		rp->r_fh = *fh;
		rtov(rp)->v_count = 1;
		rtov(rp)->v_op = &nfs_vnodeops;
		if (attr) {
			rtov(rp)->v_type = n2v_type(attr);
			rtov(rp)->v_rdev = n2v_rdev(attr);
		}
		rtov(rp)->v_data = (caddr_t)rp;
		rtov(rp)->v_vfsp = vfsp;
		rsave(rp);
		((struct mntinfo *)(vfsp->vfs_data))->mi_refct++;
		newnode++;
	}
	if (attr) {
		nfs_attrcache(rtov(rp), attr, (newnode? NOFLUSH: SFLUSH));
	}
	return (rtov(rp));
}

/*
 * Rnode lookup stuff.
 * These routines maintain a table of rnodes hashed by fhandle so
 * that the rnode for an fhandle can be found if it already exists.
 * NOTE: RTABLESIZE must be a power of 2 for rtablehash to work!
 */

#define	RTABLESIZE	16
#define	rtablehash(fh) \
    ((fh->fh_data[2] ^ fh->fh_data[5] ^ fh->fh_data[15]) & (RTABLESIZE-1))

struct rnode *rtable[RTABLESIZE];

/*
 * Put a rnode in the hash table
 */
rsave(rp)
	struct rnode *rp;
{

	rp->r_next = rtable[rtablehash(rtofh(rp))];
	rtable[rtablehash(rtofh(rp))] = rp;
}

/*
 * Remove a rnode from the hash table
 */
runsave(rp)
	struct rnode *rp;
{
	struct rnode *rt;
	struct rnode *rtprev = NULL;
	 
	rt = rtable[rtablehash(rtofh(rp))]; 
	while (rt != NULL) { 
		if (rt == rp) { 
			if (rtprev == NULL) {
				rtable[rtablehash(rtofh(rp))] = rt->r_next;
			} else {
				rtprev->r_next = rt->r_next;
			}
			return; 
		}	
		rtprev = rt;
		rt = rt->r_next;
	}	
}

/*
 * Put an rnode on the free list and take it out if
 * the hash table.
 */
rfree(rp)
	register struct rnode *rp;
{

	runsave(rp);
	rp->r_next = rpfreelist;
	rpfreelist = rp;
}

/*
 * Lookup a rnode by fhandle
 */
struct rnode *
rfind(fh, vfsp)
	fhandle_t *fh;
	struct vfs *vfsp;
{
	register struct rnode *rt;
	 
	rt = rtable[rtablehash(fh)]; 
	while (rt != NULL) { 
		if (bcmp((caddr_t)rtofh(rt), (caddr_t)fh, sizeof(*fh)) == 0 &&
		    vfsp == rtov(rt)->v_vfsp) { 
			rtov(rt)->v_count++; 
			ractive++;
			return (rt); 
		}	
		rt = rt->r_next;
	}	
	return (NULL);
}

/*
 * remove buffers from the buffer cache that have this vfs.
 * NOTE: assumes buffers have been written already.
 */
rinval(vfsp)
	struct vfs *vfsp;
{
	int	i;
	register struct rnode *rp, *rptmp;

	for (i = 0; i < RTABLESIZE; i++) {
		for (rp = rtable[i]; rp; ) {
			if (rp->r_vnode.v_vfsp == vfsp) {
				/*
				 * Need to hold the vp because binval releases
				 * it which will call rfree if the ref count
				 * goes to zero and rfree will mess up rtable.
				 */
				VN_HOLD(rtov(rp));
				rptmp = rp;
				rp = rp->r_next;
				binvalfree(rtov(rptmp));
				VN_RELE(rtov(rptmp));
			} else {
				rp = rp->r_next;
			}
		}
	}
}

/*
 * Flush dirty buffers for all vnodes in this vfs
 */
rflush(vfsp)
	struct vfs *vfsp;
{
	int	i;
	register struct rnode *rp;

	for (i = 0; i < RTABLESIZE; i++) {
		for (rp = rtable[i]; rp; rp = rp->r_next) {
			if (rp->r_vnode.v_vfsp == vfsp) {
				bflush(rtov(rp));
			}
		}
	}
}

#define	PREFIXLEN	4
static char prefix[PREFIXLEN+1] = ".nfs";

char *
newname()
{
	char *news;
	register char *s1, *s2;
	int id;
	static int newnum;

	news = (char *)kmem_alloc((u_int)NFS_MAXNAMLEN);
	for (s1 = news, s2 = prefix; s2 < &prefix[PREFIXLEN]; ) {
		*s1++ = *s2++;
	}
	if (newnum == 0) {
		id = time.tv_sec & 0xffff;
	}
	id = newnum++;
	while (id) {
		*s1++ = "0123456789ABCDEF"[id & 0x0f];
		id = id >> 4;
	}
	*s1 = '\0';
	return (news);
}

/*
 * General utilities
 */

/*
 * Returns the prefered transfer size in bytes based on
 * what network interfaces are available.
 */
nfstsize()
{
	register struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp->if_name[0] == 'e' && ifp->if_name[1] == 'c') {
#ifdef NFSDEBUG
			dprint(nfsdebug, 3, "nfstsize: %d\n", ECTSIZE);
#endif
			return (ECTSIZE);
		}
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 3, "nfstsize: %d\n", IETSIZE);
#endif
	return (IETSIZE);
}

#ifdef NFSDEBUG
/*
 * Utilities used by both client and server
 * Standard levels:
 * 0) no debugging
 * 1) hard failures
 * 2) soft failures
 * 3) current test software
 * 4) main procedure entry points
 * 5) main procedure exit points
 * 6) utility procedure entry points
 * 7) utility procedure exit points
 * 8) obscure procedure entry points
 * 9) obscure procedure exit points
 * 10) random stuff
 * 11) all <= 1
 * 12) all <= 2
 * 13) all <= 3
 * ...
 */

/*VARARGS2*/
dprint(var, level, str, a1, a2, a3, a4, a5, a6, a7, a8, a9)
	int var;
	int level;
	char *str;
	int a1, a2, a3, a4, a5, a6, a7, a8, a9;
{
	if (var == level || (var > 10 && (var - 10) >= level))
		printf(str, a1, a2, a3, a4, a5, a6, a7, a8, a9);
}
#endif
