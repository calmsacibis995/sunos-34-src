#ifndef lint
static	char sccsid[] = "@(#)mem.c 1.5 87/03/02";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
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
#include "../machine/eeprom.h"

#define	M_MEM		0	/* /dev/mem - physical main memory */
#define	M_KMEM		1	/* /dev/kmem - virtual kernel memory & I/O */
#define	M_NULL		2	/* /dev/null - EOF & Rathole */
#define	M_MBMEM		3	/* /dev/mbmem - (not supported) */
#define	M_MBIO		4	/* /dev/mbio - (not supported) */
#define M_VME16D16	5	/* /dev/vme16d16 - VME 16bit addr/16bit data */
#define M_VME24D16	6	/* /dev/vme24d16 - VME 24bit addr/16bit data */
#define M_VME32D16	7	/* /dev/vme32d16 - VME 32bit addr/16bit data */
#define M_VME16D32	8	/* /dev/vme16d32 - VME 16bit addr/32bit data */
#define M_VME24D32	9	/* /dev/vme24d32 - VME 24bit addr/32bit data */
#define M_VME32D32	10	/* /dev/vme32d32 - VME 32bit addr/32bit data */
#define M_EEPROM	11	/* /dev/eeprom - on board eeprom device */

/* transfer sizes */
#define SHORT	1
#define LONG	2

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
		/* standard devices */
		break;

	case M_EEPROM:
		/* all Sun-3 machines must have EEPROM */
		break;

	case M_VME16D16:
	case M_VME24D16:
	case M_VME32D16:
	case M_VME16D32:
	case M_VME24D32:
	case M_VME32D32:
		/* SUN3_50 & SUN3_60 are the only Sun-3 machines w/o VMEbus */
		if (cpu == CPU_SUN3_50 || cpu == CPU_SUN3_60)
			return (EINVAL);
		break;

	case M_MBMEM:
	case M_MBIO:
	default:
		/* Unsupported or unknown type */
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
	register int o, xfersize;
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
			 * no more than the rest of the target page.
			 */
			c = min(c, (u_int)(NBPG -
				((int)iov->iov_base&PGOFSET)));
			error = uiomove((caddr_t)&vmmap[o], (int)c, rw, uio);
			/* Mapping is invalid here, page flush. */
			vac_pageflush(vmmap);
			break;

		case M_KMEM:
			c = iov->iov_len;
			if (kernacc((caddr_t)uio->uio_offset, c,
			    rw == UIO_READ ? B_READ : B_WRITE)) {
				error = uiomove((caddr_t)uio->uio_offset,
					(int)c, rw, uio);
				continue;
			}
			error = mmpeekio(uio, rw, (caddr_t)uio->uio_offset,
			    (int)c, SHORT); 
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

		case M_EEPROM:
			error = mmeeprom(uio, rw, (caddr_t)uio->uio_offset,
			    iov->iov_len);
			if (error == -1)
				return (0);		/* EOF */
			break;

		case M_VME16D16:
			if (uio->uio_offset >= VME16_SIZE)
				goto fault;
			v = uio->uio_offset + VME16_BASE;
			pgsp = PGT_VME_D16;
			xfersize = SHORT;
			goto vme;

		case M_VME16D32:
			if (uio->uio_offset >= VME16_SIZE)
				goto fault;
			v = uio->uio_offset + VME16_BASE;
			pgsp = PGT_VME_D32;
			xfersize = LONG;
			goto vme;

		case M_VME24D16:
			if (uio->uio_offset >= VME24_SIZE)
				goto fault;
			v =  uio->uio_offset + VME24_BASE;
			pgsp = PGT_VME_D16;
			xfersize = SHORT;
			goto vme;

		case M_VME24D32:
			if (uio->uio_offset >= VME24_SIZE)
				goto fault;
			v =  uio->uio_offset + VME24_BASE;
			pgsp = PGT_VME_D32;
			xfersize = LONG;
			goto vme;

		case M_VME32D16:
			pgsp = PGT_VME_D16;
			v =  uio->uio_offset;
			xfersize = SHORT;
			goto vme;

		case M_VME32D32:
			pgsp = PGT_VME_D32;
			v =  uio->uio_offset;
			xfersize = LONG;
			/* FALL THROUGH */

		vme:
			v = btop(v);

			/* Mapin for VME, no cache operation is involved. */
			mapin(mmap, btop(vmmap), pgsp | v, 1,
				rw == UIO_WRITE ? PG_V|PG_KW : PG_V|PG_KR);
			o = (int)uio->uio_offset & PGOFSET;
			c = min((u_int)(NBPG - o), (u_int)iov->iov_len);
			error = mmpeekio(uio, rw, &vmmap[o], (int)c, xfersize);
			break;

		}
	}
	return (error);
