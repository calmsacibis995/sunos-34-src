	.data
	.asciz	"@(#)fvector.s 1.1 86/09/24 Copyr 1983 Sun Micro"
	.even
	.text

|	Copyright (c) 1984 by Sun Microsystems, Inc.

	.globl floatflavor, ieeeused
floatflavor:
ieeeused: .asciz "IEEE software+SKY"
	.globl fvflti, fvfltis, fvfixi, fvfixis, fvdoublei, fvsinglei
	.globl fvcmpi, fvcmpis, fvaddi, fvaddis, fvsubi, fvsubis
	.globl fvmuli, fvmulis, fvdivi, fvdivis, fvmodi, fvmodis

	.globl Fflti, Ffltis, Ffixi, Ffixis, Fdoublei, Fsinglei
	.globl Fcmpi, Fcmpis, Faddi, Faddis, Fsubi, Fsubis
	.globl Fmuli, Fmulis, Fdivi, Fdivis

	.globl Sflti, Sfltis, Sfixi, Sfixis, Sdoublei, Ssinglei
	.globl Scmpi, Scmpis, Saddi, Saddis, Ssubi, Ssubis
	.globl Smuli, Smulis, Sdivi, Sdivis

| transfer vectors, rearranged when and if we find a SKY board.
		.data
		.even
arith_vec:
fvflti: 	jsr	discoverSky:L
fvfltis: 	jsr	discoverSky:L
fvfixi: 	jsr	discoverSky:L
fvfixis: 	jsr	discoverSky:L
fvdoublei: 	jsr	discoverSky:L
fvsinglei: 	jsr	discoverSky:L
fvcmpi: 	jsr	discoverSky:L
fvcmpis: 	jsr	discoverSky:L
fvaddi: 	jsr	discoverSky:L
fvaddis: 	jsr	discoverSky:L
fvsubi: 	jsr	discoverSky:L
fvsubis: 	jsr	discoverSky:L
fvmuli: 	jsr	discoverSky:L
fvmulis: 	jsr	discoverSky:L
fvdivi: 	jsr	discoverSky:L
fvdivis: 	jsr	discoverSky:L
fvmodi: 	jsr	discoverSky:L
fvmodis: 	jsr	discoverSky:L
		.word	0	| extra slop incase length%4 != 0
		.text
soft_vec: | must be JUST LIKE ABOVE!!
 		jmp	Fflti:L
	 	jmp	Ffltis:L
 		jmp	Ffixi:L
	 	jmp	Ffixis:L
	 	jmp	Fdoublei:L
	 	jmp	Fsinglei:L
 		jmp	Fcmpi:L
	 	jmp	Fcmpis:L
 		jmp	Faddi:L
	 	jmp	Faddis:L
 		jmp	Fsubi:L
	 	jmp	Fsubis:L
 		jmp	Fmuli:L
	 	jmp	Fmulis:L
 		jmp	Fdivi:L
	 	jmp	Fdivis:L
 		jmp	_abort:L
	 	jmp	_abort:L
		.even
sky_vec: | must be just like the above!!
		jmp	Sflti:L
		jmp	Sfltis:L
		jmp	Sfixi:L
		jmp	Sfixis:L
		jmp	Sdoublei:L
		jmp	Ssinglei:L
		jmp	Scmpi:L
		jmp	Scmpis:L
		jmp	Saddi:L
		jmp	Saddis:L
		jmp	Ssubi:L
		jmp	Ssubis:L
		jmp	Smuli:L
		jmp	Smulis:L
		jmp	Sdivi:L
		jmp	Sdivis:L
		jmp	_abort:L
		jmp	_abort:L
		nsky =  (.-sky_vec)/4

	.globl	__skyinit, skyvector
	SAVETMP	=	0xC0C0
	RESTTMP	=	0x0303
discoverSky:
	| first discover if there is a sky board and map it in. If there
	| is a sky board, then change the transfer vector above to point
	| to the sky routines, otherwise, make it point to the software 
	| routines.
	moveml	#SAVETMP,sp@-	| save d0,d1,a0,a1
	jsr	__skyinit
	tstl	d0
	beqs	1f
	lea	sky_vec,a0
	bras	2f
1:	lea	soft_vec,a0
2:	movw	#nsky-1,d0
	lea	fvflti,a1
3:	movl	a0@+,a1@+
	dbra	d0,3b
	moveml	sp@+,#RESTTMP	| restore d0,d1,a0,a1
	| now back up pc and do it again.
	subl	#6,sp@
	rts

skyvector:
	| we get here when we know a priori that the sky board is 
	| present, and wish to remap the floating point transfer vectors.
	| it is just the loop from discoverSky.
	lea	sky_vec,a0
	movw	#nsky-1,d0
	lea	arith_vec,a1
1:	movl	a0@+,a1@+
	dbra	d0,1b
	rts
	
	.globl	_sfloat_
_sfloat_:
	moveml	#SAVETMP,sp@-	| save d0,d1,a0,a1
	jsr	__skyinit
	lea	sky_vec,a0
	movw	#nsky-1,d0
	lea	arith_vec,a1
1:	movl	a0@+,a1@+
	dbra	d0,1b
	moveml	sp@+,#RESTTMP	| restore d0,d1,a0,a1
	rts
	.globl	_ffloat_
_ffloat_:
	moveml	#SAVETMP,sp@-	| save d0,d1,a0,a1
	lea	soft_vec,a0
	movw	#nsky-1,d0
	lea	arith_vec,a1
1:	movl	a0@+,a1@+
	dbra	d0,1b
	moveml	sp@+,#RESTTMP	| restore d0,d1,a0,a1
	rts
	.globl	_vfloat_
_vfloat_:
	moveml	#SAVETMP,sp@-	| save d0,d1,a0,a1
	jsr	__skyinit
	tstl	d0
	beqs	1f
	lea	sky_vec,a0
	bras	2f
1:	lea	soft_vec,a0
2:	movw	#nsky-1,d0
	lea	arith_vec,a1
1:	movl	a0@+,a1@+
	dbra	d0,1b
	moveml	sp@+,#RESTTMP	| restore d0,d1,a0,a1
	rts

	|	The sky/software mapping for functions relies on the entry
	| points being of the form
	|	
	|	fvXXX:	jsr	sky_switch:L
	|		jmp	FXXX:L
	|		jmp	SXXX:L
	|
	| First discover if there is a sky board and map it in. If there
	| is a sky board, then change the calling short transfer vector
	| to point
	| to the sky routine SXXX, otherwise, make it point to the software 
	| routine FXXX.
	
	.globl	sky_switch
	SAVETMP	=	0xC0C0
	RESTTMP	=	0x0303
sky_switch:
	link	a6,#0
	moveml	#SAVETMP,sp@-	| save d0,d1,a0,a1
	jsr	__skyinit
	movl	a6@(4),a0	| a0 gets fvX+4.
	tstl	d0
	beqs	nosky
	movl	a0@(6),a0@(-6)
	movw	a0@(10),a0@(-2)
	addql	#6,a6@(4)	| Fiddle return address for rts.
	bras	sky_ret
nosky:
	movl	a0@,a0@(-6)
	movw	a0@(4),a0@(-2)
sky_ret:
	moveml	sp@+,#RESTTMP	| restore d0,d1,a0,a1
	unlk	a6
	rts
