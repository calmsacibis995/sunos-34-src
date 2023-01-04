	.data
	.asciz	"@(#)locore.s 1.1 86/09/25"
	.even
	.text
/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "../h/param.h"
#include "../h/vmparam.h"
#include "../h/errno.h"
#include "../netinet/in_systm.h"

#include "../machine/pte.h"
#include "../machine/trap.h"
#include "../machine/reg.h"
#include "../machine/psl.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../machine/pcb.h"
#include "../machine/buserr.h"
#include "../machine/enable.h"
#include "../machine/clock.h"
#include "../machine/asm_linkage.h"

#include "assym.s"

/* 
 * Absolute external symbols
 *
 * We now put the redzone 2 pages below the u area.  The page
 * directly below the u area is the yellowzone, a global kernel
 * overflow page.  If you are using the yellowzone, trying to
 * do a context switch will cause a panic.  This page is used
 * to allow the per process stack to overflow the current
 * KERNELSTACK sized stack with out requiring each process
 * to lug around an extra physical page per process which is
 * rarely used.  This is a band-aid for now.  In the long run
 * we will probably have to bite the bullet and actually use
 * another page per process.
 */
	.globl	_scb, _u, _yellowzone, _redzone, _mbio, _DVMA
_scb		= KERNELBASE + 0
_u		= UADDR
_yellowzone	= _u - NBPG
_redzone	= _yellowzone - NBPG
_mbio		= 0xEB0000	/* monitor-established base for Multibus I/O */
_DVMA		= 0xF00000

/*
 * The interrupt stack.  This must be the first thing in the data
 * segment (other than an sccs string) so that we don't stomp
 * on anything important during interrupt handling.  We get a
 * red zone below this stack for free when the kernel text is
 * write protected.  Since the kernel is loaded with the "-N"
 * flag, we pad this stack by a page because when the page
 * level protection is done, we will lose part of this interrupt
 * stack.  Thus the true interrupt stack will be at least MIN_INTSTACK_SZ
 * bytes and at most MIN_INTSTACK_SZ+NBPG bytes.  The interrupt entry
 * code assumes that the interrupt stack is at a higher address than
 * both _etext and the kernel stack in the u area.
 */
#define	MIN_INTSTACK_SZ	NBPG
	.data
	.globl _intstack, eintstack
_intstack:				| bottom of interrupt stack
	. = . + NBPG + MIN_INTSTACK_SZ
eintstack:				| end (top) of interrupt stack

/*
 * System software page tables
 */
#define vaddr(x)	((((x)-_Sysmap)/4)*NBPG + KERNELBASE)
#define SYSMAP(mname, vname, npte)	\
	.globl	mname;			\
mname:	.=.+(4*npte);			\
	.globl	vname;			\
vname = vaddr(mname);
	.data
	SYSMAP(_Sysmap   ,_Sysbase	,SYSPTSIZE	)
	SYSMAP(_Usrptmap ,_usrpt 	,USRPTSIZE	)
	SYSMAP(_Forkmap  ,_forkutl	,UPAGES		)
	SYSMAP(_Xswapmap ,_xswaputl	,UPAGES		)
	SYSMAP(_Xswap2map,_xswap2utl	,UPAGES		)
	SYSMAP(_Swapmap  ,_swaputl	,UPAGES		)
	SYSMAP(_Pushmap  ,_pushutl	,UPAGES		)
	SYSMAP(_Vfmap    ,_vfutl	,UPAGES		)
	SYSMAP(_CMAP1    ,_CADDR1	,1		) /* local tmp */
	SYSMAP(_CMAP2    ,_CADDR2	,1		) /* local tmp */
	SYSMAP(_mmap     ,_vmmap        ,1		)
	SYSMAP(_msgbufmap,_msgbuf 	,MSGBUFPTECNT	)
	SYSMAP(_Mbmap    ,_mbutl        ,NMBCLUSTERS*CLSIZE)
	SYSMAP(_ESysmap	 ,_Syslimit	,0		) /* must be last */

	.globl  _Syssize
_Syssize = (_ESysmap-_Sysmap)/4

/*
 * Software copy of system enable register
 * This word is always atomically updated
 */
	.data
	.globl	enablereg
enablereg: .word	0

/*
 * Macro to save all registers and switch to kernel context
 * 1: Save and switch context
 * 2: Save all registers
 * 3: Save user stack pointer
 */
