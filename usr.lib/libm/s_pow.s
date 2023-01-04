        .data
        .asciz  "@(#)s_pow.s 1.1 86/09/25 Copyr 1984 Sun Micro"
        .even
        .data

|       Copyright (c) 1984 by Sun Microsystems, Inc.

	.globl	fvpowis
fvpowis:        jsr     sky_switch:L
                jmp     Fpowis:L
                jmp     Spowis:L
        
	.even
        .text
        .globl  Fpowis
Fpowis:
        movl    d0,a0           | Save x argument.
        movl    d1,d0           | d0 gets y argument.
        jsr     fvdoublei       | d0/d1 gets double(y).
        moveml  #0xc000,sp@-    | Push double(y) on stack.
        movl    a0,d0           | d0 gets x.
        jsr     fvdoublei       | d0/d1 gets double(x).
        moveml  #0xc000,sp@-    | Push double(x).
        jsr     _s_pow          | d0/d1 gets pow(x,y).
        addl    #16,sp          | Remove arguments.
        jsr     fvsinglei       | d0 gets single result.
        rts
 
