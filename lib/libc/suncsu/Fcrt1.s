|        .data
|        .asciz  "@(#)Fcrt1.s 1.1 86/09/24 Copyr 1986 Sun Micro"
|        .even
        .text

|       Copyright (c) 1986 by Sun Microsystems, Inc.

	.globl	start_float,fsoft_used

start_float:
fsoft_used:

	jmp	_finitfp_
