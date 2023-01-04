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

#include "../machine/asm_linkage.h"
#include "../machine/buserr.h"
#include "../machine/clock.h"
#include "../machine/cpu.h"
#include "../machine/diag.h"
#include "../machine/enable.h"
#include "../machine/interreg.h"
#include "../machine/memerr.h"
#include "../machine/mmu.h"
#include "../machine/pcb.h"
#include "../machine/psl.h"
#include "../machine/pte.h"
#include "../machine/reg.h"
#include "../machine/trap.h"
#include "fpa.h"

#include "assym.s"

/* 
 * Absolute external symbols
 */
	.globl	_msgbuf, _scb, _u, _DVMA
/*
 * On the sun3 we put the message buffer, the scb and the pte's
 * for proc 0 on the same page, since we have such a large page size.
 * We set things up so that the first page of KERNELBASE is illegal
 * to act as a redzone during copyin/copyout type operations.  One of
 * the reasons the message buffer is allocated in low memory to
 * prevent being overwritten during booting operations (besides
 * the fact that it is small enough to share a page with others).
 */
_msgbuf	= KERNELBASE + NBPG	| base addr for message buffer
_scb	= _msgbuf + MSGBUFSIZE	| base addr for vector table
_u	= UADDR			| 32 bit virtual address for u-area
_DVMA	= 0x0FF00000		| 28 bit virtual address for system DVMA

#if (MSGBUFSIZE + SCBSIZE) > (NBPG - (4 * UPAGES))
ERROR - msgbuf and scb larger than a page minus pte space
#endif


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
 * code assumes that the interrupt stack is at a lower address than
 * both eintstack and the kernel stack in the u area.
 */
#define	MIN_INTSTACK_SZ	0x800
	.data
	.align 4
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
	SYSMAP(_Sysmap   ,_Sysbase	,SYSPTSIZE	)
	SYSMAP(_Usrptmap ,_usrpt 	,USRPTSIZE	)
	SYSMAP(_Forkmap  ,_forkutl	,UPAGES		)
	SYSMAP(_Xswapmap ,_xswaputl	,UPAGES		)
	SYSMAP(_Xswap2map,_xswap2utl	,UPAGES		)
	SYSMAP(_Swapmap  ,_swaputl	,UPAGES		)
	SYSMAP(_Pushmap  ,_pushutl	,UPAGES		)
	SYSMAP(_Vfmap    ,_vfutl	,UPAGES		)
	SYSMAP(_CMAP1    ,_CADDR1	,1		) | local tmp
	SYSMAP(_CMAP2    ,_CADDR2	,1		) | local tmp
	SYSMAP(_mmap     ,_vmmap        ,1		)
	SYSMAP(_Mbmap    ,_mbutl        ,NMBCLUSTERS*CLSIZE)
	SYSMAP(_ESysmap	 ,_Syslimit	,0		) | must be last

	.globl  _Syssize
_Syssize = (_ESysmap-_Sysmap)/4

/*
 * Software copy of system enable register
 * This is always atomically updated
 */
	.data
	.globl	_enablereg
_enablereg:	.byte	0			| UNIX's system enable register
	.even
	.text

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
 * UNIX receives control at the label `_start' which
 * must be at offset zero in this file; this file must
 * be the first thing in the boot image.
 */
	.text
	.globl	_start
_start:
/*
 * We should reset the world here, but it screws the UART settings.
 *
 * Do a halfhearted job of setting up the mmu so that we can run out
 * of the high address space.  We do this by reading the current pmegs
 * for the `real' locations and using them for the virtual relocation.
 * NOTE - Assumes that the real and virtual locations have the same
 * segment offsets from 0 and KERNELBASE!!!
 *
 * We make the following assumptions about our environment
 * as set up by the monitor:
 *
 *	- we have enough memory mapped for the entire kernel + some more
 *	- all pages are writable
 *	- the last pmeg [SEGINV] has no valid pme's
 *	- the highest virtual segment has a pmeg allocated to it
 *	- when the monitor's romp->v_memorybitmap points to a zero
 *	    - each low segment i is mapped to use pmeg i
 *	    - each page map entry i maps physical page i
 *	- the monitor's scb is NOT in low memory
 *	- on systems w/ ecc memory, that the monitor has set the base
 *	    addresses and enabled all the memory cards correctly
 *
 * We will set the protection properly in startup().
 */

/*
 * Before we set up the new mapping and start running with the correct
 * addresses, all of the code must be carefully written to be position
 * independent code, since we are linked for running out of high addresses,
 * but we get control running in low addresses.  We continue to run
 * off the stack set up by the monitor until after we set up the u area.
 */
	movw	#SR_HIGH,sr		| lock out interrupts
	moveq	#FC_MAP,d0
	movc	d0,sfc			| set default sfc to FC_MAP
	movc	d0,dfc			| set default dfc to FC_MAP
	moveq	#KCONTEXT,d0	
	movsb	d0,CONTEXTBASE		| now running in KCONTEXT

