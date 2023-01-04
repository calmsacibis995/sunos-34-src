#ifndef lint
static	char sccsid[] = "@(#)vm_machdep.c 1.1 86/09/25 SMI"; /* from UCB 6.1 83/07/29 */
#endif

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/cmap.h"
#include "../h/vfs.h"
#include "../ufs/mount.h"
#include "../h/vm.h"
#include "../h/text.h"

#include "../vax/mtpr.h"

/*
 * Set a red zone in the kernel stack after the u. area.
 */
setredzone(pte, vaddr)
	register struct pte *pte;
	caddr_t vaddr;
{
#ifdef notdef
	register int i;

	pte -= CLSIZE;
	if (vaddr)
		vaddr -= CLBYTES;
	for_CLSIZE(i) {
		*(int *)pte++ = 0;
		if (vaddr) {
			mtpr(TBIS, vaddr);
			vaddr += NBPG;
		}
	}
#else
	pte += (sizeof (struct user) + NBPG - 1) / NBPG;
	*(int *)pte &= ~PG_PROT;
	*(int *)pte |= PG_URKR;
	mtpr(TBIS, vaddr + sizeof (struct user));
#endif
}

#ifndef mapin
mapin(pte, v, pfnum, count, prot)
	struct pte *pte;
	u_int v, pfnum;
	int count, prot;
{

	while (count > 0) {
		*(int *)pte++ = pfnum | prot;
		mtpr(TBIS, ptob(v));
		v++;
		pfnum++;
		count--;
	}
}
#endif

#ifdef notdef
/*ARGSUSED*/
mapout(pte, size)
	register struct pte *pte;
	int size;
{

	panic("mapout");
}
#endif

/*
 * Check for valid program size
 */
chksize(ts, ds, ss)
	register unsigned ts, ds, ss;
{
	static int maxdmap = 0;

	if (ts > MAXTSIZ || ds > MAXDSIZ || ss > MAXSSIZ) {
		u.u_error = ENOMEM;
		return (1);
	}
	/* check for swap map overflow */
	if (maxdmap == 0) {
		register int i, blk;

		blk = dmmin;
		for (i = 0; i < NDMAP; i++) {
			maxdmap += blk;
			if (blk < dmmax)
				blk *= 2;
		}
	}
	if (ctod(ts) > NXDAD * dmtext ||
	    ctod(ds) > maxdmap || ctod(ss) > maxdmap) {
		u.u_error = ENOMEM;
		return (1);
	}
	/*
	 * Make sure the process isn't bigger than our
	 * virtual memory limit.
	 *
	 * THERE SHOULD BE A CONSTANT FOR THIS.
	 */
	if (ts + ds + ss + LOWPAGES + HIGHPAGES > btoc(USRSTACK)) {
		u.u_error = ENOMEM;
		return (1);
	}
	return (0);
}

/*ARGSUSED*/
newptes(pte, v, size)
	register struct pte *pte;
	u_int v;
	register int size;
{
	register caddr_t a = ptob(v);

#ifdef lint
	pte = pte;
#endif
	if (size >= 8) {
		mtpr(TBIA, 0);
		return;
	}
	while (size > 0) {
		mtpr(TBIS, a);
		a += NBPG;
		size--;
	}
}

/*
 * Change protection codes of text segment.
 * Have to flush translation buffer since this
 * affect virtual memory mapping of current process.
 */
chgprot(addr, tprot)
	caddr_t addr;
	long tprot;
{
	unsigned v;
	int tp;
	register struct pte *pte;
	register struct cmap *c;

	v = clbase(btop(addr));
	if (!isatsv(u.u_procp, v)) {
		u.u_error = EFAULT;
		return (0);
	}
	tp = vtotp(u.u_procp, v);
	pte = tptopte(u.u_procp, tp);
	if (pte->pg_fod == 0 && pte->pg_pfnum) {
		c = &cmap[pgtocm(pte->pg_pfnum)];
		if (c->c_blkno && c->c_vp != swapdev_vp)
			munhash(c->c_vp, (daddr_t)(u_long)c->c_blkno);
	}
	*(int *)pte &= ~PG_PROT;
	*(int *)pte |= tprot;
	distcl(pte);
	tbiscl(v);
	return (1);
}

