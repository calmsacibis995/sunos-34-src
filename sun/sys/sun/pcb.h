/*	@(#)pcb.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Sun software process control block
 */

#ifndef LOCORE
struct pcb {
	label_t	pcb_regs;	/* saved registers */
	int	pcb_sr; 	/* program status word */
	struct	pte *pcb_p0br;	/* pseudo-P0BR for Sun */
	int	pcb_p0lr;	/* pseudo-P0LR for Sun */
	struct	pte *pcb_p1br;	/* pseudo-P1BR for Sun */
	int	pcb_p1lr;	/* pseudo-P1LR for Sun */
	int	pcb_szpt; 	/* number of pages of user page table */
	int	*pcb_sswap;
};
#endif

#define	AST_SCHED	0x80000000	/* force a reschedule */
#define	AST_STEP	0x40000000	/* force a single step */
#define	TRACE_USER	0x20000000	/* user has requested tracing */
#define	TRACE_AST	0x10000000	/* AST has requested tracing */
#define	TRACE_PENDING	0x08000000	/* trace caught in supervisor mode */
#define	AST_CLR		0xf8000000
#define	PME_CLR		0
#define	AST_NONE	0

#define	aston()		{u.u_pcb.pcb_p0lr |= AST_SCHED;}
