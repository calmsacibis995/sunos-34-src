|	.asciz	"@(#)ffpa.s 1.1 86/09/25 Copyr 1986 Sun Micro"
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

|	-ffpa Inline expansion file.

	.inline	Fc_minus,16	
	fpmoves	sp@+,fpa0
	fpmoves	sp@+,fpa1
	fpsubs	sp@+,fpa0
	fpsubs	sp@+,fpa1
	fpmoves	fpa0,d0
	fpmoves	fpa1,d1
	.end

	.inline	Fc_add,16	
	fpmoves	sp@+,fpa0
	fpmoves	sp@+,fpa1
	fpadds	sp@+,fpa0
	fpadds	sp@+,fpa1
	fpmoves	fpa0,d0
	fpmoves	fpa1,d1
	.end

		| 	convert complex  to int
	.inline	Fc_conv_i,8
	fpstol	sp@+,fpa0	| pick up argument
	movl	sp@+,d0		| ignore this
	fpmoves	fpa0,d0
	.end

		| 	convert int to complex 
	.inline	Fi_conv_c,4
	fpltos	sp@+,fpa0	| pick up argument
	clrl    d1
	fpmoves	fpa0,d0
	.end

		|	convert complex  to double
	.inline	Fc_conv_d,8
	fpstod sp@+,fpa0   	| pick up argument
	movl	sp@+,d1		| ignore this
	fpmoved fpa0,d0:d1
	.end

		|	convert double  to complex
	.inline	Fd_conv_c,8
	fpdtos	sp@+,fpa0
	fpmoves	fpa0,d0
	clrl	d1
	.end

		| 	convert complex to double complex 
		|	arg1 is address of result; arg2 is complex as a double
	.inline	Fc_conv_z,12
	movel	sp@+,a0		| a0 gets address of result.
	fpstod	sp@+,fpa0	| fpa0 gets re.
	fpstod	sp@+,fpa1	| fpa1 gets im.
	fpmoved	fpa0,a0@+
	fpmoved	fpa1,a0@+
	.end

	.inline	_c_abs,4
        movl    sp@+,a0
        fmoves  a0@+,fp0
        fmulx   fp0,fp0
        fmoves  a0@,fp1
        fmulx   fp1,fp1
        faddx   fp1,fp0
        fsqrtx  fp0,fp0
        fmoves  fp0,d0
	.end

	.inline	_d_dim,8
	movel	sp@+,a0
	fpmoved	a0@,fpa0
	movel	sp@+,a0
	fpsubd	a0@,fpa0
	fpmoved	fpa0,d0:d1
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
	fmoved	a0@,fp0
	movel	sp@+,a1
	fmodd	a1@,fp0
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline	_pow_dd,8
	movel	sp@+,a0
	movel	a0@+,d0
	movel	a0@,d1
	movel	sp@+,a0
	jsr	Mpowd
	.end

|	.asciz	"@(#)pow_rr.s 1.1 85/09/03 Copyr 1985 Sun Micro"

	.inline	_pow_rr,8
	movel	sp@+,a0
	movel	a0@,d0
	movel	sp@+,a0
	movel	a0@,d1
	jsr	Mpows
	.end

	.inline	_r_dim,8
	movel	sp@+,a0
	fpmoves	a0@,fpa0
	movel	sp@+,a0
	fpsubs	a0@,fpa0
	fpmoves	fpa0,d0
	tstl	d0
	jpl	1f		| Branch if positive sign.
	cmpl	#0xff800000,d0
	jgt	1f		| Branch if nan.
	clrl	d0
