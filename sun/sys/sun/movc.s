	.data
	.asciz  "@(#)movc.s 1.5 86/12/26 Copyr 1984 Sun Micro"
	.even
	.text

|	Copyright (c) 1984 by Sun Microsystems, Inc.

#include "../machine/asm_linkage.h"
#include "../machine/param.h"

#ifdef VAC
#include "../machine/mmu.h"
#include "../machine/pte.h"
#include "assym.s"
	.data
bcopys:	.byte	0		| semaphore for bcopy
ctx_save: .byte 0
	.even
	.long 	0, 0, 0, 0
blockz:
	.long 	0, 0, 0, 0
	.text
#endif VAC

| Copy a block of storage - must not overlap ( from + len <= to)
| Usage: bcopy(from, to, count)
	ENTRY(bcopy)
	movl	sp@(4),a0	| from
	movl	sp@(8),a1	| to
	movl	sp@(12),d0	| get count
	jle	ret		| return if not positive
| If from address odd, move one byte to 
| try to make things even
	movw	a0,d1		| from
	btst	#0,d1		| test for odd bit in from
	jeq	1$		| even, skip
	movb	a0@+,a1@+	| move a byte
	subql	#1,d0		| adjust count
| If to address is odd now, we have to do byte moves
1$:	movl	a1,d1		| low bit one if mutual oddness
	btst	#0,d1		| test low bit
	jne	bytes		| do it slow and easy
| The addresses are now both even 
| Now move longs
	movl	d0,d1		| save count
	lsrl	#2,d0		| get # longs
#ifdef VAC
VAC_MINBCOPY = 2*VAC_LINESIZE
/*
 * WARNING - THIS CODE IS NOT REENTRANT
 * Fast copy routine uses the Block Copy commands of a VAC machine.
 * The Block Copy Read and Write commands copy VAC_LINESIZE (16) bytes
 * at a time and avoid the displacement of any valid data from the cache.
 * We use Block Copy Read and Write until there are less than
 * VAC_LINESIZE bytes to move.  Here, a0 = from, a1 = to, (a0 and a1 are
 * in word boundary. d0 = count in long words, and d1 = count in bytes.
 * Doesn't worth doing Block Copy Read/Write if 2 * VAC_LINESIZE >= byte count
 */
	tstl	_vac			| test if there is VAC
	jeq	skip			| ENA_CACHE bit is off, skip fast copy
	cmpl	#VAC_MINBCOPY,d1
	jlt	skip			| 2*VAC_LINESIZE >= byte count
	tas	bcopys			| only one person at a time
	jne	skip			| busy, skip fast copy
	moveml	d0/a0,sp@-		| save d0, a0
	movl	a0,sp@-			| check from address
	jsr	_getpgmap		| is this OBMEM?
	addqw	#4,sp
	andl	#PG_TYPE,d0
	jeq	2$			| yes - check next address
	bra	3$
2$:
	movl	a1,sp@-			| check to address
	jsr	_getpgmap		| is this OBMEM
	addqw	#4,sp
	andl	#PG_TYPE,d0
	jeq	4$
3$:
	moveml	sp@+,d0/a0		| restore d0, a0
	jmp 	skip
4$:
	moveml	sp@+,d0/a0		| restore d0, a0
	moveml	d2/d3/d4,sp@-		| save d2, d3, d4
/* Use normal bcopy if a0 and a1 are not alligned according to line size */
	movl	a0,d2
	andl	#0xF,d2			| get byte residual of line size of a0
	movl	a1,d3
	andl	#0xF,d3			| get byte residual of line size of a1
	cmpl	d2,d3
	jne	done			| not line size alligned
	cmpl	#0,d2			| skip word move if in 16 byte boundary
	jeq	3f
/*
 * A0 (from) and a1(to) are aligned to each other in terms of line size (16).
 * Move a word (2 bytes) at a time until byte residual of "from" is zero.
 * (A0 and a1 are in word boundary when "fast" bcopy is entered.)
 */
	movl	#VAC_LINESIZE,d2	| figure out number of words to move
	subl	d3,d2			| d2 = byte to move to align to 16
	subl	d2,d1			| update byte count
	movl	d1,d0			| updated long word count
	lsrl	#2,d0
	lsrl	#1,d2			| no. of words to move in the residual
	jra	2f			| enter word move loop
1:	movw	a0@+,a1@+		| move a word in the residual part
2:	dbra	d2,1b			| decr, br if >= 0
3:
	movl	d0,d2			| save long word count
	lsrl	#VAC_LINE_SHIFT,d0	| no. of lines to move
