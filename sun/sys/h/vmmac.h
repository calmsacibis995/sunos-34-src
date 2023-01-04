/*	@(#)vmmac.h 1.1 86/09/25 SMI; from UCB 4.7 83/07/01	*/

/*
 * Virtual memory related conversion macros
 */

/* Core clicks to number of pages of page tables needed to map that much */
#define	ctopt(x)	(((x)+NPTEPG-1)/NPTEPG)

/* Virtual page numbers to text|data|stack segment page numbers and back */
#define	vtotp(p, v)	((int)(v) - LOWPAGES)
#define	vtodp(p, v) \
	((int)((v) - ((p)->p_tsize ? \
	    stoc(ctos((p)->p_tsize + LOWPAGES)) : LOWPAGES)))
#define	vtosp(p, v)	((int)(btop(USRSTACK) - 1 - (v)))
#define	tptov(p, i)	((unsigned)(i) + LOWPAGES)
#define	dptov(p, i) \
	((unsigned)((p)->p_tsize ? \
	    (stoc(ctos((p)->p_tsize + LOWPAGES)) + (i)) : (LOWPAGES + (i))))
#define	sptov(p, i)	((unsigned)(btop(USRSTACK) - 1 - (i)))

/* Tell whether virtual page numbers are in text|data|stack segment */
#define	isassv(p, v)	((v) >= (btop(USRSTACK) - (p)->p_ssize))
#define	isatsv(p, v)	(((v) - LOWPAGES) < (p)->p_tsize)
#define isadsv(p, v) \
	((v) >= ((p)->p_tsize ? \
	    stoc(ctos((p)->p_tsize + LOWPAGES)) : LOWPAGES) && !isassv(p, v))

/* Tell whether pte's are text|data|stack */
#define	isaspte(p, pte)		((pte) > sptopte(p, (p)->p_ssize))
#define	isatpte(p, pte)		((pte) < dptopte(p, 0))
#define	isadpte(p, pte)		(!isaspte(p, pte) && !isatpte(p, pte))

/* Text|data|stack pte's to segment page numbers and back */
#define	ptetotp(p, pte)		((pte) - (p)->p_p0br)
#define	ptetodp(p, pte)		(((pte) - (p)->p_p0br) - (p)->p_tsize)
#define	ptetosp(p, pte)		((p)->p_p1br + P1PAGES - HIGHPAGES - 1 - (pte))

#define	tptopte(p, i)		((p)->p_p0br + (i))
#define	dptopte(p, i)		((p)->p_p0br + ((p)->p_tsize + (i)))
#define	sptopte(p, i)		((p)->p_p1br + P1PAGES - HIGHPAGES - 1 - (i))

/* Bytes to pages without rounding, and back */
#define	btop(x)		(((unsigned)(x)) >> PGSHIFT)
#define	ptob(x)		((caddr_t)((x) << PGSHIFT))

/*
 * Turn virtual addresses into kernel map indices.  Note that some
 * trickery involving types and pointer conversions is employed, as
 * well as misnomers and funny full unions; to whit:
 *
 * "Usrptmap" is an array of page table entries used to map virtual
 * addresses, starting at (kernel virtual address) usrpt, to many
 * different things (the funny full union).  On the VAX, this mapped
 * (kernel virtual) space is where the user page tables were allocated,
 * hence the name.  We at Sun use this virtual address space for all
 * sorts of things, including addresses to reference peripherals (a
 * misnomer).  Usrptmap is managed throught the resource map named
 * "kernelmap".  kmx means kernelmap index, the index (into Usrptmap)
 * returned by rmalloc(kernelmap, ...).
 *
 * kmxtob expects an (integer) kernel map index and returns the virtual
 * address (through magic constants and implicit conversions) that is
 * mapped by that pte.  But the result is typed as a pte.  (This is
 * made sense on the VAX.)  btokmx expects a (struct pte *) virtual
 * address and returns the integer kernel map index.
 *
 * Recasts are often needed and used when invoking these macros.
 */
#define	kmxtob(a)	(usrpt + (a) * NPTEPG)
#define	btokmx(b)	(((b) - usrpt) / NPTEPG)

/* User area address and pcb bases */
#define	uaddr(p)	(&((p)->p_p0br[(p)->p_szpt * NPTEPG - UPAGES]))
#define	pcbb(p)		(p)

/* Average new into old with aging factor time */
#define	ave(smooth, cnt, time) \
	smooth = ((time - 1) * (smooth) + (cnt)) / (time)

/* Abstract machine dependent operations */
#ifdef vax
#define	setp0br(x)	(u.u_pcb.pcb_p0br = (x), mtpr(P0BR, x))
#define	setp0lr(x)	(u.u_pcb.pcb_p0lr = \
			    (x) | (u.u_pcb.pcb_p0lr & AST_CLR), \
			 mtpr(P0LR, x))
#define	setp1br(x)	(u.u_pcb.pcb_p1br = (x), mtpr(P1BR, x))
#define	setp1lr(x)	(u.u_pcb.pcb_p1lr = (x), mtpr(P1LR, x))
#define	initp1br(x)	((x) - P1PAGES)
#endif
#ifdef sun
#define	setp0br(x)	u.u_pcb.pcb_p0br = (x)
#define	setp0lr(x)	(u.u_pcb.pcb_p0lr = (x) | (u.u_pcb.pcb_p0lr & AST_CLR))
#define	setp1br(x)	u.u_pcb.pcb_p1br = (x)
#define	setp1lr(x)	u.u_pcb.pcb_p1lr = (x)
#define	initp1br(x)	((x) - P1PAGES - UPAGES)
#endif

#define	outofmem()	wakeup((caddr_t)&proc[2]);

/*
 * Page clustering macros.
 * 
 * dirtycl(pte)			is the page cluster dirty?
 * anycl(pte,fld)		does any pte in the cluster has fld set?
 * zapcl(pte,fld) = val		set all fields fld in the cluster to val
 * distcl(pte)			distribute high bits to cluster; note that
 *				distcl copies everything but pg_pfnum,
 *				INCLUDING pg_m!!!
 *
 * In all cases, pte must be the low pte in the cluster, even if
 * the segment grows backwards (e.g. the stack).
 */
#define	H(pte)	((struct hpte *)(pte))

#if CLSIZE==1
#define	dirtycl(pte)	dirty(pte)
#define	anycl(pte,fld)	((pte)->fld)
#define	zapcl(pte,fld)	(pte)->fld
#define	distcl(pte)
#endif

#if CLSIZE==2
#define	dirtycl(pte)	(dirty(pte) || dirty((pte)+1))
#define	anycl(pte,fld)	((pte)->fld || (((pte)+1)->fld))
#define	zapcl(pte,fld)	(pte)[1].fld = (pte)[0].fld
#endif

#if CLSIZE==4
#define	dirtycl(pte) \
    (dirty(pte) || dirty((pte)+1) || dirty((pte)+2) || dirty((pte)+3))
#define	anycl(pte,fld) \
    ((pte)->fld || (((pte)+1)->fld) || (((pte)+2)->fld) || (((pte)+3)->fld))
#define	zapcl(pte,fld) \
    (pte)[3].fld = (pte)[2].fld = (pte)[1].fld = (pte)[0].fld
#endif

#ifndef distcl
#define	distcl(pte)	zapcl(H(pte),pg_high)
#endif
