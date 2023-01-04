|	.asciz	"@(#)common.s 1.3 87/01/06 Copyr 1986 Sun Micro"
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

|	Generic inline expansion file.

| 	Single Precision libF77 routines.

	.inline	_r_atn2,8
	movel	sp@+,a0
	movel	a0@,d0
	movel	sp@+,a0
	movel	a0@,d1
	jsr	P/**/atan2s
	.end
	
	.inline	_r_dim,8
	movel	sp@+,a0
	movel	a0@,d0
	movel	sp@+,a0
	movel	a0@,d1
	jsr	P/**/subs
	tstl	d0
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
	jsr	P/**/mods
	.end

	.inline	_r_sign,8
	movel	sp@+,a0
	movel	a0@,d0
	movel	sp@+,a0
	cmpl	#0x80000000,a0@
	jls	1f		| Branch if y >= +0.
3:
	bset	#31,d0
	jra	2f
1:
	bclr	#31,d0
2:
	.end

	.inline	_i_nint,4
	movel	sp@+,a0
	movel	a0@,d0
	jsr	P/**/nints
	.end

	.inline	_pow_ri,8
	movel	sp@+,a0
	movel	a0@,d0
	movel	sp@+,a0
	movel	a0@,d1
	jsr	P/**/powis
	.end

	.inline	_pow_rr,8
	movel	sp@+,a0
	movel	a0@,d0
	movel	sp@+,a0
	movel	a0@,d1
	jsr	P/**/pows
	.end

|	Double Precision libF77 routines.

	.inline	_d_atn2,8
	movel	sp@+,a0
	movel	a0@+,d0
	movel	a0@,d1
	movel	sp@+,a0
	jsr	P/**/atan2d
	.end

	.inline	_d_dim,8
	movel	sp@+,a0
	movel	a0@+,d0
	movel	a0@,d1
	movel	sp@+,a0
	jsr	P/**/subd
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
	jsr	P/**/modd
	.end

	.inline	_d_sign,8
	movel	sp@+,a0
	movel	a0@+,d0
	movel	a0@,d1
	movel	sp@+,a0
	cmpl	#0x80000000,a0@
	jhi	3f		| Branch if y < -0.
	jcs	1f		| Branch if y >= +0.
	tstl	a0@(4)
	jeq	1f		| Branch if y is -0.
3:
	bset	#31,d0
	jra	2f
1:
	bclr	#31,d0
2:
	.end

	.inline	_i_dnnt,4
	movel	sp@+,a0
	movel	a0@+,d0
	movel	a0@,d1
	jsr	P/**/nintd
	.end
	
	.inline	_pow_di,8
	movel	sp@+,a0
	movel	a0@+,d0
	movel	a0@,d1
	movel	sp@+,a0
	jsr	P/**/powid
	.end

	.inline	_pow_dd,8
	movel	sp@+,a0
	movel	a0@+,d0
	movel	a0@,d1
	movel	sp@+,a0
	jsr	P/**/powd
	.end

|	Single Precision Complex libF77 routines

