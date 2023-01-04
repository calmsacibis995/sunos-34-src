c       .data
c       .asciz  "@(#)FScoshs.fs 1.1 86/09/25 Copyr 1985 Sun Micro"
c       .even
c       .text

c       Copyright (c) 1985 by Sun Microsystems, Inc.

	real*4 function FScoshs ( x )
	real*4 x,t
		t = exp(x)
		FScoshs = 0.5*t+0.5/t
	end
