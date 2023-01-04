/*	@(#)kern_exec.c 1.3 87/04/15 SMI; from UCB 6.14 85/08/12	*/

#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/psl.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/socketvar.h"
#include "../h/vnode.h"
#include "../h/pathname.h"
#include "../h/seg.h"
#include "../h/vm.h"
#include "../h/text.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/acct.h"
#include "../h/vfs.h"

#ifdef vax
#include "../vax/mtpr.h"
#endif

/*
 * texts below this size will be read in (if there is enough free memory)
 * even though the file is ZMAGIC.
 */
size_t pgthresh = clrnd(btoc(PGTHRESH));

/*
 * exec system call, with and without environments.
 */
struct execa {
	char	*fname;
	char	**argp;
	char	**envp;
};

execv()
{
	((struct execa *)u.u_ap)->envp = NULL;
	execve();
}

execve()
{
	register nc;
	register char *cp;
	register struct buf *bp;
	register struct execa *uap;
	int na, ne, ucp, ap;
	int indir, uid, gid;
	char *sharg;
	char *execnamep;
	struct vnode *vp;
	struct vattr vattr;
	daddr_t bno;
	struct pathname pn;
	char cfarg[SHSIZE];
	int resid, error;
	u_int len, cc;

	uap = (struct execa *)u.u_ap;
	u.u_error = pn_get(uap->fname, UIOSEG_USER, &pn);
	if (u.u_error)
		return;
	u.u_error = lookuppn(&pn, FOLLOW_LINK, (struct vnode **)0, &vp);
	if (u.u_error) {
		pn_free(&pn);
		return;
	}
	bno = 0;
	bp = 0;
	indir = 0;
	uid = u.u_uid;
	gid = u.u_gid;
	if (u.u_error = VOP_GETATTR(vp, &vattr, u.u_cred))
		goto bad;
	if ((vp->v_vfsp->vfs_flag & VFS_NOSUID) == 0) {
		if (vattr.va_mode & VSUID)
			uid = vattr.va_uid;
		if (vattr.va_mode & VSGID)
			gid = vattr.va_gid;
	} else if ((vattr.va_mode & VSUID) || (vattr.va_mode & VSGID)) {
		struct pathname tmppn;

		u.u_error = pn_get(uap->fname, UIOSEG_USER, &tmppn);
		if (u.u_error)
			return;
		printf("%s: Setuid execution not allowed\n", tmppn.pn_buf);
		pn_free(&tmppn);
	}

  again:
	/*
	 * XXX should change VOP_ACCESS to not let super user always have it
	 * for exec permission on regular files.
	 */
	if (u.u_error = VOP_ACCESS(vp, VEXEC, u.u_cred))
		goto bad;
	if ((u.u_procp->p_flag & STRC)
	    && (u.u_error = VOP_ACCESS(vp, VREAD, u.u_cred)))
		goto bad;
	if (vp->v_type != VREG ||
	   (vattr.va_mode & (VEXEC|(VEXEC>>3)|(VEXEC>>6))) == 0) {
		u.u_error = EACCES;
		goto bad;
	}

	/*
	 * Read in first few bytes of file for segment sizes, ux_mag:
	 *	OMAGIC = plain executable
	 *	NMAGIC = RO text
	 *	ZMAGIC = demand paged RO text
	 * Also an ASCII line beginning with #! is
	 * the file name of a ``shell'' and arguments may be prepended
	 * to the argument list if given here.
	 *
	 * SHELL NAMES ARE LIMITED IN LENGTH.
	 *
	 * ONLY ONE ARGUMENT MAY BE PASSED TO THE SHELL FROM
	 * THE ASCII LINE.
	 */
	u.u_exdata.ux_shell[0] = '\0';	/* for zero length files */
	u.u_error =
	    vn_rdwr(UIO_READ, vp, (caddr_t)&u.u_exdata, sizeof (u.u_exdata),
		0, UIOSEG_KERNEL, IO_UNIT, &resid);
	if (u.u_error)
		goto bad;
#ifndef lint
	if (resid > sizeof (u.u_exdata) - sizeof (u.u_exdata.Ux_A) &&
	    u.u_exdata.ux_shell[0] != '#') {
		u.u_error = ENOEXEC;
		goto bad;
	}
#endif
	switch (u.u_exdata.ux_mag) {

	case OMAGIC:
		u.u_exdata.ux_dsize += u.u_exdata.ux_tsize;
		u.u_exdata.ux_tsize = 0;
		break;

	case ZMAGIC:
	case NMAGIC:
		if (u.u_exdata.ux_tsize == 0) {
			u.u_error = ENOEXEC;
			goto bad;
		}
		break;

	default:
		if (u.u_exdata.ux_shell[0] != '#' ||
		    u.u_exdata.ux_shell[1] != '!' ||
		    indir) {
			u.u_error = ENOEXEC;
			goto bad;
		}
		cp = &u.u_exdata.ux_shell[2];		/* skip "#!" */
		while ((u_int)cp < (u_int)&u.u_exdata.ux_shell[SHSIZE]) {
			if (*cp == '\t')
				*cp = ' ';
			else if (*cp == '\n') {
				*cp = '\0';
				break;
			}
			cp++;
		}
		if (*cp != '\0') {
			u.u_error = ENOEXEC;
			goto bad;
		}
		cp = &u.u_exdata.ux_shell[2];
		while (*cp == ' ')
			cp++;
		execnamep = cp;
		while (*cp && *cp != ' ')
			cp++;
		cfarg[0] = '\0';
		if (*cp) {
			*cp++ = '\0';
			while (*cp == ' ')
				cp++;
			if (*cp)
				bcopy((caddr_t)cp, (caddr_t)cfarg, SHSIZE);
		}
		indir = 1;
		VN_RELE(vp);
		vp = (struct vnode *)0;
		if (u.u_error = pn_set(&pn, execnamep))
			goto bad;
		u.u_error = lookuppn(&pn, FOLLOW_LINK, (struct vnode **)0, &vp);
		if (u.u_error)
			goto bad;
		if (u.u_error = VOP_GETATTR(vp, &vattr, u.u_cred))
			goto bad;
		goto again;
	}

	/*
	 * Collect arguments on "file" in swap space.
	 */
	na = 0;
	ne = 0;
	nc = 0;
	cc = 0;
	bno = (daddr_t)rmalloc(argmap, (long)ctod(clrnd((int)btoc(NCARGS))));
	if (bno == 0) {
		swkill(u.u_procp, "exece: no swap space for arg list");
		goto bad;
	}
	if (bno % CLSIZE)
		panic("execa rmalloc");
	/*
	 * Copy arguments into file in argdev area.
	 */
	if (uap->argp) for (;;) {
		ap = NULL;
		sharg = NULL;
		if (indir && na == 0) {
			sharg = pn.pn_buf;
			ap = (int)sharg;
			uap->argp++;		/* ignore argv[0] */
		} else if (indir && (na == 1 && cfarg[0])) {
			sharg = cfarg;
			ap = (int)sharg;
		} else if (indir && (na == 1 || na == 2 && cfarg[0]))
			ap = (int)uap->fname;
		else if (uap->argp) {
			ap = fuword((caddr_t)uap->argp);
			uap->argp++;
		}
		if (ap == NULL && uap->envp) {
			uap->argp = NULL;
			if ((ap = fuword((caddr_t)uap->envp)) != NULL)
				uap->envp++, ne++;
		}
		if (ap == NULL)
			break;
		na++;
		if (ap == -1) {
			u.u_error = EFAULT;
			break;
		}
		do {
			/*
			 * Since we don't want to depend on NCARGS being
			 * a multiple of CLBYTES we cannot simply cleck
			 * for overflow before each buffer allocation.
			 */
			if (nc >= NCARGS-1) {
				error = E2BIG;
				break;
			}
			if ((int)cc <= 0) {
				if (bp)
					bdwrite(bp);
				cc = CLBYTES;
				bp = getblk(argdev_vp,
				    (daddr_t)(bno + ctod(nc / NBPG)), (int)cc);
				cp = bp->b_un.b_addr;
			}
			if (sharg) {
				error = copystr(sharg, cp, cc, &len);
				sharg += len;
			} else {
				error = copyinstr((caddr_t)ap, cp, cc, &len);
				ap += len;
			}
			cp += len;
			nc += len;
			cc -= len;
		} while (error == ENAMETOOLONG);
		if (error) {
			u.u_error = error;
			if (bp)
				brelse(bp);
			bp = 0;
			goto badarg;
		}
	}
	if (bp)
		bdwrite(bp);
	bp = 0;
	nc = (nc + NBPW-1) & ~(NBPW-1);
	getxfile(vp, nc + (na+4)*NBPW, uid, gid, &pn);
	if (u.u_error) {
badarg:
		for (cc = 0; cc < nc; cc += CLBYTES) {
			bp = baddr(argdev_vp, (daddr_t)(bno + ctod(cc / NBPG)),
			    CLBYTES);
			if (bp) {
				bp->b_flags |= B_AGE;		/* throw away */
				bp->b_flags &= ~B_DELWRI;	/* cancel io */
				brelse(bp);
				bp = 0;
			}
		}
		goto bad;
	}
	VN_RELE(vp);
	vp = NULL;

	/*
	 * Copy back arglist.
	 */
	ucp = USRSTACK - nc - NBPW;
	ap = ucp - na*NBPW - 3*NBPW;
	u.u_ar0[SP] = ap;
	(void) suword((caddr_t)ap, na-ne);
	nc = 0;
	cc = 0;
	for (;;) {
		ap += NBPW;
		if (na == ne) {
			(void) suword((caddr_t)ap, 0);
			ap += NBPW;
		}
		if (--na < 0)
			break;
		(void) suword((caddr_t)ap, ucp);
		do {
			if ((int)cc <= 0) {
				if (bp)
					brelse(bp);
				cc = CLBYTES;
				bp = bread(argdev_vp,
				    (daddr_t)(bno + ctod(nc / NBPG)), (int)cc);
				bp->b_flags |= B_AGE;		/* throw away */
				bp->b_flags &= ~B_DELWRI;	/* cancel io */
				cp = bp->b_un.b_addr;
			}
			error = copyoutstr(cp, (caddr_t)ucp, cc, &len);
			ucp += len;
			cp += len;
			nc += len;
			cc -= len;
		} while (error == ENAMETOOLONG);
		if (error == EFAULT)
			panic("exec: EFAULT");
	}
	(void) suword((caddr_t)ap, 0);
#ifdef VAC
	/*
	 * Flush the args to memory so that ps etc. can find them
	 * through /dev/mem.
	 */
	ap += NBPW;
	vac_flush((caddr_t)ap, USRSTACK - ap);
#endif VAC

	/*
	 * Reset caught signals.  Held signals
	 * remain held through p_sigmask.
	 */
	while (u.u_procp->p_sigcatch) {
		nc = ffs((long)u.u_procp->p_sigcatch);
		u.u_procp->p_sigcatch &= ~sigmask(nc);
		u.u_signal[nc] = SIG_DFL;
	}

	/*
	 * Reset stack state to the user stack.
	 * Clear set of signals caught on the signal stack.
	 */
	u.u_onstack = 0;
	u.u_sigsp = 0;
	u.u_sigonstack = 0;

	for (nc = 0; nc < NOFILE; nc++) {
		register struct file *f;

		/* close all Close-On-Exec files */
		if (u.u_pofile[nc] & UF_EXCLOSE) {
			f = u.u_ofile[nc];

			/* Release all System-V style record locks, if any */
			(void) vno_lockrelease(f);

			closef(f);
			u.u_ofile[nc] = NULL;
			u.u_pofile[nc] = 0;
		}
		u.u_pofile[nc] &= ~UF_MAPPED;
	}
	setregs(u.u_exdata.ux_entloc);

	/*
	 * Remember file name for accounting.
	 */
	u.u_acflag &= ~AFORK;
	if (pn.pn_pathlen > MAXCOMLEN)
		pn.pn_pathlen = MAXCOMLEN;
	bcopy((caddr_t)pn.pn_buf, (caddr_t)u.u_comm,
	    (unsigned)(pn.pn_pathlen + 1));

bad:
	pn_free(&pn);
	if (bp)
		brelse(bp);
	if (bno)
		rmfree(argmap, (long)ctod(clrnd((int)btoc(NCARGS))), (long)bno);
	if (vp)
		VN_RELE(vp);
}