#define SAVEALL() \
	clrw	sp@-;\
	moveml	#0xFFFF,sp@-;\
	movl	usp,a0;\
	movl	a0,sp@(R_SP)

/* Normal trap sequence */
#define TRAP(type) \
	SAVEALL();\
	movl	#type,sp@-;\
	jra	trap

/*
 * System initialization
 * UNIX receives control at the label '_start' which
 * must be at offset zero in this file; this file must
 * be the first thing in the boot image.
 */
	.text
	.globl	_start
_start:
	movw	#SR_HIGH,sr		| lock out interrupts

| zero u page
	lea	_u,a2
	movl	#(UPAGES*NBPG/4)-1,d2
1$:	clrl	a2@+
	dbra	d2,1$
| set the stack pointer to what will be the kernel U area
	lea	_u+U_STACK+KERNSTACK,sp | top of stack

/*
 * We should reset the world here, but it screws the UART settings
 *
 * Initialize all contexts:
 * All but the kernel context are initialized to be
 * invalid in all segments.
 * For now, addresses above MAXKSEG are not remapped
 * to avoid messing up monitor messages, etc.
 */
	lea	FC_MAP,a0		| set to access enable reg
	movc	a0,sfc			|   source and destination
	movsw	ENABLEREG,d0		| get the enable register
	movw	d0,enablereg		| save soft copy	
	lea	FC_UD,a0		| set default value for
	movc	a0,sfc			|   source and destination
	movc	a0,dfc			|   function code registers
	movl	#KCONTEXT,sp@-
	jsr	_setusercontext		| set user context to zero
					| leave an empty space on the stack

| Loop through all contexts [1 : NCONTEXT-1]
	movl	#1,d2
ctxtlp: 
	movl	d2,sp@
	jsr	_setusercontext
| Loop through each segment in context [0 : NSEGMAP-1]
	clrl	d3
seglp:
	movl	#SEGINV,sp@
	movl	d3,sp@-
	jsr	_setsegmap		| setsegmap(segment, SEGINV)
	addl	#4,sp
	addql	#1,d3			| bump to next segment
	cmpl	#NSEGMAP,d3		| done?
	jne	seglp			| end loop per segment
	addql	#1,d2			| bump to next context
	cmpl	#NCONTEXT,d2		| done?
	jne	ctxtlp			| end loop per context
/*
 * At this point every segment in all contexts except ours is invalid.
 *
 * We make the following assumptions about our environment
 * as set up by the monitor:
 *
 *	- we have at least 1MB of memory, mapped as follows
 *	- each segment i is mapped to use pmeg i
 *	- each page map entry i maps physical page i
 *	- all pages all valid and all protection is set to rwxrwx
 *
 * We will set the protection properly in startup().
 */
	movl	#KCONTEXT,sp@
	jsr	_setusercontext		| so can manipulate kernel context
|... zero bss
	movl	#_end-1,d2		| get UNIX end
	lea	_edata,a1		| get bss start
	subl	a1,d2			| get bss length
	lsrl	#2,d2			| shift for long count
2$:	clrl	a1@+			| clear long word and incr
	dbra	d2,2$			| decr count and loop

#ifdef GPROF
	jsr	kprofinit
#endif

#ifdef notyet
	/* DOESN'T WORK WITH REV N PROTOTYPE PROMS!!! */
/*
 * Check to see if memory was all mapped in correctly.  On versions >= 'N',
 * if ROMP_ROMVEC_VERSION is greater than zero, then ROMP_MEMORYBITMAP
 * contains the address of a pointer to an array of bits
 * If this pointer is non-zero, then we had some bad pages
 * Until we get smarter, we give up if we find this condition.
 */
	tstl	ROMP_ROMVEC_VERSION
	ble	1f			| field is <= zero, don't do next test
	movl	ROMP_MEMORYBITMAP,a0
	tstl	a0@
	jeq	1f			| pointer is zero, all ok
	pea	0f
	movl	ROMP_PRINTF,a0		| print message
	jsr	a0@
	rts				| back to monitor
	.data
0:	.asciz "Memory bad, Unix quits\012"
	.even
	.text
1:
#endif notyet

	.globl	_trap, _syscall, _main
| dummy up an interrupt stack so process 1 can find saved registers
	movl	#USRSTACK,a0		| init user stack pointer
	movl	a0,usp
	clrw	sp@-			| dummy fmt & vor
	movl	#USRTEXT,sp@-		| push pc
	movw	#SR_USER,sp@-		| push sr
	lea	0,a6			| stack frame link 0 in main
