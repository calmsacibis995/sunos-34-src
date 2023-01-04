#ifndef lint
static char sccsid[] = "@(#)machdep.c 1.5 87/02/27";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "diag.h"
#include <sys/types.h>
#include <sys/vmmac.h>

extern char end[];
int cpudelay;

getchar()
{

	return (*romp->v_getchar)();
}

maygetchar()
{

	return (*romp->v_mayget)();
}

putchar(c)
{

	(*romp->v_putchar)(c);
}

/*
 * Go to high priority and set default sfc and dfc to FC_MAP (3)
 */
asminit()
{

	asm("movw #0x2700,sr");
	asm("moveq #3,d0");		/* FC_MAP */
	asm("movc d0,sfc");
	asm("movc d0,dfc");
}

/*
 * Virtual addr for devaddr mapping - we chose to put it near the end of
 * virtual memory to be "out of the way" so that fragile things don't
 * break.  In the sun2 case, this area is mapped to Multibus Memory or
 * main memory (DVMA).  In the sun3 case, this is in DVMA where MONSHORTSEG
 * is.  In any case, there is a pmeg there which can have pte's remapped.
 */
#define	DEVADDR	(MONSHORTPAGE-NBPG)


#ifdef sun2
#include <sun2/mmu.h>
#include <sun2/cpu.h>
#include <sun2/pte.h>

machinit()
{

	asminit();
	cpudelay = 5;

#ifndef lint
	asm("moveq #0,d0");
	asm("movsb d0,7");		/* setusercontext(KCONTEXT) */
#endif
}

/*
 * virtual address for mbio space
 * (or vme16 space) as preset by the monitor
 */
#define	MBIO_BASE	0xeb0000

/*
 * If the address is less than 64Kb, assume Multibus I/O address,
 * (vme16 space) else if less than 1Meg, assume Multibus memory,
 * else if not a monitor address assume VME24 and map in the page by hand.
 */
setdevaddr(addr)
	int addr;
{
	int pageval;

	onboard = 0;
	si_ha_type = 0;

	if (addr < 0x10000) {
		devaddr = addr + MBIO_BASE;	/* multibus i/o */
	} else if (addr < 0x100000) {
		devaddr = addr + DVMA;		/* multibus memory */
	} else if (addr >= MONSTART && addr < MONEND) {
		devaddr = addr;			/* use the virt addr we got */
	} else {
		if (addr < VME0_SIZE)
			pageval = PGT_VME0 | btop(addr);
		else
			pageval = PGT_VME8 | btop(addr - VME0_SIZE);
		setpgmap((int)DEVADDR, PG_V | PG_KW | pageval);
		devaddr = DEVADDR + (addr & PGOFSET);
	}
}

/*
 * After reset instruction we must send out an escape sequence to
 * reenble the video, too bad if this is an RS232 port.
 */
machreset()
{

	printf("[s");			/* restore video enable */
}

/*
 * Should be defines for these elsewhere
 */
#define	PAGEADDRBITS	0x00fff800
#define	PAGEBASE	0x00000000
#define	PTEBADBITS	0x03ff0000

/*
 * The Sun-2 software pte has the type bits in a different place
 * than does the hardware pme, we take care of that here
 */
setpgmap(addr, pte)
	int addr;
	register int pte;		/* known to be d7 below */
{
	register int *xaddr;		/* known to be a5 below */

	xaddr = (int *)((addr & PAGEADDRBITS) | PAGEBASE);
	pte = (pte & ~PTEBADBITS) | ((pte & PGT_MASK) << 6);
#ifdef lint
	*xaddr = pte;
#else
	asm("movsl d7,a5@");
#endif
}
#endif sun2

#ifdef sun3
#include <sun3/mmu.h>
#include <sun3/cpu.h>
#include <sun3/pte.h>
#include <sun3/clock.h>
#include <sun3/interreg.h>

/*
 * Map in the clock and interrupt register,
 * in addition to seting up mapping in the DVMA area
 */
