c       .data
c       .asciz  "@(#)FSlog2s.fs 1.1 86/09/25 Copyr 1985 Sun Micro"
c       .even
c       .text

c       Copyright (c) 1985 by Sun Microsystems, Inc.

	real*4 function FSlog2s ( x )
	real*4 x
	FSlog2s = log( x ) / 0.69314 71805 59945 30941 72321
	end
