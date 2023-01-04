/*      @(#)nfs_vfsops.c 1.1 86/09/25 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
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
#include "../h/mount.h"
#include "../net/if.h"


#ifdef NFSDEBUG
extern int nfsdebug;
#endif

#ifdef NFSROOT
/*
 * this is the default filesystem type.
 * this should be setup by the configurator
 */
int nfs_mountroot();
int (*rootfsmount)() = nfs_mountroot;

#ifdef PUMPKINSEED
fhandle_t nfsrootfh = {      /* speed:/usr.MC68020/swap/pumpkinseed */
	0x00, 0x00, 0x03, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
	0x00, 0x00, 0x18, 0x00, 0x53, 0x68, 0x19, 0x6b, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00,
	};

int nfsrootaddr = 0x0C0090161;
char *nfsrootname = "speed";
#endif

#ifdef GUPPY

fhandle_t nfsrootfh = {	/* phoenix:/usr/guppy */
	0x00, 0x00, 0x03, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 
	0x00, 0x00, 0x31, 0x86, 0x17, 0xce, 0x51, 0x82, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 
	};
int nfsrootaddr = 0x0C0090135;
char *nfsrootname = "phoenix";
#endif
#endif

struct vnode *nfsrootvp();
struct vnode *makenfsnode();
int nfsmntno;

/*
 * nfs vfs operations.
 */
int nfs_mount();
int nfs_unmount();
int nfs_root();
int nfs_statfs();
int nfs_sync();
extern int nfs_badop();

struct vfsops nfs_vfsops = {
	nfs_mount,
	nfs_unmount,
	nfs_root,
	nfs_statfs,
	nfs_sync,
	nfs_badop
};

#ifdef NFSROOT
/*
 * Called by vfs_mountroot when ufs is going to be mounted as root
 */
nfs_mountroot()
{
	struct vnode *rtvp;
	struct sockaddr_in saddr;	/* server's address */
	struct vfs *vfsp;

	vfsp = (struct vfs *)kmem_alloc((u_int)sizeof(*vfsp));
	bzero((caddr_t)vfsp, sizeof(*vfsp));
	VFS_INIT(vfsp, &nfs_vfsops, (caddr_t)0);

	saddr.sin_family = AF_INET;
	saddr.sin_port = NFS_PORT;
	saddr.sin_addr.s_addr = (u_int)nfsrootaddr;
	rtvp = nfsrootvp(vfsp, &saddr, &nfsrootfh, nfsrootname);
	if (rtvp) {
		if (vfs_add((struct vnode *)0, vfsp, 0)) {
			panic("nfs_mountroot: vfs_add failed");
		}
		vfs_unlock(rtvp->v_vfsp);
		rootvp = rtvp;
		return (0);
	}
	kmem_free(vfsp, sizeof(*vfsp));
	return (EBUSY);
}
#endif

#ifdef NFSSWAP

#ifdef GUPPY
/*
 * The server port, addr, fhandle and name have to come from a swap service.
 */
fhandle_t swap_fhandle = {	/* phoenix:/usr/guppy.swap/swap */
	0x00, 0x00, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 
	0x00, 0x00, 0x00, 0x04, 0x1f, 0xd0, 0xe7, 0xb7, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 
	};
char	*swap_hostname = "phoenix";
int	swap_addr = (192 << 24) + (9 << 16) + (1 << 8) + 53;	/* phoenix */
#endif

#ifdef PUMPKINSEED
fhandle_t swap_fhandle = {    /* speed:/usr.MC68020/swap/blyon/swap.blyon */
	0x00, 0x00, 0x03, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
	0x00, 0x00, 0x10, 0x01, 0x31, 0xa0, 0x0e, 0xe2, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00,
        };
char	*swap_hostname = "speed";
int	swap_addr = (192 << 24) + (9 << 16) + (1 << 8) + 97;	/* speed */
#endif

caddr_t dump_clnt_handle;

/*
 * Set up for swapping to NFS.
 * Call nfsrootvp to set up the
 * RPC/NFS machinery.
 */
