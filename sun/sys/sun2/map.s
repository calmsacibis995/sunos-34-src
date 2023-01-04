/*	@(#)map.s 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Memory Mapping and Paging on the Sun-2
 */

#include "../h/param.h"
#include "../machine/asm_linkage.h"
#include "../machine/mmu.h"
#include "../sun2/pte.h"
#include "assym.s"

/*
 * Routine variations which have inner loops embedded.
 */

/*
 * Symbols which should have been defined already
 */

#define	MMU_R	0x00200000	/* MMU referenced bit */
#define MMU_M	0x00100000	/* MMU modified bit */

/*
 * unloadpgmap(page, ppte, kk)
 * 	int page,		| Page #
 *	    kk;			| # pte's to scan
 *	struct pte *ppte;	| Software pte corresponding to page #
 * Scan kk software pte's.  For each valid one, and for which the corresponding
 * MMU pte is also valid, merge the MMU referenced and modified bits into the
 * software pte entries, and clear those bits in the MMU.
 */

#define	page	a6@(8)		/* Argument offsets */
#define ppte	a6@(12)
#define	kk	a6@(16)
#define ssfc	a6@(-2)		/* Temporaries */
#define sdfc	a6@(-1)

	.text
ENTRY(unloadpgmap)
	link	a6,#-2		| Set up temporaries, adjust stack
	movl	ppte,a1		| Get ppte to register to step over entries
2$:				| Here to loop over initial invalid entries
	subql	#1,kk		| Any more to process?
	blt	1$		| Branch to exit if not
	movl	a1@,d0		| Otherwise, pick up software version of pte
	blt	3$		| Branch if valid, to process MMU version
	lea	a1@(PTE_SIZE),a1| Invalid entry, pte++
	addql	#1,page		| Step MMU page number
	jra	2$		| Process until find valid or run out of pmeg
3$:				| Here on first valid software pte, do setup
	movc	sfc,d0		| Save contents of alternate address spaces
	movb	d0,ssfc		|   in temporaries allocated for this purpose
	movc	dfc,d0		| "
	movb	d0,sdfc		|   "
	movl	#FC_MAP,d0	| Specify function is to use the MMU space
	movc	d0,sfc		| For both fetches and
	movc	d0,dfc		|   stores
	movl	page,d0		| Get MMU page number
	moveq	#PGSHIFT,d1	| Get amount to shift it for addressing
	lsll	d1,d0		|   MMU pte and do the shift
	movl	d0,a0		| From here on, use this as an indirect reg
4$:				| Here to process MMU pte after setup
	movsl	a0@,d0		| Get MMU entry
	movl	d0,d1		| Make copy to change and restore to MMU
				|   and ALSO to set condition codes for branch
	bge	5$		| Skip all this if *it* is not valid
	lsrl	#2,d0		| Shift MMU referenced/modified bits
	andl	#(PG_R + PG_M),d0 | Save just those
	orl	d0,a1@		|   and merge them into the software table
	andl	#~(MMU_R + MMU_M),d1 | Clear them from MMU pte
	movsl	d1,a0@		|   and put the hardware entry back
5$:				| Here to step over an entry after setup
	lea	a1@(PTE_SIZE),a1| ppte++
	lea	a0@(NBPG),a0	| MMU++
	subql	#1,kk		| Decrement counter
	blt	6$		|   and begin exit if loop exhausted
	movl	a1@,d0		| Otherwise, obtain software pte
	blt	4$		| If valid process it
	jra	5$		|   otherwise, continue to next one
6$:				| Here to restore alternate address space
	movb	ssfc,d0		|   codes on the way out
	movc	d0,sfc		| Restore source
	movb	sdfc,d0		|   and the destination
	movc	d0,dfc		| ...
1$:				| Here to unlink and exit
	unlk	a6		| Scrap local frame
	rts			|   and return

#undef	page			/* Clear out frame definitions */
#undef	ppte
#undef	kk
#undef	ssfc
#undef	sdfc

/*
 * loadpgmap(page, ppte, new, kk)
 *	int page,		| Page #
 *	    new,		| Flag indicating whether to preserve M & R
 *	    kk;			| # pte's to load
 *	struct pte *ppte;	| Software pte corresponding to page #
 * Load kk software pte's into the MMU.  Perform the appropriate transforms
 * on the software pte structure to fit the MMU.
 */

#define	page	a6@(8)		/* Argument offsets */
#define	ppte	a6@(12)
#define	new	a6@(16)
#define	kk	a6@(20)
#define	ssfc	a6@(-2)
#define	sdfc	a6@(-1)

ENTRY(loadpgmap)
	link	a6,#-2		| Set up temporaries, adjust stack
	subql	#1,kk		| Any count at all?
	blt	1$		| No, forget this thing
	movc	sfc,d0		| Save away the current settings of the
	movb	d0,ssfc		|   alternate address spaces so that
	movc	dfc,d0		|   we can change them to refer to
	movb	d0,sdfc		|   MMU space
	movl	#FC_MAP,d0	| Do so now --
	movc	d0,sfc		|   movsx fetches come from MMU,
	movc	d0,dfc		|   and now movsx stores go to it
	movl	ppte,a1		| Obtain the pointer to the first software pte
	movl	page,d0		| Now get the page number,
	movl	#PGSHIFT,d1	|   and a shift factor to position the
	lsll	d1,d0		|   bits for the MMU
	movl	d0,a0		| Place it to index through MMU registers
