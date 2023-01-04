c       .data
c       .asciz  "@(#)FSpow2d.fs 1.1 86/09/25 Copyr 1985 Sun Micro"
c       .even
c       .text

c       Copyright (c) 1985 by Sun Microsystems, Inc.

	real*8 function FSpow2d ( x )
	real*8 x,dix,z,FFscaled
	integer ix
	if (abs(x) .lt. 2048.0d0) then 
	ix = nint(x)
	dix = ix
	if (dix .eq. x) then
		z = 1.0d0
	else
		z = exp( (x-dix) * 0.6931471805599453094172321d0)
	endif
	z = FFscaled ( z, ix )
	else
	z = exp( x * 0.6931471805599453094172321d0 )
	endif
	FSpow2d = z
	end
