        .data
        .asciz  "@(#)Scrt1.s 1.1 86/09/24 Copyr 1985 Sun Micro"
        .even
        .text

|       Copyright (c) 1985 by Sun Microsystems, Inc.

	.globl	start_float,fsky_used,skyused

start_float:
fsky_used:
skyused:			| obsolete	
	jmp	Sswitch
