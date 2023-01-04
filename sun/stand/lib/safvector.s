	.data
	.asciz	"@(#)safvector.s 1.1 9/25/86 Copyright Sun Micro"
	.even
	.text

|	Copyright (c) 1984 by Sun Microsystems, Inc.
|	blame kevin@uranus

	.globl floatflavor, ieeeused
floatflavor:
ieeeused: .asciz "IEEE software"
	.globl fvflti, fvfltis, fvfixi, fvfixis, fvdoublei, fvsinglei
	.globl fvcmpi, fvcmpis, fvaddi, fvaddis, fvsubi, fvsubis
	.globl fvmuli, fvmulis, fvdivi, fvdivis, fvmodi, fvmodis

	.globl Fflti, Ffltis, Ffixi, Ffixis, Fdoublei, Fsinglei
	.globl Fcmpi, Fcmpis, Faddi, Faddis, Fsubi, Fsubis
	.globl Fmuli, Fmulis, Fdivi, Fdivis

| transfer vectors
		.text
		.even
fvflti:  	jra	Fflti
fvfltis:  	jra	Ffltis
fvfixi:  	jra	Ffixi
fvfixis:  	jra	Ffixis
fvdoublei:  	jra	Fdoublei
fvsinglei:  	jra	Fsinglei
fvcmpi:  	jra	Fcmpi
fvcmpis:  	jra	Fcmpis
fvaddi:  	jra	Faddi
fvaddis:  	jra	Faddis
fvsubi:  	jra	Fsubi
fvsubis:  	jra	Fsubis
fvmuli:  	jra	Fmuli
fvmulis:  	jra	Fmulis
fvdivi:  	jra	Fdivi
fvdivis:  	jra	Fdivis
fvmodi:  	jra	aborter
fvmodis:  	jra	aborter
		.even
		.data
abortmess:	.asciz		"Someone near 0x%x called abort in floating routines\012"
		.even
		.text

aborter:	movl	#abortmess, sp@-	| address is on stack, looks like parameter
		jsr	_printf			| printf message
		addql	#4, sp			| clean up stack
		rts				| and go back and f... up