int execwarn = 1;

/*
 * Read in and set up memory for executed file.
 */
getxfile(vp, nargc, uid, gid, pn)
	register struct vnode *vp;
	int nargc, uid, gid;
	struct pathname *pn;
{
	register size_t ts, ds, ss;	/* size info */
	int pagi;
	struct dmap *tdmap = 0;		/* temporaries */
	caddr_t getdmem(), gettmem();
	size_t getts(), getds();

	/*
	 * Check to make sure nobody is modifying the text right now
	 */
	if ((vp->v_flag & VTEXTMOD) != 0) {
		u.u_error  = ETXTBSY;
		goto bad;
	}

	if ((u.u_exdata.ux_tsize != 0) && ((vp->v_flag & VTEXT) == 0) &&
	    (vp->v_count != 1)) {
		register struct file *fp;

		for (fp = file; fp < fileNFILE; fp++) {
			if (fp->f_type == DTYPE_VNODE &&
			    fp->f_count > 0 &&
			    (struct vnode *)fp->f_data == vp &&
			    (fp->f_flag & FWRITE)) {
				u.u_error = ETXTBSY;
				goto bad;
			}
		}
	}

	/*
	 * Compute text and data sizes and make sure not too large.
	 */
	u.u_error = chkaout();
	if (u.u_error)
		goto bad;
	ts = (size_t)getts();
	ds = (size_t)getds();
	ss = clrnd(SSIZE + btoc(nargc));
	if (chksize((unsigned)ts, (unsigned)ds, (unsigned)ss))
		goto bad;

