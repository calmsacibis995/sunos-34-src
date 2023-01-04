        .data
        .asciz  "@(#)d_pow.s 1.1 86/09/25 Copyr 1984 Sun Micro"
        .even
        .data

|       Copyright (c) 1984 by Sun Microsystems, Inc.

	.globl	fvpowi
fvpowi:         jsr     sky_switch:L
                jmp     Fpowi:L
                jmp     Spowi:L
        
	.even
        .text
        .globl  Fpowi,Spowi
Fpowi:
Spowi:
        movl    d1,sp@-
        movl    d0,sp@-
        jsr     _pow
        addl    #8,sp
        rts
 
