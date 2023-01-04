|	.asciz	"@(#)f68881.s 1.1 86/09/25 Copyr 1986 Sun Micro"
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

|	-f68881 Inline expansion file.

	.inline	Fc_minus,16	
        fmoves sp@+,fp0
        fmoves sp@+,fp1
        fsubs  sp@+,fp0
        fsubs  sp@+,fp1
        fmoves fp0,d0
        fmoves fp1,d1
	.end

	.inline	Fc_add,16	
        fmoves sp@+,fp0
        fmoves sp@+,fp1
        fadds  sp@+,fp0
        fadds  sp@+,fp1
        fmoves fp0,d0
        fmoves fp1,d1
	.end

		| 	convert complex  to int
	.inline	Fc_conv_i,8
        fintrzs  sp@+,fp0       | pick up argument
        movl    sp@+,d1         | ignore this
        fmovel 	fp0,d0
	.end

		| 	convert int to complex 
	.inline	Fi_conv_c,4
        fmovel  sp@+,fp0       | pick up argument
        clrl    d1
	fmoves  fp0,d0
	.end

		|	convert complex  to double
	.inline	Fc_conv_d,8
	fmoves  sp@+,fp0   	| pick up argument
	movl	sp@+,d1		| ignore this
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
	.end

		|	convert double  to complex
	.inline	Fd_conv_c,8
	fmoved  sp@+,fp0   
	fmoves	fp0,d0
	clrl	d1
	.end

		| 	convert complex to double complex 
		|	arg1 is address of result; arg2 is complex as a double
	.inline	Fc_conv_z,12
        movel   sp@+,a0         | a0 gets address of result.
        fmoves sp@+,fp0       | fpa0 gets re.
        fmoves sp@+,fp1       | fpa1 gets im.
        fmoved fp0,a0@+
        fmoved fp1,a0@+
	.end

	.inline	_c_abs,4
	movl	sp@+,a0
	fmoves	a0@+,fp0
	fmulx	fp0,fp0
	fmoves	a0@,fp1
	fmulx	fp1,fp1
	faddx	fp1,fp0
	fsqrtx	fp0,fp0
	fmoves	fp0,d0
	.end

	.inline	_d_dim,8
        movel   sp@+,a0
        fmoved  a0@,fp0
        movel   sp@+,a0
        fsubd   a0@,fp0
	fjugt	1f		| Accept difference if ? or >0.
	clrl	d0		| Return +0 if difference is <= 0.
	clrl	d1
	jra	2f
1:
        fmoved  fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
2:
	.end

	.inline	_d_mod,8
        movel   sp@+,a0
        fmoved  a0@,fp0
        movel   sp@+,a1
        fmodd   a1@,fp0
        fmoved  fp0,sp@-
        movel   sp@+,d0
        movel   sp@+,d1
 	.end

	.inline	_r_dim,8
        movel   sp@+,a0
        fmoves 	a0@,fp0
        movel   sp@+,a0
        fsubs  	a0@,fp0
	fjugt	1f
	clrl	d0		| Return 0 if diff <=0.
	jra	2f
1:
        fmoves 	fp0,d0		| Accept difference if ? or >0.
