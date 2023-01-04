/*      @(#)map.s 1.1 86/09/25 SMI      */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Memory Mapping and Paging on the Sun-3.
 * NOTE:  All of these routines assume that the
 * default sfc and dfc have been preset to FC_MAP.
 */

#include "../h/param.h"
#include "../machine/asm_linkage.h"
#include "../machine/mmu.h"
#include "../machine/pte.h"
#include "assym.s"

	.text

|
| Sets the pte referenced and modified bits based on the pme for
| pageno, and then clears these bits in the pme.
|
| unloadpgmap(pageno, ppte, count)
|	u_int pageno;		/* Starting page frame number */
|	struct pte *ppte;	/* Pointer to starting pte entry */
|	int count;		/* Number of pte's to process */
|

#define	pageno	sp@(4)
#define	ppte	sp@(8)
#define	count	sp@(12)

ENTRY(unloadpgmap)
	movl	ppte,a1			| a1 -> register pte *ppte
1$:					| Loop to find initial valid entry
	subql	#1,count		| Any more to process?
	blt	0$			|   Leave if not
	movl	a1@,d0			| Get this pte entry
	bpl	11$			|   If invalid, skip
	btst	#PG_FOD_BIT,d0		| If FOD, this page is mmap'ed
	beq	2$			|   If not, start scanning MMU
11$:	lea	a1@(PTE_SIZE),a1	| ppte++
	addql	#1,pageno		| pageno++
	jra	1$			| Continue looking for first entry
2$:					| Initialize MMU index value
	movl	pageno,d0		| Get integer page frame index
	moveq	#PGSHIFT,d1		| Value to turn index to address
	lsll	d1,d0			| (pageno << PGSHIFT) -> MMU offset
	orl	#PAGEBASE,d0		| Form actual MMU address
	movl	d0,a0			| a0 -> register "page" *pageno
4$:					| Handle valid pte entry
	movsl	a0@,d0			| Get MMU value
	movl	d0,d1			| Copy for later use and conditions
	bge	3$			| If this is invalid, skip it all
	andl	#(PG_R+PG_M),d0		| Clear all but referenced/modified
	orl	d0,a1@			| Merge them into pte 
	andl	#~(PG_R+PG_M),d1	| Clear them
	movsl	d1,a0@			|   and put them back in MMU
3$:					| Loop close
	subql	#1,count		| count--
	blt	0$			|   If exhausted, leave
	lea	a1@(PTE_SIZE),a1	| ppte++
	lea	a0@(NBPG),a0		| vaddr += NBPG
	movl	a1@,d0			| Get new entry
	bpl	3$			|   Skip if not valid
	btst	#PG_FOD_BIT,d0		| Check if mmap'ed
	beq	4$			| Process if valid
	jra	3$			| Otherwise, look for another one
0$:
	rts

#undef	pageno
#undef	ppte
#undef	count

	|
	| Read the page map entry for the given address v
	| and return it in a form suitable for software use.
	|
	| long
	| getpgmap(v)
	| caddr_t v;
	ENTRY(getpgmap)
	movl	sp@(4),d0		| get access address
	andl	#PAGEADDRBITS,d0	| clear extraneous bits
	orl	#PAGEBASE,d0		| set to page map base offset
	movl	d0,a0
	movsl	a0@,d0			| read page map entry
					| no mods needed to make pte from pme
	rts				| done

	|
	| Set the pme for address v using the software pte given.
	|
	| setpgmap(v, pte)
	| caddr_t v;
	| long pte;
	ENTRY(setpgmap)
	movl	sp@(4),d0		| get access address
	andl	#PAGEADDRBITS,d0	| clear extraneous bits
	orl	#PAGEBASE,d0		| set to page map base offset
	movl	d0,a0
	movl	sp@(8),d0		| get page map entry to write
					| no mods need to make pme from pte
	movsl	d0,a0@			| write page map entry
	rts				| done

|
| Load the pme for count pages starting at pageno and using the
| pte given.  If the original pme is valid and we are not on a 
| new pmeg, then the referenced and modified bits are preserved
| from the original pme.
|
| loadpgmap(pageno, ppte, new, count)
|	u_int pageno;		/* virtual page to set up */
|	struct pte *ppte	/* pointer to starting pte */
|	int new;		/* new pmeg flag */
|	int count;		/* count of ptes to load */

#define	pageno	sp@(4)
#define	ppte	sp@(8)
#define	new	sp@(12)
#define	count	sp@(16)

