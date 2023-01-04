c       .data
c       .asciz  "@(#)FSpow2s.fs 1.1 86/09/25 Copyr 1985 Sun Micro"
c       .even
c       .text

c       Copyright (c) 1985 by Sun Microsystems, Inc.

	real*4 function FSpow2s ( x )
	real*4 x,six,z,FFscales
	integer ix
	if (abs(x) .lt. 256.0) then 
	ix = nint(x)
	six = ix
	if (six .eq. x) then
		z = 1.0
	else
		z = exp( (x-six) * 0.6931471805599453094172321)
	endif
	z = FFscales ( z, ix )
	else
	z = exp( x * 0.6931471805599453094172321 )
	endif
	FSpow2s = z
	end
