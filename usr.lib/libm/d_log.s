        .data
        .asciz  "@(#)d_log.s 1.1 86/09/25 Copyr 1984 Sun Micro"
        .even
        .data

|       Copyright (c) 1984 by Sun Microsystems, Inc.

	.globl	fvlogi
fvlogi:         jsr     sky_switch:L
                jmp     Flogi:L
                jmp     Slogi:L
        
	.even
        .text
        .globl  Flogi,Slogi
Flogi:
Slogi:
        movl    d1,sp@-
        movl    d0,sp@-
        jsr     _log
        addql   #8,sp
        rts
 
