
/*
 * @(#)monalloc.c 1.1 86/09/27
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * monalloc.c
 *
 * ROM Monitor's routines for allocating resources needed on a temporary
 * basis (eg, for initialization or for boot drivers).
 *
 * Note, all requests are rounded up to fill a page.  This is not a
 * malloc() replacement!
 */

#include "../sun3/cpu.map.h"
#include "../sun3/cpu.addrs.h"
#include "../dev/saio.h"
#include "../h/globram.h"

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
	unsigned bytes;
{
	register char *	addr;	/* Allocated address */
	register char *	raddr;	/* Running addr in loop */

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
	for (raddr = addr;
	     bytes > 0;
	     raddr += BYTESPERPG, bytes -= BYTESPERPG,
	      gp->g_nextmainmap.pm_page -= 1) {
		setpgmap(raddr, gp->g_nextmainmap);
	} 

	return addr;
}

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


/*
 * devalloc() allocates virtual memory and maps it to a device
 * at a specific physical address.
 *
 * It returns the virtual address of that physical device.
 */
char *
devalloc(devtype, physaddr, bytes)
	enum MAPTYPES	devtype;
	char *		physaddr;
	unsigned	bytes;
{
	char *		addr;
	register char *	raddr;
	int		pages;
	struct pgmapent	mapper;

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
	      mapper.pm_page -= 1) {
		setpgmap(raddr, mapper);
	} 

	return addr + ((int)(physaddr) & (BYTESPERPG-1));
}

/*
 * reset_alloc() does all the setup and all the releasing for the PROMs.
 */
reset_alloc()
{
	gp->g_nextrawvirt = BOOTMAP_BASE;
	gp->g_nextdmaaddr = DVMA_BASE;
	gp->g_nextmainmap = mainmapinit;
	gp->g_nextmainmap.pm_page = (gp->g_memoryavail>>BYTES_PG_SHIFT) - 1;
}
