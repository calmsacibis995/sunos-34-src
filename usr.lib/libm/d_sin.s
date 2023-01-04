        .data
        .asciz  "@(#)d_sin.s 1.1 86/09/25 Copyr 1984 Sun Micro"
        .even
        .data

|       Copyright (c) 1984 by Sun Microsystems, Inc.

	.globl	fvsini
fvsini:         jsr     sky_switch:L
                jmp     Fsini:L
                jmp     Ssini:L
        
	.even
        .text
        .globl  Fsini,Ssini
Fsini:
Ssini:
        movl    d1,sp@-
        movl    d0,sp@-
        jsr     _sin
        addql   #8,sp
        rts
 
