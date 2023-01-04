/*	@(#)kern_mman.c 1.1 86/09/25 SMI; from UCB 1.12 83/07/01	*/

#include "../machine/reg.h"
#include "../machine/psl.h"
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/vnode.h"
#include "../h/seg.h"
#include "../h/acct.h"
#include "../h/wait.h"
#include "../h/vm.h"
#include "../h/text.h"
#include "../h/file.h"
#include "../h/vadvise.h"
#include "../h/cmap.h"
#include "../h/trace.h"
#include "../h/mman.h"
#include "../h/conf.h"

sbrk()
{

}

sstk()
{

}

getpagesize()
{

	u.u_r.r_val1 = NBPG * CLSIZE;
}

int	minhole = 128*1024;	/* swap hole must be at least this big */

smmap()
{
#ifdef sun
	struct a {
		caddr_t	addr;
		int	len;
		int	prot;
		int	share;
		int	fd;
		off_t	pos;
	} *uap = (struct a *)u.u_ap;
	struct file *fp;
	register struct vnode *vp;
	register struct fpte *pte;
	int off;
	unsigned fv, lv, pm;
	dev_t dev;
	int (*mapfun)();
	int hole = 0;
	int dss;
	swblk_t blk;

	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error)
		return;
	vp = (struct vnode *)fp->f_data;
	if (vp->v_type != VCHR) {
		u.u_error = EINVAL;
		return;
	}
	dev = vp->v_rdev;
	mapfun = cdevsw[major(dev)].d_mmap;
	if (mapfun == NULL) {
		u.u_error = EINVAL;
		return;
	}
	if (((int)uap->addr & CLOFSET) || (uap->len & CLOFSET) ||
	    (uap->pos & CLOFSET)) {
		u.u_error = EINVAL;
		return;
	}
	if ((uap->prot & PROT_WRITE) && (fp->f_flag&FWRITE) == 0) {
		u.u_error = EINVAL;
		return;
	}
	if ((uap->prot & PROT_READ) && (fp->f_flag&FREAD) == 0) {
		u.u_error = EINVAL;
		return;
	}
	if (uap->share != MAP_SHARED) {
		u.u_error = EINVAL;
		return;
	}
	fv = btop(uap->addr);
	lv = btop(uap->addr + uap->len - 1);
	dss = dptov(u.u_procp, 0);
	if (lv < fv || !(fv >= dss && lv < dss + u.u_procp->p_dsize)) {
		u.u_error = EINVAL;
		return;
	}
	for (off = 0; off < uap->len; off += NBPG) {
		if ((*mapfun)(dev, uap->pos+off, uap->prot) == -1) {
			u.u_error = EINVAL;
			return;
		}
	}
	/*
	 * If the size of the new hole is large enough and there
	 * isn't already a hole, mark this as the hole.
	 */
	if (lv - fv + 1 >= btop(minhole) && u.u_hole.uh_last == 0) {
		u.u_hole.uh_first = vtodp(u.u_procp, fv);
		u.u_hole.uh_last = vtodp(u.u_procp, lv);
		hole = 1;
	}
	if (uap->prot & PROT_WRITE)
		pm = PG_UW;
	else
		pm = PG_URKR;

	pm |= PG_V | PG_FOD;		/* mark as an mmap'ed page */
#ifdef sun3
	pm |= PG_NC;			/* muptiple mapping, turn off cache */
#endif

	vac_flush(uap->addr, uap->len); /* in-mmu mapping is changed */
	for (off = 0; off < uap->len; off += NBPG) {
		pte = (struct fpte *)vtopte(u.u_procp, fv);
		u.u_procp->p_rssize -= vmemfree((struct pte *)pte, 1);
		/*
		 * Call map function to set page frame number and page
		 * type for the pte, or'ing in the access permissions
		 * and finally filling in the fileno for the (f)pte.
		 * This stuff relies heavily on the pte/fpte relationship.
		 */
		*(int *)pte =
		    ((*mapfun)(dev, uap->pos+off, uap->prot) & PG_PFNUM) | pm;
		pte->pg_fileno = uap->fd;
		/* free the swap space for pages in the hole */
		if (hole) {
			blk = vtod(u.u_procp, fv, &u.u_dmap, &u.u_smap);
			rmfree(swapmap, (long)ctod(1), blk);
		}
		fv++;
	}
	fv = btop(uap->addr);
	newptes(vtopte(u.u_procp, fv), fv, (int)btoc(uap->len));
	u.u_pofile[uap->fd] |= UF_MAPPED;
