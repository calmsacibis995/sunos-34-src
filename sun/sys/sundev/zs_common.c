#ifndef lint
static	char sccsid[] = "@(#)zs_common.c 1.4 87/02/16 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 *  Sun USART(s) driver - common code for all protocols
 */
#include "zs.h"
#if NZS > 0
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"

#include "../machine/enable.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../machine/scb.h"

#include "../sun/fault.h"
#include "../sun/consdev.h"

#include "../sundev/mbvar.h"
#include "../sundev/zsreg.h"
#include "../sundev/zscom.h"

#define NZSLINE	(2*NZS)
struct zscom zscom[NZSLINE];
struct zscom *zscurr = &zscom[1];
struct zscom *zslast = &zscom[0];

#define	ZREADA(n)	zszread((struct zscc_device *)((int)zs->zs_addr|4), n)
#define	ZREADB(n)	zszread((struct zscc_device *)((int)zs->zs_addr&~4), n)
#define	ZWRITEA(n, v)	zszwrite((struct zscc_device *)((int)zs->zs_addr|4), \
				n, v)
#define	ZWRITEB(n, v)	zszwrite((struct zscc_device *)((int)zs->zs_addr&~4), \
				n, v)

/*
 * Driver information for auto-configuration stuff.
 */
int	zsprobe(), zsattach(), zsintr();
struct	mb_device *zsinfo[NZS];
struct	mb_driver zsdriver = {
	zsprobe, 0, zsattach, 0, 0, zsintr,
	2 * sizeof(struct zscc_device), "zs", zsinfo, 0, 0, 0,
};

char	zssoftCAR[NZSLINE];

/*ARGSUSED*/
zsprobe(reg, unit)
	caddr_t reg;
{
	register struct zscc_device *zsaddr = (struct zscc_device *)reg;
	struct zscom tmpzs, *zs = &tmpzs;
	short speed[2];
	register int c, loops;
	label_t jb;

	/* get in sync with the chip */
	if ((c = peekc((char *)&zsaddr->zscc_control)) == -1)
		return (0);
	/*
 	 * We see if it's a Z8530 by looking at register 15
	 * which always has two bits as zero.  If it's not a
	 * Z8530 then setting control to 15 will probably set 
	 * those bits.  Hack, hack.
	 */
	if (pokec((char *)&zsaddr->zscc_control, 15))	/* set reg 15 */
		return (0);
	if ((c = peekc((char *)&zsaddr->zscc_control)) == -1)
		return (0);
	if (c & 5)
		return (0);
	/* 
	 * Well, that test wasn't strong enough for the damn UARTs 
 	 * on the video board in P2 memory, so here comes some more
	 * Anywhere in the following process, the non-existent video
	 * board may decide to give us a parity error, so we use nofault
	 * to catch any errors from here to the end of the probe routine
	 */
	zs->zs_addr = zsaddr;		/* for zszread/write */
	nofault = &jb;
	if (setjmp(nofault)) {
		/* error occurred */
		goto error;
	}
	/*
	 * we can't trust the drain bit in the uart cause we don't know
	 * that the uart is really there.
	 * We need this because trashing the speeds below causes garbage
	 * to be sent.
	 */
	loops = 0;
	while ((ZREADA(1) & ZSRR1_ALL_SENT) == 0 ||
	    (ZREADB(1) & ZSRR1_ALL_SENT) == 0 ||
	    (ZREADA(0) & ZSRR0_TX_READY) == 0 ||
	    (ZREADB(0) & ZSRR0_TX_READY) == 0) {
		DELAY(1000);
		if (loops++ > 500)
			break;
	}
	/* must preserve speeds for monitor / console */
	speed[0] = ZREADA(12);
	speed[0] |= ZREADA(13) << 8;
	speed[1] = ZREADB(12);
	speed[1] |= ZREADB(13) << 8;
	ZWRITEA(12, 17);
	ZWRITEA(13, 23);
	ZWRITEB(12, 29);
	ZWRITEB(13, 37);
	if (ZREADA(12) != 17)
		goto error;
	if (ZREADA(13) != 23)
		goto error;
	if (ZREADB(12) != 29)
		goto error;
	if (ZREADB(13) != 37)
		goto error;
	/* restore original speeds */
	ZWRITEA(12, speed[0]);
	ZWRITEA(13, speed[0] >> 8);
	ZWRITEB(12, speed[1]);
	ZWRITEB(13, speed[1] >> 8);
	nofault = 0;
	return (2 * sizeof (struct zscc_device));
error:
	nofault = 0;
	return (0);
}