leax:	lea	pc@(_start-(leax+2)),a2	| a2 = true current location of _start
	movl	a2,d2			| real start address
	andl	#SEGMENTADDRBITS,d2	| clear extraneous bits
	orl	#SEGMENTBASE,d2		| set to segment map offset
	movl	d2,a2			| a2 = real adddress map pointer

	movl	#_start,d3		| virtual start address
	andl	#SEGMENTADDRBITS,d3	| clear extraneous bits
	orl	#SEGMENTBASE,d3		| set to segment map offset
	movl	d3,a3			| a3 = virtual address map pointer

/*
 * Compute the number used to control the dbra loop.
 * By doing ((end - 1) - KERNELBASE) >> SGSHIFT we
 * essentially get ctos(btoc(end - KERNELBASE)) - 1
 * where the - 1 is adjustment for the dbra loop.
 */
	movl	#_end-1,d1		| get virtual end
	subl	#KERNELBASE,d1		| subtract off base address
	movl	#SGSHIFT,d0		| load up segment shift value
	lsrl	d0,d1			| d1 = # of segments to map - 1

/*
 * Now loop through the real addresses where we are loaded and set
 * up the virtual segments for where we want to be virtually to be the same.
 */
0:
	movsb	a2@,d0			| get real segno value
	movsb	d0,a3@			| set virtual segno value
	addl	#NBSG,a2		| bump real address map pointer
	addl	#NBSG,a3		| bump virtual address map pointer
	dbra	d1,0b			| decrement count and loop

	movl	#CACHE_CLEAR+CACHE_ENABLE,d0
	movc	d0,cacr			| clear (and enable) the cache

	jmp	cont:l			| force non-PC rel branch
cont:

/*
 * PHEW!  Now we are running with correct addresses
 * and can use non-position independent code.
 */

/*
 * Now map in our own copies of the eeprom, clock, memory error
 * register, interrupt control register, and ecc regs into the
 * last virtual segment which already has a pmeg allocated to it
 * when we get control from the monitor.
 */
	lea	EEPROM_ADDR_MAPVAL,a0	| map in eeprom
	movl	#EEPROM_ADDR_PTE,d0
	movsl	d0,a0@

	lea	CLKADDR_MAPVAL,a0	| map in clock
	movl	#CLKADDR_PTE,d0
	movsl	d0,a0@

	lea	MEMREG_MAPVAL,a0	| map in memory error register
	movl	#MEMREG_PTE,d0
	movsl	d0,a0@

	lea	INTERREG_MAPVAL,a0	| map in interrupt control reg
	movl	#INTERREG_PTE,d0
	movsl	d0,a0@

	lea	ECCREG_MAPVAL,a0	| map in ecc regs
	movl	#ECCREG_PTE,d0
	movsl	d0,a0@

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
	jsr	_halt
	addqw	#4,sp			| in case they really want to try this
	.data
0:	.asciz "Memory bad"
	.even
	.text
1:

/*
 * Set up mapping for the u page now, using the physical page 0.
 * Assumes that UPAGES == 1.  This code and uinit() are closely related.
 */
#if UPAGES != 1
ERROR - "UPAGES != 1"
#endif
	lea	U_MAPVAL,a0		| address needed for u area pte
	movl	#(PG_V+PG_S+PG_W+0),d0	| pte for sys writable phys page 0
	movsl	d0,a0@			| write page map entry

| zero user page
	movl	#(NBPG/4)-1,d0		| dbra long count
	lea	_u,a0			| start at _u
0:	clrl	a0@+			| clear long word and increment
	dbra	d0,0b			| decrement count and loop

| zero scb to start (NOTE - depends on msgbuf / scb layout above)
	movl	#((NBPG-MSGBUFSIZE)/4)-1,d0| dbra long count
	lea	_scb,a0			| starting address
0:	clrl	a0@+			| clear long word and increment
	dbra	d0,0b			| decrement count and loop

| zero bss
	movl	#_end,d0		| get bss end
	lea	_edata,a0		| get bss start
	subl	a0,d0			| get bss length
	lsrl	#2,d0			| shift for long count
0:	clrl	a0@+			| clear long word and increment
	dbra	d0,0b			| decrement count and loop

