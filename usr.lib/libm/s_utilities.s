        .data 
        .asciz  "@(#)s_utilities.s 1.1 86/09/25 Copyr 1984 Sun Micro"
        .even
        .text

|	Copyright (c) 1983 by Sun Microsystems, Inc.

| utilities used for floating point  routines; taken from fp simulation files
| not externs, not profiled

	.globl	f_snan
f_snan:
	movl	#0x7fbfffff,d0	| Standard 68881 error nan.
	rts

	SAVED0D1 = 0x0003
	RESTD0D1 = 0x0003
	SAVEALL	 = 0x3F00	| registers d2-d4
	RESTALL	 = 0x00fc	

| decode type of arg in d1 and return  1-5 n d0 for zero gu plain inf and nan

	.globl	f_tst
f_tst:
	lsll	#1,d1		| toss sign
	roll	#8,d1		| exp in low byte
	tstb	d1
	bnes	1$			| branch if not gu or zero
	movl	#1,d0		| assume zero
	tstl	d1	
	beqs	3$			| it is
	movl	#2,d0		| else it's a gu
	bras	3$
1$:	cmpb	#255,d1		| inf or nan ?
	bnes	2$			| nope - plain
	movl	#4,d0		| assume inf
	andl	#0xFFFFFF00,d1  | Clear exponent field.
	beqs	3$			| it is
	movl	#5,d0		| else it'a a nan
	rts
2$:	movl	#3,d0
3$:	rts

| utility routines for converting between integers and
| the unpacked-floating-point format in registers.
|

	.globl	i_unpk
i_unpk:
	clrw	d2	| exponent
	moveq	#3,d3	| type: normal number, sign: +
	tstl	d1
	bpls	1$
	negl	d1	| (largest neg number is ok here)
	bset	#15,d3	| sign flag
1$:	clrl	d0	| upper
	rts

	.globl	i_pack
i_pack:
	cmpb	#4,d3
	bges	3$	| infinity or nan
	bsrs	gnsh	| shift by exponent
	orw	d2,d0
	tstl	d0
	bnes	3$	| too big or bits shifted out
	tstl	d1
	bmis	3$	| too negative
	tstw	d3	| sign
	bpls	2$
	negl	d1	| change sign
2$:	rts

3$:	movl	#0x80000000,d1
	rts

| 
|  shift the mantissa by the amount in the exponent.
|  if result will not fix in 64 bits, the exponent is left non-zero,
|  garbage in upper/lower.
|  input:
| 	d0-d3	unpacked record
|  output:
| 	d0-d3	unpacked record
| 	d2.l	zero if shift could be done.
| 

	.globl	gnsh
gnsh:
	moveq	#32,d4
	tstw	d2	| exponent
	bmis	3$	| right shift
	| left shift
1$:	cmpw	d4,d2
	blts	2$	| simple left
	tstl	d0
	bnes	7$	| too big
	subw	d4,d2
	exg	d1,d0	| shift 32
	bras	1$
2$:	roll	d2,d0
	roll	d2,d1
	moveq	#1,d4
	asll	d2,d4
	subql	#1,d4	| form mask
	movl	d0,d2
	andl	d4,d2	| overflow part
	bnes	7$
	andl	d1,d4	| between parts
	eorl	d4,d1	| clear bottom
	orl	d4,d0	| into top
	bras	6$	| and go

	| shift right
3$:	negw	d2
4$:	cmpw	d4,d2
	blts	5$
	movl	d0,d1	| shift 32
	clrl	d0
	subw	d4,d2
	bras	4$
5$:	moveq	#1,d4
	asll	d2,d4
	subql	#1,d4	| form mask
	notl	d4
	andl	d4,d1	| lower, 0
	andl	d0,d4	| upper, 0
	eorl	d4,d0	| 0, mid
	orl	d0,d1	| lower, mid
	rorl	d2,d4	| 0, upper
	rorl	d2,d1	| mid, lower
	movl	d4,d0	| upper
6$:	clrl	d2
7$:	rts

|  single-precision utility operations
|
|  copyright 1981, 1982 Richard E. James III
|  translated to SUN idiom 11 March 1983 rt
|  

	NSAVED	 = 3*4	| save registers d2-d4
	SAVEMASK = 0x3c00
	RESTMASK = 0x003c

|  unpack a single-precision number into the unpacked record format:
|  d0/d1	mantissa
|  d2.w	exponent
|  d3.w	sign in upper bit
|  d3.b	type ( zero, gu, plain, inf, nan )
|  
	.globl	f_unpk
f_unpk:
	movl	d1,d3
	swap	d3	| sign in bit 15
	lsll	#1,d1	| toss sign
	roll	#8,d1
	clrw	d2
	movb	d1,d2	| exponent
	bnes	1$	| not gu or zero
	movb	#1,d3
	tstl	d1
	beqs	3$
	movb	#2,d3	| gu -- gradual underflow unnormalized
	bras	3$
1$:	cmpb	#255,d2	| inf or nan
	bnes	2$	| nope, plain number
	movw	#0x6000,d2
	clrb	d1	| erase exponent
	movb	#4,d3	| infinity
	tstl	d1
	beqs	4$
2$:	movb	#1,d1	| hidden bit
	subqw	#1,d2
	movb	#3,d3
3$:	subw	#(126+23),d2
4$:	rorl	#1,d1
	lsrl	#8,d1
	clrl	d0
	rts

|
| reconstruct a single precision number from its pieces
| returns packed value in d0
|

	.globl	f_pack
f_pack:
	movw	d2,d4	| exponent
	cmpb	#4,d3	| is type inf or nan ?
	blts	1$	| no, then branch
	orl	d0,d1
	orl	#0x7f800000,d1	| exponent for inf/nan
	lsll	#1,d1
	bras	6$
1$:
|	clrb	d2	| for sticky
	tstl	d0
	beqs	3$
	| shift from upper into lower
2$:
|	orb	d1,d2	| sticky
	movb	d0,d1
	addqw	#8,d4	| adjust exponent
	rorl	#8,d1
	lsrl	#8,d0
	bnes	2$	| loop until top == 0
3$:	movl	d1,d0
	beqs	6$
	| find top bit
	bmis	5$
4$:	subqw	#1,d4	| adjust exponent
	lsll	#1,d0	| normalize
	bpls	4$
5$:	addw	#(126+23+9),d4
	jbsr	f_rcp
	rorl	#8,d0
	movl	d0,d1
6$:	lslw	#1,d3
	roxrl	#1,d1	| append sign
	rts
