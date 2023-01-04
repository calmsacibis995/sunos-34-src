        .data
        .asciz  "@(#)d_tan.s 1.1 86/09/25 Copyr 1984 Sun Micro"
        .even
        .data

|       Copyright (c) 1984 by Sun Microsystems, Inc.

	.globl	fvtani
fvtani:         jsr     sky_switch:L
                jmp     Ftani:L
                jmp     Stani:L
        
	.even
        .text
Ftani:
Stani:
        movl    d1,sp@-
        movl    d0,sp@-
        jsr     _tan
        addql   #8,sp
        rts
 
