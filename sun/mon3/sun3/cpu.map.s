
|
|	@(#)cpu.map.s 1.1 86/09/27
|	Copyright (c) 1986 by Sun Microsystems, Inc.
|
| /*
|  * Memory Mapping and Paging on the Sun 3
|  *
|  * This is intended to be used for both standalone code (ROM Monitor, 
|  * Diagnostics, boot programs, etc) and for the Unix kernel.  IF YOU HAVE
|  * TO CHANGE IT to fit your application, MOVE THE CHANGE BACK TO THE PUBLIC
|  * COPY, and make sure the change is upward-compatible.  The last thing we 
|  * need is seventeen different copies of this file, like we have with the
|  * Sun-1 header files.
|  *
|  * This file corresponds to version 1.0 of the Sun-3 Architecture Manual
|  * (1 Nov 84)
|  */
| 
| /*
|  * Mapping registers are accessed thru the  movs  instruction in the Sun-3.
|  *
|  * The following subroutines accept any address in the mappable range
|  * (256 megs).  They access the map for the current context register.  They
|  * assume that currently we are running in supervisor state.
|  */ 
| 
| segnum_t
| getsegmap(addr);
| 
| setsegmap(addr, entry);
| 
| struct pgmapent
| getpgmap(addr);
| 
| setpgmap(addr, entry);
| 
| context_t
| getcontext();
| 
| setcontext(entry);
|
| setcxsegmap(context, addr, entry);
|	-- set the specific segment map in the specified context, from boot
|	-- state.  This is how you set up things before you have ANY segs
|	-- mapped in a new context.
|
| setcxsegmap_noint(context, addr, entry);
|	-- same as setcxsegmap, but:
|	-- (1) interrupts must be disabled upon entry to the routine; and
|	-- (2) the interrupt register is not assumed to be in its usual
|	   place; it is not accessed.
|	-- This is how you set things up in the first place before you
|	   have even mapped your first context.

#include "../sun3/assym.h"

	.text

	.globl	_PAGE_INVALID
_PAGE_INVALID:
	.long	PME_INVALID	| "The" invalid page entry.

	.globl	_getpgmap
_getpgmap:
	movl	sp@(4),d0	| Get access address
	andl	#MAPADDRMASK,d0	| Mask out irrelevant bits
	addl	#PMAPOFF,d0	| Offset to map address
	movl	d0,a0
	movc	sfc,d1		| Save source function code
	lea	FC_MAP,a1	| Get function code in a reg
	movc	a1,sfc		| Set source function code
	movsl	a0@,d0		| Read page map entry
	movc	d1,sfc		| Restore source function code
	rts			| done

	.globl	_getsegmap
_getsegmap:
	movl	sp@(4),d0	| Get access address
	andl	#MAPADDRMASK,d0	| Mask out irrelevant bits
	addl	#SMAPOFF,d0	| Bump to segment map offset
	movl	d0,a0
	movc	sfc,d1		| Save source function code
	lea	FC_MAP,a1	| Get function code in a reg
	movc	a1,sfc		| Set source function code
	moveq	#0,d0		| Clear upper part of register
	movsb	a0@,d0		| Read segment map entry
	movc	d1,sfc		| Restore source function code
	rts			| done

	.globl	_setpgmap
_setpgmap:
	movl	sp@(4),d0	| Get access address
	andl	#MAPADDRMASK,d0	| Mask out irrelevant bits
	addl	#PMAPOFF,d0	| Offset to page maps
	movl	d0,a0
	lea	FC_MAP,a1	| Get function code in a reg
| The following code can be used to verify that the only page
| table entries written to the Invalid Pmeg are invalid pages, since the 
| consequences of writing valid entries here are somewhat spectacular.
	movc	sfc,d0		| Save source function code
	movc	a1,sfc		| Set source function code, too.
	addl	#SMAPOFF-PMAPOFF,a0	| Offset to segment map
	movsb	a0@,d1		| Get segment map entry number
	subl	#SMAPOFF-PMAPOFF,a0	| Offset back to page map
	movc	d0,sfc		| Restore source function code
| If debug code is removed, next line must remain!
	movl	sp@(8),d0	| Get page map entry to write
| More debug code
#if NUMPMEGS != 256
	andb	#NUMPMEGS-1,d1	| Top bit ignored in 128-pmeg version
#endif
	cmpb	#NUMPMEGS-1,d1	| Are we writing in the Invalid Pmeg?
	jne	1$		| Nope, go on.
	cmpl	#PME_INVALID,d0	| Writing the Invalid Page Map Entry?
	jeq	1$		| Yes, it's ok.
	bras	.-1		| Fault out, this is nasty thing to do.
1$:
| End of debugging code.
	movc	dfc,d1		| Save dest function code
	movc	a1,dfc		| Set destination function code
	movsl	d0,a0@		| Write page map entry
	movc	d1,dfc		| Restore dest function code
	rts			| done

	.globl	_setsegmap
_setsegmap:
	movl	sp@(4),d0	| Get access address
	andl	#MAPADDRMASK,d0	| Mask out irrelevant bits
	addl	#SMAPOFF,d0	| Bump to segment map offset
	movl	d0,a0
	movl	sp@(8),d0	| Get seg map entry to write
	movc	dfc,d1		| Save dest function code
	lea	FC_MAP,a1	| Get function code in a reg
	movc	a1,dfc		| Set destination function code
	movsb	d0,a0@		| Write segment map entry
	movc	d1,dfc		| Restore dest function code
	rts			| done

	.globl	_getcontext
_getcontext:
	movc	sfc,d1		| Save source function code
	lea	FC_MAP,a1	| Get function code in a reg
	movc	a1,sfc		| Set source function code
	moveq	#0,d0		| Clear upper part of result
	movsb	CONTEXTOFF,d0	| Move context reg into result
	andb	#CONTEXTMASK,d0	| Clear high-order bits
	movc	d1,sfc		| Restore source function code
	rts

	.globl	_setcontext
_setcontext:
	movb	sp@(7),d0	| Get context value to set
	movc	dfc,d1		| Save dest function code
	lea	FC_MAP,a1	| Get function code in a reg
	movc	a1,dfc		| Set destination function code
	movsb	d0,CONTEXTOFF	| Move value into context register
	movc	d1,dfc		| Restore dest function code
	rts

|
| Setcxsegmap
|
	.globl	_setcxsegmap
_setcxsegmap:
	movb	INTERRUPT_BASE,d0	| Save old interrupt mask
	movl	d0,sp@-
	andb	#~IR_ENA_INT,d0		| Disable all ints (but don't
	movb	d0,INTERRUPT_BASE	| toggle the other enables)
	movl	sp@(16),sp@-		| Copy three parameters
	movl	sp@(16),sp@-
	movl	sp@(16),sp@-
	jbsr	_setcxsegmap_noint	| Call the real routine
	lea	sp@(12),sp
	movl	sp@+,d0
	movb	d0,INTERRUPT_BASE	| Restore interrupt mask
	rts				| Wave byebye