/* Set up Block Copy R/W commands to a0 (from) and a1 (to). */
	movl	a0,d4			| to set up Block Copy R/W command
	andl	#VAC_BLOCK_OFF,d4	| mask off bits <31,28>
	orl	#VAC_BLOCK_CPCMD,d4	| set up "from"
	movl	d4,a0
	movl	a1,d4			| to set up Block Copy R/W command
	andl	#VAC_BLOCK_OFF,d4	| mask off bits <31,28>
	orl	#VAC_BLOCK_CPCMD,d4 	| set up "to"
	movl	d4,a1
/* Block Copy R/W loop, one line at a time. */
	jra	5f			| enter block copy loop
4:	movsl	a0@,d4			| Block Copy Read, d4 is dummy
	movsl	d4,a1@			| Block Copy Write, d4 is dummy
	addl	#VAC_LINESIZE,a0	| update "from"
	addl	#VAC_LINESIZE,a1	| update "to"
5:	dbra	d0,4b			| decr, br if >= 0
	clrw	d0			| Clear low 
	subql	#1,d0			| Borrow from high half if != 0
	jpl	4b			| loop if high half was not 0
/* Mask off Block Copy R/W commands in a0 (from) and a1 (to). */
	movl	a0,d4			| to mask off Block Copy R/W command
	andl	#VAC_BLOCK_OFF,d4	| mask off "from"
	movl	d4,a0
	movl	a1,d4			| to mask off Block Copy R/W command
	andl	#VAC_BLOCK_OFF,d4	| mask off "to"
	movl	d4,a1
/*
 * Update long count in d0, since only the last two bits of d1 (byte count)
 * are to be used and the last two bits of d1 are not affected by the Block
 * Copy R/W, we don't have to update d1.
 */
	movl	d2,d0
	andl	#VAC_LINE_RESIDU,d0	| update long word count
done:
	clrb	bcopys			| reset semaphore
	moveml	sp@+,d2/d3/d4		| restore d2, d3, and d4
skip:
#endif VAC
	jra	3$		| enter long move loop
| The following loop runs in loop mode on 68010
2$:	movl	a0@+,a1@+	| move a long
3$:	dbra	d0,2$		| decr, br if >= 0
	clrw	d0		| Clear low 
	subql	#1,d0		| Borrow from high half, get back F's in lower
	jpl	2$		| loop if more (if top non-negative, still more)
| Now up to 3 bytes remain to be moved
	movl	d1,d0		| restore count
	andl	#3,d0		| mod sizeof long
	jra	bytes		| go do bytes

| Here if we have to move byte-by-byte because
| the pointers didn't line up.  68010 loop mode is used.
bloop:	movb	a0@+,a1@+	| loop mode byte moves
bytes:	dbra	d0,bloop
ret:	rts

| Block copy with possibly overlapped operands
	ENTRY(ovbcopy)
	movl	sp@(4),a0	| from
	movl	sp@(8),a1	| to
	movl	sp@(12),d0	| get count
	jle	ret		| return if not positive
	cmpl	a0,a1		| check direction of copy
	jgt	bwd		| do it backwards
| Here if from > to - copy bytes forward
	jra	2$
| Loop mode byte copy
1$:	movb	a0@+,a1@+
2$:	dbra	d0,1$
	rts
| Here if from < to - copy bytes backwards
bwd:	addl	d0,a0		| get end of from area
	addl	d0,a1		| get end of to area
	jra	2$		| enter loop
| Loop mode byte copy
1$:	movb	a0@-,a1@-
2$:	dbra	d0,1$
	rts


| Zero block of storage
| Usage: bzero(addr, length)
	ENTRY2(bzero,blkclr)
	movl	sp@(4),a1	| address
	movl	sp@(8),d0	| length
	clrl	d1		| use zero register to avoid clr fetches
	btst	#0,sp@(7)	| odd address?
	jeq	1$		| no, skip
	movb	d1,a1@+		| do one byte
	subql	#1,d0		| to adjust to even address
1$:	movl	d0,a0		| save possibly adjusted count
	lsrl	#2,d0		| get count of longs
