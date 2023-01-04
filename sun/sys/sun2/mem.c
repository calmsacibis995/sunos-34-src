#ifndef lint
static	char sccsid[] = "@(#)mem.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Memory special file
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/vm.h"
#include "../h/cmap.h"
#include "../h/uio.h"

#include "../machine/pte.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"

#define	M_MEM	0	/* /dev/mem - physical main memory */
#define	M_KMEM	1	/* /dev/kmem - virtual kernel memory & I/O */
#define	M_NULL	2	/* /dev/null - EOF & Rathole */
#define	M_MBMEM	3	/* /dev/mbmem - Multibus memory space (1 Meg) */
#define	M_MBIO	4	/* /dev/mbio - Multibus I/O space (64K) */
#define M_VME16	5	/* /dev/vme16 - VME 16 bit addr space */
#define M_VME24	6	/* /dev/vme24 - VME 24 bit addr space */
#define M_VME32	7	/* /dev/vme32 - VME 32 bit addr space */

#define	VME_16		0xff0000

/*
 * Check bus type memory spaces for accessibility on this machine
 */
mmopen(dev) 
	dev_t dev;
{
	switch (minor(dev)) {
	case M_MEM:
	case M_KMEM:
	case M_NULL:
		break;

	case M_MBMEM:
	case M_MBIO:
		if (cpu != CPU_SUN2_120)
			return (EINVAL);
		break;

	case M_VME16:
	case M_VME24:
		if (cpu != CPU_SUN2_50)
			return (EINVAL);
		break;

	case M_VME32:
		/* not yet */
	default:
		return (EINVAL);
	}
	return (0);
}

mmread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (mmrw(dev, uio, UIO_READ));
}

mmwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (mmrw(dev, uio, UIO_WRITE));
}

mmrw(dev, uio, rw)
	dev_t dev;
	struct uio *uio;
	enum uio_rw rw;
{
	register int o;
	register u_int c, v;
	register struct iovec *iov;
	int error = 0;
	int pgsp;


	while (uio->uio_resid > 0 && error == 0) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("mmrw");
			continue;
		}
		switch (minor(dev)) {

		case M_MEM:
			v = btop(uio->uio_offset);
			if (v >= physmem)
				goto fault;
			mapin(mmap, btop(vmmap), v, 1, PG_V |
				(rw == UIO_READ ? PG_KR : PG_KW));
			o = (int)uio->uio_offset & PGOFSET;
			c = min((u_int)(NBPG - o), (u_int)iov->iov_len);
			/*
			 * I don't know why we restrict ourselves to
			 * no more than the  rest of the target page.
			 */
			c = min(c, (u_int)(NBPG -
				((int)iov->iov_base&PGOFSET)));
			error = uiomove((caddr_t)&vmmap[o], (int)c, rw, uio);
			break;

		case M_KMEM:
			c = iov->iov_len;
			if (kernacc((caddr_t)uio->uio_offset, c,
			    rw == UIO_READ ? B_READ : B_WRITE)) {
				error = uiomove((caddr_t)uio->uio_offset,
					(int)c, rw, uio);
				continue;
			}
			error = mmpeekio(uio, rw,
				    (caddr_t)uio->uio_offset, (int)c); 
			break;

		case M_NULL:
			if (rw == UIO_READ)
				return (0);
			c = iov->iov_len;
			iov->iov_base += c;
			iov->iov_len -= c;
			uio->uio_offset += c;
			uio->uio_resid -= c;
			break;

		case M_MBMEM:
			if (uio->uio_offset >= MBMEM_SIZE)
				goto fault;
			pgsp = PGT_MBMEM;
			v = btop(uio->uio_offset);
			goto mb;
 
		case M_MBIO:
			if (uio->uio_offset >= MBIO_SIZE)
				goto fault;
			pgsp = PGT_MBIO;
			v = btop(uio->uio_offset);
			goto mb;

		case M_VME16:
			if (uio->uio_offset >= 1 << 16)
				goto fault;
			v = uio->uio_offset + VME_16;
			goto vme;

		case M_VME24:
			if (uio->uio_offset >= 1 << 24)
				goto fault;
			v =  uio->uio_offset;
			/* FALL THROUGH */

		vme:
			if (v < VME0_SIZE)
				pgsp = PGT_VME0;
			else {
				pgsp = PGT_VME8;
				v -= VME0_SIZE;
			}
			v = btop(v);
			/* FALL THROUGH */

		mb:
			mapin(mmap, btop(vmmap), pgsp | v, 1,
				rw == UIO_WRITE ? PG_V|PG_KW : PG_V|PG_KR);
			o = (int)uio->uio_offset & PGOFSET;
			c = min((u_int)(NBPG - o), (u_int)iov->iov_len);
			error = mmpeekio(uio, rw, &vmmap[o], (int)c);
			break;

		}
	}
	return (error);
fault:
	return (EFAULT);
}

mmpeekio(uio, rw, addr, len)
	struct uio *uio;
	enum uio_rw rw;
	caddr_t addr;
	int len;
{
	register int c, o;
	short sh;

	while (len > 0) {
		if ((len|(int)addr) & 1) {
			c = sizeof (char);
			if (rw == UIO_WRITE) {
				if ((o = uwritec(uio)) == -1)
					return (EFAULT);
				if (pokec(addr, (char)o))
					return (EFAULT);
			} else {
				if ((o = peekc(addr)) == -1)
					return (EFAULT);
				if (ureadc((char)o, uio))
					return (EFAULT);
			}
		} else {
			c = sizeof (short);
			if (rw == UIO_READ) {
				if ((o = peek((short *)addr)) == -1)
					return (EFAULT);
				sh = o;
			}
			if (uiomove((caddr_t)&sh, c, rw, uio))
				return (EFAULT);
			if (rw == UIO_WRITE) {
				if (poke((short *)addr, sh))
					return (EFAULT);
			}
		}
		addr += c;
		len -= c;
	}
	return (0);
}

/*ARGSUSED*/
mmmmap(dev, off, prot)
	dev_t dev;
	off_t off;
{
	int pf;

	switch (minor(dev)) {

	case M_MEM:
		pf = btop(off);
		if (pf < physmem)
			return (pf);
		break;

	case M_MBMEM:
		pf = btop(off);
		if (pf < btop(MBMEM_SIZE))
			return (PGT_MBMEM | pf);
		break;

	case M_MBIO:
		pf = btop(off);
		if (pf < btop(MBIO_SIZE))
			return (PGT_MBIO | pf);
		break;

	case M_VME16:
		if (off >= 1 << 16)
			break;
		off += VME_16;
		/* fall through */

	case M_VME24:
		if (off >= 1 << 24)
			break;
		pf = btop(off);
		if (pf < btop(VME0_SIZE))
			return (PGT_VME0 | pf);
		if (pf < btop(VME0_SIZE)<<1)
			return (PGT_VME8 | pf);
		break;

	}
	return (-1);
}