/*
 * Set up the stack.  From now we continue to use the 68020 ISP
 * (interrupt stack pointer).  This is because the MSP (master
 * stack pointer) as implemented by Motorola is too painful to
 * use since we have to play lots of games and add extra tests
 * to set something in the master stack if we running on the
 * interrupt stack and we are about to pop off a throw away
 * stack frame.
 *
 * Thus it is possible to having naming conflicts.  In general,
 * when the term "interrupt stack" (no pointer) is used, it
 * is referring to the software implemented interrupt stack
 * and the "kernel stack" is the per user kernel stack in the
 * user area.  We handling switching between the two different
 * address ranges upon interrupt entry/exit.  We will use ISP
 * and MSP if we are referring to the hardstack stack pointers.
 */
	lea	_u+U_STACK+KERNSTACK,sp | set to top of kernel stack

	/*
	 * See if we have a 68881 attached.
	 * _fppstate is 0 if no fpp,
	 * 1 if fpp is present and enabled,
	 * and -1 if fpp is present but disabled
	 * (not currently used).
	 */
	.data
	.globl	_fppstate
_fppstate:	.word 1		| mark as present until we find out otherwise
	.text

flinevec = 0x2c
	movsb	ENABLEREG,d0		| get the current enable register
	orb	#ENA_FPP,d0		| or in FPP enable bit
	movsb	d0,ENABLEREG		| set in the enable register
	movl	sp,a1			| save sp in case of fline fault
	movc	vbr,a0			| get vbr
	movl	a0@(flinevec),d1	| save old f line trap handler
	movl	#ffault,a0@(flinevec)	| set up f line handler
	frestore fnull
	jra	1f

fnull:	.long	0			| null fpp internal state

ffault:					| handler for no fpp present
	movw	#0,_fppstate		| set global to say no fpp
	andb	#~ENA_FPP,d0		| clear ENA_FPP enable bit
	movl	a1,sp			| clean up stack

1:
	movl	d1,a0@(flinevec)	| restore old f line trap handler
	movsb	d0,ENABLEREG		| set up enable reg
	movb	d0,_enablereg		| save soft copy of enable register

| dummy up a stack so process 1 can find saved registers
	movl	#USRSTACK,a0		| init user stack pointer
	movl	a0,usp
	clrw	sp@-			| dummy fmt & vor
	movl	#USRTEXT,sp@-		| push pc
	movw	#SR_USER,sp@-		| push sr
	lea	0,a6			| stack frame link 0 in main
| invoke main, we will return as process 1 (init)
	SAVEALL()
	jsr	_main			| simulate interrupt -> main
	clrl	d0			| fake return value from trap
	jra	rei

/*
 * Entry points for interrupt and trap vectors
 */
	.globl	buserr, addrerr, coprocerr, fmterr, illinst, zerodiv, chkinst
	.globl	trapv, privvio, trace, emu1010, emu1111, spurious
	.globl	badtrap, brkpt, floaterr, level2, level3, level4, level5
	.globl	_level7, errorvec

buserr:
	TRAP(T_BUSERR)

addrerr:
	TRAP(T_ADDRERR)

coprocerr:
	TRAP(T_COPROCERR)

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

floaterr:
	TRAP(T_M_FLOATERR)

errorvec:
	TRAP(T_M_ERRORVEC)

level2:
	IOINTR(2)

level3:
	IOINTR(3)

level4:
	IOINTR(4)

	.data
	.globl	_ledcnt
ledpat:
	.byte	~0x80
	.byte	~0x40
	.byte	~0x20
	.byte	~0x10
	.byte	~0x08
	.byte	~0x04
	.byte	~0x02
	.byte	~0x01
	.byte	~0x02
	.byte	~0x04
	.byte	~0x08
	.byte	~0x10
	.byte	~0x20
	.byte	~0x40
endpat:
	.even
ledptr:		.long	ledpat
flag5:		.word	0
flag7:		.word	0
_ledcnt:	.word	50		| once per second min LED update rate
ledcnt:		.word	0
	.text

/*
 * This code assumes that the real time clock interrupts 100 times
 * a second and that we want to only call hardclock 50 times/sec
 * We update the LEDs with new values so at least a user can tell
 * that something it still running before calling hardclock().
 */
level5:					| default clock interrupt
	tstb	CLKADDR+CLK_INTRREG	| read CLKADDR->clk_intrreg to clear
	andb	#~IR_ENA_CLK5,INTERREG	| clear interrupt request
	orb	#IR_ENA_CLK5,INTERREG	| and re-enable
	tstb	CLKADDR+CLK_INTRREG	| clear interrupt register again,
					| if we lost interrupt we will
					| resync later anyway.
| for 100 hz operation, comment out from here ...
	notw	flag5			| toggle flag
	jeq	0f			| if result zero skip ahead
	rte
0:
| ... to here
	moveml	#0xC0E0,sp@-		| save d0,d1,a0,a1,a2

	movl	sp,a2			| save copy of previous sp
	cmpl	#eintstack,sp		| on interrupt stack?
	jls	1f			| yes, skip
	lea	eintstack,sp		| no, switch to interrupt stack
