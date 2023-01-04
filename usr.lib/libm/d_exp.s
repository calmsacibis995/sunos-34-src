        .data
        .asciz  "@(#)d_exp.s 1.1 86/09/25 Copyr 1984 Sun Micro"
        .even
        .data

|       Copyright (c) 1984 by Sun Microsystems, Inc.

	.globl	fvexpi
fvexpi:         jsr     sky_switch:L
                jmp     Fexpi:L
                jmp     Sexpi:L
        
	.even
        .text
        .globl  Fexpi,Sexpi
Fexpi:
Sexpi:
        movl    d1,sp@-
        movl    d0,sp@-
        jsr     _exp
        addql   #8,sp
        rts
 