machinit()
{
	register int i;
	int pg = btoc(end + 0x10000);	/* start of physical page free list */

	asminit();
	cpudelay = 3;

#ifndef lint
	asm("moveq #0,d0");
	asm("movsb d0,0x30000000");	/* setcontext(KCONTEXT) */
#endif

	setpgmap((int)CLKADDR, PG_V | PG_KW | PGT_OBIO | btop(OBIO_CLKADDR));
	setpgmap((int)INTERREG, PG_V | PG_KW | PGT_OBIO | btop(OBIO_INTERREG));

	/* mapin DVMA space */
	for (i = 0; i < btop(DVMA_MAP_SIZE); i++, pg++)
		setpgmap(DVMA + (int)ptob(i), PG_V | PG_KW | PGT_OBMEM | pg);
}

/*
 * Must determine whether or not we have an onboard interface.
 * To do this read the machine id and set variable. Sun3/50 onboard
 * scsi interface is fixed at 0x140000. If not Sun3/50 then assume vme.
 * If addr less than VME16_SIZE, assume vme16, else if addr less than
 * VME24_SIZE, assume vme24, else assume vme32.  The vme16 and
 * vme24 are set up for 16 bit transfers, the vme32 is set for
 * 32 bit transfers.
 */
setdevaddr(addr)
	int addr;
{
	int pageval;

	onboard = 0;
	if (machine_id() == CPU_SUN3_50 || machine_id() == CPU_SUN3_60) {
		onboard = 1;
		si_ha_type = 1;
	}

	if ((addr == 0x140000) && onboard) {
		pageval = PGT_OBIO | btop(addr);
	} else if (addr < VME16_SIZE) {
		pageval = PGT_VME_D16 | btop(addr + VME16_BASE);
	} else if (addr < VME24_SIZE) {
		pageval = PGT_VME_D16 | btop(addr + VME24_BASE);
	} else {
		pageval = PGT_VME_D32 | btop(addr);
	}

	setpgmap((int)DEVADDR, PG_V | PG_KW | pageval);
	devaddr = DEVADDR + (addr & PGOFSET);
	if (scsi && (onboard == 0)) {
		if (peek((int)devaddr+0x800) == -1) {
			si_ha_type = 1;
		} else {
			si_ha_type = 0;
		}
	}
}

/*
 * After reset instruction we must reenable the interrupt
 * control register and reset the clock
 */
machreset()
{
	u_char i;

 	*INTERREG &= ~(IR_ENA_CLK7 | IR_ENA_CLK5); 	/* clear flops */
	CLKADDR->clk_intrreg = 0;			/* disable TOD intrs */
	i = CLKADDR->clk_intrreg;			/* clear pending intr */
#ifdef lint
	i = i;
#endif
	*INTERREG |=  IR_ENA_CLK7;			/* enable lvl 7 flop */
	CLKADDR->clk_intrreg = CLK_INT_HSEC;		/* allow TOD intrs */
	*INTERREG |= IR_ENA_INT;			/* enable intrs */
	printf("[s");				/* restore video enable */
}

setpgmap(addr, pte)
	int addr;
	register int pte;		/* known to be d7 below */
{
	register int *xaddr;		/* known to be a5 below */

	xaddr = (int *)((addr & PAGEADDRBITS) | PAGEBASE);
#ifdef lint
	*xaddr = pte;
#else
	asm("movsl d7,a5@");
#endif
}

machine_id()
{
	register u_char *x = IDPROMBASE;	/* known to be a5 below */
	register u_char val;			/* knwon to be d7 below */

#ifdef lint
	val = *x++;
#else
	asm("movsb a5@+,d7");			/* read format type */
#endif
	if (val != 1) {
		return(0);			 /* make sure format is 1 */
	}
	
#ifdef lint
	val = *x;
#else
	asm("movsb a5@,d7");			/* read machine type */
#endif
	return(val);
}
#endif sun3