settprot(tprot)
	long tprot;
{
	register int *ptaddr, i;

	ptaddr = (int *)mfpr(P0BR);
	for (i = 0; i < u.u_tsize; i++) {
		ptaddr[i] &= ~PG_PROT;
		ptaddr[i] |= tprot;
	}
	mtpr(TBIA, 0);
}

/*
 * Rest are machine-dependent
 */

getmemc(addr)
	caddr_t addr;
{
	register int c;
	struct pte savemap;

	savemap = mmap[0];
	*(int *)mmap = PG_V | PG_KR | btop(addr);
	mtpr(TBIS, vmmap);
	c = *(char *)&vmmap[(int)addr & PGOFSET];
	mmap[0] = savemap;
	mtpr(TBIS, vmmap);
	return (c & 0377);
}

putmemc(addr, val)
	caddr_t addr;
{
	struct pte savemap;

	savemap = mmap[0];
	*(int *)mmap = PG_V | PG_KW | btop(addr);
	mtpr(TBIS, vmmap);
	*(char *)&vmmap[(int)addr & PGOFSET] = val;
	mmap[0] = savemap;
	mtpr(TBIS, vmmap);
}

/*
 * Move pages from one kernel virtual address to another.
 * Both addresses are assumed to reside in the Sysmap,
 * and size must be a multiple of CLSIZE.
 */
pagemove(from, to, size)
	register caddr_t from, to;
	int size;
{
	register struct pte *fpte, *tpte;

	if (size % CLBYTES)
		panic("pagemove");
	fpte = &Sysmap[btop(from - KERNELBASE)];
	tpte = &Sysmap[btop(to - KERNELBASE)];
	while (size > 0) {
		*tpte++ = *fpte;
		*(int *)fpte++ = 0;
		mtpr(TBIS, from);
		mtpr(TBIS, to);
		from += NBPG;
		to += NBPG;
		size -= NBPG;
	}
}

/*
 * Routine used to check to see if an a.out can be executed
 * by the current machine/architecture.
 */
chkaout()
{

	return (0);
}

/* 
 * The following routines return information about an a.out
 * which is used when a program is executed.
 */

/* 
 * Return the size of the text segment adjusted for the type of a.out.
 * An old a.out will eventually become OMAGIC so it has no text size.
 */
size_t
getts()
{

	return (clrnd(btoc(u.u_exdata.ux_tsize)));
}

/* 
 * Return the size of the data segment depending on the type of a.out.
 * For the case of an old a.out we need to allow for the old segment
 * alignment and the fact that the text segment starts at 32k and not 8k.
 * To do this we calculate the size of the text segment and round
 * it to the next old Sun-2 segment boundary.  The size of the data 
 * and bss segment are then added.
 */
size_t
getds()
{

	return (clrnd(btoc(u.u_exdata.ux_dsize + u.u_exdata.ux_bsize)));
}

/* 
 * Return the load memory address for the data segment.
 */
caddr_t
getdmem()
{

	return ((caddr_t)ctob(dptov(u.u_procp, 0)));
}

/* 
 * Return the starting disk address for the data segment.
 */
getdfile()
{

	if (u.u_exdata.ux_mag == ZMAGIC)
		return (CLBYTES + u.u_exdata.ux_tsize);
	else
		return (sizeof (u.u_exdata) + u.u_exdata.ux_tsize);
}

/* 
 * Return the load memory address for the text segment.
 */
caddr_t
gettmem()
{

	return ((caddr_t)USRTEXT);
}

/* 
 * Return the file byte offset for the text segment.
 */
gettfile()
{

	if (u.u_exdata.ux_mag == ZMAGIC)
		return (CLBYTES);
	else
		return (sizeof (u.u_exdata));
}
