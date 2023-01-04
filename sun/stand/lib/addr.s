	.data
	.asciz	"@(#)addr.s 1.1 86/09/25 Copyr 1986 Sun Micro"
	.even
	.text

|	Copyright (c) 1986 by Sun Microsystems, Inc.
|
|	Your basic vanilla address tests
|
|	[b|w|l]adrfill(address, size, compl)
|		each memory location is set to addr|~addr
|		mod word size of course.
|	[o_][b|w|l]adrcheck(address, size, compl)
|		checks to see that each location has addr|~addr
|		in it.  the o_& versions give you back an observed
|		value for later analysis.  There are two flavors
|		cuz the observed value costs you a move from memory
|		to a register.
|
	.globl	_badrfill, _wadrfill, _ladrfill
	.globl	_badrcheck, _wadrcheck, _ladrcheck
	.globl	_o_badrcheck, _o_wadrcheck, _o_ladrcheck

	.globl	bdb_unpack, wdb_unpack, ldb_unpack
	.globl	bdb_pack, wdb_pack, ldb_pack
	.globl	_obs_value, _exp_value

adrstart:
	movl	sp@+, a1		| save return (off stack tho)
	link	a6, #0
	movl	a6@(12), d0		| get count
	tstl	d0			| if count zero, return now
	beqs	adrend2
	movl	d7, sp@-		| save d7
	movl	d6, sp@-		| and little sister
	movl	d5, sp@-		| only used in o_&, but easier here
	clrl	d5			| zero out obs pattern
	movl	a6@(8), a0		| get buffer address
	movl	a6@(16), d6		| get compl mask
	jmp	a1@			| use rts address (unpack now)

adrend:
	movl	d5, _obs_value		| only good after o_&check
	movl	d7, _exp_value		| only good after &check
	movl	sp@+, d5		| only used in o_&, but easier here
	movl	sp@+, d6
	movl	sp@+, d7
adrend2:
	unlk	a6
	rts

_badrfill:
	jsr	adrstart		| set up fill registers
	jsr	bdb_unpack		| get d0 setup for dbra
1$:	movw	a0, d7			| bump (damn, no loop here)
	eorb	d6, d7
	movb	d7, a0@+		| stuff into memory
	dbra	d0, 1$			| decrement until not equal
	dbra	d1, 1$			| decrement until not equal
	jra	adrend			| do cleanup

_wadrfill:
	jsr	adrstart		| set up fill registers
	jsr	wdb_unpack		| get d0 setup for dbra
1$:	movw	a0, d7			| bump (damn, no loop here)
	eorw	d6, d7
	movw	d7, a0@+		| stuff into memory
	dbra	d0, 1$			| decrement until not equal
	dbra	d1, 1$			| decrement until not equal
	jra	adrend			| do cleanup

_ladrfill:
	jsr	adrstart		| set up fill registers
	jsr	ldb_unpack		| get d0 setup for dbra
1$:	movl	a0, d7			| bump (damn, no loop here)
	eorl	d6, d7
	movl	d7, a0@+		| stuff into memory
	dbra	d0, 1$			| decrement until not equal
	dbra	d1, 1$			| decrement until not equal
	jra	adrend			| do cleanup
|
|	checks come in two flavors, regular (fast) and o_& (slow)
|	the o_ version returns obs_value at the end, which costs
|	us the move to register each loop.
|
_badrcheck:
	jsr	adrstart		| get prams (if needed)
	jsr	bdb_unpack
	andl	#0xff, d7		| clear unused bits of expected
1$:	movw	a0, d7			| bump (damn, no loop here)
	eorb	d6, d7
	cmpb	a0@+, d7		| check value
	dbne	d0, 1$
	dbne	d1, 1$			| loop till value mismatch or done
	jsr	bdb_pack		| construct return
	jra	adrend			| then finish up

_wadrcheck:
	jsr	adrstart		| get prams (if needed)
	jsr	wdb_unpack
	andl	#0xffff, d7		| clear unused bits of expected
1$:	movw	a0, d7			| bump (damn, no loop here)
	eorw	d6, d7
	cmpw	a0@+, d7		| check value
	dbne	d0, 1$
	dbne	d1, 1$			| loop till value mismatch or done
	jsr	wdb_pack		| construct return
	jra	adrend			| then finish up

_ladrcheck:
	jsr	adrstart		| get prams (if needed)
	jsr	ldb_unpack
1$:	movl	a0, d7			| bump (damn, no loop here)
	eorl	d6, d7
	cmpl	a0@+, d7		| check value
	dbne	d0, 1$
	dbne	d1, 1$			| loop till value mismatch or done
	jsr	ldb_pack		| construct return
	jra	adrend			| then finish up
|
| return observed pattern versions
|
_o_badrcheck:
	jsr	adrstart		| get prams (if needed)
	jsr	bdb_unpack
	andl	#0xff, d7		| clear unused bits of expected
1$:	movw	a0, d7			| bump (damn, no loop here)
	eorb	d6, d7
	movb	a0@+, d5		| get value
	cmpb	d5, d7			| check value
	dbne	d0, 1$
	dbne	d1, 1$			| loop till value mismatch or done
	jsr	bdb_pack		| construct return
	jra	adrend			| then finish up

_o_wadrcheck:
	jsr	adrstart		| get prams (if needed)
	jsr	wdb_unpack
	andl	#0xffff, d7		| clear unused bits of expected
1$:	movw	a0, d7			| bump (damn, no loop here)
	eorw	d6, d7
	movw	a0@+, d5		| get value
	cmpw	d5, d7			| check value
	dbne	d0, 1$
	dbne	d1, 1$			| loop till value mismatch or done
	jsr	wdb_pack		| construct return
	jra	adrend			| then finish up

_o_ladrcheck:
	jsr	adrstart		| get prams (if needed)
	jsr	ldb_unpack
1$:	movl	a0, d7			| bump (damn, no loop here)
	eorl	d6, d7
	movl	a0@+, d5		| get value
	cmpl	d5, d7			| check value
	dbne	d0, 1$
	dbne	d1, 1$			| loop till value mismatch or done
	jsr	ldb_pack		| construct return
	jra	adrend			| then finish up