#ifdef VAC
/*
 * WARNING - THIS CODE IS NOT REENTRANT
 * Fast zero routine uses the Block Copy commands of a VAC machine.
 * The Block Copy Read and Write commands copy VAC_LINESIZE bytes
 * at a time and avoid the displacement of any valid data from the cache.
 * We use Block Copy Read and Write until there are less than
 * VAC_LINESIZE bytes to zero.
 * d0 = count in long words, d1 = 0, a0 = saved count in bytes, a1 = to.
 * Doesn't worth doing Block Copy Read/Write if 2 * VAC_LINESIZE >= byte count
 */ 
	tstl	_vac			| test if there is VAC
	jeq	skip1			| ENA_CACHE bit is off, skip fast zero
	cmpl	#VAC_MINBCOPY,a0
	jlt	skip1			| 2*VAC_LINESIZE >= byte count
	tas	bcopys			| only one person at a time
	jne	skip1			| busy, skip fast copy
	moveml	d0/a0,sp@-		| save d0, a0
	movl	a1,sp@-
	jsr	_getpgmap		| is this OBMEM
	addqw	#4,sp
	andl	#PG_TYPE,d0
	jeq	2$
	moveml	sp@+,d0/a0		| restore d0, a0
	jmp 	skip1
2$:
	moveml	sp@+,d0/a0		| restore d0, a0
	moveml	d2/d3/a2,sp@-		| save d2-d3 and a2
/* Align a1 to line size if not already there; a1 is in word boundary */
	movl	a1,d2
	andl	#0xF,d2			| get byte residual of line size of a1
	jeq	2f			| aligned to line size (16) already
	movl	#VAC_LINESIZE,d3	| figure out number of words to move
	subl	d2,d3			| d3 = bytes to clear to align a1 to 16
	subl	d3,a0			| update byte count
	movl	a0,d0			| update long count
	lsrl	#2,d0
	lsrl	#1,d3			| no of words to clear to align a1 to 16
	jra	1f			| enter word clear loop
0:	movw	d1,a1@+			| clear a word
1:	dbra	d3,0b			| decr, br if >= 0
2:
	movl	d0,d2			| save long word count
	lsrl	#VAC_LINE_SHIFT,d0	| no. of lines to move
/*
 * Set up Block Copy R/W commands. blockz points to the middle of 32 bytes
 * of zeros. "from" (a2) is in 16 byte boundary and points to 16 bytes of
 * zero's.
 */
	lea	blockz,a2		| get address of zero block
	movl	a2,d3			| set up "from"
	andl	#0x0FFFFFF0,d3		| mask off lower 16 bytes and <31,28>
	orl	#VAC_BLOCK_CPCMD,d3	
	movl	d3,a2
	movsl	a2@,d3			| Block Copy Read, d3 is dummy
	movl	a1,d3			| to set up Block Copy R/W command
	andl	#VAC_BLOCK_OFF,d3	| mask off bits <31,28> of "to"
	orl	#VAC_BLOCK_CPCMD,d3 	| set up "to"
	movl	d3,a1
/* Block Copy R/W loop to zero VAC_LINESIZE bytes at a time. */
	jra	7f			| enter block copy loop
6:	movsl	d3,a1@			| Block Copy Write, d3 is dummy
	addl	#VAC_LINESIZE,a1	| update "to"
7:	dbra	d0,6b			| decr, br if >= 0
	clrw	d0			| Clear low 
	subql	#1,d0			| Borrow from high half if != 0
	jpl	6b			| loop if high half was not 0
/* Mask off Block Copy Write command in a1 (to). */
	movl	a1,d3			| to mask off Block Copy R/W command
	andl	#VAC_BLOCK_OFF,d3	| mask off "to"
	movl	d3,a1
/*
 * Update long count in d0, since only the last two bits of a0 (byte count)
 * are to be used and the last two bits of a0 are not affected by the Block
 * Copy R/W, we don't have to update a0.
 */
	movl	d2,d0			| update long word count
	andl	#VAC_LINE_RESIDU,d0
	clrb	bcopys			| reset semaphore
	moveml	sp@+,d2/d3/a2		| restore d2, d3, and a2
skip1:
#endif VAC
	jra	3$		| go to loop test
| Here is the fast inner loop - loop mode on 68010
2$:	movl	d1,a1@+		| store long
3$:	dbra	d0,2$		| decr count; br until done
	clrw	d0		| Clear low 
	subql	#1,d0		| Borrow from high half, get back F's in lower
	jpl	2$		| loop if more (if top non-negative, still more)
| Now up to 3 bytes remain to be cleared
	movl	a0,d0		| restore count
	btst	#1,d0		| need a short word?
	jeq	4$		| no, skip
	movw	d1,a1@+		| do a short
4$:	btst	#0,d0		| need another byte
	jeq	5$		| no, skip
	movb	d1,a1@+		| do a byte
5$:	rts			| all done
