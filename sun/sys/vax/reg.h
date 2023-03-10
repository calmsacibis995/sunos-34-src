/*	@(#)reg.h 1.1 86/09/25 SMI; from UCB 4.2 81/02/19	*/

#ifndef _REG_
#define _REG_

/*
 * Location of the users' stored
 * registers relative to R0.
 * Usage is u.u_ar0[XX].
 */
#define	R0	(-18)
#define	R1	(-17)
#define	R2	(-16)
#define	R3	(-15)
#define	R4	(-14)
#define	R5	(-13)
#define	R6	(-12)
#define	R7	(-11)
#define	R8	(-10)
#define	R9	(-9)
#define	R10	(-8)
#define	R11	(-7)
#define	R12	(-21)
#define	R13	(-20)

#define AP	(-21)
#define	FP	(-20)
#define	SP	(-5)
#define	PS	(-1)
#define	PC	(-2)

/*
 * And now for something completely the same...
 */
#ifndef LOCORE
/* THIS IS NOT COMPLETE BUT WILL SUFFICE FOR NOW */
struct	regs {	
	int	r_pc;		/* program counter */
	int	r_ps;		/* processor status longword */
};
#endif !LOCORE
#endif !_REG_