| invoke main
| return to init
	SAVEALL()
	jsr	_main			| simulate interrupt -> main
	clrl	d0			| fake return value from trap
	jra	rei

/*
 * Support for parity error scanning,
 * assumes that _scb is vector base.
 */
	.globl	_partest
_partest:
	movl	sp,a1			| save sp in case of fault
	movl	sp@(4),a0		| get address
	movl	_scb+8,d1		| save bus error vector
	movl	#parbuserr,_scb+8	| set up to catch fault
	movw	a0@,d0			| try to read it
	nop
	movl	d1,_scb+8		| restore bus error vector
	rts
parbuserr:
	movl	d1,_scb+8		| restore bus error vector
	movc	sfc,a0			| save sfc
	movl	#FC_MAP,d1		| set to address FC_MAP space
	movc	d1,sfc
	movsw	BUSERRREG,d7		| get the buserr register
	movc	a0,sfc			| restore sfc
	movl	a1,sp			| restore sp
	rts

/*
 * Entry points for interrupt and trap vectors
 */
	.globl	buserr, addrerr, fmterr, illinst, zerodiv, chkinst
	.globl	trapv, privvio, trace, emu1010, emu1111, spurious
	.globl	badtrap, brkpt, level2, level3, level4, level5
	.globl	errorvec

buserr:
	TRAP(T_BUSERR)

addrerr:
	TRAP(T_ADDRERR)

fmterr:
	TRAP(T_FMTERR)

illinst:
	TRAP(T_ILLINST)

zerodiv:
	TRAP(T_ZERODIV)

chkinst:
	TRAP(T_CHKINST)

trapv:
	TRAP(T_TRAPV)

privvio:
	TRAP(T_PRIVVIO)

trace:
	TRAP(T_TRACE)

emu1010:
	TRAP(T_EMU1010)

emu1111:
	TRAP(T_EMU1111)

spurious:
	TRAP(T_SPURIOUS)

badtrap:
	TRAP(T_M_BADTRAP)

brkpt:
	TRAP(T_BRKPT)

errorvec:
	TRAP(T_M_ERRORVEC)

level2:
	IOINTR(2)

level3:
	IOINTR(3)

level4:
	IOINTR(4)

/*
 * Special handling for mouse tracking.
 */
	.globl	_hardclock
#include "pi.h"
#if NPI > 0
	.data
	.globl	_clkrate,_clkticks
_clkrate:
	.word	0
_clkticks:
	.word	1
lastmouse:
	.word	0

	.text
	.globl	_piaddr, _pilast
	.globl	_clkpiscan, _clknopiscan
_clkpiscan:
	movl	a1,sp@-
	movl	_piaddr,a1
	movw	a1@,a1
	cmpw	_pilast,a1
	beq	11$
	moveml	#0xC080,sp@-		| save C regs
	jsr	_piintr
	moveml	sp@+,#0x0103		| restore regs
11$:	movl	sp@+,a1
	movw	#CLK_REFR,CLKADDR+2	| CLKADDR->clk_cmd = CLK_REFR;
	subqw	#1,_clkticks
	ble	9$
	rte
9$:
	movw	_clkrate,_clkticks
_clknopiscan:
#endif NPI
level5:				/* default clock interrupt */
	movw	#CLK_REFR,CLKADDR+2	| CLKADDR->clk_cmd = CLK_REFR
	moveml	#0xC0E0,sp@-		| save regs we trash <d0,d1,a0,a1,a2>
	movl	sp,a2			| save copy of previous sp
	cmpl	#_etext,sp		| on interrupt stack?
	bgt	1f			| yes, skip
	lea	eintstack,sp		| no, switch to interrupt stack
1:	movw	a2@(5*4),d0		| get saved sr
	movl	d0,sp@-			| push it as a long
	movl	a2@(5*4+2),sp@-		| push saved pc
	jsr	_hardclock
	movl	a2,sp			| restore old sp
	moveml	sp@+,#0x0703		| restore saved regs <a2,a1,a0,d1,d0>
	jra	rei_io

nmi_nop:
	moveml	#0xC0C0,sp@-		| save d0,d1,a0,a1
	movw	#CLK_REFR_NMI,CLKADDR+2	| CLKADDR->clk_cmd = CLK_REFR_NMI;
	movc	dfc,a0
	moveq	#FC_MAP,d1
	movc	d1,dfc
	movb	sp@(16),d0	
	eorb	#0xFF,d0
	movsb	d0,0xB			| upper half of sr to leds
	movc	a0,dfc
	moveml	sp@+,#0x0303		| restore regs
