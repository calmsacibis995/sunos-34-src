#include <sys/types.h>
#include <machdep.h>

static char	sccsid[] = "@(#)machdep.c 1.1 9/25/86 Copyright Sun Micro";
/*
 *	these are the routines in machdep
 *	addr is usually a u_long
 *	entry is a ??_size
 *	routines for mapping and registers return ??_size
 *	most of the set* routines return what was there before
 *	%'ed routines return nothing
		cx_size		getcxreg()
		cx_size		setcxreg(entry)
		sm_size		getsmreg(addr)
		sm_size		setsmreg(addr,entry)
		pg_size		getpgreg(addr)
		pg_size		setpgreg(addr,entry)
		id_size		getidprom(addr)
		led_size	setledreg(entry)
		berr_size	getberrreg()
		enable_size	getenablereg()
		enable_size	setenablereg(entry)
		int  %  	map(virt, size, phys, space)
		int  %		parity(gen,check)
 *
 */
/*
 * get/set fc3 are to eliminate the assembly all over the place,
 * and to let users do their own self abuse
 */
u_long
getfc3(size,addr)
register u_long			size, *addr;
{
	MOVL(#3, d0);
	MOVC(d0, sfc);
	if (size == sizeof(u_char))
		MOVSB(a5@, d7);
	else if (size == sizeof(u_short))
		MOVSW(a5@, d7);
	else if (size == sizeof(u_long))
		MOVSL(a5@, d7);
	return(size);
}

u_long
setfc3(size,addr,entry)
register u_long			size, *addr,entry;
{
	MOVL(#3, d0);
	MOVC(d0, dfc);
	if (size == sizeof(u_char))
		MOVSB(d6, a5@);
	else if (size == sizeof(u_short))
		MOVSW(d6, a5@);
	else if (size == sizeof(u_long))
		MOVSL(d6, a5@);
}
cx_size
getcxreg()
{
	return((cx_size) getfc3(sizeof(cx_size), CX_OFF));
}
cx_size
setcxreg(entry)
register cx_size		entry;
{
	register cx_size	ret = getcxreg();

	setfc3(sizeof(cx_size), CX_OFF, entry);
	return(ret);
}

sm_size
getsmreg(addr)
register u_long			addr;
{
	addr = ((addr & ~SEGMASK) + SM_OFF) & ADDRMASK;
	return((sm_size) getfc3(sizeof(sm_size), addr));
}

sm_size
setsmreg(addr,entry)
register u_long			addr;
register sm_size		entry;
{
	register sm_size	ret = getsmreg(addr);

	addr = ((addr & ~SEGMASK) + SM_OFF) & ADDRMASK;
	setfc3(sizeof(sm_size), addr, entry);
	return(ret);
}

pg_size
getpgreg(addr)
register u_long			addr;
{
	addr = ((addr & ~PAGEMASK) + PG_OFF) & ADDRMASK;
	return((pg_size) getfc3(sizeof(pg_size), addr));
}

pg_size
setpgreg(addr,entry)
register u_long			addr;
register pg_size		entry;
{
	register pg_size	ret = getpgreg(addr);

	addr = ((addr & ~PAGEMASK) + PG_OFF) & ADDRMASK;
	setfc3(sizeof(pg_size), addr, entry);
	return(ret);
}

map(virt, size, phys, space)
u_long				virt, size, phys;
enum pm_type			space;
{
	pg_t				page;
	register struct pg_field	*pgp = &page.pg_field;
	register			i;

	pgp->pg_valid = 1;
	pgp->pg_permission = PMP_ALL;
	pgp->pg_space = space;

	phys = BTOP(phys);
	size = BTOP(size);

	for (i = 0; i < size; i++){		/* for each page, */
		pgp->pg_pagenum = phys++;
		setpgreg(virt + PTOB(i), page.pg_whole);
	}
}

id_size
getidprom(addr)
register u_long			addr;
{
	return((id_size)getfc3(sizeof(id_size), (PAGESIZE * addr) + ID_OFF));
}

led_size
setledreg(entry)
led_size	entry;
{
	setfc3(sizeof(led_size), LED_OFF, ~entry);
	return(0);
}

berr_size
getberrreg()
{
	return((berr_size) getfc3(sizeof(berr_size), BERR_OFF));
}

enable_size
getenablereg()
{
	return((enable_size) getfc3(sizeof(enable_size), ENABLE_OFF));
}

enable_size
setenablereg(entry)
register enable_size		entry;
{
	register enable_size	ret = getenablereg();

	setfc3(sizeof(enable_size), ENABLE_OFF, entry);
	return(ret);
}

parity(gen, check)
register u_long			gen,check;
{
	enable_t		ereg;
	struct enable_field	*ep = &ereg.enable_field;

	ereg.enable_whole = getenablereg();

	ep->enable_pargen = gen;
	ep->enable_parcheck = check;
	setenablereg(ereg.enable_whole);
}
