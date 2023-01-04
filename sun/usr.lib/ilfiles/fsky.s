|	.asciz	"@(#)fsky.s 1.1 86/09/25 Copyr 1986 Sun Micro"
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

|	-fsky inline expansion file.

	.inline	Fc_minus,16	
	movel	__skybase,a0
	movew	#0x1048,a0@(-4)
	movel	sp@+,a0@
	movel	sp@(4),a0@
	movel	sp@+,a0@
	movel	sp@+,d0
	movel	sp@+,a0@
1:
	tstw	a0@(-2)
	jge	1b
	movel	a0@,d0
	movel	a0@,d1
	.end

	.inline	Fc_add,16	
	movel	__skybase,a0
	movew	#0x1047,a0@(-4)
	movel	sp@+,a0@
	movel	sp@(4),a0@
	movel	sp@+,a0@
	movel	sp@+,d0
	movel	sp@+,a0@
1:
	tstw	a0@(-2)
	jge	1b
	movel	a0@,d0
	movel	a0@,d1
	.end

		| 	convert complex  to int
	.inline	Fc_conv_i,8
        movl    __skybase,a1
        movw    #0x1027,a1@(-4)
        movl    sp@+,a1@
	movl	sp@+,d1		| ignore this
        movl    a1@,d0
	.end

		| 	convert int to complex 
	.inline	Fi_conv_c,4
	movl    __skybase,a1
        movw    #0x1024,a1@(-4)
        movl    sp@+,a1@
        movl    a1@,d0
	clrl    d1
	.end

		|	convert complex  to double
	.inline	Fc_conv_d,8
        movl    __skybase,a1
        movw    #0x1042,a1@(-4)
        movl    sp@+,a1@
	movl	sp@+,d1		| ignore this
        movl    a1@,d0
        movl    a1@,d1
 	.end

		|	convert double  to complex
	.inline	Fd_conv_c,8
        movl    __skybase,a1
        movw    #0x1043,a1@(-4)
        movl    sp@+,a1@
        movl    sp@+,a1@
        movl    a1@,d0
	clrl	d1
	.end

	.inline	_d_dim,8
        movl    __skybase,a1
        movw    #0x1008,a1@(-4)
	movel	sp@+,a0
        movl    a0@+,a1@
        movl    a0@,a1@
	movel	sp@+,a0
        movl    a0@+,a1@
        movl    a0@,a1@
1:      tstw    a1@(-2)
        jge    1b
        movl    a1@,d0
        movl    a1@,d1
	tstl	d0
	jpl	1f		| Branch if positive sign bit.
	cmpl	#0xfff00000,d0
	jlt	2f		| Branch if finite.
	jgt	1f		| Branch if nan.
	tstl	d1
	jne	1f		| Branch if nan.
2:
	clrl	d0
	clrl	d1
1:
	.end

	.inline	_d_mod,8
	movel	sp@+,a0
	movel	a0@+,d0
	movel	a0@,d1
	movel	sp@+,a0
	jsr	Fmodd
	.end

	.inline	_i_dnnt,4
	movel	sp@+,a0
	movel	a0@+,d0
	movel	a0@,d1
	jsr	Fnintd
	.end
	
	.inline	_i_nint,4
	movel	sp@+,a0
	movel	a0@,d0
	jsr	Fnints
	.end

	.inline	_r_dim,8
        movl    __skybase,a1
        movw    #0x1007,a1@(-4)
	movel	sp@+,a0
        movel   a0@,a1@
	movel	sp@+,a0
	movel	a0@,a1@
1:
        tstw    a1@(-2)
        jge    1b
        movl    a1@,d0
	jpl	1f		| Branch if positive sign.
	cmpl	#0xff800000,d0
	jgt	1f		| Branch if nan.
	clrl	d0
1:
	.end

	.inline	_r_mod,8
	movel	sp@+,a0
	movel	a0@,d0
	movel	sp@+,a0
	movel	a0@,d1
	jsr	Fmods
	.end

	.inline	Fc_mult,16
	movel	__skybase,a0
	movew	#0x1011,a0@(-4)
	movel	sp@+,a0@
	movel	sp@+,a0@
	movel	sp@+,a0@
	movel	sp@+,a0@
