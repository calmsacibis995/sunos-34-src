/*	standalloc.c	1.1	86/09/25	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * standalloc.c
 *
 * ROM Monitor's routines for allocating resources needed on a temporary
 * basis (eg, for initialization or for boot drivers).
 *
 * Note, all requests are rounded up to fill a page.  This is not a
 * malloc() replacement!
 */

/* This flag causes printfs */
#undef PD

#include "../mon/cpu.map.h"
#include "../mon/cpu.addrs.h"
#include "saio.h"

/*
 * Artifice so standalone code uses same variable names as monitor's
 * for debugging.  FIXME?  Or leave this way?
 */
struct globram {
	char *g_nextrawvirt;
	char *g_nextdmaaddr;
	struct pgmapent g_nextmainmap;
} gp[1];


/*
 * Valid, supervisor-only, memory page's map entry.
 * (To be copied to a map entry and then modified.)
 */
struct pgmapent mainmapinit = 
	{1, PMP_SUP, VPM_MEMORY, 0, 0, 0};


/*
 * Say Something Here FIXME
 */
char *
resalloc(type, bytes)
	enum RESOURCES type;
	register unsigned bytes;
{
	register char *	addr;	/* Allocated address */
	register char *	raddr;	/* Running addr in loop */

	/* Initialize if needed. */
	if (gp->g_nextrawvirt == 0) {
#ifdef SUN2
		reset_alloc(0x100000);
#endif
#ifdef SUN3
		reset_alloc(*romp->v_memoryavail);
#endif
	}

#ifdef PD
printf("resalloc(%x, %x) %x %x ", type, bytes,
	gp->g_nextrawvirt, gp->g_nextdmaaddr);
#endif PD
	if (bytes == 0)
		return (char *)0;

	bytes = (bytes + (BYTESPERPG - 1)) & ~(BYTESPERPG - 1);

	switch (type) {

	case RES_RAWVIRT:
		addr = gp->g_nextrawvirt;
		gp->g_nextrawvirt += bytes;
		return addr;

	case RES_DMAVIRT:
		addr = gp->g_nextdmaaddr;
		gp->g_nextdmaaddr += bytes;
		return addr;

	case RES_MAINMEM:
		addr = gp->g_nextrawvirt;
		gp->g_nextrawvirt += bytes;
		break;

	case RES_DMAMEM:
		addr = gp->g_nextdmaaddr;
		gp->g_nextdmaaddr += bytes;
		break;

	default:
		return (char *)0;
	}
	
	/*
	 * Now map in main memory.
	 * Note that this loop goes backwards!!
	 */
#ifdef PD
printf("mapping to %x returning %x\n", gp->g_nextmainmap, addr);
#endif
	for (raddr = addr;
	     bytes > 0;
	     raddr += BYTESPERPG, bytes -= BYTESPERPG,
	      gp->g_nextmainmap.pm_page -= 1) {
		setpgmap(raddr, gp->g_nextmainmap);
		bzero((caddr_t)raddr, BYTESPERPG);
	} 

	return addr;
}

#ifdef SUN2
struct pgmapent devmaps[] = {
/* MAINMEM */
	{1, PMP_SUP, MPM_MEMORY, 0, 0, 0},		
/* OBIO */
	{1, PMP_SUP, MPM_IO, 0, 0, 0},			
/* MBMEM */
	{1, PMP_SUP, VPM_VME0, 0, 0, 0xFF000000 >> BYTES_PG_SHIFT},		
/* MBIO */
	{1, PMP_SUP, VPM_VME8, 0, 0, 0xFFFF0000 >> BYTES_PG_SHIFT},		
/* VME16A16D */
	{1, PMP_SUP, VPM_VME8, 0, 0, 0xFFFF0000 >> BYTES_PG_SHIFT},		
/* VME16A32D -- invalid */
	{0, PMP_SUP, VPM_VME8, 0, 0, 0xFFFF0000 >> BYTES_PG_SHIFT},		
/* VME24A16D -- kludge low 8 megs only */
	{1, PMP_SUP, VPM_VME0, 0, 0, 0xFF000000 >> BYTES_PG_SHIFT},		
/* VME24A32D -- invalid */
	{0, PMP_SUP, VPM_VME0, 0, 0, 0xFF000000 >> BYTES_PG_SHIFT},		
/* VME32A16D -- invalid */
	{0, PMP_SUP, VPM_VME0, 0, 0, 0x00000000 >> BYTES_PG_SHIFT},		
/* VME32A32D -- invalid */
	{0, PMP_SUP, VPM_VME0, 0, 0, 0x00000000 >> BYTES_PG_SHIFT},		
};
#endif SUN2