struct vnode *
config_nfsswap()
{
	struct sockaddr_in swap_sin;
	struct vnode *rvp;	/* DEBUG */
	struct vfs *vfsp;

	vfsp = (struct vfs *)kmem_alloc((u_int)sizeof(*vfsp));
	bzero((caddr_t)vfsp, sizeof(*vfsp));
	VFS_INIT(vfsp, &nfs_vfsops, (caddr_t)0);

	bzero((caddr_t)&swap_sin, sizeof swap_sin);
	swap_sin.sin_family = htons(AF_INET);
	swap_sin.sin_port = NFS_PORT;
	swap_sin.sin_addr.s_addr = swap_addr;
	rvp = nfsrootvp(vfsp, &swap_sin, &swap_fhandle, swap_hostname);
	if (rvp) {
		dump_clnt_handle = (caddr_t)clget(
		    (struct mntinfo *) rvp->v_vfsp->vfs_data, u.u_cred);
	} else {
		kmem_free((caddr_t)vfsp, sizeof (*vfsp));
		dump_clnt_handle = NULL;
	}
	return (rvp);
	/*
	return (nfsrootvp(NULL, &swap_sin, &swap_fhandle, &swap_hostname));
	*/
}
#endif

/*
 * nfs mount vfsop
 * Set up mount info record and attach it to vfs struct.
 */
/*ARGSUSED*/
nfs_mount(vfsp, path, data)
	struct vfs *vfsp;
	char *path;
	caddr_t data;
{
	int error;
	struct vnode *rtvp = NULL;	/* the server's root */
	struct mntinfo *mi;		/* mount info, pointed at by vfs */
	fhandle_t fh;			/* root fhandle */
	struct sockaddr_in saddr;	/* server's address */
	char hostname[HOSTNAMESZ];	/* server's name */
	struct nfs_args args;		/* nfs mount arguments */

	/*
	 * get arguments
	 */
	error = copyin(data, (caddr_t)&args, sizeof (args));
	if (error) {
		goto errout;
	}

	/*
	 * Get server address
	 */
	error = copyin((caddr_t)args.addr, (caddr_t)&saddr,
	    sizeof(saddr));
	if (error) {
		goto errout;
	}
	/*
	 * For now we just support AF_INET
	 */
	if (saddr.sin_family != AF_INET) {
		error = EPFNOSUPPORT;
		goto errout;
	}

	/*
	 * Get the root fhandle
	 */
	error = copyin((caddr_t)args.fh, (caddr_t)&fh, sizeof(fh));
	if (error) {
		goto errout;
	}

	/*
	 * Get server's hostname
	 */
	if (args.flags & NFSMNT_HOSTNAME) {
		error = copyin((caddr_t)args.hostname, (caddr_t)hostname,
		    HOSTNAMESZ);
		if (error) {
			goto errout;
		}
	} else {
		addr_to_str(&saddr, hostname);
	}

	/*
	 * Get root vnode.
	 */
	rtvp = nfsrootvp(vfsp, &saddr, &fh, hostname);
	if (!rtvp) {
		error = EBUSY;
		goto errout;
	}

	/*
	 * Set option fields in mount info record
	 */
	mi = vtomi(rtvp);
	mi->mi_hard = ((args.flags & NFSMNT_SOFT) == 0);
	mi->mi_int = ((args.flags & NFSMNT_INT) == NFSMNT_INT);
	if (args.flags & NFSMNT_RETRANS) {
		mi->mi_retrans = args.retrans;
		if (args.retrans < 0) {
			error = EINVAL;
			goto errout;
		}
	}
	if (args.flags & NFSMNT_TIMEO) {
		mi->mi_timeo = args.timeo;
		if (args.timeo <= 0) {
			error = EINVAL;
			goto errout;
		}
	}
	if (args.flags & NFSMNT_RSIZE) {
		if (args.rsize <= 0) {
			error = EINVAL;
			goto errout;
		}
		mi->mi_tsize = MIN(mi->mi_tsize, args.rsize);
	}
	if (args.flags & NFSMNT_WSIZE) {
		if (args.wsize <= 0) {
			error = EINVAL;
			goto errout;
		}
		mi->mi_stsize = MIN(mi->mi_stsize, args.wsize);
	}

#ifdef NFSDEBUG
	dprint(nfsdebug, 1,
	    "nfs_mount: hard %d timeo %d retries %d wsize %d rsize %d\n",
	    mi->mi_hard, mi->mi_timeo, mi->mi_retrans, mi->mi_stsize,
	    mi->mi_tsize);
#endif

errout:
	if (error) {
		if (rtvp) {
			VN_RELE(rtvp);
		}
	}
	return (error);
}

