
	.data
	.asciz "@(#)copy.s 1.1 9/25/86 Copyright Sun Micro"
	.even
	.text
	.globl	_bcopy, _wcopy, _lcopy

_bcopy:
	link	a6, #0
	movl	a6@(8), a0		| first buffer address
	movl	a6@(12), a1		| second buffer address
	movl	a6@(16), d0		| count
	tstl	d0			| check if zero
	beqs	2$			| if so, exit
	subql	#1, d0			| bump for dbra brain damage
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1
1$:	movb	a0@+, a1@+		| compare the two buffers
	dbra	d0, 1$			| decrement until not equal
	dbra	d1, 1$			| decrement until not equal
2$:	unlk	a6			| exit point
	rts

_wcopy:
	link	a6, #0
	movl	a6@(8), a0		| first buffer address
	movl	a6@(12), a1		| second buffer address
	movl	a6@(16), d0		| count
	tstl	d0			| check if zero
	beqs	2$			| if so, exit
	subql	#1, d0			| bump for dbra brain damage
	lsrl	#1, d0			| make short count
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1
1$:	movw	a0@+, a1@+		| compare the two buffers
	dbra	d0, 1$			| decrement until not equal
	dbra	d1, 1$			| decrement until not equal
2$:	unlk	a6			| exit point
	rts

_lcopy:
	link	a6, #0
	movl	a6@(8), a0		| first buffer address
	movl	a6@(12), a1		| second buffer address
	movl	a6@(16), d0		| count
	tstl	d0			| check if zero
	beqs	2$			| if so, exit
	subql	#1, d0			| bump for dbra brain damage
	lsrl	#2, d0			| make long count
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1
1$:	movl	a0@+, a1@+		| compare the two buffers
	dbra	d0, 1$			| decrement until not equal
	dbra	d1, 1$			| decrement until not equal
2$:	unlk	a6			| exit point
	rts