|
| void
| c_cmplx(resp,fp1,fp2)
|	register complex *resp;
|	register float *fp1, *fp2;
| {
| 	resp->real = *fp1;
| 	resp->imag = *fp2;
| }
| 
	.inline	_c_cmplx,12
	movl	sp@+,a1
	movl	sp@+,a0
	movl	sp@+,d0
	movl	a0@,a1@+
	movl	d0,a0
	movl	a0@,a1@
	.end

	.inline	_r_imag,4
	movl	sp@+,a0
	movel	a0@(4),d0
	.end

	.inline _r_cnjg,8
        movl    sp@+,a0
        movl    sp@+,a1
        movl    a1@+,a0@+
        movl    a1@,a0@
        bchg    #7,a0@
	.end

	.inline	_c_abs,4
	movl	sp@+,a0
	movel	a0@+,d0
	movel	a0@,d1
	jsr	P/**/length2s
	.end

	.inline	Fc_neg,8
	movl	sp@+,d0		| pick up real part of argument
	bchg    #31,d0		| and change sign
	movl	sp@+,d1		| pick up imag part of argument
	bchg    #31,d1
	.end

        .inline Fc_add,16
        movl    sp@(4),d0       | pick up imaginary part of 1st argument
        movl    sp@(12),d1      | pick up imaginary part of 2nd argument
        jsr     P/**/adds           | add them and
        movl    d0,sp@(12)      | save result
        movl    sp@+,d0         | pick up real part of 1st argument
        movl    sp@+,d1         | Skip arg.
        movl    sp@+,d1         | pick up real part of 2nd argument
        jsr     P/**/adds           | add them
        movl    sp@+,d1         | load imag result in return register
        .end

        .inline Fc_minus,16
        movl    sp@(4),d0       | pick up imaginary part of 1st argument
        movl    sp@(12),d1      | pick up imaginary part of 2nd argument
        jsr     P/**/subs           | add them and
        movl    d0,sp@(12)      | save result
        movl    sp@+,d0         | pick up real part of 1st argument
        movl    sp@+,d1         | Skip arg.
        movl    sp@+,d1         | pick up real part of 2nd argument
        jsr     P/**/subs           | add them
        movl    sp@+,d1         | load imag result in return register
        .end

	.inline	Fc_mult,16
	movel	d2,sp@-
	movel	d3,sp@-
        movl    sp@(8),d0       | pick up real part of 1st argument
        movl    sp@(20),d1      | pick up imaginary part of 2nd argument
        jsr     P/**/muls           |
        movl    d0,d2      | save intermediate product
        movl    sp@(12),d0       | pick up imaginary part of 1st argument
        movl    sp@(16),d1      | pick up real part of 2nd argument
        jsr     P/**/muls         |
        movl    d2,d1
        jsr     P/**/adds         |
        movl    d0,d3      | now have imaginary part of result
        movl    sp@(12),d0      | pick up imaginary part of 1st argument
        movl    sp@(20),d1      | pick up imaginary part of 2nd argument
        jsr     P/**/muls         |
        movl    d0,d2
        movl    sp@(8),d0       | pick up real part of 1st argument
        movl    sp@(16),d1      | pick up real part of 2nd argument
        jsr     P/**/muls         |
        movl    d2,d1
        jsr     P/**/subs         | d0 will have real part of result
        movl    d3,d1      | load imag result in return register
	movel	sp@+,d3
	movel	sp@+,d2
	lea	sp@(16),sp	| Bypass arguments.
	.end

        .inline Fc_div,16
        movel   sp@(12),sp@-	| Copy second argument.
        movel   sp@(12),sp@-
        pea     sp@		| Push address of copy of second argument.
        pea     sp@(12)		| Push address of first argument.
        pea     sp@(24)		| Push address of result.
        jsr     _c_div
        lea     sp@(20),sp
        movel   sp@+,d0		| Clear original first argument.
        movel   sp@+,d0
        movl    sp@+,d0		| Place result in registers.
        movl    sp@+,d1
        .end

	.inline Fc_ne,16
	movel	sp@,d0
	movel	sp@(8),d1
	jsr	P/**/cmps
	jne	1f
	movel	sp@(4),d0
	movel	sp@(12),d1
	jsr 	P/**/cmps
	jne	1f
	clrl	d0
	clrl	d1
	jra	2f	
1:
	moveq	#1,d0
2:
	lea	sp@(16),sp
	.end
	
	.inline Fc_eq,16
	movel	sp@,d0
	movel	sp@(8),d1
	jsr	P/**/cmps
	jne	1f
	movel	sp@(4),d0
	movel	sp@(12),d1
	jsr 	P/**/cmps
	jne	1f
	moveq	#1,d0
	jra	2f	
1:
	clrl	d0
	clrl	d1
