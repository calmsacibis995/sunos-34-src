	.data
	.asciz "@(#)fill.s 1.1 9/25/86 Copyright Sun Micro";
	.even
	.text
	.globl	_bfill, _wfill, _lfill

_bfill:
	link	a6, #0
	movl	d7, sp@-		| save d7 (used for pattern)
	movl	a6@(8), a0		| buffer address
	movl	a6@(12), d0		| count
	movl	a6@(16), d7		| pattern to fill with
	subql	#1, d0			| bump for dbra brain damage
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1
1$:	movb	d7, a0@+		| stuff into memory
	dbra	d0, 1$			| decrement until not equal
	dbra	d1, 1$			| decrement until not equal
	movl	sp@+, d7		| restore d7
	unlk	a6
	rts

_wfill:
	link	a6, #0
	movl	d7, sp@-		| save d7 (used for pattern)
	movl	a6@(8), a0		| buffer address
	movl	a6@(12), d0		| count
	movl	a6@(16), d7		| pattern to fill with
	lsrl	#1, d0			| make count of words
	subql	#1, d0			| bump for dbra brain damage
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1
1$:	movw	d7, a0@+		| stuff into memory
	dbra	d0, 1$			| decrement until not equal
	dbra	d1, 1$			| decrement until not equal
	movl	sp@+, d7		| restore d7
	unlk	a6
	rts

_lfill:
	link	a6, #0
	movl	d7, sp@-		| save d7 (used for pattern)
	movl	a6@(8), a0		| buffer address
	movl	a6@(12), d0		| count
	movl	a6@(16), d7		| pattern to fill with
	lsrl	#2, d0			| make count of longs
	subql	#1, d0			| bump for dbra brain damage
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1
1$:	movl	d7, a0@+		| stuff into memory
	dbra	d0, 1$			| decrement until not equal
	dbra	d1, 1$			| decrement until not equal
	movl	sp@+, d7		| restore d7
	unlk	a6
	rts