ENTRY(loadpgmap)
	subql	#1,count		| Skip all this if no count
	blt	0$			| tradeoff(space,time) => time
	movl	pageno,d0		| At least one, get frame number
	moveq	#PGSHIFT,d1		| Value to turn index to address
	lsll	d1,d0			| (pageno << PGSHIFT) -> MMU offset
	orl	#PAGEBASE,d0		| Form actual MMU address
	movl	d0,a1			| a1 -> register "page" *pageno
	movl	ppte,a0			| a0 -> register pte *pte
2$:					| Loop over ptes
#if (PTE_SIZE != 4)		/* Paranoia if size of pte is not a long */
	movl	a0@,d0			| Get a pte
#else
	movl	a0@+,d0			| Get a *pte++
#endif
	bge	1$			|   If invalid, skip all this
	tstl	new			| New pmeg?
	bne	1$			|   Yes, don't merge old ref/mod bits
	movsl	a1@,d1			| Otherwise, get the current pme
	andl	#(PG_R+PG_M),d1		| Mask off ref/mod bits
	orl	d1,d0			| Merge back into pte
1$:
	movsl	d0,a1@			| Put it in the MMU
	subql	#1,count		| Any more?
	blt	0$			|  Leave if not
#if (PTE_SIZE != 4)		/* For when size of pte is not a long */
	lea	a0@(PTE_SIZE),a0	| ppte++
#endif
	lea	a1@(NBPG),a1		| vaddr += NBPG
	jra	2$			| Continue loading
0$:
	rts

#undef	pageno
#undef	ppte
#undef	new
#undef	count

	|
	| Return the 8 bit segment map entry for the given segment number.
	|
	| u_char
	| getsegmap(segno)
	| u_int segno;
	ENTRY(getsegmap)
	movl	sp@(4),d0		| get segment number
	moveq	#SGSHIFT,d1		| get count for shift
	lsll	d1,d0			| convert to address
	andl	#SEGMENTADDRBITS,d0	| clear extraneous bits
	orl	#SEGMENTBASE,d0		| set to segment map offset
	movl	d0,a0
	moveq	#0,d0			| clear (upper part of) register
	movsb	a0@,d0			| read segment map entry
	rts				| done

	|
	| Set the segment map entry for segno to pm.
	|
	| setsegmap(segno, pm)
	| u_int segno;
	| u_char pm;
	ENTRY(setsegmap)
	movl	sp@(4),d0		| get segment number
	moveq	#SGSHIFT,d1		| get count for shift
	lsll	d1,d0			| convert to address
	andl	#SEGMENTADDRBITS,d0	| clear extraneous bits
	orl	#SEGMENTBASE,d0		| set to segment map offset
	movl	d0,a0
	movl	sp@(8),d0		| get seg map entry to write
	movsb	d0,a0@			| write segment map entry
	rts				| done

	|
	| Return the current [user] context number.
	|
	| int
	| getcontext()
	ENTRY2(getcontext,getusercontext)
	movsb	CONTEXTBASE,d0		| move context reg into result
	andl	#CONTEXTMASK,d0		| clear high-order bits
	rts				| done

	|
	| Set the current [user] context number to uc.
	|
	| setcontext(uc)
	| int uc;
	ENTRY2(setcontext,setusercontext)
	movb	sp@(7),d0		| get context value to set
	movsb	d0,CONTEXTBASE		| move value into context register
	rts				| done

#ifdef SUN3_260
	|
	| VAC (Virtual Address Cache) Flush by Context Match:
	| We issue the context flush command VAC_CTXFLUSH_COUNT times.
	| Each time we increment flush address by VAC_FLUSH_INCRMNT(2^9).
	| Context no. is in the context register.
	|
	| vac_ctxflush()
	ENTRY(vac_ctxflush)
	tstl	_vac			| test if there is VAC
	jeq	1f			| if not, just return

	addql	#1,_flush_cnt+FM_CTX	| increment flush_cnt.f_ctx
	movl	#VAC_FLUSH_BASE,a0	| get flush command base address
	movl	#VAC_CTXFLUSH_COUNT-1,d0 | loop this many of times
	movb	#VAC_CTXFLUSH,d1	| context flush "command"
0:	movsb	d1,a0@			| do a unit of ctx flush
	addl	#VAC_FLUSH_INCRMNT,a0	| the address of next ctx flush
	dbra	d0,0b			| do next flush cycle until done