2:
	lea	sp@(16),sp
	.end
		| 	convert float to complex 
	.inline	Ff_conv_c,4
	movl	sp@+,d0		| pick up argument
	clrl    d1
	.end

		| 	convert complex  to float
	.inline	Fc_conv_f,8
	movl	sp@+,d0		| pick up argument
	movl	sp@+,d1		| ignore this
	.end

		| 	convert int to complex 
	.inline	Fi_conv_c,4
	movl	sp@+,d0		| pick up argument
	jsr     P/**/flts
	clrl    d1
	.end

		| 	convert complex  to int
	.inline	Fc_conv_i,8
	movl	sp@+,d0		| pick up argument
	movl	sp@+,d1		| ignore this
	jsr     P/**/ints
	.end

		|	convert complex  to double
	.inline	Fc_conv_d,8
	movl    sp@+,d0   	| pick up argument
	movl	sp@+,d1		| ignore this
	jsr     P/**/stod
	.end

		|	convert double  to complex
	.inline	Fd_conv_c,8
	movl    sp@+,d0   
	movl    sp@+,d1   	| pick up argument
	jsr     P/**/dtos
	clrl	d1
	.end

|	Double Precision Complex libF77 routines

|
| void
| d_cmplx(resp,dp1,dp2)
|	register dcomplex *resp;
|	register double *dp1, *dp2;
| {
| 	resp->real = *dp1;
| 	resp->imag = *dp2;
| }
| 
	.inline	_d_cmplx,12
	movl	sp@+,a1
	movl	sp@+,a0
	movl	sp@+,d0
	movl	a0@+,a1@+
	movl	a0@,a1@+
	movl	d0,a0
	movl	a0@+,a1@+
	movl	a0@,a1@
	.end

	.inline	_d_imag,4
	movl	sp@+,a0
	movel	a0@(8),d0
	movel	a0@(12),d1
	.end

	.inline _d_cnjg,8
        movl    sp@+,a0
        movl    sp@+,a1
        movl    a1@+,a0@+
        movl    a1@+,a0@+
        movl    a1@+,a0@+
        movl    a1@,a0@
        bchg    #7,a0@(-4)
        .end

	.inline	_z_abs,4
	movl	sp@+,a0
	movel	a0@+,d0
	movel	a0@+,d1
	jsr	P/**/length2d
	.end

|
| Fz_neg(c, a)
| dcomplex *c, *a;
| {
| 	c->dreal = - a->dreal;
| 	c->dimag = - a->dimag;
| }
|
	.inline	Fz_neg,8
        movl    sp@+,a0         | a0 = c;
        movl    sp@+,a1         | a1 = a;
        movl    a1@+,a0@+       | c->dreal = - a->dreal;
        bchg    #7,a0@(-4)
        movl    a1@+,a0@+       | c->dreal = - a->dreal;
        movl    a1@+,a0@+       | c->imag = - a->imag;
        bchg    #7,a0@(-4)
        movl    a1@,a0@         | c->imag = - a->imag;
	.end

	.inline	__z_neg,8
        movl    sp@+,a0         | a0 = c;
        movl    sp@+,a1         | a1 = a;
        movl    a1@+,a0@+       | c->dreal = - a->dreal;
        bchg    #7,a0@(-4)
        movl    a1@+,a0@+       | c->dreal = - a->dreal;
        movl    a1@+,a0@+       | c->imag = - a->imag;
        bchg    #7,a0@(-4)
        movl    a1@,a0@         | c->imag = - a->imag;
	.end

