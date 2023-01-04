|
|	Your basic vanilla address/uniqueness tests
|
|	[b|w|l]ufill(address, size, start, inc)
|		each memory location is set to (start + index*inc)
|		mod word size of course.
|	[o_][b|w|l]ucheck(address, size, start, inc)
|		checks to see that each location has (start + index*inc)
|		in it.  the o_& versions give you back an observed
|		value for later analysis.  There are two flavors
|		cuz the observed value costs you a move from memory
|		to a register.
|
|	.data
|	.asciz "@(#)uniq.s 1.1 86/09/25 Copyr 1986 Sun Micro"
|	.even
	.text
	.globl	_bufill, _wufill, _lufill
	.globl	_bucheck, _wucheck, _lucheck
	.globl	_o_bucheck, _o_wucheck, _o_lucheck

	.globl	bdb_unpack, wdb_unpack, ldb_unpack
	.globl	bdb_pack, wdb_pack, ldb_pack
	.globl	_obs_value, _exp_value

uniqstart:
	movl	sp@+, a1		| save return (off stack tho)
	link	a6, #0
	movl	a6@(12), d0		| get count
	tstl	d0			| if count zero, return now
	beqs	uniqend2
	movl	d7, sp@-		| save d7
	movl	d6, sp@-		| and little sister
	movl	d5, sp@-		| only used in o_&, but easier here
	clrl	d5			| zero out obs pattern
	movl	a6@(8), a0		| get buffer address
	movl	a6@(16), d7		| get start value
	movl	a6@(20), d6		| get increment in d6
	subl	d6, d7			| bump back so preadd used later works
	jmp	a1@			| use rts address (unpack now)

uniqend:
	movl	d5, _obs_value		| only good after o_&check
	movl	d7, _exp_value		| only good after &check
	movl	sp@+, d5		| only used in o_&, but easier here
	movl	sp@+, d6
	movl	sp@+, d7
uniqend2:
	unlk	a6
	rts

_bufill:
	jsr	uniqstart		| set up fill registers
	jsr	bdb_unpack		| get d0 setup for dbra
1$:	addb	d6, d7			| bump (damn, no loop here)
	movb	d7, a0@+		| stuff into memory
	dbra	d0, 1$			| decrement until not equal
	dbra	d1, 1$			| decrement until not equal
	jra	uniqend			| do cleanup

_wufill:
	jsr	uniqstart		| set up fill registers
	jsr	wdb_unpack		| get d0 setup for dbra
1$:	addw	d6, d7			| bump (damn, no loop here)
	movw	d7, a0@+		| stuff into memory
	dbra	d0, 1$			| decrement until not equal
	dbra	d1, 1$			| decrement until not equal
	jra	uniqend			| do cleanup

_lufill:
	jsr	uniqstart		| set up fill registers
	jsr	ldb_unpack		| get d0 setup for dbra
1$:	addl	d6, d7			| bump (damn, no loop here)
	movl	d7, a0@+		| stuff into memory
	dbra	d0, 1$			| decrement until not equal
	dbra	d1, 1$			| decrement until not equal
	jra	uniqend			| do cleanup
|
|	checks come in two flavors, regular (fast) and o_& (slow)
|	the o_ version returns obs_value at the end, which costs
|	us the move to register each loop.
|
_bucheck:
	jsr	uniqstart		| get prams (if needed)
	jsr	bdb_unpack
	andl	#0xff, d7		| clear unused bits of expected
1$:	addb	d6, d7			| bump (damn, no loop here)
	cmpb	a0@+, d7		| check value
	dbne	d0, 1$
	dbne	d1, 1$			| loop till value mismatch or done
	jsr	bdb_pack		| construct return
	jra	uniqend			| then finish up

_wucheck:
	jsr	uniqstart		| get prams (if needed)
	jsr	wdb_unpack
	andl	#0xffff, d7		| clear unused bits of expected
1$:	addw	d6, d7			| bump (damn, no loop here)
	cmpw	a0@+, d7		| check value
	dbne	d0, 1$
	dbne	d1, 1$			| loop till value mismatch or done
	jsr	wdb_pack		| construct return
	jra	uniqend			| then finish up

_lucheck:
	jsr	uniqstart		| get prams (if needed)
	jsr	ldb_unpack
1$:	addl	d6, d7			| bump (damn, no loop here)
	cmpl	a0@+, d7		| check value
	dbne	d0, 1$
	dbne	d1, 1$			| loop till value mismatch or done
	jsr	ldb_pack		| construct return
	jra	uniqend			| then finish up
|
| return observed pattern versions
|
_o_bucheck:
	jsr	uniqstart		| get prams (if needed)
	jsr	bdb_unpack
	andl	#0xff, d7		| clear unused bits of expected
1$:	addb	d6, d7			| bump (damn, no loop here)
	movb	a0@+, d5		| get value
	cmpb	d5, d7			| check value
	dbne	d0, 1$
	dbne	d1, 1$			| loop till value mismatch or done
	jsr	bdb_pack		| construct return
	jra	uniqend			| then finish up

_o_wucheck:
	jsr	uniqstart		| get prams (if needed)
	jsr	wdb_unpack
	andl	#0xffff, d7		| clear unused bits of expected
1$:	addw	d6, d7			| bump (damn, no loop here)
	movw	a0@+, d5		| get value
	cmpw	d5, d7			| check value
	dbne	d0, 1$
	dbne	d1, 1$			| loop till value mismatch or done
	jsr	wdb_pack		| construct return
	jra	uniqend			| then finish up

_o_lucheck:
	jsr	uniqstart		| get prams (if needed)
	jsr	ldb_unpack
1$:	addl	d6, d7			| bump (damn, no loop here)
	movl	a0@+, d5		| get value
	cmpl	d5, d7			| check value
	dbne	d0, 1$
	dbne	d1, 1$			| loop till value mismatch or done
	jsr	ldb_pack		| construct return
	jra	uniqend			| then finish up
