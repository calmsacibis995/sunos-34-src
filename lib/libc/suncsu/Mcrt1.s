        .data
        .asciz  "@(#)Mcrt1.s 1.1 86/09/24 Copyr 1985 Sun Micro"
        .even
        .text

|       Copyright (c) 1985 by Sun Microsystems, Inc.

	.globl	start_float,f68881_used

start_float:
f68881_used:
	
	jmp	Mswitch