|
| Fz_add(c, a, b)
|	dcomplex *a, *b, *c;
| {
|	c->dreal = a->dreal + b->dreal;
|	c->dimag = a->dimag + b->dimag;
| }
|
	.inline	Fz_add,12
	moveml	#0x38,sp@-
	moveml	sp@(12),#0x1c00
				| a2 = sp@(12) = c;
				| a3 = sp@(16) = a;
				| a4 = sp@(20) = b;
	movel	a3@+,d0	| d0/d1 gets re(a)
	movel	a3@+,d1	| d0/d1 gets re(a)
	movel	a4,a0
	jsr	P/**/addd
	movel	d0,a2@+
	movel	d1,a2@+
	movel	a3@+,d0	| d0/d1 gets im(a)
	movel	a3@,d1	| d0/d1 gets im(a)
	lea	a4@(8),a0
	jsr	P/**/addd
	movel	d0,a2@+
	movel	d1,a2@
	moveml	sp@+,#0x1c00
	movel	sp@+,d0		| Clear.
	movel	sp@+,d0		| Clear.
	movel	sp@+,d0		| Clear.
	.end

	.inline	__z_add,12
	moveml	#0x38,sp@-
	moveml	sp@(12),#0x1c00
				| a2 = sp@(12) = c;
				| a3 = sp@(16) = a;
				| a4 = sp@(20) = b;
	movel	a3@+,d0	| d0/d1 gets re(a)
	movel	a3@+,d1	| d0/d1 gets re(a)
	movel	a4,a0
	jsr	P/**/addd
	movel	d0,a2@+
	movel	d1,a2@+
	movel	a3@+,d0	| d0/d1 gets im(a)
	movel	a3@,d1	| d0/d1 gets im(a)
	lea	a4@(8),a0
	jsr	P/**/addd
	movel	d0,a2@+
	movel	d1,a2@
	moveml	sp@+,#0x1c00
	movel	sp@+,d0		| Clear.
	movel	sp@+,d0		| Clear.
	movel	sp@+,d0		| Clear.
	.end

|
| Fz_minus(c, a, b)
|	dcomplex *a, *b, *c;
| {
| 	c->dreal = a->dreal - b->dreal;
| 	c->dimag = a->dimag - b->dimag;
| }
|
	.inline	Fz_minus,12
	moveml	#0x38,sp@-
	moveml	sp@(12),#0x1c00
				| a2 = sp@(12) = c;
				| a3 = sp@(16) = a;
				| a4 = sp@(20) = b;
	movel	a3@+,d0	| d0/d1 gets re(a)
	movel	a3@+,d1	| d0/d1 gets re(a)
	movel	a4,a0
	jsr	P/**/subd
	movel	d0,a2@+
	movel	d1,a2@+
	movel	a3@+,d0	| d0/d1 gets im(a)
	movel	a3@,d1	| d0/d1 gets im(a)
	lea	a4@(8),a0
	jsr	P/**/subd
	movel	d0,a2@+
	movel	d1,a2@
	moveml	sp@+,#0x1c00
	movel	sp@+,d0		| Clear.
	movel	sp@+,d0		| Clear.
	movel	sp@+,d0		| Clear.
	.end

	.inline	__z_minus,12
	moveml	#0x38,sp@-
	moveml	sp@(12),#0x1c00
				| a2 = sp@(12) = c;
				| a3 = sp@(16) = a;
				| a4 = sp@(20) = b;
	movel	a3@+,d0	| d0/d1 gets re(a)
	movel	a3@+,d1	| d0/d1 gets re(a)
	movel	a4,a0
	jsr	P/**/subd
	movel	d0,a2@+
	movel	d1,a2@+
	movel	a3@+,d0	| d0/d1 gets im(a)
	movel	a3@,d1	| d0/d1 gets im(a)
	lea	a4@(8),a0
	jsr	P/**/subd
	movel	d0,a2@+
	movel	d1,a2@
	moveml	sp@+,#0x1c00
	movel	sp@+,d0		| Clear.
	movel	sp@+,d0		| Clear.
	movel	sp@+,d0		| Clear.
	.end

	.inline	Fz_mult,0
	jsr	__z_mult
	.end

	.inline	Fz_div,0
	jsr	__z_div
	.end

	.inline	_z_div,0
	jsr	__z_div
	.end

|
| Ff_conv_z(dp,f)	/* convert float to double complex */
|	dcomplex *dp;
|	union {float f; int i;}f;
| {
|	dp->real = (double)f.f;
|	dp->imag = 0.0;
| }
|
	.inline	Ff_conv_z,8
	movel	sp@(4),d0
	jsr	P/**/stod
	movl	sp@+,a0
	movl	d0,a0@+
	movl	d1,a0@+
	clrl    a0@+
	clrl    a0@
	movel	sp@+,d0			| Throw away second argument.
	.end