struct vnode *
nfsrootvp(vfsp, sin, fh, hostname)
	struct vfs *vfsp;		/* vfs of fs, if NULL amke one */
	struct sockaddr_in *sin;	/* server address */
	fhandle_t *fh;			/* swap file fhandle */
	char *hostname;			/* swap server name */
{
	struct vnode *rtvp = NULL;	/* the server's root */
	struct mntinfo *mi = NULL;	/* mount info, pointed at by vfs */
	struct vattr va;		/* root vnode attributes */
	struct nfsfattr na;		/* root vnode attributes in nfs form */
	struct statfs sb;		/* server's file system stats */
	struct ifnet *ifp;		/* interface */
	extern struct ifnet loif;

	/*
	 * Get the internet address for the first AF_INET interface
	 * using reverse arp. This is not quite right because the
	 * interface may not be ethernet! XXX
	 */
	if ((ifp = if_ifwithaf(AF_INET)) == 0 || ifp == &loif) {
		panic("nfs_mountroot: no interface\n");
	}
	if (!address_known(ifp)) {
		revarp_myaddr(ifp);
	}

	/*
	 * create a mount record and link it to the vfs struct
	 */
	mi = (struct mntinfo *)kmem_alloc((u_int)sizeof(*mi));
	bzero((caddr_t)mi, sizeof(*mi));
	mi->mi_hard = 1;
	mi->mi_addr = *sin;
	mi->mi_retrans = NFS_RETRIES;
	mi->mi_timeo = NFS_TIMEO;
	mi->mi_mntno = nfsmntno++;
	bcopy(hostname, mi->mi_hostname, HOSTNAMESZ);

	/*
	 * Make a vfs struct for nfs.  We do this here instead of below
	 * because rtvp needs a vfs before we can do a getattr on it.
	 */
	vfsp->vfs_fsid.val[0] = mi->mi_mntno;
	vfsp->vfs_fsid.val[1] = MOUNT_NFS;
	vfsp->vfs_data = (caddr_t)mi;

	/*
	 * Make the root vnode, use it to get attributes, then remake it
	 * with the attributes
	 */
	rtvp = makenfsnode(fh, (struct nfsfattr *) 0, vfsp);
	if ((rtvp->v_flag & VROOT) != 0) {
		goto bad;
	}
	if (VOP_GETATTR(rtvp, &va, u.u_cred)) {
		goto bad;
	}
	VN_RELE(rtvp);
	vattr_to_nattr(&va, &na);
	rtvp = makenfsnode(fh, &na, vfsp);
	rtvp->v_flag |= VROOT;
	mi->mi_rootvp = rtvp;

	/*
	 * Get server's filesystem stats.  Use these to set transfer
	 * sizes, filesystem block size, and read-only.
	 */
	if (VFS_STATFS(vfsp, &sb)) {
		goto bad;
	}
	mi->mi_tsize = min(NFS_MAXDATA, (u_int)nfstsize());

	/*
	 * Set filesystem block size to at least CLBYTES and at most MAXBSIZE
	 */
	mi->mi_bsize = MAX(va.va_blocksize, CLBYTES);
	mi->mi_bsize = MIN(mi->mi_bsize, MAXBSIZE);
	vfsp->vfs_bsize = mi->mi_bsize;

	/*
	 * Need credentials in the rtvp so do_bio can find them.
	 */
	crhold(u.u_cred);
	vtor(rtvp)->r_cred = u.u_cred;

	return (rtvp);
bad:
	if (mi) {
		kmem_free((caddr_t)mi, sizeof (*mi));
	}
	if (rtvp) {
		VN_RELE(rtvp);
	}
	return (NULL);
}

