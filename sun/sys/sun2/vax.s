	.data
	.asciz	"@(#)vax.s 1.1 86/09/25"
	.even
	.text
/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Emulate VAX instructions on the 68010.
 */

#include "../h/param.h"
#include "../machine/asm_linkage.h"
#include "../machine/mmu.h"
#include "../machine/psl.h"
#include "assym.s"

/*
 * Macro to raise prio level, avoid dropping prio if already
 * at high level
 */
#define	RAISE(level)	\
	movw	sr,d0;	\
	andw	#(SR_SMODE+SR_INTPRI),d0; 	\
	cmpw	#(SR_SMODE+(/**/level*0x100)),d0;	\
	jge	1f;	\
	movw	#(SR_SMODE+(/**/level*0x100)),sr;	\
1:	rts

#define	SETPRI(level)	\
	movw	sr,d0;	\
	movw	#(SR_SMODE+(/**/level*0x100)),sr;	\
	rts

	ENTRY(splimp)
	RAISE(3)

	ENTRY(splnet)
	RAISE(1)

	ENTRY(splie)
	RAISE(3)

	ENTRY(splclock)
	RAISE(5)

	ENTRY(splzs)
	SETPRI(6)

	ENTRY(spl7)
	SETPRI(7)

	ENTRY2(spl6,spl5)
	SETPRI(5)

	ENTRY(spl4)
	SETPRI(4)

	ENTRY(spl3)
	SETPRI(3)

	ENTRY(spl2)
	SETPRI(2)

	ENTRY2(spl1,splsoftclock)
	SETPRI(1)

	ENTRY(spl0)
	SETPRI(0)

	ENTRY(splx)
	movw	sr,d0
	movw	sp@(6),sr
	rts

/*
 * splr is like splx but will only raise the priority and never drop it
 */
	ENTRY(splr)
	movw	sr,d0
	andw	#(SR_SMODE+SR_INTPRI),d0
	movw	sp@(6),d1
	andw	#(SR_SMODE+SR_INTPRI),d1
	cmpw	d1,d0
	jge	0f
	movw	d1,sr
0:
	rts

	ENTRY(_insque)
	movl	sp@(4),a0
	movl	sp@(8),a1
	movl	a1@,a0@
	movl	a1,a0@(4)
	movl	a0,a1@
	movl	a0@,a1
	movl	a0,a1@(4)
	rts

	ENTRY(_remque)
	movl	sp@(4),a0
	movl	a0@,a1
	movl	a0@(4),a0
	movl	a1,a0@
	movl	a0,a1@(4)
	rts

	ENTRY(scanc)
	movl	sp@(8),a0	| string
	movl	sp@(12),a1	| table
	movl	sp@(16),d1	| mask
	movl	d2,sp@-
	clrw	d2
	movl	sp@(8),d0	| len
	subqw	#1,d0		| subtract one for dbxx
	jmi	1f
2:
	movb	a0@+,d2		| get the byte from the string
	movb	a1@(0,d2:w),d2	| get the corresponding table entry
	andb	d1,d2		| apply the mask
	dbne	d0,2b		| check for loop termination
1:
	addqw	#1,d0		| dbxx off by one
	movl	sp@+,d2
	rts

/*
 * _whichqs tells which of the 32 queues _qs have processes in them
 * setrq puts processes into queues, remrq removes them from queues
 * The running process is on no queue, other processes are on a
 * queue related to p->p_pri, divided by 4 actually to shrink the
 * 0-127 range of priorities into the 32 available queues.
 */

