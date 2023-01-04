c       .data
c       .asciz  "@(#)FSacoss.fs 1.1 86/09/25 Copyr 1986 Sun Micro"
c       .even
c       .text

c       Copyright (c) 1986 by Sun Microsystems, Inc.

	real*4 function FSacoss ( x )
	real*4 x
	FSacoss = 2.0 * atan( sqrt((1.0-x)/(1.0+x)))
	end
