c       .data
c       .asciz  "@(#)FSasins.fs 1.1 86/09/25 Copyr 1986 Sun Micro"
c       .even
c       .text

c       Copyright (c) 1986 by Sun Microsystems, Inc.

	real*4 function FSasins ( x )
	real*4 x,d
	d  = (1.0-x) * (1.0+x)
	FSasins = atan( x + x *(x*x) /( d + sqrt(d)))
	end
