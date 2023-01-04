        .data
        .asciz  "@(#)d_atan.s 1.1 86/09/25 Copyr 1984 Sun Micro"
        .even
        .data

|       Copyright (c) 1984 by Sun Microsystems, Inc.

	.globl	fvatani
fvatani:        jsr     sky_switch:L
                jmp     Fatani:L
                jmp     Satani:L
        
	.even
        .text
        .globl  Fatani,Satani
Fatani:
Satani:
        movl    d1,sp@-
        movl    d0,sp@-
        jsr     _atan
        addql   #8,sp
        rts
 