1:
	tstw	a0@(-2)
	jge	1b
	movel	a0@,d0
	movel	a0@,d1
	.end

	.inline	_ceil,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	Fceild
	.end

	.inline	_floor,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	Ffloord
	.end

	.inline	_rint,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	Farintd
	.end

	.inline	_sqrt,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	Fsqrtd
	.end

	.inline	_drem,16
	movl	sp@+,d0
	movl	sp@+,d1
	lea	sp@,a0
	jsr	Fremd
	movel	sp@+,a0
	movel	sp@+,a0
	.end

	.inline	_fmod,16
	movl	sp@+,d0
	movl	sp@+,d1
	lea	sp@,a0
	jsr	Fmodd
	movel	sp@+,a0
	movel	sp@+,a0
	.end

	.inline	_ldexp,12
	movl	sp@+,d0
	movl	sp@+,d1
	lea	sp@,a0
	jsr	Fscaleid
	movel	sp@+,a0
	.end

	.inline	_scalb,12
	movl	sp@+,d0
	movl	sp@+,d1
	lea	sp@,a0
	jsr	Fscaleid
	movel	sp@+,a0
	.end

	.inline	Satans,0
         movl    __skybase,a1
        movw    #0x102b,a1@(-4)
        movl    d0,a1@
1: 	tstw    a1@(-2)
        jge    1b
        movl    a1@,d0
	.end

	.inline	Ssins,0
         movl    __skybase,a1
        movw    #0x1029,a1@(-4)
        movl    d0,a1@
1: 	tstw    a1@(-2)
        jge    1b
        movl    a1@,d0
	.end

	.inline	Scoss,0
         movl    __skybase,a1
        movw    #0x1028,a1@(-4)
        movl    d0,a1@
1: 	tstw    a1@(-2)
        jge    1b
        movl    a1@,d0
	.end

	.inline	Stans,0
         movl    __skybase,a1
        movw    #0x102a,a1@(-4)
        movl    d0,a1@
1: 	tstw    a1@(-2)
        jge    1b
        movl    a1@,d0
	.end

	.inline	Sexps,0
         movl    __skybase,a1
        movw    #0x102c,a1@(-4)
        movl    d0,a1@
1: 	tstw    a1@(-2)
        jge    1b
        movl    a1@,d0
	.end

	.inline	Slogs,0
         movl    __skybase,a1
        movw    #0x102d,a1@(-4)
        movl    d0,a1@
1: 	tstw    a1@(-2)
        jge    1b
        movl    a1@,d0
	.end

	.inline	Ssqrts,0
         movl    __skybase,a1
        movw    #0x102f,a1@(-4)
        movl    d0,a1@
1: 	tstw    a1@(-2)
        jge    1b
        movl    a1@,d0
	.end

	.inline	Ssqrs,0
        movl    __skybase,a1
        movw    #0x100b,a1@(-4)
        movl    d0,a1@
        movl    d0,a1@
        movl    a1@,d0
	.end

	.inline Sdtos,0
	movl    __skybase,a1
        movw    #0x1043,a1@(-4)
        movl    d0,a1@
        movl    d1,a1@
        movl    a1@,d0
	.end

	.inline	Sstod,0
	movl    __skybase,a1
        movw    #0x1042,a1@(-4)
        movl    d0,a1@
        movl    a1@,d0
        movl    a1@,d1
	.end

	.inline	Ssqrd,0
	movl    __skybase,a1
        movw    #0x100c,a1@(-4)
        movl    d0,a1@
        movl    d1,a1@
        movl    d0,a1@
        movl    d1,a1@
1:      tstw    a1@(-2)
        jge    1b
        movl    a1@,d0
        movl    a1@,d1
	.end

	.inline	Ssqrtd,0
	jsr	Fsqrtd
	.end