|
| float
| Fz_conv_f(dp)		/* convert double complex to float */
|	dcomplex *dp;
| {
|	return (float)dp->dreal;
| }
|
	.inline	Fz_conv_f,4
	movl    sp@+,a0   
	movl	a0@+,d0
	movl	a0@,d1
	jsr	P/**/dtos
	.end

|
| int
| Fz_conv_i(dp)		/* convert double complex to int */
|	dcomplex *dp;
| {
|	return (int)dp->dreal;
| }
|
	.inline	Fz_conv_i,4
	movl    sp@+,a0   
	movl	a0@+,d0
	movl	a0@,d1
	jsr     P/**/intd
	.end

|
| Fi_conv_z(dp,i)	/* convert int to double complex */
|	dcomplex *dp;
|	int i;
| {
|	dp->dreal = (double)i;
|	dp->dimag = 0.0;
| }
|
	.inline	Fi_conv_z,8
	movel	sp@(4),d0
	jsr     P/**/fltd
	movl	sp@+,a0
	movel	d0,a0@+
	movel	sp@+,d0		| Trash.
	movel	d1,a0@+
	clrl    a0@+
	clrl    a0@
	.end

|
| double
| Fz_conv_d(dp)		/* return real part of double complex */
|	dcomplex *dp;
| {
|	return dp->dreal;
| }
|
	.inline	Fz_conv_d,4
	movl    sp@+,a0
	movl    a0@+,d0
	movl    a0@,d1
	.end

|
| Fd_conv_z(dp,d)	/* convert double to double complex */
|	dcomplex *dp;
|	double d;
| {
|	dp->dreal = d;
|	dp->dimag = 0.0;
| }
|
	.inline	Fd_conv_z,12
	movl    sp@+,a0   
	movl    sp@+,a0@+
	movl    sp@+,a0@+
	clrl    a0@+
	clrl    a0@
	.end

|
| double
| Fz_conv_c(dp)         /* convert double complex to complex  */
|       dcomplex *dp;
| {
|       union { double d; complex c; } sleaze;
|       sleaze.cval.real = (float)dp->dreal;
|       sleaze.cval.imag = (float)dp->dimag;
|       return sleaze.dval;
| }
|
        .inline Fz_conv_c,4
	movl	sp@,a0		| pick up real part of argument
	movl	a0@(8),d0
	movl	a0@(12),d1
	jsr	P/**/dtos
	movel	sp@+,a0		| Restore a0.
	movl	d0,sp@-		| Save second part of result.
	movl	a0@+,d0
	movl	a0@,d1
	jsr	P/**/dtos
	movl	sp@+,d1		| Restore second part of result.
	.end

		| 	convert complex to double complex 
		|	arg1 is address of result; arg2 is complex as a double
	.inline	Fc_conv_z,12
	movl	sp@(4),d0	| pick up real part of argument
	jsr	P/**/stod
	movl	sp@,a0		| get address of result
	movl	d0,a0@+
	movl	d1,a0@
	movl	sp@(8),d0	| pick up imaginary part of argument
	jsr	P/**/stod
	movl	sp@+,a0		| get address of result
	movl	d0,a0@(8)
	movl	d1,a0@(12)
	lea	sp@(8),sp	| Bypass second argument.
	.end

|	Miscellaneous routines from libF77.

|
| i_conv_c(cp,len,ip)
|       char *cp;
|       int len;
|       int *ip;
|
        .inline _i_conv_c,12
        movl    sp@+,a0
        movl    sp@+,d0
        movl    sp@+,a1
        movb    a1@(3),a0@
        .end

|
| c_conv_i(cp)
|       char *cp;
|
        .inline _c_conv_i,4
        movl    sp@+,a0
        movb    a0@,d0
        extw    d0
        extl    d0
        .end

|
| do_l_in(cp1,cp2,cp3,cp4)
|
	.inline	_do_l_in,0
	jbsr	_do_lio
	.end