1:

			/* check for LED update */
	movl	a2@(5*4+2),a1		| get saved pc
	cmpl	#idle,a1		| were we idle?
	beq	0f			| yes, do LED update
	subqw	#1,ledcnt
	bge	2f			| if positive skip LED update
0:
	movw	_ledcnt,ledcnt		| reset counter
	movl	ledptr,a0		| get pointer
	movb	a0@+,d0			| get next byte
	cmpl	#endpat,a0		| are we at the end?
	bne	1f			| if not, skip
	lea	ledpat,a0		| reset pointer
1:
	movl	a0,ledptr		| save pointer
	movsb	d0,DIAGREG		| d0 to diagnostic LEDs

2:					| call hardclock
	movw	a2@(5*4),d0		| get saved sr
	movl	d0,sp@-			| push it as a long
	movl	a1,sp@-			| push saved pc
	jsr	_hardclock		| call UNIX routine
	movl	a2,sp			| restore old sp
	moveml	sp@+,#0x0703		| restore all saved regs
	jra	rei_io			| all done

/*
 * Level 7 interrupts can be caused by parity/ECC errors or the
 * clock chip.  The clock chip is tied to level 7 interrupts
 * only if we are profiling.  Because of the way nmi's work,
 * we clear any level 7 clock interrupts first before
 * checking the memory error register.
 */
_level7:
#ifdef GPROF
	tstb	CLKADDR+CLK_INTRREG	| read CLKADDR->clk_intrreg to clear
	andb	#~IR_ENA_CLK7,INTERREG	| clear interrupt request
	orb	#IR_ENA_CLK7,INTERREG	| and re-enable
#endif GPROF

	moveml	#0xC0C0,sp@-		| save C regs
	movb	MEMREG,d0		| read memory error register
	andb	#ER_INTR,d0		| a parity/ECC interrupt pending?
	jeq	0f			| if not, jmp
	jsr	_memerr			| dump memory error info
	/*MAYBE REACHED*/		| if we do return to here, then
	jra	1f			| we had a non-fatal memory problem
0:

#ifdef GPROF
| for 100 hz profiling, comment out from here ...
	notw	flag7			| toggle flag
	jne	1f			| if result non-zero return
| ... to here
	jsr	kprof			| do the profiling
#else GPROF
	pea	0f			| push message printf
	jsr	_printf			| print the message
	addqw	#4,sp			| pop argument
	.data
0:	.asciz	"stray level 7 interrupt\012"
	.even
	.text
#endif GPROF
1:
	moveml	sp@+,#0x0303		| restore regs
	rte

/*
 * Called by trap #2 to do an instruction cache flush operation
 */
	.globl	flush
flush:
	movl	#CACHE_CLEAR+CACHE_ENABLE,d0
	movc	d0,cacr			| clear (and enable) the cache
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
	movl	a0@,d0			| get the syscall code
syscont:
	clrl	_u+U_LOFAULT		| clear lofault
	movl	d0,sp@-			| push syscall code
	jsr	_syscall		| go to C routine
	addqw	#4,sp			| pop arg
	orw	#SR_INTPRI,sr		| need to test atomicly, rte will lower
	bclr	#AST_STEP_BIT-24,_u+PCB_P0LR | need to single step?
	jne	4f
	bclr	#AST_SCHED_BIT-24,_u+PCB_P0LR | need to reschedule?
	jeq	3f			| no, get out
4:	bset	#TRACE_AST_BIT-24,_u+PCB_P0LR | say that we're tracing for AST
	jne	3f			| if already doing it, skip
	bset	#SR_TRACE_BIT-8,sp@(R_SR) | set trace mode
	jeq	3f			| if wasn't set, continue
	bset	#TRACE_USER_BIT-24,_u+PCB_P0LR | save fact that trace was set
3:	
#if NFPA > 0
	tstl	_u+U_FPA_FLAGS		| to check u.u_fpa_flags
	beq	2f			| FPA is not used, skip
	movl	_u+U_FPA_PC,d0		| saved user PC on FPA exception
	cmpl	sp@(R_PC),d0		| if equal, should rte to fpa user code
	bne	2f			| not equal, skip the following
	clrl	_u+U_FPA_PC		| clear u.u_fpa_pc
					| sp points to d0 of kernel stack
	lea	sp@(R_KSTK),a0		| a0 points to bottom of kernel stack
					| (a short beyond <0000, voffset>)
	movl	_u+U_FPA_FMTPTR,a1	| a1 points to saved area
	movw	a1@+,d0			| d0 has the size to restore
	extl	d0			| make it a long
	subl	d0,a0			| a0 points to top of new kernel stack
					| (d0), also (to) in dbra loop below
	addql	#2,a1			| a1 points to saved AST bits
