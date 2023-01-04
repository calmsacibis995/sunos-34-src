
| 
| probe.s
| 
| Determine if an interface is present on the bus. 
|
| @(#)probe.s 1.7 83/11/30 Copyright (c) 1983 by Sun Microsystems, Inc.
|  

|
| peek(addr)
|
| Temporarily re-routes Bus Errors, and then tries to
| read a short from the specified address.  If a Bus Error occurs,
| we return -1; otherwise, we return the unsigned short that we read.
|

	.text
	.globl	_peek
_peek:
	movl	a7@(4),a0	| Get address to probe
	movl	sp,a1		| save current stack pointer
	movl	8:w,d1		| and bus error handler
	movl	a0,d0
	btst	#0,d0		| See if odd address
	bne	BEhand		| Yes, the probe fails.
	movl	#BEhand,8:w	| Set up our own handler
	moveq	#0,d0		| Clear top half
	movw	a0@,d0		| Read a shortword.
PAexit:
	movl	d1,8:w		| It worked; restore bus error handler
	rts
BEhand:
	movl	a1,sp		| Restore stack after bus error
	moveq	#-1,d0		| Set result of -1, indicating fault.
	bra	PAexit

|
| pokec(a,c)
|  
| This routine is the same, but uses a store instead of a read, due to
| stupid I/O devices which do not respond to reads.
|
| if (pokec (charpointer, bytevalue)) itfailed;
|

	.globl	_pokec
_pokec:
	movl	a7@(4),a0	| Get address to probe
	movl	sp,a1		| save current stack pointer
	movl	8:w,d1		| and bus error handler
	movl	#BEhand,8:w	| Set up our own handler
	movb	a7@(11),a0@	| Write a byte
| A fault in the movb will vector us to BEhand above.
	moveq	#0,d0		| It worked; return 0 as result.
	movl	d1,8:w		| restore bus error handler
	rts

|
| poke(a,c)
|  
| if (poke(pointer, value)) itfailed;
|

	.globl	_poke
_poke:
	movl	a7@(4),a0	| Get address to probe
	movl	sp,a1		| save current stack pointer
	movl	8:w,d1		| and bus error handler
	movl	#BEhand,8:w	| Set up our own handler
	movw	a7@(10),a0@	| Write a word
| A fault in the movb will vector us to BEhand above.
	moveq	#0,d0		| It worked; return 0 as result.
	movl	d1,8:w		| restore bus error handler
	rts
