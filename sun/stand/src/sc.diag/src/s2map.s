| 	@(#)s2map.s 1.8 83/09/19 Copyright (c) 1983 by Sun Microsystems, Inc.
|
| /*
|  * Memory Mapping and Paging on the Sun 2
|  *
|  * This is intended to be used for both standalone code (ROM Monitor, 
|  * Diagnostics, boot programs, etc) and for the Unix kernel.  IF YOU HAVE
|  * TO CHANGE IT to fit your application, MOVE THE CHANGE BACK TO THE PUBLIC
|  * COPY, and make sure the change is upward-compatible.  The last thing we 
|  * need is seventeen different copies of this file, like we have with the
|  * Sun-1 header files.
|  *
|  * This file corresponds to version 1.0 of the Sun-2 Design Reference.
|  * (1 April 83)
|  */
| 
| /*
|  * Mapping registers are accessed thru the  movs  instruction in the Sun-2.
|  *
|  * The following subroutines accept any address in the mappable range
|  * (16 megs).  They access the map for the current context register.  They
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
| getsupcontext();
| context_t
| getusercontext();
| 
| setsupcontext(entry);
| setusercontext(entry);

#include "assym.h"

	.text

	.globl	_PAGE_INVALID
_PAGE_INVALID:
	.long	PME_INVALID	| "The" invalid page entry: valid bit on,
				| but no permissions enabled.

	.globl	_getpgmap
_getpgmap:
	movl	sp@(4),d0	| Get access address
	andw	#LOWMASK,d0	| Mask out low bits
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
	andw	#LOWMASK,d0	| Mask out low bits
	addql	#SMAPOFF,d0	| Bump to segment map offset
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
	andw	#LOWMASK,d0	| Mask out low bits
	movl	d0,a0
	lea	FC_MAP,a1	| Get function code in a reg
| The following code can be used to verify that the only page
| table entries written to the Invalid Pmeg are invalid pages, since the 
| consequences of writing valid entries here are somewhat spectacular.
	movc	sfc,d0		| Save source function code
	movc	a1,sfc		| Set source function code, too.
	movsb	a0@(SMAPOFF),d1	| Get segment map entry number
	movc	d0,sfc		| Restore source function code
| If debug code is removed, next line must remain!
	movl	sp@(8),d0	| Get page map entry to write
| More debug code
	cmpb	#256-1,d1	| Are we writing in the Invalid Pmeg?
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
	andw	#LOWMASK,d0	| Mask out low bits
	addql	#SMAPOFF,d0	| Bump to segment map offset
	movl	d0,a0
	movl	sp@(8),d0	| Get seg map entry to write
	movc	dfc,d1		| Save dest function code
	lea	FC_MAP,a1	| Get function code in a reg
	movc	a1,dfc		| Set destination function code
	movsb	d0,a0@		| Write segment map entry
	movc	d1,dfc		| Restore dest function code
	rts			| done

	.globl	_getusercontext
_getusercontext:
	movc	sfc,d1		| Save source function code
	lea	FC_MAP,a1	| Get function code in a reg
	movc	a1,sfc		| Set source function code
	moveq	#0,d0		| Clear upper part of result
	movsb	USERCONTEXTOFF:w,d0 | Move context reg into result
	andb	#CONTEXTMASK,d0	| Clear high-order bits
	movc	d1,sfc		| Restore source function code
	rts

	.globl	_getsupcontext
_getsupcontext:
	movc	sfc,d1		| Save source function code
	lea	FC_MAP,a1	| Get function code in a reg
	movc	a1,sfc		| Set source function code
	moveq	#0,d0		| Clear upper part of result
	movsb	SUPCONTEXTOFF:w,d0 | Move context reg into result
	andb	#CONTEXTMASK,d0	| Clear high-order bits
	movc	d1,sfc		| Restore source function code
	rts

	.globl	_setusercontext
_setusercontext:
	movb	sp@(7),d0	| Get context value to set
	movc	dfc,d1		| Save dest function code
	lea	FC_MAP,a1	| Get function code in a reg
	movc	a1,dfc		| Set destination function code
	movsb	d0,USERCONTEXTOFF:w | Move value into context register
	movc	d1,dfc		| Restore dest function code
	rts

	.globl	_setsupcontext
_setsupcontext:
	movc	dfc,d1		| Save dest function code
	movb	sp@(7),d0	| Get context value to set
	lea	FC_MAP,a1	| Get function code in a reg
	movc	a1,dfc		| Set destination function code
	movsb	d0,SUPCONTEXTOFF:w | Move value into context register
	movc	d1,dfc		| Restore dest function code
	rts