|
| The guts of setcxsegmap, called from mapmem() early in initialization.
|
	.globl	_setcxsegmap_noint
_setcxsegmap_noint:
| d0 = context, and segmap entry
| d1 = temp
| d2 = saved enable reg value
| d3 = saved context reg value
| a0 = segment addr
| a1 = saved dfc
	movl	d2,sp@-
	movl	d3,sp@-
#define push 8
| Set up address of segmap entry
	movl	sp@(8+push),d0	| Get access address
	andl	#MAPADDRMASK,d0	| Mask out irrelevant bits
	addl	#SMAPOFF,d0	| Bump to segment map offset
	movl	d0,a0
| Set up for control space accesses
	movc	dfc,a1		| Save dest function code
	moveq	#FC_MAP,d1	| Get function code in a reg
	movc	d1,dfc		| Set destination function code
	movsb	ENABLEOFF,d2	| Save enable reg
	movw	#~ENA_NOTBOOT,d0
	andb	d2,d0
	movsb	d0,ENABLEOFF
|vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv|
| Start of boot-state code						|
| Get the seg map entry and the context number
	movl	sp@(12+push),d0	| Get seg map entry to write
	movb	sp@(7+push),d1	| Get context value to set
| Go into strange context, do it, get back out alive
	movsb	CONTEXTOFF,d3
	movsb	d1,CONTEXTOFF	| Move value into context register
	movsb	d0,a0@		| Write segment map entry
	movsb	d3,CONTEXTOFF
	movsb	d2,ENABLEOFF
| End of boot-state code						|
|vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv|
	movc	a1,dfc		| Restore dest function code
| Clean up and return
	movl	sp@+,d3
	movl	sp@+,d2
	rts			| done

