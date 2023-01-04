c       .data
c       .asciz  "@(#)FSsinhs.fs 1.1 86/09/25 Copyr 1986 Sun Micro"
c       .even
c       .text

c       Copyright (c) 1986 by Sun Microsystems, Inc.

	real*4 function FSsinhs ( x )
	real*4 x,t, FSexp1s, FFscales, CopySign
	real*4 lnmaxa, lnmaxb
	parameter (lnmaxa = 8.872283935546875000e+1)
        parameter (lnmaxb = -2.437957391521194950e-7)
	if (abs(x) .lt. lnmaxa) then
		t = FSexp1s(abs(x))
		t = 0.5 * (t + t / (t + 1.0) )
	else
		t = exp((abs(x)-lnmaxa)-lnmaxb)
		t = FFscales(t,127)
	endif
	FSsinhs = CopySign( t, x )
	end

	real*4 function FScoshs ( x )
	real*4 x,t, FFscales
	real*4 lnmaxa, lnmaxb
	parameter (lnmaxa = 8.872283935546875000e+1)
        parameter (lnmaxb = -2.437957391521194950e-7)
	if (abs(x) .lt. lnmaxa) then
		t = exp(abs(x))
		FScoshs = 0.5 * (t + 1.0 / t )
	else
		t = exp((abs(x)-lnmaxa)-lnmaxb)
		FScoshs = FFscales(t,127)
	endif
	end
