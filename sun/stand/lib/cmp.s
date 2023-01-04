	.data
	.asciz "@(#)cmp.s 1.1 9/25/86 Copyright Sun Micro";
	.even
	.text
	.globl	_bcmp, _wcmp, _lcmp

_bcmp:
	link	a6, #0
	movl	a6@(8), a0		| first buffer address
	movl	a6@(12), a1		| second buffer address
	movl	a6@(16), d0		| count
	tstl	d0			| if zero, return OK
	beqs	2$
	subql	#1, d0			| bump for dbra brain damage
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1
1$:	cmpmb	a0@+, a1@+		| compare the two buffers
	dbne	d0, 1$			| decrement until not equal
	dbne	d1, 1$			| decrement until not equal
	swap	d1			| move bits back
	orl	d1, d0			| put in d0
	addql	#1, d0			| bump d0
2$:	unlk	a6			| exit point
	rts

_wcmp:
	link	a6, #0
	movl	a6@(8), a0		| first buffer address
	movl	a6@(12), a1		| second buffer address
	movl	a6@(16), d0		| count
	tstl	d0			| if zero, return OK
	beqs	2$
	subql	#1, d0			| bump for dbra brain damage
	lsrl	#1, d0			| make short count
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1
1$:	cmpmw	a0@+, a1@+		| compare the two buffers
	dbne	d0, 1$			| decrement until not equal
	dbne	d1, 1$			| decrement until not equal
	swap	d1			| move bits back
	orl	d1, d0			| put in d0
	addql	#1, d0			| bump d0
2$:	unlk	a6			| exit point
	rts

_lcmp:
	link	a6, #0
	movl	a6@(8), a0		| first buffer address
	movl	a6@(12), a1		| second buffer address
	movl	a6@(16), d0		| count
	tstl	d0			| if zero, return OK
	beqs	2$
	subql	#1, d0			| bump for dbra brain damage
	lsrl	#2, d0			| make long count
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1
1$:	cmpml	a0@+, a1@+		| compare the two buffers
	dbne	d0, 1$			| decrement until not equal
	dbne	d1, 1$			| decrement until not equal
	swap	d1			| move bits back
	orl	d1, d0			| put in d0
	addql	#1, d0			| bump d0
2$:	unlk	a6			| exit point
	rts