/*
 * vfs operations
 */

nfs_unmount(vfsp)
	struct vfs *vfsp;
{
	struct mntinfo *mi = (struct mntinfo *)vfsp->vfs_data;

#ifdef NFSDEBUG
        dprint(nfsdebug, 4, "nfs_unmount(%x) mi = %x\n", vfsp, mi);
#endif
	rflush(vfsp);
	rinval(vfsp);

	if (mi->mi_refct != 1 || mi->mi_rootvp->v_count != 1) {
		return (EBUSY);
	}
	VN_RELE(mi->mi_rootvp);
	kmem_free((caddr_t)mi, (u_int)sizeof(*mi));
	return(0);
}

/*
 * find root of nfs
 */
int
nfs_root(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{

	*vpp = (struct vnode *)((struct mntinfo *)vfsp->vfs_data)->mi_rootvp;
	(*vpp)->v_count++;
#ifdef NFSDEBUG
        dprint(nfsdebug, 4, "nfs_root(0x%x) = %x\n", vfsp, *vpp);
#endif
	return(0);
}

/*
 * Get file system statistics.
 */
int
nfs_statfs(vfsp, sbp)
register struct vfs *vfsp;
struct statfs *sbp;
{
	struct nfsstatfs fs;
	struct mntinfo *mi;
	fhandle_t *fh;
	int error = 0;

	mi = vftomi(vfsp);
	fh = vtofh(mi->mi_rootvp);
#ifdef NFSDEBUG
        dprint(nfsdebug, 4, "nfs_statfs vfs %x\n", vfsp);
#endif
	error = rfscall(mi, RFS_STATFS, xdr_fhandle,
	    (caddr_t)fh, xdr_statfs, (caddr_t)&fs, u.u_cred);
	if (!error) {
		error = geterrno(fs.fs_status);
	}
	if (!error) {
		if (mi->mi_stsize) {
			mi->mi_stsize = MIN(mi->mi_stsize, fs.fs_tsize);
		} else {
			mi->mi_stsize = fs.fs_tsize;
		}
		sbp->f_bsize = fs.fs_bsize;
		sbp->f_blocks = fs.fs_blocks;
		sbp->f_bfree = fs.fs_bfree;
		sbp->f_bavail = fs.fs_bavail;
		/*
		 * XXX This is wrong - should be a real fsid
		 */
		bcopy((caddr_t)&vfsp->vfs_fsid,
		    (caddr_t)&sbp->f_fsid, sizeof (fsid_t));
	}
#ifdef NFSDEBUG
        dprint(nfsdebug, 5, "nfs_statfs returning %d\n", error);
#endif
	return (error);
}

/*
 * Flush any pending I/O.
 */
int
nfs_sync(vfsp)
	struct vfs * vfsp;
{

#ifdef NFSDEBUG
        dprint(nfsdebug, 5, "nfs_sync %x\n", vfsp);
#endif
	rflush(vfsp);
	return(0);
}

static char *
itoa(n, str)
	u_short n;
	char *str;
{
	char prbuf[11];
	register char *cp;

	cp = prbuf;
	do {
		*cp++ = "0123456789"[n%10];
		n /= 10;
	} while (n);
	do {
		*str++ = *--cp;
	} while (cp > prbuf);
	return (str);
}

/*
 * Convert a INET address into a string for printing
 */
static
addr_to_str(addr, str)
	struct sockaddr_in *addr;
	char *str;
{
	str = itoa(addr->sin_addr.s_net, str);
	*str++ = '.';
	str = itoa(addr->sin_addr.s_host, str);
	*str++ = '.';
	str = itoa(addr->sin_addr.s_lh, str);
	*str++ = '.';
	str = itoa(addr->sin_addr.s_impno, str);
	*str = '\0';
}