| load back upper 5 bits of u.u_pcb.pcb_p0lr to be consistent w/ saved SR
	movl	_u+PCB_P0LR,d1		| get current p0lr
	andl	#NOAST_P0LR,d1		| clear upper 5 bits
	orl	a1@+,d1			| oring saved upper 5 bits; a1=+4
	movl	d1,_u+PCB_P0LR		| set p0lr

					| a1 points to saved d0
	movl	a0,sp			| sp points to top of new kernel stack
	asrl	#1,d0			| d0 gets (short count)
0:	movw	a1@+,a0@+		| move an short
	dbra	d0,0b			| decrement (short count) and loop

2:
#endif NFPA > 0
	movl	sp@(R_SP),a0		| restore user SP
	movl	a0,usp
	movl	#CACHE_CLEAR+CACHE_ENABLE,d0
	movc	d0,cacr			| clear (and enable) the cache
1:
	moveml	sp@,#0x7FFF		| restore all but SP
	addw	#R_SR,sp		| pop all saved regs
	rte				| and return!

syserr:
	movl	#-1,d0			| set err code
	jra	syscont			| back to mainline

/*
 * We reset the sfc and dfc to FC_MAP in case we came in from
 * a trap while in the monitor since the monitor uses movs
 * instructions after dorking w/ sfc and dfc during its operation.
 */
trap:
	moveq	#FC_MAP,d0
	movc	d0,sfc	
	movc	d0,dfc
	jsr	_trap			| Enter C trap routine
	addqw	#4,sp			| Pop trap type

/*
 * Return from interrupt or trap, check for AST's.
 * d0 contains the size of info to pop (if any)
 */
rei:
	btst	#SR_SMODE_BIT-8,sp@(R_SR) | SR_SMODE ?
	bne	1f			| skip if system
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
	jne	4f
	bclr	#AST_SCHED_BIT-24,_u+PCB_P0LR | need to reschedule?
	jeq	3f			| no, get out
4:	bset	#TRACE_AST_BIT-24,_u+PCB_P0LR | say that we're tracing for AST
	bne	3f			| if already doing it, skip
	bset	#SR_TRACE_BIT-8,sp@(R_SR) | set trace mode
	jeq	3f			| if wasn't set, continue
	bset	#TRACE_USER_BIT-24,_u+PCB_P0LR | save fact that trace was set
3:	movl	sp@(R_SP),a0		| restore user SP
	movl	a0,usp
	movl	#CACHE_CLEAR+CACHE_ENABLE,a0
	movc	a0,cacr			| clear (and enable) the cache
1:
	tstl	d0			| any cleanup needed?
	beq	2f			| no, skip
	movl	sp,a0			| get current sp
	addw	d0,a0			| pop off d0 bytes of crud
	clrw	a0@(R_VOR)		| dummy VOR
	movl	sp@(R_PC),a0@(R_PC)	| move PC
	movw	sp@(R_SR),a0@(R_SR)	| move SR
	movl	a0,sp@(R_SP)		| stash new sp value
	moveml	sp@,#0xFFFF		| restore all including SP
	addw	#R_SR,sp		| pop all saved regs
	rte				| and return!
2:
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
	jne	3f			| skip if system
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
	jne	4f
	bclr	#AST_SCHED_BIT-24,_u+PCB_P0LR | need to reschedule?
	jeq	3f			| no, get out
4:	bset	#TRACE_AST_BIT-24,_u+PCB_P0LR | say that we're tracing for AST
	jne	3f			| if already doing it, skip
	bset	#SR_TRACE_BIT-8,sp@	| set trace mode in SR atop stack
	jeq	3f			| if wasn't set, continue
	bset	#TRACE_USER_BIT-24,_u+PCB_P0LR | save fact that trace was set
3:
	rte				| and return!

/*
 * Handle software interrupts
 * Just call C routine
 */
	.globl	softint
softint:
	moveml	#0xC0E0,sp@-		| save regs we trash <d0,d1,a0,a1,a2>
	movl	sp,a2			| save copy of previous sp
	cmpl	#eintstack,sp		| on interrupt stack?
	jls	0f			| yes, skip
	lea	eintstack,sp		| no, switch to interrupt stack
0:
	bclr    #IR_SOFT_INT1_BIT,INTERREG| clear interrupt request
	jsr	_softint		| Call C
	movl	a2,sp			| restore old sp
	moveml	sp@+,#0x0703		| restore saved regs <a2,a1,a0,d1,d0>
	jra	rei_si

/*
 * Turn on a software interrupt (H/W level 1).
 */
	ENTRY(siron)
	bset	#IR_SOFT_INT1_BIT,INTERREG | trigger level 1 intr
	rts

/*
 * return 1 if an interrupt is being serviced (on interrupt stack),
 * otherwise return 0.
 */
	ENTRY(intsvc)
	clrl	d0			| assume false
	cmpl	#eintstack,sp		| on interrupt stack?
	bhi	1f			| no, skip
	movl	#1,d0			| return true