#ifdef GPROF
	jsr	kprof
#endif
	rte

/*
 * Called by trap #2 to do an instruction cache flush operation.
 * This is a no-op on all Sun-2's, which have no instruction cache.
 */
	.globl	flush
flush:
	rte

	.globl	syscall, trap, rei
#ifdef STREAMS
	.globl	_qrunflag, _queueflag, _queuerun
#endif
/*
 * Special case for syscall.
 * Everything in line because this is by far the most
 * common interrupt.
 */
syscall:
	subqw	#2,sp			| empty space
	moveml	#0xFFFF,sp@-		| save all regs
	movl	usp,a0			| get usp
	movl	a0,sp@(R_SP)		| save usp
	movl	#syserr,_u+U_LOFAULT	| catch a fault if and when
	movsl	a0@,d0			| get the syscall code
syscont:
	clrl	_u+U_LOFAULT		| clear lofault
	movl	d0,sp@-			| push syscall code
	jsr	_syscall		| go to C routine
	addql	#4,sp			| pop arg
	orw	#SR_INTPRI,sr		| need to test atomicly, rte will lower
	bclr	#AST_STEP_BIT-24,_u+PCB_P0LR | need to single step?
	jne	4$
	bclr	#AST_SCHED_BIT-24,_u+PCB_P0LR | need to reschedule?
	jeq	3$			| no, get out
4$:	bset	#TRACE_AST_BIT-24,_u+PCB_P0LR | say that we're tracing for AST
	jne	3$			| if already doing it, skip
	bset	#SR_TRACE_BIT-8,sp@(R_SR) | set trace mode
	jeq	3$			| if wasn't set, continue
	bset	#TRACE_USER_BIT-24,_u+PCB_P0LR | save fact that trace was set
3$:	movl	sp@(R_SP),a0		| restore user SP
	movl	a0,usp
1$:	moveml	sp@,#0x7FFF		| restore all but SP
	addw	#R_SR,sp		| pop all saved regs
	rte				| and return!

syserr:
	movl	#-1,d0			| set err code
	jra	syscont			| back to mainline

trap:
	jsr	_trap			| Enter C trap routine
	addql	#4,sp			| Pop trap type

/*
 * Return from trap, check for AST's.
 * d0 contains size of info to pop (if any)
 */
rei:
	btst	#SR_SMODE_BIT-8,sp@(R_SR) | SR_SMODE ?
	bne	1$			| skip if system
	orw	#SR_INTPRI,sr		| need to test atomicly, rte will lower
#ifdef STREAMS
	tstb	_qrunflag		| need to run stream queues?
	beq	7f			| no
	tstb	_queueflag		| already running queues?
	bne	7f			| yes
	addqb	#1,_queueflag		| mark that we're running the queues
	jsr	_queuerun		| run the queues
	clrb	_queueflag		| done running queues
	orw	#SR_INTPRI,sr		| need to test atomicly, rte will lower
7:
#endif
	bclr	#AST_STEP_BIT-24,_u+PCB_P0LR | need to single step?
	jne	4$
	bclr	#AST_SCHED_BIT-24,_u+PCB_P0LR | need to reschedule?
	jeq	3$			| no, get out
4$:	bset	#TRACE_AST_BIT-24,_u+PCB_P0LR | say that we're tracing for AST
	bne	3$			| if already doing it, skip
	bset	#SR_TRACE_BIT-8,sp@(R_SR) | set trace mode
	beq	3$			| if wasn't set, continue
	bset	#TRACE_USER_BIT-24,_u+PCB_P0LR | save fact that trace was set
3$:	movl	sp@(R_SP),a0		| restore user SP
	movl	a0,usp
1$:
	tstl	d0			| any cleanup needed?
	beq	2$			| no, skip
	movl	sp,a0			| get current sp
	addw	d0,a0			| pop off d0 bytes of crud
	clrw	a0@(R_VOR)		| dummy VOR
	movl	sp@(R_PC),a0@(R_PC)	| move PC
	movw	sp@(R_SR),a0@(R_SR)	| move SR
	movl	a0,sp@(R_SP)		| stash new sp value
	moveml	sp@,#0xFFFF		| restore all including SP
	addw	#R_SR,sp		| pop all saved regs
	rte				| and return!
