        .data
        .asciz  "@(#)Fpows.s 1.1 86/09/25 Copyr 1985 Sun Micro"
        .even
        .text

|       Copyright (c) 1985 by Sun Microsystems, Inc.

        .globl  Fpows
Fpows:
        movel	d2,sp@-
	movl    d0,d2           | Save x argument.
        movl    d1,d0           | d0 gets y argument.
        jsr     Fstod       | d0/d1 gets double(y).
        moveml  #0xc000,sp@-    | Push double(y) on stack.
        movl    d2,d0           | d0 gets x.
        jsr     Fstod       | d0/d1 gets double(x).
        moveml  #0xc000,sp@-    | Push double(x).
        jsr     _CFpows          | d0/d1 gets pow(x,y).
        addl    #16,sp          | Remove arguments.
        movel	sp@+,d2
	jmp     Fdtos       | d0 gets single result.