#endif
}

mremap()
{

}

munmap()
{
#ifdef sun
	register struct a {
		caddr_t	addr;
		int	len;
	} *uap = (struct a *)u.u_ap;
	int off;
	unsigned fv, lv;
	register struct pte *pte;

	if (((int)uap->addr & CLOFSET) || (uap->len & CLOFSET)) {
		u.u_error = EINVAL;
		return;
	}
	fv = btop(uap->addr);
	lv = btop(uap->addr + uap->len - 1);
	if (lv < fv || !isadsv(u.u_procp, fv) || !isadsv(u.u_procp, lv)) {
		u.u_error = EINVAL;
		return;
	}
	vac_flush(uap->addr, uap->len); /* in-mmu mapping is changed */
	for (off = 0; off < uap->len; off += NBPG) {
		pte = vtopte(u.u_procp, fv);
		u.u_procp->p_rssize -= vmemfree(pte, 1);
		/*
		 * If we unmap a page in the hole the page must become
		 * invalid, not zero fill on demand, because we have
		 * no swap space to back it up.
		 */
		if (vtodp(u.u_procp, fv) >= u.u_hole.uh_first &&
		    vtodp(u.u_procp, fv) <= u.u_hole.uh_last) {
			*(int *)pte = 0;
		} else {
			*(int *)pte = (PG_UW|PG_FOD);
			((struct fpte *)pte)->pg_fileno = PG_FZERO;
		}
		fv++;
	}
	fv = btop(uap->addr);
	newptes(vtopte(u.u_procp, fv), (u_int)fv, (int)btoc(uap->len));
#endif
}

munmapfd(fd)
{
#ifdef sun
	register struct fpte *pte;
	register int i;

	/*
	 * If in vfork, don't modify parent's address space.
	 */
	if (u.u_procp->p_flag&SVFORK)
		return;
	for (i = 0; i < u.u_dsize; i++) {
		pte = (struct fpte *)dptopte(u.u_procp, i);
		if (pte->pg_v && pte->pg_fod && pte->pg_fileno == fd) {
			/*
			 * If we unmap a page in the hole the page must become
			 * invalid, not zero fill on demand, because we have
			 * no swap space to back it up.
			 */
			if (i >= u.u_hole.uh_first && i <= u.u_hole.uh_last) {
				*(int *)pte = 0;
			} else {
				*(int *)pte = (PG_UW|PG_FOD);
				((struct fpte *)pte)->pg_fileno = PG_FZERO;
			}
		}
	}
	vac_flush(ptob(dptov(u.u_procp, 0)), NBPG * u.u_dsize);
	newptes(dptopte(u.u_procp, 0), dptov(u.u_procp, 0), u.u_dsize);
#endif
	u.u_pofile[fd] &= ~UF_MAPPED;
	
}

mprotect()
{

}

madvise()
{

}

mincore()
{

}

/* BEGIN DEFUNCT */
obreak()
{
	struct a {
		char	*nsiz;
	};
	register int n, d;

	/*
	 * set n to new data size
	 * set d to new-old
	 */

	n = btoc(((struct a *)u.u_ap)->nsiz) - dptov(u.u_procp, 0);
	if (n < 0)
		n = 0;
	d = clrnd(n - u.u_dsize);
	if (ctob(u.u_dsize+d) > u.u_rlimit[RLIMIT_DATA].rlim_cur) {
		u.u_error = ENOMEM;
		return;
	}
	if (chksize((u_int)u.u_tsize, (u_int)u.u_dsize+d, (u_int)u.u_ssize))
		return;
	if (swpexpand(u.u_dsize+d, u.u_ssize, &u.u_dmap, &u.u_smap)==0)
		return;
	expand(d, 0);
}