2$:
	moveml	sp@,#0x7FFF		| restore all but SP
	addw	#R_SR,sp		| pop all saved regs
	rte				| and return!

/*
 * Return from I/O interrupt, check for AST's.
 */
	.globl	rei_io
rei_iop:
	moveml	sp@+,#0x0707		| pop regs <a2,a1,a0,d2,d1,d0>
rei_io:
	addql	#1,_cnt+V_INTR		| increment io interrupt count
rei_si:
	btst	#SR_SMODE_BIT-8,sp@	| SR_SMODE?  (SR is atop stack)
	jne	3$			| skip if system
	orw	#SR_INTPRI,sr		| need to test atomicly, rte will lower
#ifdef STREAMS
	tstb	_qrunflag		| need to run stream queues?
	beq	7f			| no
	tstb	_queueflag		| already running queues?
	bne	7f			| yes
	addqb	#1,_queueflag		| mark that we're running the queues
	jsr	_queuerun		| run the queues
	clrb	_queueflag		| done running queues
	orw	#SR_INTPRI,sr		| need to test atomicly, rte will lower
7:
#endif
	bclr	#AST_STEP_BIT-24,_u+PCB_P0LR | need to single step?
	jne	4$
	bclr	#AST_SCHED_BIT-24,_u+PCB_P0LR | need to reschedule?
	jeq	3$			| no, get out
4$:	bset	#TRACE_AST_BIT-24,_u+PCB_P0LR | say that we're tracing for AST
	jne	3$			| if already doing it, skip
	bset	#SR_TRACE_BIT-8,sp@	| set trace mode in SR atop stack
	jeq	3$			| if wasn't set, continue
	bset	#TRACE_USER_BIT-24,_u+PCB_P0LR | save fact that trace was set
3$:
	rte				| and return!

/*
 * Handle software interrupts
 */
	.globl	softint
softint:
	moveml	#0xC0E0,sp@-		| save regs we trash <d0,d1,a0,a1,a2>
	movl	sp,a2			| save copy of previous sp
	cmpl	#_etext,sp		| on interrupt stack?
	bgt	1f			| yes, skip
	lea	eintstack,sp		| no, switch to interrupt stack
1:
	movc	dfc,a1
	movl	#FC_MAP,d0		| set to address FC_MAP space
	movc	d0,dfc
3$:
	andw	#~ENA_SOFT_INT_1,enablereg	| clear interrupt request
	movw	enablereg,d0		| get the damn thing in a reg
	movsw	d0,ENABLEREG		| put enable register back
	cmpw	enablereg,d0		| see if someone higher changed it
	bne	3$			| if so, try again
	movc	a1,dfc			| restore dfc
1$:
	jsr	_softint		| Call C
	movl	a2,sp			| restore old sp
	moveml	sp@+,#0x0703		| restore saved regs <a2,a1,a0,d1,d0>
	jra	rei_si

/*
 * Turn on a software interrupt (H/W level 1)..
 */
	ENTRY(siron)
	movc	dfc,a1
	movl	#FC_MAP,d0		| set to address FC_MAP space
	movc	d0,dfc
1$:
	orw	#ENA_SOFT_INT_1,enablereg	| enable the interrupt
	movw	enablereg,d0		| get it in a reg
	movsw	d0,ENABLEREG		| put enable register back
	cmpw	enablereg,d0		| see if someone higher changed it
	bne	1$			| if so, try again
	movc	a1,dfc			| restore dfc
	rts

/*
 * return 1 if an interrupt is being serviced (on interrupt stack),
 * otherwise return 0.
 */
	ENTRY(intsvc)
	clrl	d0			| assume false
	cmpl	#_etext,sp		| on interrupt stack?
	bls	1f			| no, skip
	movl	#1,d0			| return true
1:
	rts

/*
 * Force a panic (and a dump)
 * This is branched to by code in location zero
 */
	.globl	dopanic
dopanic:
	pea	1$
	jsr	_panic
1$:	.asciz	"zero"
	.even

/*
 * Force a stack trace
 * This is branched to by code in location four
 */
	.globl	dotrace
dotrace:
	jsr	_tracedump
	rts

/*
 * Enable and disable DVMA.
 */
	ENTRY(enable_dvma)
	movc	dfc,a1
	movl	#FC_MAP,d0		| set to address FC_MAP space
	movc	d0,dfc
