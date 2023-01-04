/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)netisr.h 1.3 86/12/23 SMI; from UCB 6.3 8/2/85
 */

/*
 * The networking code runs off software interrupts.
 *
 * You can switch into the network by doing splnet() and return by splx().
 * The software interrupt level for the network is higher than the software
 * level for the clock (so you can enter the network in routines called
 * at timeout time).
 */

/*
 * These definitions are only to provide compatibility
 * with old code; new stuff should use softcall directly
 */
#define	schednetisr(isrname)	softcall(isrname, (caddr_t)0)

#define	NETISR_RAW	rawintr		/* raw net intr */
#define	NETISR_IP	ipintr		/* IP net intr */
#define	NETISR_NS	nsintr		/* NS net intr */
#define NETISR_IMP	impintr		/* IMP-host protocol interrupts */

int rawintr();

#ifdef INET
int ipintr();
#endif INET

#ifdef NS
int nsintr();
#endif NS

#if NIMP>0
int impintr();
#endif NIMP