#ifdef sun3
/*
 * Base vector numbers for SCC chips
 * Each SCC chip requires 8 contiguous even or odd vectors,
 * on a multiple of 16 boundary
 * E.G., nnnnxxxn where nnnn000n is the base value
 */
short zsvecbase[] = {
	144,		/* zs0 - 1001xxx0 */
	145,		/* zs1 - 1001xxx1 */
};
#define	NZSVEC	(sizeof zsvecbase/sizeof zsvecbase[0])
#endif sun3

zsattach(md)
	register struct mb_device *md;
{
	register struct zscom *zs = &zscom[md->md_unit*2];
	register struct zsops *zso;
	register int i, j;
	short speed[2];
	int loops;
	short vector = 0;

#ifdef sun3
	/*
	 * Install the 8 vectors for this SCC chip
	 */
	if ((cpu != CPU_SUN3_50) && (cpu != CPU_SUN3_60)) {
		extern int (*zsvectab[NZS][8])();
		int (**p)(), (**q)();

		if (md->md_unit >= NZSVEC)
			panic("zsattach: too many zs units");
		vector = zsvecbase[md->md_unit];
		p = &scb.scb_user[vector - VEC_MIN];
		q = &zsvectab[md->md_unit][0];
		for (i = 0; i < 8; i++) {
			*p = *q++;
			p += 2;
		}
	}
#endif sun3
	stopnmi();
	zs->zs_addr = (struct zscc_device *)md->md_addr;
	loops = 0;
	while ((ZREADA(1) & ZSRR1_ALL_SENT) == 0 ||
	    (ZREADB(1) & ZSRR1_ALL_SENT) == 0 ||
	    (ZREADA(0) & ZSRR0_TX_READY) == 0 ||
	    (ZREADB(0) & ZSRR0_TX_READY) == 0) {
		DELAY(1000);
		if (loops++ > 500)
			break;
	}
	/* must preserve speeds over reset for monitor */
	speed[0] = ZREADA(12);
	speed[0] |= ZREADA(13) << 8;
	speed[1] = ZREADB(12);
	speed[1] |= ZREADB(13) << 8;
	ZWRITE(9, ZSWR9_RESET_WORLD); DELAY(10);
	zs->zs_wreg[9] = 0;
	for (i = 0; i < 2; i++) {
		if (i == 0) {		/* port A */
			zs->zs_addr = (struct zscc_device *)
					((int)md->md_addr | 4);
		} else {		/* port B */
			zs++;
			zs->zs_addr = (struct zscc_device *)
					((int)md->md_addr &~ 4);
			zscurr = zs;
		}
		zs->zs_unit = md->md_unit * 2 + i;
		zssoftCAR[zs->zs_unit] = md->md_flags & (1 << i);
		for (j=0; zs_proto[j]; j++) {
			zso = zs_proto[j];
			(*zso->zsop_attach)(zs, speed[i]);
		}
	}
	ZWRITE(9, ZSWR9_MASTER_IE + ZSWR9_VECTOR_INCL_STAT);
	if (vector)
		ZWRITE(2, vector);
	DELAY(4000);
	startnmi();
	zslast = zs;
	if (md->md_intpri != 3) {
		printf("zs%d: priority %d\n", md->md_unit, md->md_intpri);
		panic("bad zs priority");
	}
}

/*
 * Handle Hardware level 6 interrupts
 * These interrupts are locked out only by splzs or spl7,
 * not by spl6, so this routine may not use UNIX facilities such
 * as wakeup which depend on being able to disable interrupts with spls.
 * All communication with the rest of the world is done through the zscom
 * structure and the use of level 3 software interrupts.
 *
 * This routine is only called when the vector indicated by the most
 * recently interrupting SCC is a "special receive" interrupt.
 * This vector is used BOTH for special receive interrupts and to 
 * indicate NO interrupt pending.  In the no interrupt pending case
 * we must poll the other SCCs to find the interrupter.
 * Low level assembler code dispatches the other vectors using the zs_vec
 * array.  This assembler routine is also the code which actually clears
 * the interrupt; the argzs argument is a value/return argument which changes
 * when a different SCC interrupts.
 */
zslevel6intr(argzs)
	struct zscom *argzs;		/* NOTE: value/return argument!! */
{
	register struct zscom *zs;
	register short iinf, unit;
 