1$:
	orw	#ENA_DVMA,enablereg	| enable DVMA
	movw	enablereg,d0		| get it in a register
	movsw	d0,ENABLEREG		| put enable register back
	cmpw	enablereg,d0		| see if someone higher changed it
	bne	1$			| if so, try again
	movc	a1,dfc			| restore dfc
	rts
	
	ENTRY(disable_dvma)
	movc	dfc,a1
	movl	#FC_MAP,d0		| set to address FC_MAP space
	movc	d0,dfc
1$:
	andw	#~ENA_DVMA,enablereg	| disable DVMA
	movw	enablereg,d0		| get it in a register
	movsw	d0,ENABLEREG		| put enable register back
	cmpw	enablereg,d0		| see if someone higher changed it
	bne	1$			| if so, try again
	movc	a1,dfc			| restore dfc
	rts

	ENTRY(disable_all_interrupts)
	movc	dfc,a1
	movl	#FC_MAP,d0		| set to address FC_MAP space
	movc	d0,dfc
1$:
	andw	#~ENA_INTS,enablereg	| disable DVMA
	movw	enablereg,d0		| get it in a register
	movsw	d0,ENABLEREG		| put enable register back
	cmpw	enablereg,d0		| see if someone higher changed it
	bne	1$			| if so, try again
	movc	a1,dfc			| restore dfc
	rts

/*
 * Transfer data to and from user space
 * Note that these routines can cause faults
 */

| Fetch user byte		_fubyte(address)
	ENTRY2(fubyte,fuibyte)
	movl	#fsuerr,_u+U_LOFAULT	| catch a fault if and when
	movl	sp@(4),a0		| get address
	movsb	a0@,d0			| get the byte
	andl	#0xFF,d0
	clrl	_u+U_LOFAULT		| clear lofault
	rts


| Fetch user (short) word: _fusword(address)
	ENTRY(fusword)
	movl	#fsuerr,_u+U_LOFAULT	| catch a fault if and when
	movl	sp@(4),a0		| get address
	movsw	a0@,d0			| get the word
	andl	#0xFFFF,d0
	clrl	_u+U_LOFAULT		| clear lofault
	rts

| Fetch user (long) word:	_fuword(address)
	ENTRY2(fuword,fuiword)
	movl	#fsuerr,_u+U_LOFAULT	| catch a fault if and when
	movl	sp@(4),a0		| get address
	movsl	a0@,d0			| get the long word
	clrl	_u+U_LOFAULT		| clear lofault
	rts

| Set user byte:	_subyte(address, value)
	ENTRY2(subyte,suibyte)
	movl	#fsuerr,_u+U_LOFAULT	| catch a fault if and when
	movl	sp@(4),a0		| get address
	movl	sp@(8),d1
	movsb	d1,a0@			| set the byte
	clrl	d0			| indicate success
	clrl	_u+U_LOFAULT		| clear lofault
	rts

| Set user short word:	_susword(address, value)
	ENTRY(susword)
	movl	#fsuerr,_u+U_LOFAULT	| catch a fault if and when
	movl	sp@(4),a0		| get address
	movl	sp@(8),d1
	movsw	d1,a0@			| set the word
	clrl	d0			| indicate success
	clrl	_u+U_LOFAULT		| clear lofault
	rts

| Set user word:	_suword(address, value)
	ENTRY2(suword,suiword)
	movl	#fsuerr,_u+U_LOFAULT	| catch a fault if and when
	movl	sp@(4),a0		| get address
	movl	sp@(8),d1
	movsl	d1,a0@			| set the long word
	clrl	d0			| indicate success
	clrl	_u+U_LOFAULT		| clear lofault
	rts

fsuerr:
	movl	#-1,d0			| return error
	clrl	_u+U_LOFAULT		| clear lofault
	rts

/*
 * Copy a null terminated string from the user address space into
 * the kernel address space.
 *
 * copyinstr(udaddr, kaddr, maxlength, lencopied)
 *	caddr_t udaddr, kaddr;
 *	u_int	maxlength, *lencopied;
 */
	ENTRY(copyinstr)
	movl	sp@(4),a0			| a0 = udaddr
	movl	sp@(8),a1			| a1 = kaddr
	movl	sp@(12),d0			| d0 = maxlength
	blt	copystrfault			| if negative size fault
	movl	#copystrfault,_u+U_LOFAULT	| catch a fault if and when
	bra	1f				| enter loop at bottom
0:
	movsb	a0@+,d1				| get a byte
	movb	d1,a1@+				| and stuff away setting CCs
	beq	copystrok			| if '\0' done
