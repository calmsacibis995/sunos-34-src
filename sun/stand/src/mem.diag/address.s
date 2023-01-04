	.data
	.asciz "@(#)address.s 1.1 9/25/86 Copyright Sun Microsystems";
	.even
	.text
	.globl	_bmadrtst, _wmadrtst, _lmadrtst

_bmadrtst:
	link	a6, #0
	movl	d7, sp@-		| save d7 (used for pattern)
	movl	d6, sp@-		| save d6 (used for increment)
	movl	a6@(8), a0		| buffer address
	movl	a6@(12), d0		| count
	movl	a6@(16), d6		| eor mask (normal=0 or inverted=-1)

	subql	#1, d0			| bump for dbra brain damage
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1

1$:	movl	a0, d7			| load pattern
	eorl	d6, d7			| mask to normal or inverted
	movb	d7, a0@+		| stuff into memory
	dbra	d0, 1$
	dbra	d1, 1$
					| reload registers
	movl	a6@(8), a0		| buffer address
	movl	a6@(12), d0		| count

	subql	#1, d0			| bump for dbra brain damage
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1

2$:	movl	a0, d7			| load pattern
	eorl	d6, d7			| mask to normal or inverted
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


_wmadrtst:
	link	a6, #0
	movl	d7, sp@-		| save d7 (used for pattern)
	movl	d6, sp@-		| save d6 (used for increment)
	movl	a6@(8), a0		| buffer address
	movl	a6@(12), d0		| count
	movl	a6@(16), d6		| eor mask (normal=0, inverted=1)

	subql	#1, d0			| bump for dbra brain damage
	lsrl	#1, d0			| make word count
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1

1$:	movl	a0, d7			| load address pattern
	eorl	d6, d7			| mask to normal or inverted pattern
	movw	d7, a0@+		| stuff in memory
	dbra	d0, 1$
	dbra	d1, 1$
					| reload registers
	movl	a6@(8), a0		| buffer address
	movl	a6@(12), d0		| count

	subql	#1, d0			| bump for dbra brain damage
	lsrl	#1, d0			| make word count
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1

2$:	movl	a0, d7			| load address pattern
	eorl	d6, d7			| mask to normal or inverted pattern
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


_lmadrtst:
	link	a6, #0
	movl	d7, sp@-		| save d7 (used for pattern)
	movl	d6, sp@-		| save d6 (used for increment)
	movl	a6@(8), a0		| buffer address
	movl	a6@(12), d0		| count
	movl	a6@(16), d6		| eor mask (normal=0, inverted=1)

	subql	#1, d0			| bump for dbra brain damage
	lsrl	#2, d0			| make long count
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1

1$:	movl	a0, d7			| load address pattern
	eorl	d6, d7			| mask to normal or inverted pattern
	movl	d7, a0@+		| stuff in memory
	dbra	d0, 1$
	dbra	d1, 1$
					| reload registers
	movl	a6@(8), a0		| buffer address
	movl	a6@(12), d0		| count

	subql	#1, d0			| bump for dbra brain damage
	lsrl	#2, d0			| make long count
	movl	d0, d1			| move bits over
	swap	d1			| move hi bits to low bits
	andl	#0xffff, d0		| clear unused bits
	andl	#0xffff, d1

2$:	movl	a0, d7			| load address pattern
	eorl	d6, d7			| mask to normal or inverted pattern
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