2:
	.end

	.inline	_r_mod,8
        movel   sp@+,a0
        fmoves  a0@,fp0
        movel   sp@+,a0
        fmods   a0@,fp0
        fmoves  fp0,d0
 	.end

	.inline	_z_abs,4
	movl	sp@+,a0
	fmoved	a0@+,fp0
	fmoved	a0@,fp1
        fmulx   fp0,fp0
        fmulx   fp1,fp1
        faddx   fp1,fp0
        fsqrtx  fp0,fp0
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline	Fc_mult,16
        fmoves  sp@,fp0       		| a
        fmoves  sp@(4),fp1       	| b
        fmuls  	sp@(8),fp0        	| ac
        fmuls   sp@(12),fp1     	| bd
        fsubx 	fp1,fp0       		| fp0 = ac-bd
        fmoves 	fp0,d0
        fmoves  sp@+,fp0       		| a
        fmoves  sp@+,fp1       		| b
        fmuls	sp@+,fp1		| bc
	fmuls	sp@+,fp0		| ad
        faddx  	fp1,fp0       		| fp1 = ad+bc
        fmoves 	fp0,d1
   	.end

	.inline	Fz_mult,12	
	movl	sp@+,d0		| d0 = c;
	movl	sp@+,a1		| a0 = a;
	movl	sp@+,a0		| a1 = b;
	fmoved	a1@,fp0		| fp0 = (a->dreal *  b->dreal);
	fmuld	a0@,fp0
	fmoved	a1@(8),fp1	| fp1 = (a->dimag *  b->dimag);
	fmuld	a0@(8),fp1
	fsubx	fp1,fp0		| c->dreal = fp0 - fp1;
	exg	d0,a0
	fmoved	fp0,a0@
	exg	d0,a0
	fmoved	a1@,fp0		| fp0 = (a->dreal *  b->dimag);
	fmuld	a0@(8),fp0
	fmoved	a1@(8),fp1	| fp1 = (a->dimag *  b->dreal);
	fmuld	a0@,fp1
	faddx	fp1,fp0		| c->dimag = fp0 + fp1;
	movl	d0,a0
	fmoved	fp0,a0@(8)
	.end

	.inline	__z_mult,12
	movl	sp@+,d0		| d0 = c;
	movl	sp@+,a1		| a0 = a;
	movl	sp@+,a0		| a1 = b;
	fmoved	a1@,fp0		| fp0 = (a->dreal *  b->dreal);
	fmuld	a0@,fp0
	fmoved	a1@(8),fp1	| fp1 = (a->dimag *  b->dimag);
	fmuld	a0@(8),fp1
	fsubx	fp1,fp0		| c->dreal = fp0 - fp1;
	exg	d0,a0
	fmoved	fp0,a0@
	exg	d0,a0
	fmoved	a1@,fp0		| fp0 = (a->dreal *  b->dimag);
	fmuld	a0@(8),fp0
	fmoved	a1@(8),fp1	| fp1 = (a->dimag *  b->dreal);
	fmuld	a0@,fp1
	faddx	fp1,fp0		| c->dimag = fp0 + fp1;
	movl	d0,a0
	fmoved	fp0,a0@(8)
	.end

	.inline	Fz_minus,12
	movl	sp@+,d0		| d0 = c;
	movl	sp@+,a0		| a0 = a;
	movl	sp@+,a1		| a1 = b;
	fmoved	a0@+,fp0	| fp0 = a->dreal - b->dreal;
	fsubd	a1@+,fp0
	fmoved	a0@,fp1		| fp1 = a->dimag - b->dimag;
	fsubd	a1@,fp1
	movl	d0,a0
	fmoved	fp0,a0@+	| c->dreal = fp0;
	fmoved	fp1,a0@		| c->dimag = fp1;
	.end

	.inline	__z_minus,12
	movl	sp@+,d0		| d0 = c;
	movl	sp@+,a0		| a0 = a;
	movl	sp@+,a1		| a1 = b;
	fmoved	a0@+,fp0	| fp0 = a->dreal - b->dreal;
	fsubd	a1@+,fp0
	fmoved	a0@,fp1		| fp1 = a->dimag - b->dimag;
	fsubd	a1@,fp1
	movl	d0,a0
	fmoved	fp0,a0@+	| c->dreal = fp0;
	fmoved	fp1,a0@		| c->dimag = fp1;
	.end

	.inline	Fz_add,12
	movl	sp@+,d0		| d0 = c;
	movl	sp@+,a0		| a0 = a;
	movl	sp@+,a1		| a1 = b;
	fmoved	a0@+,fp0	| fp0 = a->dreal + b->dreal;
	faddd	a1@+,fp0
	fmoved	a0@,fp1		| fp1 = a->dimag + b->dimag;
	faddd	a1@,fp1
	movl	d0,a0
	fmoved	fp0,a0@+	| c->dreal = fp0;
	fmoved	fp1,a0@		| c->dimag = fp1;
	.end

	.inline	__z_add,12
	movl	sp@+,d0		| d0 = c;
	movl	sp@+,a0		| a0 = a;
	movl	sp@+,a1		| a1 = b;
	fmoved	a0@+,fp0	| fp0 = a->dreal + b->dreal;
	faddd	a1@+,fp0
	fmoved	a0@,fp1		| fp1 = a->dimag + b->dimag;
	faddd	a1@,fp1
	movl	d0,a0
	fmoved	fp0,a0@+	| c->dreal = fp0;
	fmoved	fp1,a0@		| c->dimag = fp1;
	.end

	.inline	Ff_conv_z,8
	movl	sp@+,a0
	fmoves	sp@+,fp0
	fmoved	fp0,a0@+
	clrl    a0@+
	clrl    a0@
	.end

	.inline	Fz_conv_f,4
	movl    sp@+,a0   
	fmoved	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	Fz_conv_i,4
	movl    sp@+,a0   
	fintrzd	a0@,fp0
	fmovel	fp0,d0
	.end

	.inline	Fi_conv_z,8
	movl	sp@+,a0
	fmovel	sp@+,fp0
	fmoved	fp0,a0@+
	clrl    a0@+
	clrl    a0@
	.end

	.inline	Fz_conv_c,4
	movl	sp@+,a0
	fmoved	a0@+,fp0
	fmoved	a0@+,fp1
	fmoves	fp0,d0
	fmoves	fp1,d1
	.end

	.inline Mlog10s,0
	flog10s	d0,fp0
	fmoves	fp0,d0
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

	.inline	_atan,8
	fatand	sp@,fp0
	fmoved	fp0,sp@
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_cos,8
	fcosd	sp@,fp0
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

	.inline	_exp,8
	fetoxd	sp@,fp0
	fmoved	fp0,sp@
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_expm1,8
	fetoxm1d	sp@,fp0
	fmoved	fp0,sp@
	movl	sp@+,d0
	movl	sp@+,d1
	.end

|				Comment out due to A79J bug.
|	.inline	_log,8
|	flognd	sp@,fp0
|	fmoved	fp0,sp@
|	movl	sp@+,d0
|	movl	sp@+,d1
|	.end

	.inline	_log1p,8
	flognp1d	sp@,fp0
	fmoved	fp0,sp@
	movl	sp@+,d0
	movl	sp@+,d1
	.end

|				Comment out due to A79J bug.
|	.inline	_log10,8
|	flog10d	sp@,fp0
|	fmoved	fp0,sp@
|	movl	sp@+,d0
|	movl	sp@+,d1
|	.end

	.inline	_sin,8
	fsind	sp@,fp0
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

	.inline	_sqrt,8
	fsqrtd	sp@,fp0
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
