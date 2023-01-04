c	.data
c	.asciz	"@(#)FSlog1s.fs 1.1 86/09/25 Copyr 1986 Sun Micro"
c	.even
c	.text

c	Copyright (c) 1986 by Sun Microsystems, Inc.

	real function FSlogapprox (x)
	real x
c	FSlogapprox computes loge(1+x)
c	for 2**-0.5 <= x <= 2**+0.5
	real s1, s2, s3
	parameter ( s1 = 0.66666948800000000e+00 )
	parameter ( s2 = 0.39965793500000000e+00 )
	parameter ( s3 = 0.30100327700000000e+00 )
	real z, z2

	z = x/(2.0+x)
	z2 = z * z
	FSlogapprox = x - z * ( x - z2 * ( s1 + z2 * (s2  + z2 * s3)))
	end

	real function FSlog1s(x)
	real x
c	FSlog1s computes loge(1+x)

	real FSlogapprox
	real twom25, sqrthalfm1, sqrt2m1, sqrteighthm1
	parameter (twom25     = 2.980232238769531250e-8)
c	twom25 = 2**-25 = 33 00 00 00
	parameter (sqrteighthm1 = -6.464465856552124023e-1)
	parameter (sqrthalfm1 = -2.928932309150695801e-1)
	parameter (sqrt2m1    =  4.142135679721832275e-1)
	real loge2a, loge2b
	parameter (loge2a =  6.931457519531250000e-1)
	parameter (loge2b =  1.428606765330187045e-6)

	if (abs(x) .lt. twom25) then
		FSlog1s = x
	else if (x .gt. 0.0) then
		if (x .le. sqrt2m1) then
			FSlog1s = FSlogapprox(x)
		else
			FSlog1s = log(1.0+x)
		endif
	else
		if (x .ge. sqrthalfm1) then
			FSlog1s = FSlogapprox(x)
		else if (x .ge. sqrteighthm1) then
			FSlog1s = (FSlogapprox(2.0*(x+0.5)) - loge2b) - loge2a
		else
			FSlog1s = log(1.0+x)
		endif
	endif
	end