	if ((u.u_exdata.ux_mag == ZMAGIC) &&
	    ((ts + clrnd(btoc(u.u_exdata.ux_dsize))) > MIN(freemem, pgthresh))
#ifdef sun
	    && (u.u_exdata.ux_mach != M_OLDSUN2)	/* old a.out */
#endif sun
	    ) {
		if (vp->v_vfsp->vfs_bsize < CLBYTES) {
			if (execwarn) {
				uprintf("Warning: file system block size ");
				uprintf("too small, '%s' not demand paged\n",
				    pn->pn_buf);
			}
			pagi = 0;
		} else {
			pagi = SPAGI;
		}
	} else
		pagi = 0;

	/*
	 * Make sure there is enough space to start process.
	 */
	tdmap = (struct dmap *)kmem_alloc(2 * sizeof (struct dmap));
	tdmap[0] = zdmap;
	tdmap[1] = zdmap;
	if (swpexpand(ds, ss, &tdmap[0], &tdmap[1]) == NULL)
		goto bad;

	/*
	 * At this point, we are committed to the new image!
	 * Release virtual memory resources of old process, and
	 * initialize the virtual memory of the new process.
	 * If we resulted from vfork(), instead wakeup our
	 * parent who will set SVFDONE when he has taken back
	 * our resources.
	 */
	if ((u.u_procp->p_flag & SVFORK) == 0) {
#ifdef IPCSHMEM
		/* PRE-VM-REWRITE */
		shmexec();		/* release all shared-memory */
		/* END ... PRE-VM-REWRITE */
#endif IPCSHMEM
		vrelvm();
	} else {
		u.u_procp->p_flag &= ~SVFORK;
		u.u_procp->p_flag |= SKEEP;
		wakeup((caddr_t)u.u_procp);
		while ((u.u_procp->p_flag & SVFDONE) == 0)
			(void) sleep((caddr_t)u.u_procp, PZERO - 1);
		u.u_procp->p_flag &= ~(SVFDONE|SKEEP);
	}
	u.u_procp->p_flag &= ~(SPAGI|SSEQL|SUANOM);
	u.u_procp->p_flag |= pagi;
	u.u_dmap = tdmap[0];
	u.u_smap = tdmap[1];
#ifdef sun
	u.u_hole.uh_first = u.u_hole.uh_last = 0;
#endif
	vgetvm(ts, ds, ss);

