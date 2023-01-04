|
| 	@(#)diag.s 1.18 84/11/29 Copyright (c) 1983 by Sun Microsystems, Inc.
|
|	Power-Up Diagnostics for the Sun-2 Processor Board
|
|	This diagnostic routine attempts to return information about
|	which parts of the machine are broken, as bitmaps, so they can
|	be configured out of the system and it can continue to run.
|
|	Global Register allocation:
|	d0 = Contains the actual (eg read from memory) data for errors
|	d1 = 
|	d2*= damage control bits for what failed the tests
|	d3*= count of total errors detected
|	d4*= count of transient errors detected
|	d5 = 
|	d6 = Contains the secondary data value for the test
|	d7 = Contains the primary (eg expected) data value for the test
|	a0*= Address of failing test (for looping tests), or address of
|	     error message for tests that exit with a message
|	a1*= Address in use as of last error detected in loops
|	a2*= Data written   as of last error detected in loops
|	a3*= Data read back as of last error detected in loops
|	a4 = 
|	a5 = Points to the memory or map entry under test
|	a6*= Contains first address after main memory (=size of main mem)
|	a7 = 
|
|	  *= This information is valid upon exit to diagret.
|

#include "assym.h"
#include "../h/s2led.h"

	.globl	_qpdiag
	.text

| FIXME: Vector bus errors somewhere as soon as practical.
| FIXME: Write all values to some address in each map, to verify stuck bits.
| FIXME: Walk a one and a zero thru each map, to verify stuck bits.
| FIXME: Test parity circuitry, and protection.
|
|	Exit to here for errors that prevent successful return.
|	Example: all contexts are unusable.
|	We just re-run the tests in a big loop to aid diagnosis.
|
error:
	addql	#1,d3			| Count one more error
	movl	#LEDQUICK,d0		| Pause to human speeds
LElong:	subql	#1,d0
	bne	LElong
	jra	a0@			| Rerun failing test

|
|	Start of quick processor diagnostics for Sun-2.
|
_qpdiag:
	subl	a6,a6			| Memory size = 0 so far
	moveq	#-1,d2			| Assume all is OK for now.
	moveq	#0,d3			| No errors counted so far.


	movw	#FC_MAP,a7		| Set up to read and write maps
	movc	a7,sfc
	movc	a7,dfc

|
|	Cycle the LED's quickly so we can see if they work.
|
	movw	#0x7FFF,d7		| Start with no bits lit
LEloop:
	movsb	d7,LEDOFF

	movl	#LEDQUICK,d0		| Pause to human speeds
LEquick:subql	#1,d0
	bne	LEquick

	rolw	#1,d7			| Shift the bit
	cmpb	#0xFF,d7
	jmi	LEloop			| Loop unless 0-bit shifted out

|
|	Test all values of the context registers.
|
|	This does not test that setting the context reg to different values
|	has any effect on mapping, it just tests that all values can be held.
|
|	FIXME, this does not test byte accesses.
|
BOTHMASK = (CONTEXTMASK*256) + CONTEXTMASK
CXstart:
	lea	CXstart,a0	| Indicate we're in the context reg test
	moveq	#~L_CONTEXT,d0
	movsb	d0,LEDOFF

	movw	#BOTHMASK,d7
CXloop:
	andw	#BOTHMASK,d7	| Kill off intermediate bits between user&sys
	movsw	d7,CONTEXTOFF
	movsw	CONTEXTOFF,d0
	andw	#BOTHMASK,d0	| Some bits read back indeterminate
	cmpw	d7,d0
	dbne	d7,CXloop	| Loop til done, or brown on top.
	jeq	CXdone		| If OK, go on to next.
| Context reg failure.
	bclr	#CXBIT,d2	| Indicate failure of context reg
	jra	error		| If bad, tell somebody

CXdone:
 
|
|	Test all bits of the segment maps with constant data.
|
	movw	#0xAA,d7	| First test with bit config AA.
SMcontop:
	lea	SMcontop,a0	| We're in the seg map constant data test
	moveq	#~L_SM_CONST,d0
	movsb	d0,LEDOFF

SMconst:
	moveq	#NUMCONTEXTS-1,d5	| Start with highest context
SMcon1:
	movsb	d5,USERCONTEXTOFF
	lea	ADRSPC_SIZE,a5		| Start beyond highest addr in context
|
| Fill the segment map entries for this context.
|
SMcon2:
	subl	#PGSPERSEG*BYTESPERPG,a5| Back down to the next segment
	movsb	d7,a5@(SMAPOFF)		| Fill in its map entry
	movl	a5,d0			| Was that address 0 we just filled?
	jne	SMcon2			| (No; loop in this context)
	dbra	d5,SMcon1		| (yes; do next context or fall thru)

| Now read back the data and check it.
	moveq	#NUMCONTEXTS-1,d5
SMcon3:
	movsb	d5,USERCONTEXTOFF
	lea	ADRSPC_SIZE,a5
SMcon4:
	subl	#PGSPERSEG*BYTESPERPG,a5
SMconerr:
	movsb	a5@(SMAPOFF),d0		| Read the byte
	cmpb	d0,d7
	jeq	SMcok
| Write and read in a tight loop for scoping.
	addql	#1,d3			| Bump the error count
	bclr	#SEGBIT,d2		| Indicate seg map error
	movsb	d7,a5@(SMAPOFF)		| Write the data back to there again.
	jra	SMconerr		| Loop closely waiting til it's right.
SMcok:
	movl	a5,d0
	jne	SMcon4
SMconlz:
	dbra	d5,SMcon3

|
|	Repeat constant data test with the opposite set of bits.
|
	cmpb	#0x55,d7	| Did we test with these bits yet?
	jeq	SMcondone
	movw	#0x55,d7	| Nope, go back and do it.
	jra	SMconst

|
|	Test that segment map data lines are not shorted.
|
SMcondone:
	lea	SMcondone,a0
	moveq	#~L_SM_DATA,d0
	movsb	d0,LEDOFF

	movw	#0xFF,d7	| Start at FF and work downward
SMdata: movsb	d7,SMAPOFF	| Write it to first map entry
	movsb	SMAPOFF,d0	| Read it back from there
	cmpb	d7,d0		| See if they're the same.
	dbne	d7,SMdata	| Loops in if no error
	jeq	SMdataok	| Skip if all is OK.
| Error detected.  Loop closely until it fixes itself.
	addql	#1,d3		| Bump the error count
	bclr	#SEGBIT,d2	| Indicate failure in seg map
	jra	SMdata		| Loop closely if error

SMdataok:

|
|	Test address line independence in the segment map.
|
|	Write a unique value to each address to determine address lines
|	that are permanently stuck or tied to some value.
|
|	We write different values to the the same address in different
|	contexts, to make sure the context register (high-order address
|	lines) is also working.  Note this only tests the User half of the
|	context register selection logic, since the Super half is not
|	used for map access.  We could test that in the memory test.
|	Keep that in mind -- FIXME.
|
SMaddr:
	lea	SMaddr,a0	| Beginning seg map address test
	moveq	#~L_SM_ADDR,d0
	movsb	d0,LEDOFF

	clrw	d7
	moveq	#NUMCONTEXTS-1,d5
SMaddr1:
	movsb	d5,USERCONTEXTOFF
	movl	d5,d7		| Initialize to diff value by context
	lea	ADRSPC_SIZE,a5
SMaddr2:
	subl	#PGSPERSEG*BYTESPERPG,a5
	movsb	d7,a5@(SMAPOFF)
	addqb	#1,d7
	movl	a5,d0
	jne	SMaddr2
	dbra	d5,SMaddr1

| Now read back the data and check it.
	moveq	#NUMCONTEXTS-1,d5
SMaddr3:
	movsb	d5,USERCONTEXTOFF
	movl	d5,d7		| Initialize to diff value by context
	lea	ADRSPC_SIZE,a5
SMaddr4:
	subl	#PGSPERSEG*BYTESPERPG,a5
	movsb	a5@(SMAPOFF),d0
	cmpb	d0,d7
	jne	SMaddrerr	| It didn't work...
	addqb	#1,d7
	movl	a5,d0
	jne	SMaddr4
SMaddrlz:
	dbra	d5,SMaddr3
	jra	SMaddrok

| Failure in seg map addr test
| FIXME to loop a lot closer.  Ask Wayne what would be best here?  Bruce?
SMaddrerr:
	bclr	#SEGBIT,d2	| Indicate segment map failure
	jra	error

SMaddrok:

#ifdef FIXME
|
|	Test address line independence, and leaky memory cells.
|	Write alternating data to the segment maps.
|
|	FIXME: Is this useful, considering the context register
|	value is fed as low-order bits?  Should we be varying low-order
|	bits by changing context reg instead?  Should we skip this test?
|
|	Andy claims that static RAMs don't show this kind of failures.
|	No single address line is likely to fail -- it will be single
|	cell failures or whole chip failures.  At least I think that's
|	what he said.
|
|	FIXME: consider removing this whole test.  Ask Doug, Bernard, Peter...
|
	lea	SMaddrok,a0	| We're in the seg map alternating test
	moveq	#~L_SM_ALT,d0
	movsb	d0,LEDOFF

	movb	#0xAA,d6	| Alternate these two values thru map
	movb	#0x55,d7
	moveq	#NUMCONTEXTS-1,d5
SMalt1:
	movsb	d5,USERCONTEXTOFF
	lea	ADRSPC_SIZE,a5
SMalt2:
	subl	#PGSPERSEG*BYTESPERPG,a5
	movsb	d7,a5@(SMAPOFF)
	subl	#PGSPERSEG*BYTESPERPG,a5
	movsb	d6,a5@(SMAPOFF)
	movl	a5,d0
	jne	SMalt2
	dbra	d5,SMalt1

| Now read back the data and check it.
	moveq	#NUMCONTEXTS-1,d5
SMalt3:
	movsb	d5,USERCONTEXTOFF
	lea	ADRSPC_SIZE,a5
SMalt4:
	subl	#PGSPERSEG*BYTESPERPG,a5
	movsb	a5@(SMAPOFF),d0
	cmpb	d0,d7
	jne	error		| It didn't work...
|	Interesting registers at this point:
|	d0 = data read back from segment map (low byte)
|	d5 = context number we are testing
|	d7 = data written to segment map (low byte) (even addrs)
|	a5 = address in segment map where data was read
	subl	#PGSPERSEG*BYTESPERPG,a5
	movsb	a5@(SMAPOFF),d0
	cmpb	d0,d6
	jne	error		| It didn't work...
|	Interesting registers at this point:
|	d0 = data read back from segment map (low byte)
|	d5 = context number we are testing
|	d6 = data written to segment map (low byte) (odd addrs)
|	a5 = address in segment map where data was read
	movl	a5,d0
	jne	SMalt4
SMaltlz:
	dbra	d5,SMalt3
#endif FIXME

|
|	Segment map is tested.  Set up for page map tests.
|
|	Then, initialize cx 0's segment map to map all the pmegs sequentially
|	into segments.  This assumes that there are always at least as many
|	segments as there are pmegs -- currently there are twice as many.
|	If this ever changes, we'll have to mess around inside the loops
|	in the page map tests, to remap segments to pmegs.
|
|	This setting up allows us to address the page map ram in the following
|	tests by generating a 1 to 1 correspondence between program address
|	bits and bits that come out of the segment map (which actually provide
|	input to the page map ram).
|
|	PMEG:	address lines that come from the segment map to the page map.
|		They provide the most significant bits to the page map ram
|		address inputs (the least significant bits come from the sys-
|		tem address bus).  In the current Sun 2 architecture, the
|		PMEG is a block of 16 page map entries; i.e. leaves 4 low
|		bits from the address bus. Those bits are 11, 12, 13, 14.
|
|	Register allocation:
|	d0 = 
|	d1 = 
|	d5 = Counts how many page map entries are left to work on
|	d6 = Contains the secondary data value for the test
|	d7 = Contains the primary (eg expected) data value for the test
|	a5 = Points to the page map entry under test
|	a6 = 
|	a7 = 
|

	moveq	#0,d0
	movsb	d0,USERCONTEXTOFF
	movl	#(NUMPMEGS-1)*PGSPERSEG*BYTESPERPG,a5
	movw	#NUMPMEGS-1,d5
SMok2:
	movsb	d5,a5@(SMAPOFF)
	subl	#PGSPERSEG*BYTESPERPG,a5
	dbra	d5,SMok2

|
|	Test page map.
|
|	First a pair of constant data tests that ensure that each bit
|	in the page map can take on both the values zero and one.
|
|	We write zeros into the "unimplemented" section in the
|	middle of the maps.  We do not check the readback data, though.
|
|	FIXME:  None of the page map tests currently detect short between
|		MA22 and MA21 (and possibly many other shorts).  Find out
|		why and fix it.  (This may not still be true, it's an old
|		comment.)
|
|	Register allocation:
|	d0 = Contains the value read back from the maps
|	d1 = Mask for which page map bits are implemented in hardware
|	d5 = Counts how many page map entries are left to work on
|	d6 = 
|	d7 = Contains the primary (eg expected) data value for the test
|	a5 = Points to the page map entry under test
|	a6 = 
|	a7 = 

	movl	#PMREALBITS,d1	| Mask for implemented pagemap bits
	movl	#0x33333333,d7	| First test configuration...

PMconst:
	andl	d1,d7		| Mask out the don'tcares.
	lea	PMconst,a0	| We're in page map constant test
	moveq	#~L_PM_CONST,d0
	movsb	d0,LEDOFF

	movl	#(NUMPMEGS*PGSPERSEG)-1,d5
	subl	a5,a5
PMcon1:
	movsl	d7,a5@(PMAPOFF)
	addw	#BYTESPERPG,a5
	dbra	d5,PMcon1

| Now read back the data and check it.
	movl	#(NUMPMEGS*PGSPERSEG)-1,d5
	subl	a5,a5

PMcon2:
	movsl	a5@(PMAPOFF),d0
	andl	d1,d0
	cmpl	d0,d7
	jne	PMconerr
PMconlz:
	addw	#BYTESPERPG,a5
	dbra	d5,PMcon2

|
|	Repeat constant data test with the opposite set of bits.
|
	cmpb	#0xCC,d7	| Did we test with these bits yet?
	jeq	PMcondone
	notl	d7		| Invert and try again.
	jra	PMconst

| Failure in page map constant data test.
PMconerr:
	addql	#1,d3		| Count one more error
	bclr	#PMBIT,d2	| Indicate page map failure
	movsl	d7,a5@(PMAPOFF)	| Write the test pattern back into page map
	jra	PMcon2		| Scope loop...

| Page map address test failure.
| FIXME to loop a lot closer!
PMerror:
	bclr	#PMBIT,d2	| Indicate page map failure
	jra	error

|
|	Test for shorted data lines in the page map.
|
|	Walk a zero thru an entry of ones, and vice verse.
|
PMcondone:
	lea	PMcondone,a0	| We are in the Page map data test
	moveq	#~L_PM_DATA,d0
	movsb	d0,LEDOFF

	moveq	#0xFFFFFFFE,d7	| Start with bottom bit zero, rest one

PMdata1:
	moveq	#31,d5		| Count 32 rotations of the bit
PMdatloop:
	movsl	d7,PMAPOFF	| Write the entry to the map
	movsl	PMAPOFF,d0	| Read it back
	eorl	d7,d0		| Compare the two
	andl	d1,d0		| (Mask out unimplemented bits)
	jeq	PMdatok		| Hop if no error
	addql	#1,d3		| "Four more years!"
	bclr	#PMBIT,d2	| Indicate page map failure
	jra	PMdatloop	| Write the entry again & loop for scoping
PMdatok:
	roll	#1,d7		| Walk the bit to the left
	dbra	d5,PMdatloop

	notl	d7		| Switch to walking one thru zero entry
	jpl	PMdata1		| And run the test again.

|
|	Test address line independence in the page map.
|
|	Write a unique value to each address to determine address lines
|	that are permanently stuck or tied to some value.
|
|	This test will have to change if we ever have more page map
|	entries than we have low-order bits implemented in the maps.
|	We currently *JUST* fit.
|
PMaddr:
	lea	PMaddr,a0	| We are in the Page map address test
	moveq	#~L_PM_ADDR,d0
	movsb	d0,LEDOFF

	movl	#(NUMPMEGS*PGSPERSEG)-1,d5
	subl	a5,a5
PMaddr1:
	movsl	d5,a5@(PMAPOFF)
	addw	#BYTESPERPG,a5
	dbra	d5,PMaddr1

| Now read back the data and check it.
	movl	#(NUMPMEGS*PGSPERSEG)-1,d5
	subl	a5,a5
PMaddr2:
	movsl	a5@(PMAPOFF),d0
	andl	d1,d0		| Only keep the implemented bits
	cmpl	d0,d5
	jne	PMerror		| failure
| FIXME to loop a lot closer on failure
PMaddrlz:
	addw	#BYTESPERPG,a5
	dbra	d5,PMaddr2

PMaddrok:

|
| Test that PROM can be read with I/O cycles as well as boot state.
| Also, add up all the bytes in the PROM; maybe someday we'll actually
| burn in the checksum and compare it.  (If we do, we should fold carries
| into the low order bit to fix simple sum problems.)
|
Mprom:
	lea	Mprom,a0
	moveq	#~L_PROM,d0		| Testing the PROM readout
	movsb	d0,LEDOFF

|
| First, map in the PROM, starting at location 0.  Note that the same
| page map entry is used for all pages of the PROM; the virtual address
| bits are used to pick what part of the PROM we get to (hack, hack).
|
	movl	#PME_PROM,d0
	subl	a5,a5

Mprom1:
	movsl	d0,a5@(PMAPOFF)
	addw	#BYTESPERPG,a5
	cmpl	#PROMSIZE,a5		| Loop til we've mapped it all in
	jne	Mprom1
	
	moveq	#FC_SP,d0		| Set up to read in Super Pgm space
	movc	d0,sfc			| in movespace instructions
	moveq	#0,d7			| Clean out checksum
	moveq	#0,d0			| Clean out for adding to cksm

Mprom2:
	addl	d0,d7			| Add byte into checksum
	subql	#1,a5			| Back up by a byte.
	movl	a5,d6			| Set condition codes from new addr
	jmi	Mprom3			| If we just did byte 0, get out.
Mprom4:
	movsb	a5@,d0			| Read a byte from boot state
	cmpb	a5@,d0			| Read it from I/O space
	jeq	Mprom2			| If OK, loop.
	bclr	#PROMBIT,d2		| Indicate transient prom failure
	addql	#1,d3			| Bump the error count
	jra	Mprom4			| If not, loop even closer.

Mprom3:
| FIXME, test checksum (d7) against stored checksum...	
	moveq	#FC_MAP,d0		| Go back to accessing the maps
	movc	d0,sfc
| End of prom test.


|
| End of tests that loop forever if they fail.  Move the error counter
| to another register -- this will give us the total count of transient
| errors which occurred while in the above tests.  (They had to be transient
| otherwise we'd still be looping...)
|
	movl	d3,d4			| D4 is early transient err count

|
| Set up for the memory test.
|
| To do this, we map in as much memory as possible.
|
| The page map entries used to map memory give full permission
| for both user and supervisor.  If the memory fails, we drop the
| user permission bit(s).
|
| Register usage:
|	d7 = running pmeg #
|	a6 = running address
|
| FIXME, note that this maps virt->phys directly.  This won't
| catch wiring errors where the wrong bits are being fed, but this is 
| hopefully not a problem on PC boards.  Might be good to run backwards
| in this loop to map eg virt 0 to pmeg FF.
|
	moveq	#~L_M_MAP,d0	| Set up LEDs in case sizing fails
	movsb	d0,LEDOFF

	subl	a0,a0		| Clear error message pointer.
	
	moveq	#0,d7		| Running pmeg number
	subl	a6,a6		| Running segment address

Mmap1:
	movsb	d7,a6@(SMAPOFF)		| Set segment map to map this pmeg
	addl	#PGSPERSEG*BYTESPERPG,a6| Move to next segment
	addqb	#1,d7			| Ditto
	cmpb	#NUMPMEGS,d7		| Did we look at all of them?
	jne	Mmap1

|
|	We have mapped a set of segments in context 0
|	to all pmegs, up thru (not including) address (a6).
|	Now set up those page map entries to map physical memory,
|	and see how much of it passes a simple test for existence.
|	We remember the highest virtual (= physical page) address
|	that can read back its own page map entry, and treat that as
|	the size of memory.
|
|	d0 = running page map entry to map this page
|	a5 = running virtual address where we map pages in
|	a6 = highest virtual address for which we have pmegs (+1 byte)
|
| FIXME, 
|        We also have to deal with the frame buffer better.  Idea:
|	 map ourself thru to all pages which can remember their own page
|	 number, as an initial guess at memory size.
|FIXME, use symbolic constant for frame buffer physical address.
#ifndef VME
#ifdef S2COLOR
| FIXME, this is a temporary color hack.
	cmpl	#0x200000,a6		| Memory can only go thru meg 1
	jle	Mmap1x
	movl	#0x200000,a6		| Meg 2 is reserved for colorbuf.
#else  S2COLOR
	cmpl	#0x700000,a6		| Memory can only go thru meg 6
	jle	Mmap1x
	movl	#0x700000,a6		| Meg 7 is reserved for framebuf.
#endif S2COLOR
#endif VME
Mmap1x:
	movl	#PME_MEM_0,d0		| First page map entry
	subl	a5,a5			| First address to map

| Special test that page zero works, at least a little...
Mmapzerot:
	movsl	d0,a5@(PMAPOFF)		| Write page map entry
	movl	d0,a5@			| Write map entry into page
	cmpl	a5@,d0			| Does it read back OK?
| FIXME, should bump error count here
	jne	Mmapzerot		| (Nope, loop forever.)

Mmap2:
	movsl	d0,a5@(PMAPOFF)		| Write page map entry
	movl	d0,a5@			| Write map entry into page
	cmpl	a5@,d0			| Does it read back OK?
	jne	Mmap3			| (Nope, just keep going.)
	movl	a5,a7			| (Remember highest working addr)
Mmap3:
	addqw	#1,d0			| Bump page number in map entry
	addw	#BYTESPERPG,a5		| Bump page address
	cmpl	a5,a6			| Did we hit last address yet?
	jne	Mmap2			| Nope, loop.

	movl	a7,a6			| Set size of main memory
	addw	#BYTESPERPG,a6		| The last working page counts too.

|
|	Begin constant data test on main memory.
|
|	d2 = damage control bits
|	d5 = running count of number of longwords left to do
|	d7 = pattern word to store and check, either AAAAAAAA or 55555555
|	a5 = running address to store or compare at
|
	movl	#0xAAAAAAAA,d7	| Start with pattern of A's, later do 5's.
Mcontop:
	moveq	#~L_M_CONST,d0
	movsb	d0,LEDOFF

Mconst:
	subl	a5,a5		| Start at address 0
	movl	a6,d5		| Ending address
	lsrl	#2,d5		| Make it a longword count
	jra	Mconst1db	| Go for it
Mconst1:
	movl	d7,a5@+		| Fill memory with the pattern.
Mconst1db:
	dbra	d5,Mconst1	| Loop if it is.
	clrw	d5		| Cliche for long dbra emulation
	subql	#1,d5
	jcc	Mconst1

|
| Wait awhile to let memory get corrupted
| FIXME, we should force a bunch of timeouts here too.
|

	movl	#4000,d5	| Random number
L105:
	subql	#1,d5
	jgt	L105

| Now read back the data and see if it's right.

	subl	a5,a5		| Start back at zero again.
	movl	a6,d5		| Ending address
	lsrl	#2,d5		| Make it a longword count
	moveq	#0,d0		| Set condition code "equal" for dbne
	jra	Mconstdb	| Go for it
Mconst3x:
	cmpl	a5@+,d7		| See if memory is still good.
Mconstdb:
	dbne	d5,Mconst3x	| Loop if it is.
	jne	Mconerr		| Oops out if memory failed
	clrw	d5		| Cliche for long dbra emulation
	subql	#1,d5
	jcc	Mconst3x

| We're done this pass of the constant test.  See if all done.

	notl	d7		| Flip all the bits
	cmpb	#0xAA,d7	| Which constant test did we do?
	jne	Mconst		| Go back, we only did one so far.
	jra	Maddr		| Go do memory address line test

|
|	Error in constant data test.
|
|	Unfortunately, for speed, we don't save the value we read out
|	of memory.  This means we have to read it again and hope it
|	fails again.  If it doesn't fail, we mark it a transient error
|	anyway.
|
|	Interesting registers at this point:
|	d0 = scratch, can be clobberred
|	d1 = scratch, can be clobberred
|	d5 = iteration number (not yet decremented for this longword)
|	d7 = data written to memory (long)
|	a5 = 4 + virtual address in memory where data was read
|	ccr= flags from comparison with the data read back
|

Mconerr:
	movw	sr,d1		| Save condition codes from error
	movl	a5@-,d0		| Read the word again
	cmpl	d0,d7		| Did it fail again?
	jne	Mconerr1	| (Yes, hard error)
	movl	a5@,d0		| Read it one more time
	cmpl	d0,d7		| See if bad
	jne	Mconerr1	| (yes, whew, it really failed)
	movw	d1,d0		| Put condition codes into "data read"
	moveq	#MTRANBIT,d1	| Mark a transient error
	addql	#1,d4		| Bump transient error count
	lea	Mtranmsg,a0	
	jra	Mconerr2

Mtranmsg:
	.ascii	"transient "	| Note this tail-ends into Mconmsg.
Mconmsg:
	.ascii	"memory [data]\0"
	.even

Mconerr1:
	moveq	#MCONBIT,d1	| Mark a hard error
	lea	Mconmsg,a0	| Error message
Mconerr2:
	addql	#1,d3		| Bump the error count
	bclr	#MEMBIT,d2	| Mark memory as bad
	bclr	d1,d2		| Mark transient or permanent error
	movl	a5,a1		| Physical (& virtual) address in error
	movl	d7,a2		| Data written
	movl	d0,a3		| Data read (or condition code for transients)
	movl	a5,d0		| Address page map of failing address
	andl	#LOWMASK,d0	| Masking out low order bits helps
	movl	d0,a7		
	movsl	a7@(PMAPOFF),d0	| Get the entry
	bclr	d1,d0		| Clear a bit, indicating failed page
	movsl	d0,a7@(PMAPOFF)	| Put it back
	addqw	#4,a5		| Bump to next longword
	moveq	#0,d0		| Set condition code "equal" for dbne
	jra	Mconstdb	| ...and go on testing.

|
|	Now do an address line independence test on main mammary.
|
|	d5 = running count of how many shortwords are left to go
|	a5 = running address where we are storing d5
|

Maddr:
	moveq	#~L_M_ADDR,d0	| We're now in the memory address test
	movsb	d0,LEDOFF

	subl	a5,a5		| Start at address 0
	movl	a6,d5		| Ending address
	lsrl	#1,d5		| Make it a word count
	jra	Maddr1db	| Go for it
Maddr1:
	movw	d5,a5@+		| Fill memory with the pattern.
Maddr1db:
	dbra	d5,Maddr1	| Loop until 64K words done
	clrw	d5		| Cliche for long dbra emulation
	subql	#1,d5
	jcc	Maddr1

| Wait awhile to let memory get corrupted
| FIXME, take timeouts here.
	movl	#4000,d5	| Random number
Maddr2:
	subql	#1,d5
	jgt	Maddr2

| Now read back the data and see if it's right.

	subl	a5,a5		| Start back at zero again.
	movl	a6,d5		| Ending address
	lsrl	#1,d5		| Make it a longword count
	moveq	#0,d0		| Set condition code "equal" for dbne
	jra	Maddrdb		| Go for it
Maddr3x:
	cmpw	a5@+,d5		| See if memory is still good.
Maddrdb:
	dbne	d5,Maddr3x	| Loop if it is.
	jne	Maddrerr	| Oops out if memory failed
	clrw	d5		| Cliche for long dbra emulation
	subql	#1,d5
	jcc	Maddr3x
	jra	Mnexttest	| Skip to next test.

|
|	Error in address line independence test.
|
|	Unfortunately, for speed, we don't save the value we read out
|	of memory.  This means we have to read it again and hope it
|	fails again.  If it doesn't fail, we mark it a transient error
|	anyway.
|
|	Interesting registers at this point:
|	d0 = scratch
|	d1 = scratch
|	d5 = iteration number & data writ (not yet decremented)
|	a5 = 2 + virtual address in memory where data was read
|	ccr= flags from comparison with the data read back
|
       
Mtraddrmsg:
	.ascii	"transient "	| Tail-ends into Maddrmsg
Maddrmsg:
	.ascii	"memory [addr]\0"
	.even

Maddrerr:
	movw	sr,d1		| Save condition codes from error
	movw	a5@-,d0		| Read the word again
	cmpw	d0,d5		| Did it fail again?
	jne	Maddrerr1	| (Yes, hard failure.)
	movl	a5@,d0		| Read it one more time
	cmpw	d0,d5		| See if bad
	jne	Maddrerr1	| (yes, whew, it really failed)
	movl	d1,d0		| Save condition code in "data read"
	moveq	#MTRANBIT,d1	| Mark a transient error
	addql	#1,d4		| Bump transient error count
	lea	Mtraddrmsg,a0	
	jra	Maddrerr2

Maddrerr1:
	moveq	#MADDRBIT,d1	| Indicate at least one memory page died
	lea	Maddrmsg,a0	| Error message
Maddrerr2:
	addql	#1,d3		| Bump the error count
	bclr	#MEMBIT,d2	| Mark memory as bad
	bclr	d1,d2		| Mark transient or permanent error
	movl	a5,a1		| Record physical address of error
	movl	d5,a2		| Data written
	movl	d0,a3		| Data read
	movl	a5,d0		| Address page map of failing address
	andl	#LOWMASK,d0	| Masking out low order bits helps
	movl	d0,a7		
	movsl	a7@(PMAPOFF),d0	| Get the entry
	bclr	d1,d0		| Clear a bit, indicating failed page
	movsl	d0,a7@(PMAPOFF)	| Put it back
	addqw	#2,a5		| Bump to next word
	moveq	#0,d0		| Set condition code "equal" for dbne
	jra	Maddrdb		| ...and go on testing.

|
|	End of address line independence test.
|
| FIXME, we should have had parity enabled throughout.
| FIXME, this requires that the bus error vector be set up.
| FIXME, test should be more stringent about seeing that the right pmap 
| entries are being used for address translation.  (We know already that
| they can be read and written.  The question is, do they get fed to the
| memory chips properly?)  For example, don't map virtual direct to
| physical; scramble or invert the bits.  Problem is, how can we tell
| what physical addrs the chips are getting?  All we can see is how
| they respond; if A5 and A6 were swapped, we'd never know the difference.

#ifdef FIXME
|
| FIXME: Uart tests not done.
|
| The following was picked up and modified a little bit from Viet Ngo's
| "midiag" for the Model 50 bringup tests.  It needs more work before
| it can actually be used:
|   It should use the uartinit from the mainline prom code
|   (Probably it should move to there & be writ in "c")
|   It should clear a D2 bit for failure.
|   It should work!  (movb  #0xff, UARTACNRL  !!!)
|
Uartloop:
	lea	Uartloop,a0	| We're in the UART wrap back test.
	movl	#~L_UART,d0
	movsw	d0,LEDOFF

|****************************************************************************|
|* INITIALIZE UART FOR WRAP BACK TEST                                       *|
|****************************************************************************|

	lea	uartinit,a0			| set up pram xfer for A
uartst1:
	movsb	a0@,d0				| Pick up next byte of init
	cmpb	#0xff, a0@			| are we done yet??
	jne	uartst2
	jra	uartst4		                |
uartst2:
	movb	#0xff, UARTACNTL		| stuff
	jra	uartst2
	movl	#0x100, d0			| and wait
uartst3:
	dbra	d0, uartst3
	jra	uartst1   			| then do next byte
uartst4:
	movl	#uartinit+2, a0			| set up pram xfer - resetworld
uartst5:
	cmpb	#0xff, a0@			| are we done yet??
	jne	uartst6
	jra	uartst8				| yep, do special
uartst6:
	movb	a0@+, UARTBCNTL			| stuff
	movl	#0x100, d0			| and wait
uartst7:
	dbra	d0, uartst7
	jra	uartst5				| then do next byte
uartst8:
	movb	#14, UARTBCNTL			| set up register 14
	movl	#0x100, d0			| and wait
uartst9:
	movb	#0x13, UARTBCNTL		| set up local loopback
	dbra	d0, uartst9
|****************************************************************************|
|* CHECK FOR TX READY FLAG                                                  *| 
|****************************************************************************|
1$:	movl	#0xff, d1		| start at DEL, goto NULL
	clrl	d0			| clear readback register
	moveq	#1, d6			| set first char flag
2$:	moveq	#-1, d7			| set up timeout counter
3$:	btst	#TXREADY, UARTBCNTL	| check for TX ready
	dbne	d7, 3$			| loop 64K times
	movl 	#0xFFFFFF70,d2		| Set d2 to 4F for time out err
	movsw	d2,LEDOFF		| copy d2's low byte into LEDs.
	jra	3$
	jne	4$			| if never came ready, tell them
|****************************************************************************|
|* OUTPUT ERROR MSG TO LEDS - TX TIME OUT ERROR -                           *| 
|****************************************************************************|
	movl 	#0xFFFFFF70,d2		| Set d2 to 4F for time out err
	movsw	d2,LEDOFF		| copy d2's low byte into LEDs.
	jra	2$			| And loop back for scopping purposes
|****************************************************************************|
|* CHECK FOR RX READY FLAG                                                  *| 
|****************************************************************************|
4$:	movb	d1, UARTBDATA		| stuff char out
	moveq	#-1, d7			| set up receive timeout
5$:	btst	#RXREADY, UARTBCNTL	| check for RX ready
	dbne	d7, 5$
	jne	6$			| if it came, go on
|****************************************************************************|
|* OUTPUT ERROR MSG TO LEDS - RX TIME OUT ERROR -                           *| 
|****************************************************************************|
	movl 	#0xFFFFFF71,d2		| Set d2 to 4E for RX time out err
	movsw	d2,LEDOFF		| copy d2's low byte into LEDs.
	jra	2$			| And loop back for scopping purposes
|****************************************************************************|
|* READ BACK DATA AND COMPARE IT                                            *|
|****************************************************************************|
6$:	movb	UARTBDATA, d0		| get char back
	tstl	d6			| is first char??
	jeq	7$
	clrl	d6			| clear flag
	moveq	#-1, d7			| and go wait for real char
	jra	5$
7$:	cmpb	d0,d1
	jeq	8$			| if they compare, do next char
|****************************************************************************|
|* OUTPUT ERROR MSG TO LEDS - DATA MISCOMPARE -                             *| 
|****************************************************************************|
	movl 	#0xFFFFFF72,d2		| Set d2 to 4D for data miscompare.
	movsw	d2,LEDOFF		| copy d2's low byte into LEDs.
8$:	dbra	d1, 2$			| do next char
	jra	midiag			| Loop tru test again
#endif FIXME

#ifdef FIXME
|
|	Test parity generation and checking circuitry
|
|	Turn parity generation on, and write even and odd parity values
|	into memory.  Then turn it off, and write odd and even (making
|	FIXME, finish this.
|	FIXME, this should come before the "real" memory tests, then
|	they should run with parity errors enabled.
|
Mparity:
	lea	Mparity,a0	| We are in the memory parity test
	moveq	#~L_PARITY,d0
	movsb	d0,LEDOFF

	lea	#0x100,a7	| Start at location 100 (leave 0 for vectors)
	


#endif FIXME

#ifdef FIXME
|
|	Test timer chip.
|
|	First reset it, so it can't claim it's in a funny state.
|	Then R/W all the R/W registers.  Set up each timer and
|	make sure they are ticking.  Finally, test whether we can
|	generate an interrupt from each.  We could also test the
|	"gate" inputs.  FIXME.
|
CLKtop:
	lea	CLKtop,a0		| We are in the timer test
	moveq	#~L_TIMER,d0
	movsb	d0,LEDOFF

	movl	#PME_TIMER,d7		| Page map entry for timer
	movsl	d7,TIMER_BASE+PMAPOFF	| Set it.
	lea	TIMER_BASE+clk_cmd,a5
	lea	TIMER_BASE+clk_data,a6
	movw	#CLK_RESET,a5@		| Go thru the prescribed reset sequence
	movw	#CLK_RESET,a5@
	movw	#CLK_LOAD+CLK_ALL,a5@
	movw	#CLK_16BIT,a5@

	movw	#0xAAAA,d6

CLKtwo:
	movw	#CLK_ACC_MODE+1,a5@	| Start at mode reg of timer 1.
	moveq	#CLK_LAST-1,d7		| Loop thru all.
CLKone:
	movw	#CLKM_DEFAULT,a6@	| Write mode as default
	movw	d6,a6@			| Write test patt to load reg
	movw	d6,a6@			| ...and to hold reg
	dbra	d7,CLKone		| Do for all five timers.

	movw	#CLK_ACC_MODE+1,a5@	| Start over at mode reg of timer 1.
	moveq	#CLK_LAST-1,d7		| Loop thru all.
CLKthree:
	cmpw	#CLKM_DEFAULT,a6@	| Check mode is default
	jne	error
	cmpw	a6@,d6			| Check test patt in load reg
	jne	error
	cmpw	a6@,d6			| ...and in hold reg
	jne	error
	dbra	d7,CLKthree		| Do for all five timers.

	notw	d6			| Negate the pattern.
	jpl	CLKtwo			| Repeat with opposite bits.

| Now test if they tick.

	movw	#CLK_LOAD_ARM+CLK_ALL,a5@	| Start all counters ticking
	moveq	#30,d7
CLK4:	dbra	d7,CLK4			| Waste time
	movw	#CLK_SAVE_DISARM+CLK_ALL,a5@	| Stop and save 'em all.
	
	movw	#CLK_ACC_MODE+1,a5@	| Start over at mode reg of timer 1.
	moveq	#CLK_LAST-1,d7		| Loop thru all.
CLK5:
	cmpw	#CLKM_DEFAULT,a6@	| Check mode is default
	jne	error
	cmpw	a6@,d6			| Check test patt in load reg
	jne	error			| (Should be the same)
	cmpw	a6@,d6			| ...and in hold reg
	jeq	error			| (Should be different after ticking)
	dbra	d7,CLK5			| Do for all five timers.

| Now test that the NMI timer generates an interrupt when it hits TC.	

	movl	#PME_MEM_0,d7		| First set up memory map
	movsl	d7,0+PMAPOFF		| for NMI vector and stack
	lea	BYTESPERPG,sp		| Set up stack pointer there
	movl	#CLKnmi,EVEC_LEVEL7	| Set up nmi pointer
	movsw	ENABLEOFF,d6		| Enable interrupts
#ifdef FIXME
| FIXME, we have to map ourselves in, and turn off boot state.
	movw	d6,d7
	orw	#ena_ints,d7
	movsw	d7,ENABLEOFF
#endif FIXME
	movw	#CLK_CLEAR+TIMER_NMI,a5@	| Clear output flop
	movw	a5@,d7			| Read status register
	andw	#CLKS_BIT_TIMER_NMI,d7	| Is output bit set?
	jne	error			| Yes, zap out.
	movw	#CLK_ACC_MODE+TIMER_NMI,a5@	| Get to NMI timer
	movw	#CLKM_DIV_BY_1+CLKM_TOGGLE,a6@	| Set mode to tick once
	movw	#10,a6@				| Count down 10 ticks then NMI
	movw	#CLK_LOAD_ARM+CLK_BIT_TIMER_NMI,a5@ | Start it going
	moveq	#50,d7
CLK6:	dbra	d7,CLK6				| Loop nonsense awhile
#ifdef FIXME
	jra	error				| No NMI from timer.
#endif FIXME

| We got an NMI, all is well.

CLKnmi:
	movsw	d6,ENABLEOFF		| Restore enable reg
	movw	a5@,d7			| Read status register
	andw	#CLKS_BIT_TIMER_NMI,d7	| Is output bit set?
	jeq	error			| No, zap out.
	movw	#CLK_CLEAR+TIMER_NMI,a5@	| Clear output of NMI timer

| End of timer test.
#endif FIXME

#ifdef OLD

_BadUart:
L158:
	movb	#24,6291462
	movb	#24,6291462
	movb	#2,6291462
	movb	#170,6291462
	movb	#2,6291462
	cmpb	#170,6291462
	jeq	L159
	jra		L160
L159:
	movb	#2,6291462
	movb	#85,6291462
	movb	#2,6291462
	cmpb	#85,6291462
	jne	L161
	jra		L162
_uart_err:
L161:
L160:
movl a5,a0
	.data1
L163:
	.ascii	"UART\0"
	.text
	movl	#L163,a5
exg a5,a0
	movl	diagret,sp
	jmp	sp@
L162:
	movw	#65535,8388610
	movw	#65375,8388610
	movw	#65519,8388610
	movw	#65303,8388610
	movw	#12800,8388608
	movw	#65507,8388610
	movw	#65505,8388610
	clrl	2097152
L62:
	clrw	14680064
	clrl	d4
	movl	#12582912,a5
L174:
	cmpl	#14680063,a5
	jhi	L173
	movl	d4,d0
	orl	#3840,d0
	movw	d0,a5@
	addql	#1,d4
L172:
	addl	#32768,a5
	jra		L174
L173:
|   /* End of page map calls */
_loc24_goto:
_loc43_goto:
L128:
	movl	#1,2097152
_qpar_alt_test:
L185:
	movw	#-23902,d7
	movw	#21845,d6
	movl	#0,a5
L188:
	cmpl	a2,a5
	jcc	L187
	movw	d7,a5@+
	movw	d6,a5@+
	movw	d7,a5@+
	movw	d6,a5@+
	movw	d7,a5@+
	movw	d6,a5@+
	movw	d7,a5@+
	movw	d6,a5@+
	movw	d7,a5@+
	movw	d6,a5@+
	movw	d7,a5@+
	movw	d6,a5@+
	movw	d7,a5@+
	movw	d6,a5@+
	movw	d7,a5@+
	movw	d6,a5@+
L186:
	jra		L188
L187:
	movl	#2200,d4
L191:
	subql	#1,d4
L190:
	tstl	d4
	jgt	L191
L189:
	movl	#0,a3
L194:
	movw	a3@,a3@+
L193:
	cmpl	#256,a3
	jls	L194
L192:
	movl	#_par_err,8
movw #1000,sp
	movl	#0,a5
L198:
	cmpl	a2,a5
	jcc	L197
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
L196:
	jra		L198
L197:
	movl	#2,a5
L201:
	cmpl	a2,a5
	jcc	L200
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
	movl	a5@+,d4
L199:
	jra		L201
L200:
	jra		L202
_par_err:
L203:
	movl	#_par2err,8
movl sp@(2),a1
_par2err:
L205:
movl a5,a0
	.data1
L206:
	.ascii	"parity\0"
	.text
	movl	#L206,a5
exg a5,a0
	movl	diagret,sp
	jmp	sp@
_par_return:
L202:
	movl	#0x100000,a6
	movl	#11534336,a2
	movl	#512,d4
L209:
	cmpl	#1024,d4
	jge	L208
	movl	d4,d0
	orl	#8192,d0
	movw	d0,a2@
	addql	#1,d4
	addl	#2048,a2
L207:
	jra		L209
L208:
	movl	#_HiTIMo,8
	movl	#_HiInt,124
	movl	#1048576,a2
	movw	#65518,8388610
	movw	#65283,8388610
	movw	#1570,8388608
	movw	#5000,8388608
	movw	#65281,8388610
	movw	#1537,8388608
	movw	#7450,8388608
	movw	#65381,8388610
	movw	#65510,8388610
L212:
	cmpl	#2031616,a2
	jcc	L213
	movw	#-24233,a2@
	cmpw	#-24233,a2@
	jeq	L214
	jra		L213
L214:
	addl	#16384,a2
	jra		L212
L213:
	jra		L215
_HiInt:
	rte
_HiTIMo:
	addl	#12,sp
L215:
	movw	#65518,8388610
	movw	#65281,8388610
	movw	#2816,8388608
	movw	#65283,8388610
	movw	#2816,8388608
	movw	#65507,8388610
	movw	#65505,8388610
	cmpl	#1048576,a2
	jne	L216
	jra		L217
L216:
	movl	#_TIMout,8
	movl	#_Int7,124
	movw	#65518,8388610
	movw	#65283,8388610
	movw	#1570,8388608
	movw	#5000,8388608
	movw	#65281,8388610
	movw	#1537,8388608
	movw	#7450,8388608
	movw	#65381,8388610
	movw	#65510,8388610
	jra		L220
L221:
_Int7:
	movw	#65507,8388610
	movw	#65518,8388610
	movw	#65349,8388610
	movw	#65510,8388610
	rte
_TIMout:
	addl	#12,sp
	movw	#65518,8388610
	movw	#65281,8388610
	movw	#2816,8388608
	movw	#65283,8388610
	movw	#2816,8388608
	movw	#65507,8388610
	movw	#65505,8388610
	jra		L222
L220:
	movw	#-21846,d7
	moveq	#81,d2
	jra		L100
_loc81_goto:
L130:
	movw	#21845,d7
	moveq	#82,d2
	jra		L100
_loc82_goto:
L132:
	moveq	#83,d2
	jra		L112
_loc83_goto:
L134:
	movw	#65518,8388610
	movw	#65281,8388610
	movw	#2816,8388608
	movw	#65283,8388610
	movw	#2816,8388608
	movw	#65507,8388610
	movw	#65505,8388610
L217:
L223:
subl	a0,a0
	movl	diagret,sp
	jmp	sp@
_mbmem_err:
L222:
movl a5,a0
	.data1
L224:
	.ascii	"Multibus memory parity\0"
	.text
	movl	#L224,a5
exg a5,a0
	movl	diagret,sp
	jmp	sp@
	jra	LE12
LE12:
	moveml	a6@(-LF12),#LS12
	unlk	a6
	rts
   LF12 = 36
	LS12 = 0x2cfc
	LP12 =	8
	.data
#endif OLD
	
Mnexttest:
| End of current tests.  Do DES tests if DES support is desired.
#ifdef DES
	jmp	_desdiag	| Test the data encryption chip
#else  DES
	jmp	_diagret	| Return from diagnostics.
#endif DES