int	both;

ovadvise()
{
	register struct a {
		int	anom;
	} *uap;
	register struct proc *rp = u.u_procp;
	int oanom = rp->p_flag & SUANOM;
	register struct pte *pte;
	register struct cmap *c;
	register int i;

#ifdef lint
	both = 0;
#endif
	uap = (struct a *)u.u_ap;
	trace(TR_VADVISE, uap->anom, u.u_procp->p_pid);
	rp->p_flag &= ~(SSEQL|SUANOM);
	switch (uap->anom) {

	case VA_ANOM:
		rp->p_flag |= SUANOM;
		break;

	case VA_SEQL:
		rp->p_flag |= SSEQL;
		break;
	}
	if ((oanom && (rp->p_flag & SUANOM) == 0) || uap->anom == VA_FLUSH) {
		for (i = 0; i < rp->p_dsize; i += CLSIZE) {
			pte = dptopte(rp, i);
			if (pte->pg_v && !pte->pg_fod) {
				c = &cmap[pgtocm(pte->pg_pfnum)];
				if (c->c_lock)
					continue;
#ifndef vax
				ptesync(u.u_procp, dptov(u.u_procp, i));
#endif
				pte->pg_v = 0;
				if (anycl(pte, pg_m))
					pte->pg_m = 1;
				distcl(pte);
				/* This page becomes invalid. */
				vac_pageflush(ptob(dptov(u.u_procp, i))); 
			}
		}
	}
	if (uap->anom == VA_FLUSH) {	/* invalidate all pages */
		for (i = 1; i < rp->p_ssize; i += CLSIZE) {
			pte = sptopte(rp, i);
			if (pte->pg_v && !pte->pg_fod) {
				c = &cmap[pgtocm(pte->pg_pfnum)];
				if (c->c_lock)
					continue;
#ifndef vax
				ptesync(u.u_procp, sptov(u.u_procp, i));
#endif
				pte->pg_v = 0;
				if (anycl(pte, pg_m))
					pte->pg_m = 1;
				distcl(pte);
			}
		}
		for (i = 0; i < rp->p_tsize; i += CLSIZE) {
			pte = tptopte(rp, i);
			if (pte->pg_v && !pte->pg_fod) {
				c = &cmap[pgtocm(pte->pg_pfnum)];
				if (c->c_lock)
					continue;
#ifndef vax
				ptesync(u.u_procp, tptov(u.u_procp, i));
#endif
				pte->pg_v = 0;
				if (anycl(pte, pg_m))
					pte->pg_m = 1;
				distcl(pte);
				distpte(rp->p_textp, i, pte);
			}
		}
		/* All pages are invalid, context flush is more efficient */
		vac_ctxflush();
	}
#ifdef vax
#include "../vax/mtpr.h"		/* XXX */
	mtpr(TBIA, 0);
#endif
#ifdef sun
#include "../machine/mmu.h"		/* XXX */
	setusercontext(KCONTEXT);
	u.u_procp->p_flag |= SPTECHG;
#endif
}
/* END DEFUNCT */

/*
 * grow the stack to include the SP
 * true return if successful.
 */
grow(sp)
	unsigned sp;
{
	register int si;

	if (sp >= USRSTACK-ctob(u.u_ssize))
		return (0);
	si = clrnd(btoc((USRSTACK-sp)) - u.u_ssize + SINCR);
	if (ctob(u.u_ssize+si) > u.u_rlimit[RLIMIT_STACK].rlim_cur)
		return (0);
	if (chksize((u_int)u.u_tsize, (u_int)u.u_dsize, (u_int)u.u_ssize+si))
		return (0);
	if (swpexpand(u.u_dsize, u.u_ssize+si, &u.u_dmap, &u.u_smap)==0)
		return (0);
	
	expand(si, 1);
	return (1);
}