|
| do_l_out(cp1,cp2,cp3,cp4)
|
	.inline	_do_l_out,0
	jbsr	 _do_lio
	.end

|
| do_f_in(cp1,cp2,cp3)
|
	.inline	_do_f_in,0
	jbsr	 _do_fio
	.end

|
| do_f_out(cp1,cp2,cp3)
|
	.inline	_do_f_out,0
	jbsr	 _do_fio
	.end

|
| do_u_in(cp1,cp2,cp3)
|
	.inline	_do_u_in,0
	jbsr	 _do_uio
	.end

|
| do_u_out(cp1,cp2,cp3)
|
	.inline	_do_u_out,0
	jbsr	 _do_uio
	.end

	.inline	_abs,4
	movel	sp@+,d0
	jpl	1f
	negl	d0
1:
	.end

	.inline	_copysign,16
	movl	sp@+,d0
	movl	sp@+,d1
       	tstl     sp@+
        jmi     1f
        bclr     #31,d0
        jra     2f
1:
        bset    #31,d0
2:
	movel	sp@+,a0
	.end

	.inline	_fabs,8
	movl	sp@+,d0
	jpl	1f
	bchg	#31,d0
1:	movl	sp@+,d1
	.end

	.inline	_acos,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	P/**/acosd
	.end

	.inline	_asin,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	P/**/asind
	.end

	.inline	_atan,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	P/**/atand
	.end

	.inline	_ceil,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	P/**/ceild
	.end

	.inline	_cos,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	P/**/cosd
	.end

	.inline	_cosh,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	P/**/coshd
	.end

	.inline	_exp,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	P/**/expd
	.end

	.inline	_expm1,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	P/**/exp1d
	.end

	.inline	_floor,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	P/**/floord
	.end

	.inline	_log,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	P/**/logd
	.end

	.inline	_log1p,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	P/**/log1d
	.end

	.inline	_log10,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	P/**/log10d
	.end

	.inline	_rint,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	P/**/arintd
	.end

	.inline	_sin,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	P/**/sind
	.end

	.inline	_sinh,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	P/**/sinhd
	.end

	.inline	_sqrt,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	P/**/sqrtd
	.end

	.inline	_tan,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	P/**/tand
	.end

	.inline	_tanh,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	P/**/tanhd
	.end

	.inline	_atan2,16
	movl	sp@+,d0
	movl	sp@+,d1
	lea	sp@,a0
	jsr	P/**/atan2d
	movel	sp@+,a0
	movel	sp@+,a0
	.end

	.inline	_cabs,16
	movl	sp@+,d0
	movl	sp@+,d1
	lea	sp@,a0
	jsr	P/**/length2d
	movel	sp@+,a0
	movel	sp@+,a0
	.end

	.inline	_drem,16
	movl	sp@+,d0
	movl	sp@+,d1
	lea	sp@,a0
	jsr	P/**/remd
	movel	sp@+,a0
	movel	sp@+,a0
	.end

	.inline	_fmod,16
	movl	sp@+,d0
	movl	sp@+,d1
	lea	sp@,a0
	jsr	P/**/modd
	movel	sp@+,a0
	movel	sp@+,a0
	.end

	.inline	_hypot,16
	movl	sp@+,d0
	movl	sp@+,d1
	lea	sp@,a0
	jsr	P/**/length2d
	movel	sp@+,a0
	movel	sp@+,a0
	.end

	.inline	_pow,16
	movl	sp@+,d0
	movl	sp@+,d1
	lea	sp@,a0
	jsr	P/**/powd
	movel	sp@+,a0
	movel	sp@+,a0
	.end

	.inline	_ldexp,12
	movl	sp@+,d0
	movl	sp@+,d1
	lea	sp@,a0
	jsr	P/**/scaleid
	movel	sp@+,a0
	.end

	.inline	_scalb,12
	movl	sp@+,d0
	movl	sp@+,d1
	lea	sp@,a0
	jsr	P/**/scaleid
	movel	sp@+,a0
	.end