1:
	dbra	d0,0b				| decrement and loop

copystrout:
	movl	#ENAMETOOLONG,d0		| ran out of space
	bra	copystrexit

copystrfault:
	movl	#EFAULT,d0			| memory fault or bad size
	bra	copystrexit

copystrok:
	moveq	#0,d0				| all ok	

/*
 * At this point we have:
 *	d0 = return value
 *	a0 = final address of `from' string
 *	sp@(4) = starting address of `from' string
 *	sp@(16) = lencopied pointer (NULL if nothing to be return here)
 */
copystrexit:
	clrl	_u+U_LOFAULT			| clear lofault
	movl	sp@(16),d1			| d1 = lencopied
	beq	2f				| skip if lencopied == NULL
	subl	sp@(4),a0			| compute nbytes moved
	movl	d1,a1
	movl	a0,a1@				| store into *lencopied
2:
	rts

/*
 * Copy a null terminated string from the kernel
 * address space to the user address space.
 *
 * copyoutstr(kaddr, udaddr, maxlength, lencopied)
 *	caddr_t kaddr, udaddr;
 *	u_int	maxlength, *lencopied;
 */
	ENTRY(copyoutstr)
	movl	sp@(4),a0			| a0 = kaddr
	movl	sp@(8),a1			| a1 = udaddr
	movl	sp@(12),d0			| d0 = maxlength
	blt	copystrfault			| if negative size fault
	movl	#copystrfault,_u+U_LOFAULT	| catch a fault if and when
	bra	1f				| enter loop at bottom
0:
	movb	a0@+,d1				| get a byte and set CCs
	movsb	d1,a1@+				| and stuff away
	beq	copystrok			| if '\0' done
1:
	dbra	d0,0b				| decrement and loop
	bra	copystrout			| ran out of space

/*
 * Copy a null terminated string from one point to another in
 * the kernel address space.
 *
 * copystr(kfaddr, kdaddr, maxlength, lencopied)
 *	caddr_t kfaddr, kdaddr;
 *	u_int	maxlength, *lencopied;
 */
	ENTRY(copystr)
	movl	sp@(4),a0			| a0 = kfaddr
	movl	sp@(8),a1			| a1 = kdaddr
	movl	sp@(12),d0			| d0 = maxlength
	blt	copystrfault			| if negative size fault
	bra	1f				| enter loop at bottom
0:
	movb	a0@+,a1@+			| move a byte and set CCs
	beq	copystrok			| if '\0' done
1:
	dbra	d0,0b				| decrement and loop
	bra	copystrout			| ran out of space

/*
 * copyctx - macro used by copyin and copyout to copy
 * between user and supervisor space.  fmov and tmov
 * are the type of move instruction (mov or movs) for
 * the from address and to address respectively.
 */
#define	copyctx(fmov, tmov) \
	movl	#cpctxerr,_u+U_LOFAULT;	/* catch faults */ \
	movl	sp@(4),a0; \
	movl	sp@(8),a1; \
	movl	sp@(12),d0; \
	jle	7$;			/* leave if ridiculous count */ \
/* If from address is odd, move one byte to make it even */ \
	movl	a0,d1; \
	btst	#0,d1; \
	jeq	1$;			/* even, skip */ \
	fmov/**/b	a0@+,d1;	/* move the byte */ \
	tmov/**/b	d1,a1@+; \
	subql	#1,d0;			/* decrement count */ \
/* Now if to address is odd, we have to do byte-by-byte moves */ \
1$:	movl	a1,d1; \
	btst	#0,d1; \
	jne	2$;			/* if odd go do bytes */ \
/* Now both addresses are even and we can do long moves */ \
	rorl	#2,d0;			/* get count as longs */ \
	jra	5$;			/* enter loop at bottom */ \
4$:	fmov/**/l	a0@+,d1;	/* move a long */ \
	tmov/**/l	d1,a1@+; \
5$:	dbra	d0,4$;			/* do until --longcount < 0 */ \
	roll	#2,d0; \
	andl	#3,d0;			/* count %= sizeof (long) */ \
	jra	2$; \
/* \
 * Here for the last 3 bytes or if we have to do byte-by-byte moves \
 * because the pointers were relatively odd \
 */ \
3$:	fmov/**/b	a0@+,d1;	/* move a byte */ \
	tmov/**/b	d1,a1@+; \
2$:	dbra	d0,3$;			/* until --count < 0 */ \
7$:	clrl	d0;			/* indicate success */ \
	clrl	_u+U_LOFAULT; \
	rts; 				/* and return */

