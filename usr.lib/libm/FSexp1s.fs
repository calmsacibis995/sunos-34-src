c	.data
c	.asciz	"@(#)FSexp1s.fs 1.1 86/09/25 Copyr 1985 Sun Micro"
c	.even
c	.text

c	Copyright (c) 1985 by Sun Microsystems, Inc.

        real function FSetwoton ( n )
        integer n
c       twoton is computes 2**n
        integer ikluge
        real rkluge
        equivalence (ikluge,rkluge)
 
        ikluge = 8388608 * (n + 127)
        FSetwoton = rkluge
        end
 
	double precision function FSexp ( x, n )
	real x
	integer n
c	computes n = nint(x * log2e)
c	computes (2**-n)*exp(x) - 1 = exp( x - n * loge(2) ) - 1
	real s1, s2, t1, t2, t3
	parameter (s1 = -0.60091983800000000e+02 )
	parameter (s2 =  0.10015315100000000e+02 )
	parameter (t1 = -0.12018396800000000e+03 )
	parameter (t2 =  0.60091983800000000e+02 )
	parameter (t3 = -0.12015315100000000e+02 )
	real xn, y 
	double precision dy
	double precision loge2a
	real loge2b
	parameter (loge2a = 0.69314 71804 85539 1383d0)
        parameter (loge2b = 7.4406  17109 80297 9273e-11)

	xn = anint( x * 1.44269 504078)
	n = xn
	dy = dble(x) - dble(xn) * loge2a
	y = real(dy)
	FSexp = dy - dble(xn * loge2b - 
     1		y * y * (s1+y*(s2-y))/(t1+y*(t2+y*(t3+y))))
	end

	real function FSexp1s(x)
	real x
c	FSexp1s computes exp(x)-1

	integer n
	double precision e, FSexp
	real p, FSetwoton
	real twom26
	parameter (twom26 = 1.490116119384765625e-8)
c	twom26 = 2**-26 = 32 80 00 00
	
	if (abs(x) .le. 87.0) then
		if (abs(x) .le. twom26) then
			FSexp1s = x
		else
			e = FSexp ( x, n )
			if (n .eq. 0) then
				FSexp1s = real(e)
			else
				p = FSetwoton(n)
				FSexp1s = real(dble(p) * e + dble(p - 1.0)) 
			endif
		endif
	else
		FSexp1s = real(exp(dble(x))-1.0d0)
	endif
	end