fault:
	return (EFAULT);
}

static
mmpeekio(uio, rw, addr, len, xfersize)
	struct uio *uio;
	enum uio_rw rw;
	caddr_t addr;
	int len;
	int xfersize;
{
	register int c;
	int o;
	short sh;
	long lsh;

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
			if ((xfersize == LONG) &&
			    (((int)addr % sizeof (long)) == 0) &&
			    (len % sizeof (long)) == 0) {
				c = sizeof (long);
				if (rw == UIO_READ) {
					if (peekl((long *)addr, (long *)&o) == -1)
						return (EFAULT);
					lsh = o;
				}
				if (uiomove((caddr_t)&lsh, c, rw, uio))
					return (EFAULT);
				if (rw == UIO_WRITE) {
					if (pokel((long *)addr, lsh))
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
		}
		addr += c;
		len -= c;
	}
	return (0);
}

/*
 * If eeprombusy is true, then the eeprom has just
 * been written to and cannot be read or written
 * until the required 10 MS has passed.  It is
 * assumed that the only way the EEPROM is written
 * is thru the mmeeprom routine.
 */
static int eeprombusy = 0;

static
mmeepromclear()
{

	eeprombusy = 0;
	wakeup((caddr_t)&eeprombusy);
}

static
mmeeprom(uio, rw, addr, len)
	struct uio *uio;
	enum uio_rw rw;
	caddr_t addr;
	int len;
{
	int o, oo;
	int s;

	if ((int)addr > EEPROM_SIZE)
		return (EFAULT);

	while (len > 0) {
		if ((int)addr == EEPROM_SIZE)
			return (-1);			/* EOF */

		s = splclock();
		while (eeprombusy)
			(void) sleep((caddr_t)&eeprombusy, PUSER);
		(void) splx(s);

		if (rw == UIO_WRITE) {
			if ((o = uwritec(uio)) == -1)
				return (EFAULT);
			if ((oo = peekc(EEPROM_ADDR + addr)) == -1)
				return (EFAULT);
			/*
			 * Check to make sure that the data is actually
			 * changing before committing to doing the write.
			 * This avoids the unneeded eeprom lock out
			 * and reduces the number of times the eeprom
			 * is actually written to.
			 */
			if (o != oo) {
				if (pokec(EEPROM_ADDR + addr, (char)o))
					return (EFAULT);
				/*
				 * Block out access to the eeprom for
				 * two clock ticks (longer than > 10 MS).
				 */
				eeprombusy = 1;
				timeout(mmeepromclear, (caddr_t)0, 2);
			}
		} else {
			if ((o = peekc(EEPROM_ADDR + addr)) == -1)
				return (EFAULT);
			if (ureadc((char)o, uio))
				return (EFAULT);
		}
		addr += sizeof (char);
		len -= sizeof (char);
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
			return (PGT_OBMEM | pf);
		break;

	case M_VME16D16:
		if (off >= VME16_SIZE)
			break;
		return (PGT_VME_D16 | btop(off + VME16_BASE));

	case M_VME16D32:
		if (off >= VME16_SIZE)
			break;
		return (PGT_VME_D32 | btop(off + VME16_BASE));

	case M_VME24D16:
		if (off >= VME24_SIZE)
			break;
		return (PGT_VME_D16 | btop(off + VME24_BASE));

	case M_VME24D32:
		if (off >= VME24_SIZE)
			break;
		return (PGT_VME_D32 | btop(off + VME24_BASE));

	case M_VME32D16:
		return (PGT_VME_D16 | btop(off));

	case M_VME32D32:
		return (PGT_VME_D32 | btop(off));

	}
	return (-1);
}
