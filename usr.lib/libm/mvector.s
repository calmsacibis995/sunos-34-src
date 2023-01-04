        .data 
        .asciz  "@(#)mvector.s 1.1 86/09/25 Copyr 1985 Sun Micro"
        .even
        .text

|	Copyright (c) 1985 by Sun Microsystems, Inc.
	
firstfunc:
	.long	fvsqrtis
	.long	fvexpis
	.long	fvlogis
	.long	fvpowis
	.long	fvsinis
	.long	fvcosis
	.long	fvtanis
	.long	fvatanis
	.long	fvsqrti
	.long	fvexpi
	.long	fvlogi
	.long	fvpowi
	.long	fvsini
	.long	fvcosi
	.long	fvtani
	.long	fvatani
lastfunc:

	SAVETMP =	0xc0e0		| d0,d1,a0,a1,a2
	RESTTMP =	0x0703

	.globl  _sfunc_
_sfunc_:
	moveml	#SAVETMP,sp@-	| save d0,d1,a0,a1
        jsr	__skyinit
_sfunc_x:
	jsr	_sfloat_
	lea	firstfunc,a0	| a0 gets current table entry.
	lea	lastfunc,a1	| a1 gets end of table address.
1$:
	movl	a0@+,a2		| a2 gets address of fvXXX.
	movw	a2@(12),a2@	| Move jmp SXXX.	
	movl	a2@(14),a2@(2)	
	cmpl	a0,a1
	bnes	1$
	moveml	sp@+,#RESTTMP	| restore d0,d1,a0,a1
        rts
        .globl  _ffunc_
_ffunc_:
	moveml	#SAVETMP,sp@-	| save d0,d1,a0,a1
_ffunc_x:
	jsr	_ffloat_
	lea	firstfunc,a0	| a0 gets current table entry.
	lea	lastfunc,a1	| a1 gets end of table address.
1$:
	movl	a0@+,a2		| a2 gets address of fvXXX.
	movw	a2@(6),a2@	| Move jmp FXXX.	
	movl	a2@(8),a2@(2)	
	cmpl	a0,a1
	bnes	1$
	moveml	sp@+,#RESTTMP	| restore d0,d1,a0,a1
        rts
        .globl  _vfunc_
_vfunc_:
	moveml	#SAVETMP,sp@-	| save d0,d1,a0,a1
	jsr	__skyinit
	tstl	d0
	beqs	_ffunc_x
	bras	_sfunc_x