	zs = zscurr;		/* always channel B */
	unit = 0;
	for (;;) {
		if (zs->zs_addr && ZREADA(3))
			break;
		zs += 2;		/* always channel B */
		if (zs > zslast)
			zs = &zscom[1];
		if (++unit >= NZS)
			return;
	}
	zscurr = zs;
	iinf = ZREAD(2);	/* get interrupt vector & status */
	if (iinf & 8)
		zs = zscurr - 1;	/* channel A */
	else
		zs = zscurr;		/* channel B */
	switch (iinf & 6) {
	case 0:		/* xmit buffer empty */
		(*zs->zs_ops->zsop_txint)(zs);
		break;

	case 2:		/* external/status change */
		(*zs->zs_ops->zsop_xsint)(zs);
		break;

	case 4:		/* receive char available */
		(*zs->zs_ops->zsop_rxint)(zs);
		break;

	case 6:		/* special receive condition or no interrupt */
		(*zs->zs_ops->zsop_srint)(zs);
		break;
	}
	argzs = zs;
#ifdef lint
	argzs = argzs;
#endif lint
}

/*
 * Install a new ops vector into low level vector routine addresses
 */
zsopinit(zs, zso)
	register struct zscom *zs;
	register struct zsops *zso;
{

	zs->zs_vec[0] = zso->zsop_txint;
	zs->zs_vec[1] = zso->zsop_xsint;
	zs->zs_vec[2] = zso->zsop_rxint;

	switch (cpu) {
#ifdef sun3
	case CPU_SUN3_160:
	case CPU_SUN3_260:
	case CPU_SUN3_110:
		/* vectored interrupts */
		zs->zs_vec[3] = zso->zsop_srint;
		break;
#endif sun3
	default:
		/* non-vectored Sun-2 and Sun-3 50 (Model 25) */
		zs->zs_vec[3] = zslevel6intr;
		break;
	}
	zs->zs_ops = zso;
}

/*
 * Handle a level 3 interrupt 
 * This is the routine found by autoconf in the driver structure
 */
zsintr()
{
	register struct zscom *zs;

	if (clrzssoft()) {
		zssoftpend = 0;
		for (zs = &zscom[0]; zs <= zslast; zs++) {
			if (zs->zs_flags & ZS_NEEDSOFT) {
				zs->zs_flags &=~ ZS_NEEDSOFT;
				(*zs->zs_ops->zsop_softint)(zs);
			}
		}
		return (1);
	}
	return (0);
}

/*
 * The "null" zs protocol
 * Called before the others to initialize things
 * and prevent interrupts on unused devices 
 */
int	zsnull_attach(), zsnull_intr(), zsnull_softint();

struct zsops zsops_null = {
	zsnull_attach,
	zsnull_intr,
	zsnull_intr,
	zsnull_intr,
	zsnull_intr,
	zsnull_softint,
};

zsnull_attach(zs, speed)
	register struct zscom *zs;
{

	/* make sure ops prt is valid */
	zsopinit(zs, &zsops_null);
	/*
	 * Set up the default asynch modes
	 * so the monitor will still work
	 */
	ZWRITE(4, ZSWR4_PARITY_EVEN + ZSWR4_1_STOP + ZSWR4_X16_CLK);
	ZWRITE(3, ZSWR3_RX_8);
	ZWRITE(11, ZSWR11_TXCLK_BAUD + ZSWR11_RXCLK_BAUD);
	ZWRITE(12, speed);
	ZWRITE(13, speed >> 8);
	ZWRITE(14, ZSWR14_BAUD_FROM_PCLK);
	ZWRITE(3, ZSWR3_RX_8 + ZSWR3_RX_ENABLE);
	ZWRITE(5, ZSWR5_TX_ENABLE + ZSWR5_TX_8 + ZSWR5_RTS + ZSWR5_DTR);
	ZWRITE(14, ZSWR14_BAUD_ENA + ZSWR14_BAUD_FROM_PCLK);
	zs->zs_addr->zscc_control = ZSWR0_RESET_ERRORS + ZSWR0_RESET_STATUS;
}

zsnull_intr(zs)
	register struct zscom *zs;
{
	register struct zscc_device *zsaddr = zs->zs_addr;
	register short c;

	zsaddr->zscc_control = ZSWR0_RESET_TXINT;
	DELAY(2);
	zsaddr->zscc_control = ZSWR0_RESET_STATUS;
	DELAY(2);
	c = zsaddr->zscc_data;
#ifdef lint
	c = c;
#endif lint
	DELAY(2);
	zsaddr->zscc_control = ZSWR0_RESET_ERRORS;
}

zsnull_softint(zs)
	register struct zscom *zs;
{
	printf("zs%d: unexpected soft int\n", zs->zs_unit);
}
#endif NZS > 0
