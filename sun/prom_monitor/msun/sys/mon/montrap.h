/*	@(#)montrap.h 1.5 83/09/16 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Header file for traps caught by the Sun ROM Monitor
 * 
 * The following little structure is built on the stack to describe
 * the state of the processor at the time of the trap.
 */

struct monintstack {
	long	mis_d0, mis_d1, mis_d2, mis_d3, mis_d4, mis_d5, mis_d6, mis_d7;
	long	mis_a0, mis_a1, mis_a2, mis_a3, mis_a4, mis_a5, mis_a6, mis_a7;
	long	mis_usp;
	long	mis_sfc, mis_dfc, mis_vbase, mis_scon, mis_ucon;
	short	mis_highsr;	/* Filler to make sr look like a longword */
	short	mis_sr;
	long	mis_pc;
	short	mis_vor;
};

/*
 * If you call your argument "monintstack" then these defines make it
 * much easier to reference the stuff.
 *
 * eg:
 *
 * traphandler(monintstack)
 * struct monintstack monintstack;
 * { ... }
 */
#define	r_d0	monintstack.mis_d0
#define	r_d1	monintstack.mis_d1
#define	r_d2	monintstack.mis_d2
#define	r_d3	monintstack.mis_d3
#define	r_d4	monintstack.mis_d4
#define	r_d5	monintstack.mis_d5
#define	r_d6	monintstack.mis_d6
#define	r_d7	monintstack.mis_d7
#define	r_a0	monintstack.mis_a0
#define	r_a1	monintstack.mis_a1
#define	r_a2	monintstack.mis_a2
#define	r_a3	monintstack.mis_a3
#define	r_a4	monintstack.mis_a4
#define	r_a5	monintstack.mis_a5
#define	r_a6	monintstack.mis_a6
#define	r_a7	monintstack.mis_a7
#define	r_ssp	monintstack.mis_a7
#define	r_usp	monintstack.mis_usp
#define	r_sfc	monintstack.mis_sfc
#define	r_dfc	monintstack.mis_dfc
#define	r_vbase	monintstack.mis_vbase
#define	r_scon	monintstack.mis_scon
#define	r_ucon	monintstack.mis_ucon
#define	r_highsr	monintstack.mis_highsr
#define	r_sr	monintstack.mis_sr
#define	r_pc	monintstack.mis_pc
#define	r_vor	monintstack.mis_vor