/*
 * setrq(p), using non-fancy 68000 instructions.
 *
 * Call should be made at spl6(), and p->p_stat should be SRUN
 */
	ENTRY(setrq)
	movl	sp@(4),a0	| get proc pointer
	tstl	a0@(P_RLINK)	| firewall: p->p_rlink must be 0
	jne	1f
	clrl	d0
	movb	a0@(P_PRI),d0	| get the priority
	lsrw	#2,d0
	movl	d0,d1
	lslw	#3,d0
	movl	#_qs+4,a1
	addw	d0,a1
	movl	a1@,a1		| get qs[p->p_pri].ph_rlink
	movl	a1@,a0@		| insque(p, blah)
	movl	a1,a0@(4)
	movl	a0,a1@
	movl	a0@,a1
	movl	a0,a1@(4)
	movl	_whichqs,d0	| set appropriate bit in whichqs
	bset	d1,d0
	movl	d0,_whichqs
	rts
1:
	pea	2f
	jsr	_panic

2:	.asciz	"setrq"
	.even

/*
 * remrq(p), using non-fancy 68000 instructions
 *
 * Call should be made at spl6().
 */
	ENTRY(remrq)
	movl	sp@(4),a0
	clrl	d0
	movb	a0@(P_PRI),d0
	lsrw	#2,d0
	movl	_whichqs,d1
	btst	d0,d1
	jeq	1f
	movl	a0@,a1		| remque(p);
	movl	a0@(4),a0
	movl	a1,a0@
	movl	a0,a1@(4)
	cmpl	a0,a1
	jne	2f		| queue not empty
	movl	_whichqs,d1	| queue empty,
	bclr	d0,d1		|   clear the bit
	movl	d1,_whichqs
2:
	movl	sp@(4),a0
	clrl	a0@(P_RLINK)
	rts
1:
	pea	3f
	jsr	_panic

3:	.asciz	"remrq"
	.even

/*
 * swtch(), using non-fancy 68000 instructions
 */
#ifdef STREAMS
	.globl	_qrunflag, _runqueues
#endif
	ENTRY(swtch)
	movw	sr,sp@-		| save processor priority
	movl	#1,_noproc
	clrl	_runrun
	bclr	#AST_SCHED_BIT-24,_u+PCB_P0LR:w
2:
	movl	_whichqs,d1	| look for non-empty queue
	clrl	d0
1:
	btst	d0,d1
	jne	3f		| found one
	addl	#1,d0
	cmpl	#32,d0
	jlt	1b
#ifdef STREAMS
	tstb	_qrunflag	| need to run stream queues?
	beq	1f		| no
	movw	#SR_LOW,sr	| run queues at priority 0
	jsr	_runqueues	| go do it
	jra	2b		| scan run queue immediately
1:
#endif
	movw	#SR_LOW,sr	| must allow interrupts here
	jra	2b		| this is an idle loop!
3:
	movw	#SR_HIGH,sr	| lock out all so _whichqs==_qs
	movl	_whichqs,d1
	bclr	d0,d1
	jeq	2b		| proc moved via lbolt interrupt
	movl	d1,_whichqs
	lslw	#3,d0
	movl	#_qs,a0
	addw	d0,a0
	movl	a0@,a0		| get qs[bit].ph_link = p = highest pri process
	movl	a0,d1		| save it
	movl	a0@,a1		| remque(p);
	cmpl	a0,a1		| is queue empty?
	jne	4f
8:
	pea	9f
	jsr	_panic
9:
	.asciz	"swtch"
	.even
4:
	movl	a0@(4),a0
	movl	a1,a0@
	movl	a0,a1@(4)
	cmpl	a0,a1		| queue empty?
	movl	d1,a0		| restore a0, doesn't change cc
	jeq	5f		| queue empty
	lsrw	#3,d0
	movl	_whichqs,d1	| queue not empty,
	bset	d0,d1		|   set the bit
	movl	d1,_whichqs
5:
	clrl	_noproc
	tstl	a0@(P_WCHAN)	|| firewalls
	jne	8b		||
	cmpb	#SRUN,a0@(P_STAT) ||
	jne	8b		||
	clrl	a0@(P_RLINK)	||
	addql	#1,_cnt+V_SWTCH
	movl	a0,sp@-
	jsr	_resume
	addqw	#4,sp
	movw	sp@+,sr		| restore processor priority
	rts