1:
	rts

/*
 * Enable and disable DVMA.
 */
	ENTRY(enable_dvma)
1:
	orb	#ENA_SDVMA,_enablereg	| enable System DVMA
	movb	_enablereg,d0		| get it in a register
	movsb	d0,ENABLEREG		| put enable register back
	cmpb	_enablereg,d0		| see if someone higher changed it
	bne	1b			| if so, try again
	rts
	
	ENTRY(disable_dvma)
1:
	andb	#~ENA_SDVMA,_enablereg	| disable System DVMA
	movb	_enablereg,d0		| get it in a register
	movsb	d0,ENABLEREG		| put enable register back
	cmpb	_enablereg,d0		| see if someone higher changed it
	bne	1b			| if so, try again
	rts

/*
 * Transfer data to and from user space -
 * Note that these routines can cause faults
 * It is assumed that the kernel has nothing at
 * less than KERNELBASE in the virtual address space.
 */

| Fetch user byte		_fubyte(address)
	ENTRY2(fubyte,fuibyte)
	movl	#fsuerr,_u+U_LOFAULT	| catch a fault if and when
	movl	sp@(4),a0		| get address
	cmpl	#KERNELBASE-1,a0	| check address range
	jhi	fsuerr			| jmp if greater than
	movb	a0@,d0			| get the byte
	andl	#0xFF,d0
	clrl	_u+U_LOFAULT		| clear lofault
	rts


| Fetch user (short) word:	 _fusword(address)
	ENTRY(fusword)
	movl	#fsuerr,_u+U_LOFAULT	| catch a fault if and when
	movl	sp@(4),a0		| get address
	cmpl	#KERNELBASE-2,a0	| check address range
	jhi	fsuerr			| jmp if greater than
	movw	a0@,d0			| get the word
	andl	#0xFFFF,d0
	clrl	_u+U_LOFAULT		| clear lofault
	rts

| Fetch user (long) word:	_fuword(address)
	ENTRY2(fuword,fuiword)
	movl	#fsuerr,_u+U_LOFAULT	| catch a fault if and when
	movl	sp@(4),a0		| get address
	cmpl	#KERNELBASE-4,a0	| check address range
	jhi	fsuerr			| jmp if greater than
	movl	a0@,d0			| get the long word
	clrl	_u+U_LOFAULT		| clear lofault
	rts

| Set user byte:		_subyte(address, value)
	ENTRY2(subyte,suibyte)
	movl	#fsuerr,_u+U_LOFAULT	| catch a fault if and when
	movl	sp@(4),a0		| get address
	cmpl	#KERNELBASE-1,a0	| check address range
	jhi	fsuerr			| jmp if greater than
	movb	sp@(8+3),a0@		| set the byte
	clrl	d0			| indicate success
	clrl	_u+U_LOFAULT		| clear lofault
	rts

| Set user short word:		_susword(address, value)
	ENTRY(susword)
	movl	#fsuerr,_u+U_LOFAULT	| catch a fault if and when
	movl	sp@(4),a0		| get address
	cmpl	#KERNELBASE-2,a0	| check address range
	jhi	fsuerr			| jmp if greater than
	movw	sp@(8+2),a0@		| set the word
	clrl	d0			| indicate success
	clrl	_u+U_LOFAULT		| clear lofault
	rts

| Set user (long) word:		_suword(address, value)
	ENTRY2(suword,suiword)
	movl	#fsuerr,_u+U_LOFAULT	| catch a fault if and when
	movl	sp@(4),a0		| get address
	cmpl	#KERNELBASE-4,a0	| check address range
	jhi	fsuerr			| jmp if greater than
	movl	sp@(8+0),a0@		| set the long word
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
	cmpl	#KERNELBASE,a0
	jcc	copystrfault			| jmp if udaddr >= KERNELBASE
	movl	sp@(8),a1			| a1 = kaddr
	movl	sp@(12),d0			| d0 = maxlength
	jlt	copystrfault			| if negative size fault
	movl	#copystrfault,_u+U_LOFAULT	| catch a fault if and when
	jra	1f				| enter loop at bottom
0:
	movb	a0@+,a1@+			| move a byte and set CC's
	jeq	copystrok			| if '\0' done
1:
	dbra	d0,0b				| decrement and loop

copystrout:
	movl	#ENAMETOOLONG,d0		| ran out of space
	jra	copystrexit

copystrfault:
	movl	#EFAULT,d0			| memory fault or bad size
	jra	copystrexit

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
	jeq	2f				| skip if lencopied == NULL
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
	cmpl	#KERNELBASE,a1
	jcc	copystrfault			| jmp if udaddr >= KERNELBASE
	movl	sp@(12),d0			| d0 = maxlength
	jlt	copystrfault			| if negative size fault
	movl	#copystrfault,_u+U_LOFAULT	| catch a fault if and when
	jra	1f				| enter loop at bottom
