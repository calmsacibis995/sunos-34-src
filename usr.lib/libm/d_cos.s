        .data
        .asciz  "@(#)d_cos.s 1.1 86/09/25 Copyr 1984 Sun Micro"
        .even
        .data

|       Copyright (c) 1984 by Sun Microsystems, Inc.

	.globl	fvcosi
fvcosi:         jsr     sky_switch:L
                jmp     Fcosi:L
                jmp     Scosi:L
        
	.even
        .text
        .globl  Fcosi,Scosi
Fcosi:
Scosi:
        movl    d1,sp@-
        movl    d0,sp@-
        jsr     _cos
        addql   #8,sp
        rts
 
