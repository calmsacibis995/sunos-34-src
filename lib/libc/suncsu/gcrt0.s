	.data
	.even
	.globl	_environ
_environ:
	.long	0
	.text
	.globl	 _on_exit
	.globl	start
	.globl	____Argv
start:
	movl	sp@,d2		| argc
	lea	sp@(4),a3	| argv
	movl	a3,____Argv	| prog name for profiling
	movl	d2,d1
	asll	#2,d1		| argc * sizeof(argv[0])
	lea	a3@(4,d1:l),a4	| environ
	movl	a4,_environ
	lea	0,a6		| stack frame link 0 in main -- for dbx
_$eprol:
	pea	_etext
	pea	_$eprol
	jsr	_monstartup
	addql	#8,sp
	pea	a4@
	pea	a3@
	movl	d2,sp@-
	jbsr	copynote
	jsr	start_float
	clrl	sp@-
	pea	__mcleanup
	jsr	_on_exit	| on_exit( _mcleanup, NULL )
	addqw	#8,sp
	jsr	_main		| main( argc, argv, environ )
	addw	#12,sp
	movl	d0,sp@-
	jsr	_exit		| exit( whatever main returned )
	addql	#4,sp
#ifdef OLD
	movl	#0,sp@-		| throw away main's return value, use 0
#else
	movl	d0,sp@-
#endif
	jsr	__exit		| _exit( exit(whatever main returned))
	| /* NOTREACHED */
copynote:
	bras	1f
	.asciz	"@(#)gcrt0.s	1.1	86/09/24	Copyr 1985 Sun Micro"
	.even
|	Copyright (c) 1985 by Sun Microsystems, Inc.
1:	rts
