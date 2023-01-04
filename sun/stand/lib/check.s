
	.data
	.asciz "@(#)check.s 1.1 9/25/86 Copyright Sun Micro";
	.even
	.text
	.globl	_bcheck, _wcheck, _lcheck

_bcheck:
	link	a6, #0
	movl	d7, sp@-		| save d7 (used for pattern)
	movl	a6@(8), a0		| buffer address
	movl	a6@(12), d0		| count
	movl	a6@(16), d7		| pattern
	subql	#1, d0			| bump for dbra brain damage
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1
1$:	cmpb	a0@+, d7		| compare pattern to buffer
	dbne	d0, 1$			| decrement until not equal
	dbne	d1, 1$			| decrement until not equal
	swap	d1			| move bits back
	orl	d1, d0			| put in d0
	addql	#1, d0			| bump d0
	movl	sp@+, d7		| restore d7
	unlk	a6
	rts

_wcheck:
	link	a6, #0
	movl	d7, sp@-		| save d7 (used for pattern)
	movl	a6@(8), a0		| buffer address
	movl	a6@(12), d0		| count
	movl	a6@(16), d7		| pattern
	lsrl	#1, d0			| make count of words
	subql	#1, d0			| bump for dbra brain damage
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1
1$:	cmpw	a0@+, d7		| compare pattern to buffer
	dbne	d0, 1$			| decrement until not equal
	dbne	d1, 1$			| decrement until not equal
	swap	d1			| move bits back
	orl	d1, d0			| put in d0
	addql	#1, d0			| bump d0
	movl	sp@+, d7		| restore d7
	unlk	a6
	rts

_lcheck:
	link	a6, #0
	movl	d7, sp@-		| save d7 (used for pattern)
	movl	a6@(8), a0		| buffer address
	movl	a6@(12), d0		| count
	movl	a6@(16), d7		| pattern
	lsrl	#2, d0			| make count of longs
	subql	#1, d0			| bump for dbra brain damage
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1
1$:	cmpl	a0@+, d7		| compare pattern to buffer
	dbne	d0, 1$			| decrement until not equal
	dbne	d1, 1$			| decrement until not equal
	swap	d1			| move bits back
	orl	d1, d0			| put in d0
	addql	#1, d0			| bump d0
	movl	sp@+, d7		| restore d7
	unlk	a6
	rts
