c       .data
c       .asciz  "@(#)FStanhs.fs 1.1 86/09/25 Copyr 1985 Sun Micro"
c       .even
c       .text

c       Copyright (c) 1985 by Sun Microsystems, Inc.

	real*4 function FStanhs ( x )
	real*4 x,t, FSexp1s, CopySign
	t = FSexp1s(-2.0 * abs(x))
	FStanhs = CopySign(-t/(2.0+t),x)
	end