0:
	movb	a0@+,a1@+			| move a byte and set CCs
	jeq	copystrok			| if '\0' done
1:
	dbra	d0,0b				| decrement and loop
	jra	copystrout			| ran out of space

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
	jlt	copystrfault			| if negative size fault
	jra	1f				| enter loop at bottom
0:
	movb	a0@+,a1@+			| move a byte and set CCs
	jeq	copystrok			| if '\0' done
1:
	dbra	d0,0b				| decrement and loop
	jra	copystrout			| ran out of space

/*
 * copyout() - except for _u+U_LOFAULT and address
 * checking this is just bcopy().
 */
	ENTRY(copyout)
	movl	#cpctxerr,_u+U_LOFAULT	| catch faults
	movl	sp@(4),a0
	movl	sp@(8),a1
	movl	sp@(12),d0
	jle	7f			| leave if ridiculous count
copyoutcheck:
	cmpl	#KERNELBASE,a1		| check starting address
	jcc	cpctxerr		| jmp on error (>= unsigned)
/* If from address is odd, move one byte to make it even */
	movl	a0,d1
	btst	#0,d1
	jeq	1f			| even, skip
	movb	a0@+,a1@+		| move the byte
	subql	#1,d0			| decrement count
/* Now if to address is odd, we have to do byte-by-byte moves */
1:	movl	a1,d1
	btst	#0,d1
	jne	2f			| if odd go do bytes
/* Now both addresses are even and we can do long moves */
	rorl	#2,d0			| get count as longs
	jra	5f			| enter loop at bottom
4:	movl	a0@+,a1@+		| move a long
5:	dbra	d0,4b			| do until --longcount < 0
	roll	#2,d0
	andl	#3,d0			| count %= sizeof (long)
	jra	2f
/*
 * Here for the last 3 bytes or if we have to do byte-by-byte moves
 * because the pointers were relatively odd
 */
3:	movb	a0@+,a1@+		| move a byte
2:	dbra	d0,3b			| until --count < 0
7:	clrl	d0			| indicate success
	clrl	_u+U_LOFAULT
	rts				| and return

/*
 * copyin() - except for _u+U_LOFAULT and address
 * checking this is just bcopy().
 */
	ENTRY(copyin)
	movl	#cpctxerr,_u+U_LOFAULT	| catch faults
	movl	sp@(4),a0
	movl	sp@(8),a1
	movl	sp@(12),d0
	jle	7f			| leave if ridiculous count
copyincheck:
	cmpl	#KERNELBASE,a0		| check starting address
	jcc	cpctxerr		| jmp on error (>= unsigned)
/* If from address is odd, move one byte to make it even */
	movl	a0,d1
	btst	#0,d1
	jeq	1f			| even, skip
	movb	a0@+,a1@+		| move the byte
	subql	#1,d0			| decrement count
/* Now if to address is odd, we have to do byte-by-byte moves */
1:	movl	a1,d1
	btst	#0,d1
	jne	2f			| if odd go do bytes
/* Now both addresses are even and we can do long moves */
	rorl	#2,d0			| get count as longs
	jra	5f			| enter loop at bottom
4:	movl	a0@+,a1@+		| move a long
5:	dbra	d0,4b			| do until --longcount < 0
	roll	#2,d0
	andl	#3,d0			| count %= sizeof (long)
	jra	2f
/*
 * Here for the last 3 bytes or if we have to do byte-by-byte moves
 * because the pointers were relatively odd
 */
3:	movb	a0@+,a1@+		| move a byte
2:	dbra	d0,3b			| until --count < 0
7:	clrl	d0			| indicate success
	clrl	_u+U_LOFAULT
	rts				| and return

cpctxerr:
	movl	#EFAULT,d0		| return error
	clrl	_u+U_LOFAULT		| clear lofault
	rts

/*
 * fetch user longwords  -- used by syscall -- faster than copyin
 * Doesn't worry about alignment of transfer, let the 68020 worry
 * about that - we won't be doing more than 8 long words anyways.
 * fulwds(uadd, sadd, nlwds)
 */
	ENTRY(fulwds)
	movl	#cpctxerr,_u+U_LOFAULT	| catch a fault if and when
	movl	sp@(4),a0		| user address
	movl	sp@(8),a1		| system address
	movl	sp@(12),d0		| number of words
	cmpl	#KERNELBASE,a0		| check starting address
	jcs	1f			| enter loop at bottom if < unsigned
	jra	cpctxerr		| error