cpctxerr:
	movl	#EFAULT,d0		| return error
	clrl	_u+U_LOFAULT		| clear lofault
	rts

	ENTRY(copyin)
	cmpl	#512,sp@(12)
	jgt	_bcopyin
	copyctx(movs, mov)

	ENTRY(copyout)
	cmpl	#512,sp@(12)
	jgt	_bcopyout
	copyctx(mov, movs)

/*
 * fetch user longwords  -- used by syscall -- faster than copyin
 * fulwds(uadd, sadd, nlwds)
 */
	ENTRY(fulwds)
	movl	#fulerr,_u+U_LOFAULT	| catch a fault if and when
	movl	sp@(4),a0		| user address
	movl	sp@(8),a1		| system address
	movl	sp@(12),d0		| number of words
	bras	1$			| enter loop at bottom
2$:	movsl	a0@+,d1			| get longword from user
	movl	d1,a1@+			| set longword in system
1$:	dbra	d0,2$			| loop on count
	clrl	d0			| indicate success
	clrl	_u+U_LOFAULT		| clear lofault
	rts

	/*
	 * if we get an error, we hope it's an address error because we
	 * didn't check for word alignment, and call copyin which
	 * does check (yes this is a kludge, but faster when it works)
	 */
fulerr:
	movl	sp@(12),d0		| get count
	lsll	#2,d0			| 4 bytes per longword
	movl	d0,sp@(12)		| store count
	bra	_copyin			| try copyin

/*
 * Get/Set vector base register
 */
	ENTRY(getvbr)
	movc	vbr,d0
	rts

	ENTRY(setvbr)
	movl	sp@(4),d0
	movc	d0,vbr
	rts

/*
 * Disable monitor NMIs
 */
.data
monnmi:	.long	0
.text
nmivec = 0x7C
	ENTRY(stopnmi)
	movc	vbr,a0
	cmpl	#nmi_nop,a0@(nmivec)
	jeq	1f
	movl	a0@(nmivec),monnmi
	movl	#nmi_nop,a0@(nmivec)
1:	rts

	ENTRY(startnmi)
	tstl	monnmi
	jeq	1f
	movc	vbr,a0
	movl	monnmi,a0@(nmivec)
1:	rts

/*
 * Enter the monitor -- called for console abort
 */
	ENTRY(montrap)
	movl	sp@(4),a0		| address to trap to
	clrw	sp@-			| dummy VOR
	pea	0f			| return address
	movw	sr,sp@-			| current sr
	movc	vbr,a1			| load current vector base register
	movl	monnmi,a1@(nmivec)	| restore monitor NMI
	jra	a0@			| trap to monitor
0:
	movc	vbr,a1			| load current vector base register
	movl	#nmi_nop,a1@(nmivec)	| disable monitor NMI
	rts

/*
 * Read the stupid ID prom
 */
	ENTRY(getidprom)
	movl	sp@(4),a0	| address to save 32 bytes
	movl	d2,sp@-		| save a reg
	movc	sfc,d0		| save source func code
	movl	#FC_MAP,d1
	movc	d1,sfc		| set space 3
	movl	#8,a1		| select id prom
	movl	#31,d1		| 32 byte loop
1$:	movsb	a1@,d2		| get a byte
	movb	d2,a0@+		| save it
	addl	#0x800,a1	| address next byte
	dbra	d1,1$		| and loop
	movc	d0,sfc		| restore sfc
	movl	sp@+,d2		| restore d2
	rts

/*
 * Read the bus error register
 */
	ENTRY(getbuserr)
	movc	sfc,a0			| save sfc
	movl	#FC_MAP,d0		| set to address FC_MAP space
	movc	d0,sfc
	movsw	BUSERRREG,d0		| get the buserr register
	movc	a0,sfc			| restore sfc
	rts

/*
 * Define some variables used by post-modem debuggers
 * to help them work on kernels with changing structures.
 */
	.globl UPAGES_DEBUG, KERNELBASE_DEBUG, VADDR_MASK_DEBUG
	.globl PGSHIFT_DEBUG, SLOAD_DEBUG

UPAGES_DEBUG		= UPAGES
KERNELBASE_DEBUG	= KERNELBASE
VADDR_MASK_DEBUG	= 0x00ffffff
PGSHIFT_DEBUG		= PGSHIFT
SLOAD_DEBUG		= SLOAD