#ifdef SUN3
struct pgmapent devmaps[] = {
/* MAINMEM */
	{1, PMP_SUP, VPM_MEMORY, 0, 0, 0},		
/* OBIO */
	{1, PMP_SUP, VPM_IO, 0, 0, 0},			
/* MBMEM */
	{1, PMP_SUP, VPM_VME16, 0, 0, 0xFF000000 >> BYTES_PG_SHIFT},		
/* MBIO */
	{1, PMP_SUP, VPM_VME16, 0, 0, 0xFFFF0000 >> BYTES_PG_SHIFT},		
/* VME16A16D */
	{1, PMP_SUP, VPM_VME16, 0, 0, 0xFFFF0000 >> BYTES_PG_SHIFT},		
/* VME16A32D */
	{1, PMP_SUP, VPM_VME32, 0, 0, 0xFFFF0000 >> BYTES_PG_SHIFT},		
/* VME24A16D */
	{1, PMP_SUP, VPM_VME16, 0, 0, 0xFF000000 >> BYTES_PG_SHIFT},		
/* VME24A32D */
	{1, PMP_SUP, VPM_VME32, 0, 0, 0xFF000000 >> BYTES_PG_SHIFT},		
/* VME32A16D */
	{1, PMP_SUP, VPM_VME16, 0, 0, 0x00000000 >> BYTES_PG_SHIFT},		
/* VME32A32D */
	{1, PMP_SUP, VPM_VME32, 0, 0, 0x00000000 >> BYTES_PG_SHIFT},		
};
#endif SUN3


/*
 * devalloc() allocates virtual memory and maps it to a device
 * at a specific physical address.
 *
 * It returns the virtual address of that physical device.
 */
char *
devalloc(devtype, physaddr, bytes)
	enum MAPTYPES	devtype;
	register char *		physaddr;
	register unsigned	bytes;
{
	char *		addr;
	register char *	raddr;
	register int	pages;
	struct pgmapent	mapper;

#ifdef PD
printf("devalloc(%x, %x, %x) ", devtype, physaddr, bytes);
#endif
	if (!bytes)
		return (char *)0;

	pages = bytes + ((int)(physaddr) & (BYTESPERPG-1));
	addr = resalloc(RES_RAWVIRT, pages);
	if (!addr)
		return (char *)0;

	mapper = devmaps[(int)devtype];		/* Set it up first */
	mapper.pm_page += (int)(physaddr) >> BYTES_PG_SHIFT;

	for (raddr = addr;
	     pages > 0;
	     raddr += BYTESPERPG, pages -= BYTESPERPG,
	      mapper.pm_page += 1) {
#ifdef PD
printf("mapping to %x ", mapper);
#endif
		setpgmap(raddr, mapper);
	} 

#ifdef PD
printf("returns roughly %x\n", addr);
#endif
	return addr + ((int)(physaddr) & (BYTESPERPG-1));
}

/*
 * reset_alloc() does all the setup and all the releasing for the PROMs.
 */
reset_alloc(memsize)
	unsigned memsize;
{
#ifdef SUN2
	gp->g_nextrawvirt = (char *)0x100000;
#endif
#ifdef SUN3
	int i, addr, pmeg;

	gp->g_nextrawvirt = (char *)0x200000;
	/*
	 * The monitor only allocates as many PMEGs as there is real
	 * memory so we have to set up more PMEGs for virtual memory
	 * on machines with only 2 megabytes.
	 */
	for (i=0; i < 0x100000; i += PGSPERSEG*BYTESPERPG) {	/* 1 Meg */
		addr = (int)gp->g_nextrawvirt + i;
		pmeg = addr / (PGSPERSEG*BYTESPERPG);
		setsegmap(addr, pmeg);
	}
#endif
	gp->g_nextdmaaddr = DVMA_BASE;
	gp->g_nextmainmap = mainmapinit;
	gp->g_nextmainmap.pm_page = (memsize>>BYTES_PG_SHIFT) - 1;
#ifdef PD
printf("reset_alloc(%x)\n", memsize);
#endif
}
