	.data
	.asciz "@(#)unique.s 1.1 9/25/86 Copyright Sun Micro";
	.even
	.text
	.globl	_bunique, _wunique, _lunique

_bunique:
	link	a6, #0
	movl	d7, sp@-		| save d7 (used for pattern)
	movl	d6, sp@-		| save d6 (used for increment)
	movl	a6@(8), a0		| buffer address
	movl	a6@(12), d0		| count
	movl	a6@(16), d6		| increment
	clrl	d7			| start with zero
	subql	#1, d0			| bump for dbra brain damage
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1

1$:	addb	d6, d7			| stuff incremented patterns
	movb	d7, a0@+		| in memory
	dbra	d0, 1$
	dbra	d1, 1$
					| reload registers
	movl	a6@(8), a0		| buffer address
	movl	a6@(12), d0		| count
	clrl	d7			| start with zero
	subql	#1, d0			| bump for dbra brain damage
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1
2$:	addb	d6, d7			| bump pattern
	cmpb	a0@+, d7		| and compare
	dbne	d0, 2$			| decrement until not equal
	dbne	d1, 2$			| decrement until not equal
	swap	d1			| move bits back
	orl	d1, d0			| put in d0
	addql	#1, d0			| bump d0
	movl	sp@+, d6		| restore d6
	movl	sp@+, d7		| restore d7
	unlk	a6
	rts


_wunique:
	link	a6, #0
	movl	d7, sp@-		| save d7 (used for pattern)
	movl	d6, sp@-		| save d6 (used for increment)
	movl	a6@(8), a0		| buffer address
	movl	a6@(12), d0		| count
	movl	a6@(16), d6		| increment
	clrl	d7			| start with zero
	subql	#1, d0			| bump for dbra brain damage
	lsrl	#1, d0			| make word count
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1

1$:	addw	d6, d7			| stuff incremented patterns
	movw	d7, a0@+		| in memory
	dbra	d0, 1$
	dbra	d1, 1$
					| reload registers
	movl	a6@(8), a0		| buffer address
	movl	a6@(12), d0		| count
	clrl	d7			| start with zero
	subql	#1, d0			| bump for dbra brain damage
	lsrl	#1, d0			| make word count
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1
2$:	addw	d6, d7			| bump pattern
	cmpw	a0@+, d7		| and compare
	dbne	d0, 2$			| decrement until not equal
	dbne	d1, 2$			| decrement until not equal
	swap	d1			| move bits back
	orl	d1, d0			| put in d0
	addql	#1, d0			| bump d0
	movl	sp@+, d6		| restore d6
	movl	sp@+, d7		| restore d7
	unlk	a6
	rts


_lunique:
	link	a6, #0
	movl	d7, sp@-		| save d7 (used for pattern)
	movl	d6, sp@-		| save d6 (used for increment)
	movl	a6@(8), a0		| buffer address
	movl	a6@(12), d0		| count
	movl	a6@(16), d6		| increment
	clrl	d7			| start with zero
	subql	#1, d0			| bump for dbra brain damage
	lsrl	#2, d0			| make long count
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1

1$:	addl	d6, d7			| stuff incremented patterns
	movl	d7, a0@+		| in memory
	dbra	d0, 1$
	dbra	d1, 1$
					| reload registers
	movl	a6@(8), a0		| buffer address
	movl	a6@(12), d0		| count
	clrl	d7			| start with zero
	subql	#1, d0			| bump for dbra brain damage
	lsrl	#2, d0			| make long count
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1
2$:	addl	d6, d7			| bump pattern
	cmpl	a0@+, d7		| and compare
	dbne	d0, 2$			| decrement until not equal
	dbne	d1, 2$			| decrement until not equal
	swap	d1			| move bits back
	orl	d1, d0			| put in d0
	addql	#1, d0			| bump d0
	movl	sp@+, d6		| restore d6
	movl	sp@+, d7		| restore d7
	unlk	a6
	rts
