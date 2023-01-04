	.data
	.asciz	"@(#)s_sin.s 1.3 84/08/30 Copyr 1983 Sun Micro"
	.even
	.text
|	Copyright (c) 1983 by Sun Microsystems, Inc.

#include "DEFS.h"

| single precison sin function from Cody and Waite "Software Manual for
| the Elementary Functions" Prentice Hall, 1980.

	SAVED0D1 = 0x0003
	RESTD0D1 = 0x0003
	SAVEALL	 = 0x3F00	| registers d2-d4
	RESTALL	 = 0x00fc	

PI:	.long	0x400921FB	| 3.141593e+00
	.long	0x54442D1A
ONEOPI:	.long	0x3EA2F983	| 3.183099e-01
YMAX:	.long	0x46C90E00	| 2.573500e+04
EPS:	.long	0x39800000	| 2.441406e-04
ONE:	.long	0x3F800000	| 1.000000e+00
R1:	.long	0xBE2AAAA4	| -1.666666e-01
R2:	.long	0x3C08873E	| 8.333026e-03
R3:	.long	0xB94FB222	| -1.980742e-04
R4:	.long	0x362E9C5B	| 2.601903e-06


| temp regs:
|			d4 holds f
|			d5 holds g
|			d7 holds y
| temps (off of a6):
|			1-4 X
|			5-8 unused
|			9-12 SGN flag
|			13-20 double precision X
	.globl	Fsinis
Fsinis:
	link	a6,#-20
	RTMCOUNT
	moveml	#SAVEALL,sp@-			| save d2-d7
	movl	d0,d1
	movl	d1,a6@(-4)			| save 
	jsr	f_tst				| test the argument
	addw	d0,d0			 | switch on the type field 
	movw	pc@(6,d0:w),d1
	jmp	pc@(2,d1:w)
Ltable:
	.word	Lerr-Ltable			| unknown type
	.word	Lzero-Ltable			| zero
	.word	Lgu-Ltable			| gradual undeflow
	.word	Lplain-Ltable			| ordinary number
	.word	Linf-Ltable			| infinity
	.word	Lnan-Ltable			| Nan
	.word	Lerr-Ltable			| unknown type
	.word	Lerr-Ltable			| unknown type
Lplain:
| the sgn flag will be eored to the result to get the right sign
	movl	a6@(-4),d7			
	movl	d7,d1
	jsr		f_unpk				| unpack argument
	movl	#0,a6@(-12)	 		| 0 -> sgn flag 
	bclr	#15,d3				| take abs of unpacked form
	beqs	1$				| if sign bit was zero continue
	bclr	#31,d7				| else abs(x) -> d7
	movl	#0x80000000,a6@(-12)		| 1 -> sgn flag
1$:	
	jsr	d_pack				| make a double precision abs(x)
	moveml	#SAVED0D1,a6@(-20)		| and save it for later use
	movl	d7,d0		
	movl	YMAX,d1				
	jsr 	fvcmpis				| if y ge ymax
	bge	Ltoobig				| error
	movl	d7,d0				| y-> d0
	movl	ONEOPI,d1			| 1/pi -> d1
	jsr	fvmulis				| y/pi->d0
	movl	d0,d1				| unpack result
	jsr	f_unpk
	tstw	d2				| test the unpacked exponent
	bpls	2$				| if > 0 value is an int - no rounding
	addql	#1,d2				| else mulitply by two
	jsr	gnsh				| apply the  exponent
	asrl	#1,d0				| divide by two : lsb of d0 ->x 
	roxrl	#1,d1				| rotate with lsb of d1 -> x
	addxl	d2,d1				| add x, strong assumption that d2 0 here 
	addxl	d2,d0				| and take care of overflow
2$:	btst 	#0,d1				| test parity of unpacked n
	beqs	3$				| if lsb 0 ( even ) ok
	bchg	#7,a6@(-12)			| if odd reverse sgn flag
3$:	jsr	d_pack				| make xn double precision ; xn now in d0/d1
	lea	PI,a0				| load pi
	jsr 	fvmuli				| xn*pi in d0/d1 
	bchg	#31,d0				| reverse its sign
	lea	a6@(-20),a0			| set a0 to abs(x)
	jsr	fvaddi				| abs(x)-xn*pi->d0/d1
	jsr	d_unpk				| range reduction done: convert back to single
	jsr	f_pack
	movl	d1,d4				| keep a copy of f
	bclr	#31,d1				| abs(f) -> d1
	movl	EPS,d0				
	jsr	fvcmpis				| if eps le abs(f) continue
	bles	4$
	movl	d4,d0				| else f -> result
	bras	L15				| and quit
4$: 	movl 	d4,d0				| f-> d0
	movl	d0,d1
	jsr	fvmulis				| f*f -> d0
	movl	d0,d5				| save g
	movl	R4,d1				| compute the polynomial approximation
	jsr	fvmulis
	movl	R3,d1				
	jsr	fvaddis
	movl	d5,d1
	jsr	fvmulis
	movl	R2,d1
	jsr	fvaddis
	movl	d5,d1
	jsr	fvmulis
	movl	R1,d1
	jsr	fvaddis
	movl	d5,d1
	jsr	fvmulis
	movl	d4,d1				| f*p(g) -> d0
	jsr	fvmulis
	movl	d4,d1				| f+d0	-> d0
	jsr	fvaddis
L15:
	movl	a6@(-12),d1
 	eorl	d1,d0				| sgn*d0 -> d0
Ldone:
	moveml sp@+,#RESTALL
	unlk	a6
	rts	
Lzero:
Lgu:
Lnan:
	movl	a6@(-4),d0
	bras	Ldone
Ltoobig:
Lerr:
Linf:
	jsr	f_snan
	bras	Ldone
