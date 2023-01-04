|
| 	@(#)trap.s 1.23.1.1 85/03/14 Copyright (c) 1984 by Sun Microsystems, Inc.
|
|	trap.s -- entered by hardware for all traps
|
|	Biggest function is handling resets.
|
|	If it is a power-up reset, run the diagnostics, map things a little,
|	then jump to C code to deal with setting up the world and booting.
|
|	If it's not a power-up reset, and the Reset vectoring information
|	in page zero is still good, vector to user code to deal with the
|	reset.
|
|	The code vectored to at reset is entered with the following environment:
|		Supervisor state.  Interrupt level 7.
|		Enable register = ena_notboot|ena_par_gen;
|			(note that this disables interrupts and VDMA.)
|		Both contexts = 0.
|		Segment map entry 0 = 0th pmeg.
|		Page map entry for g_resetaddr = g_resetmap.  (Supercedes
|			the above page map entries, if there's a conflict.)
|		No segment map entries other than #0 are modified from their
|			state before the reset.
|		Page map entry #1 (0x800-0x1000) is mapped to physical memory
|			page 1.
|		No page map entries outside segment 0 are modified from their
|			state before the reset.
|		Page map entries in segment 0 cannot be depended on.
|			(Note that g_resetaddr should point into segment
|			 zero, if you want to guarantee that you know what
|			 pmeg it is being mapped into.)
|		SSP = pointing to stacked registers and fake exception stack.
|			Base of this stack is at 0x1000.  Note that the
|			pointed-to memory may not be mapped, if its map entry
|			was overridden by the g_resetmap entry.  The actual
|			data is in physical memory page 1.
|		No other registers guaranteed.
|		g_resetaddrcomp has been invalidated, and the NMI timer has
|			been zapped, so a second Reset will not reinvoke the
|			handler unless you fix them.  You must fix the NMI
|			timer if you like NMI's.
|		Memory is not guaranteed to be initialized (i.e. may have
|			any data, and may contain parity errors), except
|			for the stack area between SSP and 0x1000 -- and
|			that might be invalid, if that chunk of physical
|			memory is bad.
|		Other than physical page 1, the proms have not remapped
|			or otherwise messed with things.
|		Code is entered at the address from g_resetaddr.
|
|	This code has to be written very carefully, as it it easy to make
|	assumptions that aren't true.  If the code fails (eg, resets again)
|	before it makes g_resetaddrcomp valid again, the PROMs will avoid
|	invoking RAM code, but will just bootstrap the system from scratch
|	as if they had detected a power-up reset.
|

#include "assym.h"
#include "../h/s2led.h"

| Pages to map various things to, temporarily.
#define TIMER_TEMP	0x0
| PROM_TEMP has to be 0 modulo 2*promsize, since high-order bits are used
| to address the PROM (as well as the value in the map).  Safe values for
| 256K proms are on 64K boundaries.
#define PROM_TEMP	0x0		

|
	.globl	_hardreset
_hardreset:
| 
| Boot State:
|
| In boot state (anytime notboot bit in Enable Reg is clear), all
| Supervisor Program fetches come from PROM.  Other accesses, including
| Supervisor Data fetches and stores, are not affected.  Note that while
| interrupts cannot occur (until we turn on the Int Enable bit in the Enable
| Reg) we can still get bus errors, etc, which fetch their vectors from
| mappable memory (not prom).
|

#ifdef M68020
| Turn on the cache...
	movw	#1,a7		| Cache enable bit
	.long	0x4e7bf002	| movc	a7,cacr
#endif M68020
| Make sure that our maps are straight, or we'll leave a good looking corpse.
	movw	#FC_MAP,a7		| SP is already clobberred, use it
	movc	a7,dfc			| Destination function code
	movc	a7,sfc			| Set source function code, too.

| Set up maps...
	subl	a7,a7			| Clear A7
	movsw	a7,CONTEXTOFF		| Clear both context registers
	movsb	a7,SMAPOFF		| Clear segment map

| FIXME, remove this when latch is gone.
	movsw	BUSERROFF,a7		| Reset latch on bus error reg

| Enable parity generation, and ensure that interrupts, etc are turned off.
| A hardware reset would clear the enable register, but if this is a software
| reset, we have to get into boot state explicitly this way.
	movw	#ena_par_gen,a7		| ena par_gen & boot, shut off rest.
	movsw	a7,ENABLEOFF		| Write to enable register

| Set initial value into diagnostic LED display.
	movl	d0,a7
	moveq	#~L_INITIAL,d0
	movsb	d0,LEDOFF
	movl	a7,d0

|
| Check the timer chip to see whether this was a power-on reset or
| a human-initiated or watchdog reset.
|
	movl	#PME_TIMER,a7		| Pg mapper for timer chip
	movsl	a7,TIMER_TEMP+PMAPOFF	| Map it in.
T_cmd = TIMER_TEMP + clk_cmd
T_data = TIMER_TEMP + clk_data
	movw	#CLK_16BIT,T_cmd	| Ensure we're in 16-bit mode
	movw	#CLK_ACC_MODE+TIMER_NMI,T_cmd	| Access nmi timer's mode
	cmpw	#CLKM_DEFAULT,T_data	| Is it set from a power-up?
	jeq	PowerUpReset		| (Yes, this is a power-up.)

|
| This is not a power-up reset; see if the soft reset vector is good.
|
| First, invalidate the timer mode register, so a second reset will 
| cause us to bootstrap instead of looping here.
|
	movw	#CLK_ACC_MODE+TIMER_NMI,T_cmd	| Set up to touch same mode reg
	movw	#CLKM_DEFAULT,T_data	| Make it the power-up default
| Now we're safe; a double bus fault here will just cause a boot.
| This is the last place that touches the timer chip; we can re-map the page.

| Map in pages 0 and 1
	movl	#PME_MEM_0,a7		| Pg mapper for 0x000-7FF
	movsl	a7,0x000+PMAPOFF	| Map in page zero.
	movl	#PME_MEM_800,a7		| Pg mapper for 0x800-FFF
	movsl	a7,0x800+PMAPOFF	| Map in stack page, too.

| Now that we're done kludging with a7 (==sp), make a stack pointer.
	movw	#INITSP,sp		| Reset stack ptr to correct value

| Save the registers that were in use just before we got reset.  These
| will be used by the user's watchdog handler.
	moveml	#0xFFFF,sp@-		| Save all regs (including SSP)
	movl	usp,a0			| Save USP
	movl	a0,sp@(15*4)		| on top of useless SSP...

	movl	g_resetaddr,a0		| Get putative reset vector
	movl	a0,d0
	movl	g_resetaddrcomp,d2	| Check its validity...
	eorl	d2,d0			| Check its validity...
| Invalidate the vector, so we won't use it on an unexpected watchdog inside
| the vectored-to code, but will reboot the system instead.  (We might
| be able to dispense with this, since timer chip is trashed too, but
| I want to play it safe here.)
	movl	a0,g_resetaddrcomp	| This is guaranteed to invalidate it.
	addql	#1,d0			| Should be ones-complement, giving
	jne	PowerUpReset		| value of zero here if valid...

	movl	g_resetmap,d1		| Get putative page map entry
	movl	d1,d0
	movl	g_resetmapcomp,d2	| Ditto the validity check
	eorl	d2,d0
	addql	#1,d0
	jne	PowerUpReset

| Tell the world we're about to enter user watchdog code.
	moveq	#~L_USERDOG,d0
	movsb	d0,LEDOFF

| Now prepare to transfer control to the mapped-in code.  This means
| we have to get out of boot state (to allow supervisor accesses via mapped
| memory), which means we'd better map ourself in before we try it.
|
| We assume that this code sits in the first page of the PROM, and that
| dfc == MAP_FC.  Don't break either assumption.  FIXME!
	movl	a0,d0			| Get address we'll jump to;
	andw	#LOWMASK,d0		| Clear low-order bits;
	movl	d0,a1			| Make it indexable.

	movl	#PME_PROM,d0		| Get our own map entry
	movsl	d0,PROM_TEMP+PMAPOFF	| Map ourself in.
	jmp	PROM_TEMP+GoOn-PROM_BASE:L	| Jump to low memory version
| If you leave off the :L on the above instruction, the assembler messes up.
| (JG & RT, Sun, 11Aug83)
GoOn:
| Disable boot state -- use the map we just set up.
	movw	#ena_notboot+ena_par_gen,d0
	movsw	d0,ENABLEOFF 		| Turn on the maps,
	movsl	d1,a1@(PMAPOFF)		| Map in the user's page of code,
| In case she maps herself in on top of wherever we happen to be, we will
| have prefetched one word (the jra a0@) and will get away by the
| skin of our teeth...
	jra	a0@			| Into the wild blue yonder...

|
PowerUpReset:
|
| The timer mode was not set to the correct value, or the Reset vector
| or its map entry was invalid.
|
| If any of the above is screwed up, assume that the rest of the system
| is not doing much better.
|
	jra	_qpdiag			| So run the diagnostics already...

	.globl	_diagret
_diagret:				| (They return to here.)
|
| We are hereby assuming that the diagnostics return with things in a 
| reasonable state.  Define that state.  FIXME.
| 
| For one thing, we assume that all the main memory decribed by a6
| (g_memorysize) is mapped sequentially starting at location 0.
| We also assume that dfc == FC_MAP.
|

| Fake the stack as if we'd taken a "reset interrupt" here.
	movw	#EVEC_RESET,INITSP-6	| Fake Reset VOR

| Clear (and set parity on) low memory so we can build a stack.
| We will clear the rest of memory in a moment after we have free regs.
	subl	a7,a7			| Start at location 0,
lowclr:
	movw	#0xFFFF,a7@+		| Clear a word
	cmpw	#INITSP-6,a7		| Clear thru already built stack
	jne	lowclr

| Now that we're done kludging with low memory, save the diagnostic info.
	moveml	#0xFFFF,g_diag_state	| All of 'em, in low memory

	movl	a6,g_memorysize		| Save size of memory from diags
| FIXME, save other things that are useful eg # pages lost,
| etc etc etc.

| Put the LEDs into a known state.
	moveq	#~L_SETUP_MEM,d0	| No problem
	movb	d0,g_leds		| Initialize low memory value
	movsb	d0,LEDOFF		| Initialize real visible value

	lea	INITSP,a7		| Get ready to clear all memory 
	moveq	#0xFFFFFFFF,d0		| to FF's to aid in random jumps.
	movl	g_memorysize,d1
	subl	a7,d1			| Subtract off initial address
	asrl	#2,d1
	jra	cloopdb

cloop:
	movl	d0,a7@+			| Clear a longword
cloopdb:
	dbra	d1,cloop		| Loop in 64K chunks
	clrw	d1			| Loop over the 64K chunks til done
	subql	#1,d1
	jcc	cloop

	moveml	g_diag_state,#0xFFFF	| Restore all regs for laziness

| The following is common to power-up and boot resets.
bootcommon:

| The following is common to power-up, boot, and soft resets.
kcmdcommon:

| The following is common to power-up, boot, soft, and watchdog resets.
dogcommon:

| The registers at this point (except A7 as usual) contain useful info and
| must be preserved.

| Disable interrupts, etc -- set enable reg to known state.
	movw	#FC_MAP,a7		| Set map function code
	movc	a7,dfc
	movw	#ena_par_gen,a7
	movw	a7,g_enable		| Save valid copy for C code
	movsw	a7,ENABLEOFF

| Now that we're done kludging with a7 (==sp), make a stack pointer and
| pretend we took a trap.  VOR is already on "stack" at INITSP-6.
| Build a return address above the fake interrupt stack
	movl	#_exit_to_mon,INITSP-4	| return address if user rts'es to us
	lea	INITSP-6,sp		| Reset stack ptr below stored stuff
	pea 	USERCODE		| fake PC = User code start addr
	movw	sr,sp@-			| Save current SR (oughta be 2700)
| The following duplicates code from _trap.
	subw	#mis_size-8,sp		| Make room for goodies after 8 bytes
					| stacked by hardware
	moveml	#0xFFFF,sp@(mis_d0)	| Store all registers, including SSP
	movl	a7,d7			| Set "reset or trap" indicator.
	jra	_resettrap		| Pretend we took a trap.
|
|
| Entries for watchdog, boot, and soft resets.
|

|
| Watchdog reset.
| Entered under scuzzy conditions like described at the top of this module.
| In particular, the code from _dogreset thru _dogreset2 is run in page zero
| outside of boot state.
| First job is to get back into boot state.  Then caper about cleaning up.
|
	.globl	_dogreset
_dogreset:
	moveq	#FC_MAP,d0		| Set map function code
	movc	d0,dfc
	moveq	#ena_par_gen,d0		| Now set enable reg into boot state
	movsw	d0,ENABLEOFF
	jmp	dogreset2:l		| Jump to our "real" location

dogreset2:
| Map in page 0
	movl	#PME_MEM_0,d0		| Pg mapper for 0x000-7FF
	movsl	d0,0x0+PMAPOFF		| Map in low memory

	moveml	sp@+,#0xFFFF		| Restore all registers.
| Fake the stack as if we'd taken a "Watchdog reset" interrupt here.
	movw	#EVEC_DOG,INITSP-6	| Fake VOR
	jra	dogcommon		| Go build rest of stack

|
| Bootstrap reset -- pretty easy, actually.
| Fake the stack as if we'd taken a "Bootstrap reset" interrupt here.
|
	.globl	_bootreset
_bootreset:
	reset				| Zap out Multibus devices
	movw	#EVEC_BOOTING,INITSP-6	| Fake VOR
	jra	bootcommon


|
| Entry for soft resets (K1).  Also trivial.
| Fake the stack as if we'd taken a "K command reset" interrupt here.
|
	.globl	_softreset
_softreset:
	reset				| Zap out Multibus devices
	movw	#EVEC_KCMD,INITSP-6	| Fake VOR
	jra	kcmdcommon		| Go build rest of stack

|
|
| exit_to_mon is used if the user program returns to us via "rts" (eg, from
| a "boot" command).  We just pretend she did a TRAP #1.
| Note that the trap vector might not be set, so we fake it.
|
	.globl	_exit_to_mon
_exit_to_mon:
	pea	_exit_to_mon		| For the next rts...
	movw	#EVEC_TRAPE,sp@-	| Fake VOR for 68010 TRAP
	pea	_exit_to_mon		| Push fake PC that brings us back
	movw	#0x2700,sp@-		| Push fake SR
|	(fall thru into _trap)


|
|	Entry point for all traps except reset, address, and bus error.
|
	.globl	_trap
_trap:
	subw	#mis_size-8,sp		| Make room for goodies after 8 bytes
					| stacked by hardware
	moveml	#0xFFFF,sp@(mis_d0)	| Store all registers, including SSP
	subl	d7,d7			| Clear "reset or trap" indicator.
_resettrap:
	addl	#mis_size,sp@(mis_a7)	| Make visible SSP == user SSP
	movc	usp,a0			| Drag usp out of the hardware
	movl	a0,sp@(mis_usp)		| Put it in the memory image
	movc	sfc,a0
	movc	dfc,a1
	movc	vbr,a2
	moveml	#0x0700,sp@(mis_sfc)	| Store sfc, dfc, vector base
	moveq	#FC_MAP,d0
	movc	d0,sfc
	movsb	SUPCONTEXTOFF,d0
	andb	#CONTEXTMASK,d0		| Throw away undefined bits
	movl	d0,sp@(mis_scon)	| Save sup context reg
	movsb	USERCONTEXTOFF,d0
	andb	#CONTEXTMASK,d0		| Throw away undefined bits
	movl	d0,sp@(mis_ucon)	| Save user context reg

	orw	#0x0700,sr		| Run at interrupt level 7 now.
	tstl	d7			| For resets, call monreset() first.
	jeq	tomon
	jbsr	_monreset		| The args are modifiable if desired.
tomon:
	jbsr	_monitor		| Call the interactive monitor

	movl	sp@(mis_usp),a0		| Get usp out of memory image
	movc	a0,usp			| Cram it into the hardware.
	moveq	#FC_MAP,d0
	movc	d0,dfc
	movl	sp@(mis_scon),d0
	movsb	d0,SUPCONTEXTOFF
	movl	sp@(mis_ucon),d0
	movsb	d0,USERCONTEXTOFF
	movl	sp@(mis_vbase),d0
	movc	d0,vbr
	movl	sp@(mis_sfc),d0
	movc	d0,sfc
	movl	sp@(mis_dfc),d0
	movc	d0,dfc
| Be very careful in the following code that you don't leave any windows
| where an NMI can come in and clobber anything on the old or new stacks.
| Be especially careful for the common case where the new and old stacks
| are exactly the same.
	movl	sp@(mis_a7),a0		| Grab new SSP value (prolly same)
	movl	sp@(mis_sr+4),a0@-	| Stack stuff below new SSP
	movl	sp@(mis_sr),a0@-
	movl	a0,sp@(mis_a7)		| Put it back (decremented) in regs
	moveml	sp@(mis_d0),#0xFFFF	| Restore all regs including new SSP
	rte				| Poof, resume from error.

|
|
|	Entry point for bus and address errors.
|
|	Save specialized info in dedicated locations for debugging things.
|	Then truncate stack to usual size, so user can resume anywhere
|	instead of being able to resume in mid-cycle (sorry...).
|
|	They could resume in mid-cycle by simply stacking this info
|	(that we save in low memory) and rts'ing.  But that's not the
|	default.
|
	.globl	_addr_error
	.globl	_bus_error
_addr_error:
_bus_error:
	moveml	#0xFFFF,g_beregs	| Store all regs
	moveq	#(sizeofbestack/2)-1,d0	| Move 29 words
	lea	g_bestack,a0		| Get set to move all the data
	movl	sp,a1
BEloop:
	movw	a1@+,a0@+		| Move a word of error info
	dbra	d0,BEloop		| Baby's very first loopmode loop
	
	movc	sfc,a0			| Save it a moment
	moveq	#FC_MAP,d0		| Get the map/CPU layer function
	movc	d0,sfc
	movsw	BUSERROFF,d0		| Get contents of bus error reg
	movsw	d0,BUSERROFF	|FIXME, clear latch (maybe?)
	movw	d0,g_because		| Save for higher level code
	movc	a0,sfc			| Restore previous value

| Truncate stack frame back to manageable proportions
	movl	sp@,   sp@(0+sizeofbestack-sizeofintstack)
	movl	sp@(4),sp@(4+sizeofbestack-sizeofintstack)
	lea	sp@(sizeofbestack-sizeofintstack),sp
	andw	#VOR_OFFSET,sp@(i_vor)	| Clear "long stack frame" format
	movl	g_beregs   ,d0		| Restore d0, a0, and a1
	movl	g_beregs+32,a0		| (Since we clobberred them.)
	movl	g_beregs+36,a1
	jra	_trap			| Enter "normal" trap code
|
|
|	Entry point for non-maskable interrupts, used to scan keyboard.
|
#if defined(KEYBOARD) || defined(BREAKKEY)

	.globl	_nmi
_nmi:
	movw	#CLK_CLEAR+TIMER_NMI,TIMER_BASE+clk_cmd | Reset interrupt
	movl	d0,sp@-			| Save a scratch register
	movl	d1,sp@-			| Save yet another register

|
| Counting the milliseconds til I see you again, dear...
| Every half-second, don't forget to breathe.
|
	movw	g_nmiclock+2,d0		| Get the clock value
	addl	#1000/NMIFREQ,g_nmiclock| Count off x ms more.
	movw	g_nmiclock+2,d1		| See if the "1/2 second" bit changed
	eorw	d1,d0
	btst	#9,d0			| (2**9 == 512, close enough)
	jeq	noheart
	movc	dfc,d1			| Save destination fn code
	moveq	#FC_MAP,d0		| Set it up to access LEDs
	movc	d0,dfc
	movb	g_leds,d0		| Grab shadow copy of LEDs
	eorb	#L_HEARTBEAT,d0		| Invert heartbeat LED
	movb	d0,g_leds		| Put it back in RAM shadow loc
	movsb	d0,LEDOFF		| Put it into real LEDs too
	movc	d1,dfc			| Restore function code register
noheart:

#ifdef KEYBOARD
#ifdef KEYBS2
|
| Keyboard scanning code for Sun-2 keyboard.
|
| Watch out, most registers are volatile, and several are played with.
|
Trykey:
	movl	a0,d1			| Save a0 temporarily
	movl	g_keybzscc,a0		| Point to keyboard
	tstb	a0@(zscc_control)	| Read to flush previous pointer.
#ifdef M68020
	nop; nop; nop; nop; nop		| Extra delays for recovery time
	nop; nop; nop; nop; nop	
#endif M68020
	nop				| Delay for recovery time
	nop
	moveq	#ZSRR0_RX_READY,d0	| Bit mask for RX ready bit
	andb	a0@(zscc_control),d0	| Now read it for real.
	jeq	s2_notnew		| If no char around, skip.

| We have a new keypress, call on C routine to mess with it.
|	moveq	#0,d0			| "purify" d0 to long. (above did it)
#ifdef M68020
	nop; nop; nop; nop; nop		| Extra delays for recovery time
	nop; nop; nop; nop; nop	
#endif M68020
	nop				| Delay for recovery time
	nop
	movb	a0@(zscc_data),d0	| Get the keycode
	movl	d1,a0			| Restore A-register temp

	moveml	#0x00C0,sp@-		| Save a0, a1.  d0&d1 already saved.
	movl	d0,sp@-			| Push it as argument to keypress().
	jbsr	_keypress
	addql	#4,sp
	moveml	sp@+,#0x0300		| Restore saved registers
	tstl	d0			| See if keypress() wants us to abort.
	jeq	Trykey			| Nope, see if more keypresses there.
	jra	abort			| Yep, abort to monitor.

s2_notnew:
	movl	d1,a0			| Restore a0 from shenanigans
	jra	Notnew

#else  KEYBS2

Trykey:
	movb	PARALLEL_BASE,d0	| Get the current keycode
	cmpb	PARALLEL_BASE,d0	| See if it's still the same.
	jne	Trykey			| Loop til it stabilizes
	cmpb	g_prevkey,d0		| Is it same as previous key read?
	jeq	Notnew			| (yes, don't bother calling C.)

| We have a new keypress, call on C routine to mess with it.
|	addql	#1,0x100		| Count how many times we call on C
| Insert this statistic if you feel like checking it.  2/3 of the time it
| saves a call on C with VT100 keyboard.
	moveml	#0x40C0,sp@-		| Save d1, a0, a1.  d0 already saved.
	movb	d0,g_prevkey		| Remember this key for next time
	moveq	#0,d1			| "purify" d0 into a long.
	movb	d0,d1
	movl	d1,sp@-			| Push it as argument to keypress().
	jbsr	_keypress
	addql	#4,sp
	moveml	sp@+,#0x0302		| Restore saved registers
	tstl	d0
	jne	abort			| If result nonzero, abort to mon.
#endif KEYBS2

|
| No keypress, or uninteresting one.  Check serial lines.
|
Notnew:
#endif KEYBOARD

#ifdef BREAKKEY
	tstb	g_insource		| Is input source a serial line?
| FIXME, g_insource should vector.
	jeq	NotSerial		| No, skip over this stuff.
	movl	a0,d1		| Save away a0
	movl	g_inzscc,a0		| Grab uart address
| FIXME, remove next 4 lines when g_inzscc is right.
	cmpb	#INUARTA,g_insource	| See if A-side or B-side
	jne	Brkt1			| (B-side; a0 is OK.)
	addw	#sizeofzscc,a0		| A-side is second in the chip.
Brkt1:
| End of g_inzscc fixup.
	tstb	a0@(zscc_control)	| Read to flush previous pointer.
#ifdef M68020
	nop; nop; nop; nop; nop		| Extra delays for recovery time
	nop; nop; nop; nop; nop	
#endif M68020
	nop				| Delay for recovery time
	nop
	movb	#ZSWR0_RESET_STATUS,a0@(zscc_control)	| Avoid latch action
#ifdef M68020
	nop; nop; nop; nop; nop		| Extra delays for recovery time
	nop; nop; nop; nop; nop	
#endif M68020
	nop				| Delay for recovery time
	moveq	#ZSRR0_BREAK+0xFFFFFF00,d0 | Who's afraid of the big bad sign?
	andb	a0@(zscc_control),d0	| Now read it for real.
	movl	d1,a0		| Restore a0
	cmpb	g_debounce,d0		| Is its state same as last time?
	jeq	NotSerial		| Yes, don't worry about it.
	movb	d0,g_debounce		| No -- remember its state,
	jeq	abort			| and if it went 1->0, abort.
| FIXME, there will be a null in the read fifo, but we will ignore it
| anyway, so probably no need to worry.  Check this.

NotSerial:
#endif BREAKKEY

	movl	sp@+,d1			| Restore other register
	movl	sp@+,d0			| Restore saved register
	rte				| Return from NMI

|
|	Somebody wants to abort whatever is going on, and return to
|	Rom Monitor control of things.  Be obliging.
|
abort:
	movl	sp@+,d1			| Restore other register
	movl	sp@+,d0			| Restore saved register
| fall thru...

	.globl	_abortent		| Entry point from outside
_abortent:
	movw	#EVEC_ABORT,sp@(i_vor)	| Indicate that this is an abort.
	jra	_trap			| And just trap out as usual.
	
#endif defined(KEYBOARD) || defined(BREAKKEY)

#ifdef KEYBS2
|
| Send a command byte to the keyboard if possible.
| Return 1 if done, 0 if not done (Uart transmit queue full).
|
	.globl	_sendtokbd
_sendtokbd:
	moveq	#ZSRR0_TX_READY,d0	| Bit mask for TX ready bit
	movl	g_keybzscc,a0
	tstb	a0@(zscc_control)	| Read to flush previous pointer.
#ifdef M68020
	nop; nop; nop; nop; nop		| Extra delays for recovery time
	nop; nop; nop; nop; nop	
#endif M68020
	nop				| Delay for recovery time
	nop
	andb	a0@(zscc_control),d0	| Now read it for real.
	jeq	sendret			| If can't xmit, just return the 0.

#ifdef M68020
	nop; nop; nop; nop; nop		| Extra delays for recovery time
	nop; nop; nop; nop; nop	
#endif M68020
	nop
	movb	sp@(7),a0@(zscc_data)	| Write the byte to the kbd uart
	moveq	#1,d0			| Indicate success
sendret:rts				| Return to caller.
#endif KEYBS2

|
| Runs thru all of memory rewriting it to clear parity errors.
| One argument, the # of bytes to clear.  Clobbers sfc and dfc.
|
	.globl	_clrparerrs
_clrparerrs:
	moveq	#FC_MAP,d0
	movc	d0,sfc
	movc	d0,dfc
	movsw	ENABLEOFF,d0
	moveq	#~ena_par_check,d1
	andw	d0,d1
	movsw	d1,ENABLEOFF		| Disable parity error detection
	movw	d1,g_enable		| Save in low memory for C code
	subl	a0,a0			| Get ready to clear all memory 
	movl	sp@(4),d1		| of parity errors...
	asrl	#2,d1
	jra	bloopdb

bloop:
	movl	a0@,a0@+		| Clear parity errs in a longword
bloopdb:
	dbra	d1,bloop		| Loop in 64K chunks
	clrw	d1			| Loop over the 64K chunks til done
	subql	#1,d1
	jcc	bloop
	movw	d0,g_enable		| Restore low memory copy of enable reg
	movsw	d0,ENABLEOFF		| Restore previous enable reg setting

	rts				| Return to caller

|
| Sets the enable register to the specified value.
|
| Note that the argument push'd by C is a halfword in the bottom half of a
| longword, since it is a
| struct enablereg rather than an unsigned short or something.
| I wish this was documented a little bit better.  FIXME, Richard!
|
	.globl	_set_enable
_set_enable:
	movw	sp@(6),d0		| Grab desired value from stack
	movc	dfc,a0			| Save dest function code
	moveq	#FC_MAP,d1		| Set up for map access
	movc	d1,dfc
	movsw	d0,ENABLEOFF		| Stuff it into enable reg
	movc	a0,dfc			| Restore dfc and exit.
	rts

|
| Set the LEDs.
|
	.globl	_set_leds
_set_leds:
	movc	dfc,a0			| Save dest function code
	moveq	#FC_MAP,d1		| Set up for map access
	movc	d1,dfc
	movb	sp@(7),d0		| Get desired LED contents
	movb	d0,g_leds		| Save in global RAM for readback
	movsb	d0,LEDOFF		| Stuff into LED reg
	movc	a0,dfc			| Restore dfc and exit.
	rts

|
| Issue a reset instruction.
|
	.globl	_resetinstr
_resetinstr:
	reset				| That's all.
	rts