0:	movl	a0@+,a1@+		| get longword
1:	dbra	d0,0b			| loop on count
	clrl	d0			| indicate success
	clrl	_u+U_LOFAULT		| clear lofault
	rts

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
 * Enter the monitor -- called for console abort
 */
	ENTRY(montrap)
	jsr	_startnmi		| enable monitor nmi routine
	movl	sp@(4),a0		| address to trap to
	clrw	sp@-			| dummy VOR
	pea	0f			| return address
	movw	sr,sp@-			| current sr
	jra	a0@			| trap to monitor
0:
	jsr	_stopnmi		| disable monitor nmi routine
	rts

/*
 * Read the ID prom.  This is mapped from IDPROMBASE for IDPROMSIZE
 * bytes in the FC_MAP address space for byte access only.  Assumes
 * that the sfc has already been set to FC_MAP.
 */
	ENTRY(getidprom)
	movl	sp@(4),a0		| address to copy bytes to
	lea	IDPROMBASE,a1		| select id prom
	movl	#(IDPROMSIZE-1),d1	| byte loop counter
0:	movsb	a1@+,d0			| get a byte
	movb	d0,a0@+			| save it
	dbra	d1,0b			| and loop
	rts

/*
 * Enable and disable video.
 */
	ENTRY(setvideoenable)
1:	
	tstl	sp@(4)			| is bit on or off
	jeq	2f
	orb	#ENA_VIDEO,_enablereg	| enable video
	jra	3f
2:
	andb	#~ENA_VIDEO,_enablereg	| disable video
3:
	movb	_enablereg,d0		| get it in a register
	movsb	d0,ENABLEREG		| put enable register back
	cmpb	_enablereg,d0		| see if someone higher changed it
	bne	1b			| if so, try again
	rts

/*
 * Enable and disable video Copy.
 */
	ENTRY(setcopyenable)
1:	
	tstl	sp@(4)			| is bit on or off
	jeq	2f
	orb	#ENA_COPY,_enablereg	| enable video copy
	jra	3f
2:
	andb	#~ENA_COPY,_enablereg	| disable copy
3:
	movb	_enablereg,d0		| get it in a register
	movsb	d0,ENABLEREG		| put enable register back
	cmpb	_enablereg,d0		| see if someone higher changed it
	bne	1b			| if so, try again
	rts

/*
 * Enable and disable video interrupt.
 */
	ENTRY(setintrenable)
	tstl	sp@(4)			| is bit on or off
	jeq	1f
	orb	#IR_ENA_VID4,INTERREG	| enable video interrupt
	rts
1:	
        jbsr    _spl4                   | disable this level to avoid race
        andb    #~IR_ENA_VID4,INTERREG  | disable video interrupt
        movl    d0, sp@-                | push spl4 result for splx arg.
        jbsr    _splx                   | restore priority
        addql   #4, sp                  | pop splx arg
	rts

/*
 * Read the bus error register
 */
	ENTRY(getbuserr)
	clrl	d0
	movsb   BUSERRREG,d0            | get the buserr register
	rts

/*
 * Set the fpp registers to the u area values
 */
	ENTRY(setfppregs)
	tstw	_fppstate		| is fpp present and enabled?
	jle	1f			| branch if not
	fmovem	_u+U_FPS_REGS,fp0-fp7	| set fp data registers
	fmovem	_u+U_FPS_CTRL,fpc/fps/fpi | set control registers
1:
	rts

/*
 * Enable bit in both Unix _enablereg and hard ENABLEREG.
 * on_enablereg(bit) turns on enable register by oring it and bit.
 * E.g. on_enablereg((u_char)ENA_FPA)
 */
	ENTRY(on_enablereg)
	movb	sp@(7),d0		| get byte
	orb	d0,_enablereg		| turn on a bit
	movb	_enablereg,d0           | get it in a register
	movsb   d0,ENABLEREG            | put into hard ENABLEREG
	rts
 
/*
 * Disable bit in both Unix _enablereg and hard ENABLEREG. 
 * off_enablereg(bit) turns off enable register by anding it and bit.
 * E.g. off_enablereg((u_char)ENA_FPA)
 */ 
	ENTRY(off_enablereg) 
	movb	sp@(7),d0		| get byte
	notb	d0			| ~bit
	andb	d0,_enablereg		| turn off a bit
	movb	_enablereg,d0		| get it in a register 
	movsb	d0,ENABLEREG		| put into hard ENABLEREG 
	rts

/*
 * Define some variables used by post-modem debuggers
 * to help them work on kernels with changing structures.
 */
	.globl UPAGES_DEBUG, KERNELBASE_DEBUG, VADDR_MASK_DEBUG
	.globl PGSHIFT_DEBUG, SLOAD_DEBUG

UPAGES_DEBUG		= UPAGES
KERNELBASE_DEBUG	= KERNELBASE
VADDR_MASK_DEBUG	= 0x0fffffff
PGSHIFT_DEBUG		= PGSHIFT
SLOAD_DEBUG		= SLOAD
