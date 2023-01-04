/*	@(#)langnames.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
#define FORTNAME( X ) UUUFORT( _/**/X/**/_ , _/**/X )
#define UUUFORT( X, Y ) asm(" .globl X ; X = Y ");

#define PASCNAME( X ) UUUPASC( ___/**/X , _/**/X )
#define UUUPASC( X, Y ) asm(" .globl X ; X = Y ");