1:
	.end

	.inline	_r_mod,8
	movel	sp@+,a0
	fmoves	a0@,fp0
	movel	sp@+,a0
	fmods	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_z_abs,4
        movl    sp@+,a0
        fmoved  a0@+,fp0
        fmoved  a0@,fp1
        fmulx   fp0,fp0
        fmulx   fp1,fp1
        faddx   fp1,fp0
        fsqrtx  fp0,fp0
        fmoved  fp0,sp@-
        movel   sp@+,d0
        movel   sp@+,d1
	.end

	.inline	Fc_mult,16
	fpstod	sp@+,fpa0	| a
	fpstod	sp@+,fpa1	| b
	fpstod	sp@,fpa2	| c
	fpstod	sp@(4),fpa3	| d
	fpmuld	fpa0,fpa2	| fpa2 = ac
	fpmuld	fpa1,fpa3	| fpa3 = bd
	fprsubd	fpa2,fpa3	| fpa3 = ac-bd
	fpdtos	fpa3,fpa3
	fpstod	sp@+,fpa2	| c
	fpmoves	fpa3,d0
	fpstod	sp@+,fpa3	| d
	fpmuld	fpa0,fpa3	| fpa3 = ad
	fpmuld	fpa1,fpa2	| fpa2 = bc
	fpaddd	fpa3,fpa2	| fpa2 = ad+bc
	fpdtos	fpa2,fpa2
	fpmoves	fpa2,d1
	.end

	.inline	Fz_mult,12	
	movl	sp@+,d0		| d0 = c;
	movl	sp@+,a1		| a0 = a;
	movl	sp@+,a0		| a1 = b;
	fpmoved	a1@,fpa0	| fp0 = (a->dreal *  b->dreal);
	fpmuld	a0@,fpa0
	fpmoved	a1@(8),fpa1	| fp1 = (a->dimag *  b->dimag);
	fpmuld	a0@(8),fpa1
	fpsubd	fpa1,fpa0	| c->dreal = fp0 - fp1;
	exg	d0,a0
	fpmoved	fpa0,a0@
	exg	d0,a0
	fpmoved	a1@,fpa0	| fp0 = (a->dreal *  b->dimag);
	fpmuld	a0@(8),fpa0
	fpmoved	a1@(8),fpa1	| fp1 = (a->dimag *  b->dreal);
	fpmuld	a0@,fpa1
	fpaddd	fpa1,fpa0	| c->dimag = fp0 + fp1;
	movl	d0,a0
	fpmoved	fpa0,a0@(8)
	.end

	.inline	__z_mult,12
	movl	sp@+,d0		| d0 = c;
	movl	sp@+,a1		| a0 = a;
	movl	sp@+,a0		| a1 = b;
	fpmoved	a1@,fpa0		| fp0 = (a->dreal *  b->dreal);
	fpmuld	a0@,fpa0
	fpmoved	a1@(8),fpa1	| fp1 = (a->dimag *  b->dimag);
	fpmuld	a0@(8),fpa1
	fpsubd	fpa1,fpa0		| c->dreal = fp0 - fp1;
	exg	d0,a0
	fpmoved	fpa0,a0@
	exg	d0,a0
	fpmoved	a1@,fpa0		| fp0 = (a->dreal *  b->dimag);
	fpmuld	a0@(8),fpa0
	fpmoved	a1@(8),fpa1	| fp1 = (a->dimag *  b->dreal);
	fpmuld	a0@,fpa1
	fpaddd	fpa1,fpa0		| c->dimag = fp0 + fp1;
	movl	d0,a0
	fpmoved	fpa0,a0@(8)
	.end
	
	.inline	Fz_minus,12
	movl	sp@+,d0		| d0 = c;
	movl	sp@+,a0		| a0 = a;
	movl	sp@+,a1		| a1 = b;
	fpmoved	a0@+,fpa0	| fp0 = a->dreal - b->dreal;
	fpsubd	a1@+,fpa0
	fpmoved	a0@,fpa1		| fp1 = a->dimag - b->dimag;
	fpsubd	a1@,fpa1
	movl	d0,a0
	fpmoved	fpa0,a0@+	| c->dreal = fp0;
	fpmoved	fpa1,a0@		| c->dimag = fp1;
	.end

	.inline	__z_minus,12
	movl	sp@+,d0		| d0 = c;
	movl	sp@+,a0		| a0 = a;
	movl	sp@+,a1		| a1 = b;
	fpmoved	a0@+,fpa0	| fp0 = a->dreal - b->dreal;
	fpsubd	a1@+,fpa0
	fpmoved	a0@,fpa1		| fp1 = a->dimag - b->dimag;
	fpsubd	a1@,fpa1
	movl	d0,a0
	fpmoved	fpa0,a0@+	| c->dreal = fp0;
	fpmoved	fpa1,a0@		| c->dimag = fp1;
	.end

	.inline	Fz_add,12
	movl	sp@+,d0		| d0 = c;
	movl	sp@+,a0		| a0 = a;
	movl	sp@+,a1		| a1 = b;
	fpmoved	a0@+,fpa0	| fp0 = a->dreal + b->dreal;
	fpaddd	a1@+,fpa0
	fpmoved	a0@,fpa1		| fp1 = a->dimag + b->dimag;
	fpaddd	a1@,fpa1
	movl	d0,a0
	fpmoved	fpa0,a0@+	| c->dreal = fp0;
	fpmoved	fpa1,a0@		| c->dimag = fp1;
	.end

	.inline	__z_add,12
	movl	sp@+,d0		| d0 = c;
	movl	sp@+,a0		| a0 = a;
	movl	sp@+,a1		| a1 = b;
	fpmoved	a0@+,fpa0	| fp0 = a->dreal + b->dreal;
	fpaddd	a1@+,fpa0
	fpmoved	a0@,fpa1		| fp1 = a->dimag + b->dimag;
	fpaddd	a1@,fpa1
	movl	d0,a0
	fpmoved	fpa0,a0@+	| c->dreal = fp0;
	fpmoved	fpa1,a0@		| c->dimag = fp1;
	.end

	.inline	Ff_conv_z,8
	movl	sp@+,a0
	fpstod	sp@+,fpa0
	fpmoved	fpa0,a0@+
	clrl    a0@+
	clrl    a0@
	.end

	.inline	Fz_conv_f,4
	movl    sp@+,a0   
	fpdtos	a0@,fpa0
	fpmoves	fpa0,d0
	.end

	.inline	Fz_conv_i,4
	movl    sp@+,a0   
	fpdtol	a0@,fpa0
	fpmovel	fpa0,d0
	.end

	.inline	Fi_conv_z,8
	movl	sp@+,a0
	fpltod	sp@+,fpa0
	fpmoved	fpa0,a0@+
	clrl    a0@+
	clrl    a0@
	.end

	.inline	Fz_conv_c,4
	movl	sp@+,a0
	fpdtos	a0@+,fpa0
	fpdtos	a0@+,fpa1
	fpmoves	fpa0,d0
	fpmoves	fpa1,d1
	.end

	.inline	Waints,0
	fintrzs	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wsqrts,0
	fsqrts	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wpow2s,0
	ftwotoxs	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wpow10s,0
	ftentoxs	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wtans,0
	ftans	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wasins,0
	fasins	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wacoss,0
	facoss	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wsinhs,0
	fsinhs	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wcoshs,0
	fcoshs	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wtanhs,0
	ftanhs	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wlog10s,0
	flog10s	d0,fp0
	fmoves	fp0,d0
	.end

	.inline Waintd,0
	movel	d1,sp@-
	movel	d0,sp@-
	fintrzd	sp@,fp0
	fmoved	fp0,sp@
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline Wpow2d,0
	movel	d1,sp@-
	movel	d0,sp@-
	ftwotoxd	sp@,fp0
	fmoved	fp0,sp@
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline Wpow10d,0
	movel	d1,sp@-
	movel	d0,sp@-
	ftentoxd	sp@,fp0
	fmoved	fp0,sp@
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline Wlog10d,0
	movel	d1,sp@-
	movel	d0,sp@-
	flog10d	sp@,fp0
	fmoved	fp0,sp@
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline Wtand,0
	movel	d1,sp@-
	movel	d0,sp@-
	ftand	sp@,fp0
	fmoved	fp0,sp@
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline Wacosd,0
	movel	d1,sp@-
	movel	d0,sp@-
	facosd	sp@,fp0
	fmoved	fp0,sp@
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline Wasind,0
	movel	d1,sp@-
	movel	d0,sp@-
	fasind	sp@,fp0
	fmoved	fp0,sp@
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline Wsinhd,0
	movel	d1,sp@-
	movel	d0,sp@-
	fsinhd	sp@,fp0
	fmoved	fp0,sp@
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline Wcoshd,0
	movel	d1,sp@-
	movel	d0,sp@-
	fcoshd	sp@,fp0
	fmoved	fp0,sp@
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline Wtanhd,0
	movel	d1,sp@-
	movel	d0,sp@-
	ftanhd	sp@,fp0
	fmoved	fp0,sp@
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline	_atan,8
	fpmoved	sp@+,fpa0
	fpatand	fpa0,fpa0
	fpmoved	fpa0,d0:d1
	.end

	.inline	_sin,8
	fpmoved	sp@+,fpa0
	fpsind	fpa0,fpa0
	fpmoved	fpa0,d0:d1
	.end

	.inline	_cos,8
	fpmoved	sp@+,fpa0
	fpcosd	fpa0,fpa0
	fpmoved	fpa0,d0:d1
	.end

	.inline	_exp,8
	fpmoved	sp@+,fpa0
	fpetoxd	fpa0,fpa0
	fpmoved	fpa0,d0:d1
	.end

	.inline	_expm1,8
	fpmoved	sp@+,fpa0
	fpetoxm1d	fpa0,fpa0
	fpmoved	fpa0,d0:d1
	.end

	.inline	_log,8
	fpmoved	sp@+,fpa0
	fplognd	fpa0,fpa0
	fpmoved	fpa0,d0:d1
	.end

	.inline	_log1p,8
	fpmoved	sp@+,fpa0
	fplognp1d	fpa0,fpa0
	fpmoved	fpa0,d0:d1
	.end

	.inline	_acos,8
	facosd	sp@,fp0
	fmoved	fp0,sp@
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_asin,8
	fasind	sp@,fp0
	fmoved	fp0,sp@
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_cosh,8
	fcoshd	sp@,fp0
	fmoved	fp0,sp@
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_log10,8
	flog10d	sp@,fp0
	fmoved	fp0,sp@
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_sinh,8
	fsinhd	sp@,fp0
	fmoved	fp0,sp@
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_tan,8
	ftand	sp@,fp0
	fmoved	fp0,sp@
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_tanh,8
	ftanhd	sp@,fp0
	fmoved	fp0,sp@
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_rint,8
	fintd	sp@,fp0
	fmoved	fp0,sp@
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline	_fmod,16
	fmoved	sp@+,fp0
	fmodd	sp@,fp0
	fmoved	fp0,sp@
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline	_ldexp,12
	fmoved	sp@+,fp0
	fscalel	sp@+,fp0
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline	_scalb,12
	fmoved	sp@+,fp0
	fscalel	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end