2$:				| Here to process a pte 
	movl	a1@+,d0		| Obtain pte and bump pointer
	bge	3$		| If not valid, simply store it in MMU
	movl	d0,d1		| Copy to obtain page type
	andl	#0x8C00FFFF,d0	| Get valid + user access bits, and then the
	andl	#0x00030000,d1	|   page type bits.  Finally shift and merge
	lsll	#6,d1		|   them to form a proper MMU entry.  Note the
	orl	d1,d0		|   complete lack of symbols for these things
	btst	#27,d0		| With similar use of constants, see if user
	beq	4$		|   read is set, take the branch if not
	bset	#25,d0		| If it is, also set execute
4$:				| Here with MMU entry set up, see if need to
	tstl	new		|   need to preserve existing MMU_R and MMU_M
	bne	3$		| Take the branch if brand new entry
	movsl	a0@,d1		| Otherwise, obtain the existing MMU entry
	andl	#(MMU_R + MMU_M),d1 | Keep just the R and M bits
	orl	d1,d0		|   and merge them into the new entry
3$:				| Here with final MMU value
	movsl	d0,a0@		| Store the MMU pte in MMU
	lea	a0@(NBPG),a0	| MMU++
	subql	#1,kk		| Any more entries to process?
	bge	2$		|   Take the branch back if so and do all
	movb	ssfc,d0		| Restore the
	movc	d0,sfc		|   source and
	movb	sdfc,d0		| likewise the
	movc	d0,dfc		|   destination
1$:				| Here to unlink and leave really
	unlk	a6		| Recover frame
	rts			|   and return

#undef	page			/* Kill local frame variables */
#undef	ppte
#undef	new
#undef	kk
#undef	ssfc
#undef	sdfc

	ENTRY(getpgmap)
	movl	sp@(4),d0	| Get access address
	clrb	d0
	movl	d0,a0
	movc	sfc,d1		| Save source function code
	lea	FC_MAP,a1	| Get function code in a reg
	movc	a1,sfc		| Set source function code
	movsl	a0@,d0		| Read page map entry
	movc	d1,sfc		| Restore source function code
	movl	d0,d1
	andl	#0xFC000FFF,d0
	andl	#0x00C00000,d1
	lsrl	#6,d1
	orl	d1,d0
	rts

	ENTRY(setpgmap)
	movl	sp@(4),d0	| Get access address
	clrb	d0
	movl	d0,a0
	movl	sp@(8),d0	| Get page map entry to write
	movl	d0,d1
	andl	#0xFC00FFFF,d0	| throw away software fields
	andl	#0x30000,d1	| save type field
	lsll	#6,d1		| move it into place
	orl	d1,d0		| or it in
	btst	#27,d0		| user read access set?
	beq	3$
	bset	#25,d0		| yes, set user execute bit
3$:
	movc	dfc,d1		| Save dest function code
	lea	FC_MAP,a1	| Get function code in a reg
	movc	a1,dfc		| Set destination function code
	movsl	d0,a0@		| Write page map entry
	movc	d1,dfc		| Restore dest function code
	rts			| done

	ENTRY(getsegmap)
	movl	sp@(4),d0	| Get access address
	movl	#SGSHIFT,d1	| convert to address
	lsll	d1,d0
	addql	#SMAPOFF,d0	| Set to segment map offset
	movl	d0,a0
	movc	sfc,d1		| Save source function code
	lea	FC_MAP,a1	| Get function code in a reg
	movc	a1,sfc		| Set source function code
	moveq	#0,d0		| Clear upper part of register
	movsb	a0@,d0		| Read segment map entry
	movc	d1,sfc		| Restore source function code
	rts			| done

	ENTRY(setsegmap)
	movl	sp@(4),d0	| Get segment number
	movl	#SGSHIFT,d1	| convert to address
	lsll	d1,d0
	addql	#SMAPOFF,d0	| Set to segment map offset
	movl	d0,a0
	movl	sp@(8),d0	| Get seg map entry to write
	movc	dfc,d1		| Save dest function code
	lea	FC_MAP,a1	| Get function code in a reg
	movc	a1,dfc		| Set destination function code
	movsb	d0,a0@		| Write segment map entry
	movc	d1,dfc		| Restore dest function code
	rts			| done

	ENTRY(getusercontext)
	movc	sfc,d1		| Save source function code
	lea	FC_MAP,a1	| Get function code in a reg
	movc	a1,sfc		| Set source function code
	moveq	#0,d0		| Clear upper part of result
	movsb	USERCONTEXTOFF,d0 | Move context reg into result
	andb	#CONTEXTMASK,d0	| Clear high-order bits
	movc	d1,sfc		| Restore source function code
	rts

	ENTRY(setusercontext)
	movb	sp@(7),d0	| Get context value to set
	movc	dfc,d1		| Save dest function code
	lea	FC_MAP,a1	| Get function code in a reg
	movc	a1,dfc		| Set destination function code
	movsb	d0,USERCONTEXTOFF | Move value into context register
	movc	d1,dfc		| Restore dest function code
	rts