	if (pagi == 0) {
		/* not paging this one */
		u.u_error = vn_rdwr(UIO_READ, vp, getdmem(),
		    (int)u.u_exdata.ux_dsize, getdfile(),
		    UIOSEG_USER, IO_UNIT, (int *)0);
		if (u.u_error == 0) {
#ifdef sun
			if (u.u_exdata.ux_mach == M_OLDSUN2) {
				/* old a.out */
				u.u_error = vn_rdwr(UIO_READ, vp, gettmem(),
				    (int)u.u_exdata.ux_tsize, gettfile(),
				    UIOSEG_USER, IO_UNIT, (int *)0);
				/* adjust sizes to OMAGIC type */
				u.u_exdata.ux_tsize = 0;
				u.u_exdata.ux_dsize = ctob(ds);
			} else
#endif sun
				xalloc(vp, 0);
		}
	} else {
		xalloc(vp, pagi);
		if (u.u_procp->p_textp) {
			vinifod((struct fpte *)dptopte(u.u_procp, 0), PG_FTEXT, 
			    u.u_procp->p_textp->x_vptr, 
#ifdef vax
			    (daddr_t)(1 + ts / CLSIZE),
#else
			    (daddr_t)(ts / CLSIZE),
#endif
			    (size_t)(clrnd(btoc(u.u_exdata.ux_dsize))));
		}
	}

#ifdef vax
	/* THIS SHOULD BE DONE AT A LOWER LEVEL, IF AT ALL */
	mtpr(TBIA, 0);
#endif

	if (u.u_error)
		swkill(u.u_procp, "getxfile: i/o error mapping pages");

	/*
	 * set SUID/SGID protections, if no tracing
	 */
	if ((u.u_procp->p_flag&STRC)==0) {
		if (uid != u.u_uid || gid != u.u_gid)
			u.u_cred = crcopy(u.u_cred);
		u.u_uid = uid;
		u.u_procp->p_uid = uid;		/* XXX - p_uid should be real UID */
		u.u_procp->p_suid = uid;
		u.u_gid = gid;
	} else
		psignal(u.u_procp, SIGTRAP);
	u.u_tsize = ts;
	u.u_dsize = ds;
	u.u_ssize = ss;
bad:
	if (tdmap)
		kmem_free((caddr_t)tdmap, 2 * sizeof (struct dmap));
}