/*
 * masterprocp is the pointer to the proc structure for the currently
 * mapped u area.  It is used to set up the mapping for the u area
 * by the debugger since the u area is not in the Sysmap.
 */
	.data
	.globl	_masterprocp
_masterprocp:	.long	0		| struct proc *masterprocp;
	.text

/*
 * resume(p)
 */
#include "sky.h"
#include "ropc.h"
#if NROPC > 0
	.data
	.globl	_mem_ropc
_mem_ropc:	.long 0			| struct memropc *mem_ropc;
	.text
#endif
#define	SAVREGS		d2-d7/a2-a7

	ENTRY(resume)
#if NSKY > 0
	tstw	_u+U_SKYUSED:w		| is user using Sky FFP?
	beq	1f
	jsr	_skysave		| yes, save state
1:
#endif NSKY > 0
#if NROPC > 0
	movl	_mem_ropc,d0
	jeq	1f
	movl	d0,a1
	movw	a1@(MRC_X15),_u+U_MEMROPC:w
1:
#endif NROPC > 0
	movl	sp@,_u+PCB_REGS:w	| save return pc in pcb
	movl	sp@(4),a0		| get proc address
	movw	sr,_u+PCB_SR+2:w
	moveml	SAVREGS,_u+PCB_REGS+4:w	| save data/address regs
	cmpl	#_u,sp			| check to see if sp is above u area
	jhi	1f			| jmp is so
	pea	0f			| panic - we tried to switch while
	jsr	_panic			|   on the overflow stack area
	/* NOTREACHED */
	.data
0:	.asciz	"kernel stack overflow"
	.even
	.text
1:
	movl	a0,_masterprocp		| set proc pointer for new process
	movw	#SR_HIGH,sr
	lea	eintstack,sp		| switch to interrupt stack
	movc	dfc,d2			| save dfc
	lea	FC_MAP,a1
	movc	a1,dfc			| set to access maps
	movl	#KCONTEXT,d0
	movsb	d0,USERCONTEXTOFF	| switch to kernel context for mapping
	lea	_u:w,a1
	movl	a0@(P_ADDR),a2		| get p_addr (address of pte's)
	movl	a2@+,d0			| map in first u page (stack)
	movsl	d0,a1@
	addl	#NBPG,a1
	movl	a2@+,d0			| map in second u page
	movsl	d0,a1@
/*
 * Check to see if we already have context.  If so and
 * SPTECHG bit is not on then set up the next context.
 */
	tstl	a0@(P_CTX)		| check p->p_ctx
	jeq	1f			| if zero, skip ahead
	btst	#SPTECHG_BIT-24,a0@(P_FLAG)	| check (p->p_flag & SPTECHG)
	jne	1f			| if SPTECHG bit is non-zero, skip ahead
	movl	a0@(P_CTX),a1
	movw	a1@(CTX_CONTEXT),d0	| get context number in d0
	movsb	d0,USERCONTEXTOFF	| set up the user context
	addql	#1,_ctxtime		| Increment context clock
	movl	_ctxtime,a1@(CTX_TIME)	| Set context LRU to updated clock
1:
	movc	d2,dfc			| restore dfc
#if NROPC > 0
	movl	_mem_ropc,d0
	jeq	1f
	movl	d0,a1
	movw	_u+U_MEMROPC:w,a1@(MRC_X15)
1:
#endif NROPC > 0
#if NSKY > 0
	tstw	_u+U_SKYUSED:w
	beq	1f
	jsr	_skyrestore
1:
#endif NSKY > 0
	moveml	_u+PCB_REGS+4:w,SAVREGS	| restore data/address regs
| Note: we just changed stacks from the interrupt stack to the new process stack
	movw	_u+PCB_SR+2:w,sr
	tstl	_u+PCB_SSWAP:w
	jeq	1f
	movl	_u+PCB_SSWAP:w,sp@(4)	| blech...
	clrl	_u+PCB_SSWAP:w
	movw	#SR_LOW,sr
	jmp	_longjmp
1:
	rts