1:
	rts

	|
	| VAC (Virtual Address Cache) Flush by Segment Match.
	| We issue the segment flush command VAC_SEGFLUSH_COUNT times.
	| Each time we increment flush address by VAC_FLUSH_INCRMNT(2^9).
	|
	| vac_segflush(segno)
	| u_int segno;
	ENTRY(vac_segflush)
	tstl	_vac			| test if there is VAC
	jeq	1f			| if not, just return

	addql	#1,_flush_cnt+FM_SEGMENT | increment flush_cnt.f_segment
	movl	sp@(4),d0		| get seg no.
	moveq	#SGSHIFT,d1		| segment shift count
	lsll	d1,d0			| convert seg no. to virtual address
	orl	#VAC_FLUSH_BASE,d0	| set to seg flush base address
	movl	d0,a0			| a0 gets seg flush base address
	movl	#VAC_SEGFLUSH_COUNT-1,d0 | loop this many of times
	movb	#VAC_SEGFLUSH,d1	| segment flush "command"
0:	movsb	d1,a0@			| do a unit of segment flush
	addl	#VAC_FLUSH_INCRMNT,a0	| the address of next segment flush
	dbra	d0,0b			| do next flush cycle until done
1:
	rts

	|
	| VAC (Virtual Address Cache) Flush by Page Match
	| We issue the page flush command VAC_PAGEFLUSH_COUNT times,
	| Each time we increment flush address by VAC_FLUSH_INCRMNT(2^9).
	|
	| vac_pageflush(vaddr)
	| caddr_t vaddr;
	ENTRY(vac_pageflush)
	tstl	_vac			| test if there is VAC
	jeq	1f			| if not, just return

	addql	#1,_flush_cnt+FM_PAGE	| increment flush_cnt.f_page
	movl	sp@(4),d0		| get vaddr of this page
	andl	#PAGEADDRBITS,d0	| clear extraneous bits
	orl	#VAC_FLUSH_BASE,d0	| set to page flush base address
	movl	d0,a0			| a0 gets page flush base address

	moveq	#VAC_PAGEFLUSH_COUNT-1,d0| loop this many of times
	movb	#VAC_PAGEFLUSH,d1	| page flush "command"
0:	movsb	d1,a0@			| do a unit of page flush
	addl	#VAC_FLUSH_INCRMNT,a0	| address of next page flush
	dbra	d0,0b			| do next flush cycle until done
1:
	rts

	|
	| Flush-by-Page-Match nbytes of bytes starting from vaddr.
	| We issue Page Match Flush commands until all nbytes
	| are flushed.  In Sun-3, each time VAC_FLUSH_INCRMNT (512)
	| bytes are flushed.
	|
	| vac_flush(vaddr, nbytes)
	| caddr_t vaddr;
	| int nbytes;
	ENTRY(vac_flush)
	tstl	_vac			| test if there is VAC
	jeq	1f			| if not, return

	addql	#1,_flush_cnt+FM_PARTIAL | incr flush_cnt.f_partial
	movl	sp@(4),d1		| get vaddr
	andl	#VAC_UNIT_MASK,d1	| get lower 9 bits
	movl	sp@(8),d0		| get nbytes
	addl	d1,d0			| d0 = no of bytes to flush
	addl	#VAC_FLUSH_INCRMNT-1,d0	| prepare for lsrl
	movl	#VAC_FLUSH_LOWBIT,d1
	lsrl	d1,d0			| d0 = no of units to flush
	movl	sp@(4),d1		| get vaddr
	andl	#VAC_UNIT_ADDRBITS,d1	| to start from flush unit boundary
	orl	#VAC_FLUSH_BASE,d1	| set to page flush base address
	movl	d1,a0			| a0 gets page flush base address
	movb	#VAC_PAGEFLUSH,d1	| page flush "command"
	jra	3f			| enter into dbra loop
2:	movsb	d1,a0@			| do a unit of Page Match Flush
	addl	#VAC_FLUSH_INCRMNT,a0	| address of next Page Match Flush
3:	dbra	d0,2b			| decr d0, br if d0 >= 0
1:
	rts

	|
	| Init the VAC by invaliding all cache tags.
	| We loop through all 64KB to reset the valid bit of each line.
	| It DOESN'T turn on cache enable bit in the enable register.
	|
	| vac_init()
	ENTRY(vac_init)
	movl	#VAC_RWTAG_BASE,a0	| set base address to R/W VAC tags
	movl	#VAC_RWTAG_COUNT-1,d0	| loop through all lines
	clrl	d1			| reset valid bit of tags
0:	movsl	d1,a0@			| invalid the tag of this line
	addl	#VAC_RWTAG_INCRMNT,a0	| address to write to next line
	dbra	d0,0b			| invalid next tag until done
	rts
#endif
