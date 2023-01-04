	.data
|	.asciz	"@(#)fpa_trans.s 1.1 86/09/24 Copyr 1986 Sun Micro"
	.even
	.text

|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

/*
 	double fpa_transcendental ( n, x )
	long n ; double x ;
	
	computes fn(x) where
	0 = sin
	1 = cos
	2 = tan
	3 = atan
	4 = exp-1
	5 = ln(1+x)
	6 = exp
	7 = ln
	8 = sqrt
*/

RTENTRY(_fpa_transcendental)
	movel	PARAM,d0
	lsll	#2,d0
	lea	table,a0
	addl	d0,a0
	movel	a0@,a0
	jmp	a0@		
table:
	.long	msind,mcosd,mtand,matand,mexp1d,mlog1d,mexpd,mlogd,msqrtd
mlogd:	
|        fmoved  PARAM2,fp0
|        fcmps   #0r0.5,fp0
|        fjule    2f              | Branch if x <= 0.5.
|        fsubl   #1,fp0
|        flognp1x fp0,fp0        | This is more accurate for x > 0.5.
|        bras    1f
|2:
|        flognx  fp0,fp0         | This is more accurate for x < 0.5.
	flognd	PARAM2,fp0
	bras	1f
msqrtd:	
	fsqrtd	PARAM2,fp0
	bras	1f
msind:	
	fsind	PARAM2,fp0
	bras	1f
mcosd:	
	fcosd	PARAM2,fp0
	bras	1f
mtand:	
	ftand	PARAM2,fp0
	bras	1f
matand:	
	fatand	PARAM2,fp0
	bras	1f
mexp1d:	
	fetoxm1d	PARAM2,fp0
	bras	1f
mexpd:	
	fetoxd	PARAM2,fp0
	bras	1f
mlog1d:	
	flognp1d	PARAM2,fp0
	bras	1f
1:
	fmoved	fp0,sp@-
	moveml	sp@+,d0/d1
	RET

/*
	double	fpa_tolong ( x )
	double x ;

	computes (double) (long) x
	according to Weitek mode
*/

RTENTRY(_fpa_tolong)
	fpmove	fpamode,d0
	andb	#2,d0
	beqs	1f		| Branch if round to current mode.
	fintrzd	PARAM,fp0
	bras	2f
1:
	fintd	PARAM,fp0
2:
	fmovel	fp0,d0
	fmovel	d0,fp0
	fmoved	fp0,PARAM
	moveml	PARAM,d0/d1
	RET

        .data
        .globl  __fpabase       | Referred to by -ffpa compiled code.
__fpabase = 0xe0000000
