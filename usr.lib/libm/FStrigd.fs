c	static	char sccsid[] = "@(#)FStrigd.fs 1.3 86/12/29 Copyr 1986 Sun Micro";

c
c 	Copyright (c) 1986 by Sun Microsystems, Inc.
c

	real*8 function Sdcosapprox ( x, ex )
	real*8 x, ex

	real*8 c1,c2,c3,c4,c5,c6


      parameter (c1 =  0.41666666666666505d-01)
c          3fa55555    5555553e
      parameter (c2 = -0.13888888888865302d-02)
c          bf56c16c    16c14199
      parameter (c3 =  0.24801587269650016d-04)
c          3efa01a0    1971caeb
      parameter (c4 = -0.27557304623183960d-06)
c          be927e4f    1314ad1a
      parameter (c5 =  0.20873958177697780d-08)
c          3e21ee3b    60dddc8c
      parameter (c6 = -0.11250289076471312d-10)
c          bda8bd59    86b2a52e

	
	real*8 z, ca

	z = x * x
	 
	ca = z * z *

     1            (c1+z*(c2+z*(c3+z*(c4+z*(c5+z*c6)))))

 	ca = ca - ex * x
	z = 0.5d0 * z
	if (z .ge. 2.6117239648121182150d-1 	) then
		Sdcosapprox = 0.5d0 - ((z - 0.5d0) - ca)
	else
		Sdcosapprox = 1.0d0 - (z - ca)
	endif
c	signal inexact
	end

	real*8 function Sdsinapprox ( x, ex )
	real*8 x, ex

	real*8 s1,s2,s3,s4,s5,s6

      parameter (s1 = -0.16666666666666463d+00)
c          bfc55555    5555550c
      parameter (s2 =  0.83333333332992771d-02)
c          3f811111    1110c461
      parameter (s3 = -0.19841269816180999d-03)
c          bf2a01a0    19746345
      parameter (s4 =  0.27557309793219877d-05)
c          3ec71de3    209cdcd9
      parameter (s5 = -0.25050225177523807d-07)
c          be5ae5c0    e319a4ef
      parameter (s6 =  0.15868926979889205d-09)
c          3de5cf61    df672b13

	real*8 z

	z = x * x
        Sdsinapprox = x + (ex + x * z *

     1            (s1+z*(s2+z*(s3+z*(s4+z*(s5+z*s6))))))

c	signal inexact
	end

	real*8 function Sdtanapprox ( x, ex )
	real*8 x, ex

	real*8 p1,p2,p3,q1,q2,q3,q4

      parameter (p1 = -0.66908293935034808d+06)
c          c1246b35    e0f28761
      parameter (p2 =  0.44639094993782339d+05)
c          40e5cbe3     a30668f
      parameter (p3 = -0.59003803970255422d+03)
c          c082704d    e7c2735c
      parameter (q1 = -0.20072488180510444d+07)
c          c13ea0d0    d16bcb12
      parameter (q2 =  0.93681681220176432d+06)
c          412c96e1    9fd8e8df
      parameter (q3 = -0.51513697029638897d+05)
c          c0e92736    4e1119ed
      parameter (q4 =  0.62589093065366808d+03)
c          40838f20    a0402411

	real*8 z

	z = x * x

 	Sdtanapprox = x + (ex + x * z * 
     1		(p1+z*(p2+z*(p3+z)))/(q1+z*(q2+z*(q3+z*(q4-z)))))

c	signal inexact
	end

	real*8 function Sdcotanapprox ( x, ex )
	real*8 x, ex

	real*8 p1,p2,p3,q1,q2,q3,q4

      parameter (p1 = -0.66908293935034808d+06)
c          c1246b35    e0f28761
      parameter (p2 =  0.44639094993782339d+05)
c          40e5cbe3     a30668f
      parameter (p3 = -0.59003803970255422d+03)
c          c082704d    e7c2735c
      parameter (q1 = -0.20072488180510444d+07)
c          c13ea0d0    d16bcb12
      parameter (q2 =  0.93681681220176432d+06)
c          412c96e1    9fd8e8df
      parameter (q3 = -0.51513697029638897d+05)
c          c0e92736    4e1119ed
      parameter (q4 =  0.62589093065366808d+03)
c          40838f20    a0402411

c	if the approximation for tan is of the form
c		tan = x + (ex + x * z * p(z)/q(z))
c	then the approximation for cotan is of the form
c	      cotan = q(z)/{ x*q(z) + (ex * q(z) + x * z * p(z))}


	real*8 z, qq

	z = x * x
	qq = q1+z*(q2+z*(q3+z*(q4-z)))
	Sdcotanapprox = qq / ( x*qq   + (ex * qq + x * z * (p1+z*(p2+z*(p3+z)))))

c	signal inexact
	end

	subroutine Sdargred ( x, rx, ex, quad )
	real*8 x, rx, ex
	integer quad

c	Sdargred returns rx+ex = x - (4n+quad) * (pi/2)
c	argument reduction is performed using a 66 bit approximation
c	to pi to mimic the 68881

c	quad is in the range 0..3

	doubleprecision dx, dex, dx0, q, dkluge
	integer ikluge(2)
	equivalence (dkluge, ikluge)

	quad = 0
	if (.not.(abs(x) .le. 1.797693134862315708d308)) then
c		handle infinity and nan
		rx = x-x
		ex = 0.0d0
		return
	endif
	dx = dble(x)
	dex = 0.0d0
 1	continue
	q = dx * 	6.366197723675813824d-1		
	if (abs(q) .gt. 1.048576d6) then
		dkluge = q
		ikluge(1) = ikluge(1) + 2
		ikluge(1) = and(ikluge(1),not(3))
		ikluge(2) = 0
		q = dkluge
		endif
	q = anint(q)
	if (abs(q) .lt. 1.048576d6) then
		quad = quad + mod(int(q),4)
	endif
	dx0 = dx - q * 	1.570796326734125614d0		
	dx =   (dx0 - q * 	6.077100506303965976d-11 	) + dex
	dex = ((dx0 - dx) - q * 	6.077100506303965976d-11 	) + dex
	if (abs(dx) .gt. 7.8539816339744839d-1) then
		goto 1
	endif
	rx = dble(dx)
	ex = dble((dx - dble(rx))+dex)
	if (quad .lt. 0) quad = quad + 4
	end	

	real*8 function FSsind ( xx )
	real*8 xx

	real*8 x, Sdsinapprox, Sdcosapprox
	real*8 ex
	integer quad

	if (abs(xx) .le. 	7.853981633974482790d-1		) then
                if (abs(xx) .le. 3.725290298461914062d-9	) then
                        FSsind = xx
                        return
                else
			x = xx
       			ex = 0.0d0
			goto 10
		endif
	endif
	call Sdargred( xx, x, ex, quad)
        goto (10,11,12,13) quad+1
 10	continue
        FSsind =   Sdsinapprox ( x, ex )
        return
 11	continue
	FSsind =   Sdcosapprox ( x, ex )
	return
 12	continue
	FSsind =  -Sdsinapprox ( x, ex )
	return
 13	continue
	FSsind =  -Sdcosapprox ( x, ex )
	return
	end	

	real*8 function FScosd ( xx )
	real*8 xx

	real*8 x, Sdsinapprox, Sdcosapprox
	real*8 ex
	integer quad

	if (abs(xx) .le. 	7.853981633974482790d-1		) then
                if (abs(xx) .le. 3.725290298461914062d-9	) then
                        FScosd = 1.0d0
                        return
                else
			x = xx
       			ex = 0.0d0
			goto 10
		endif
	endif
	call Sdargred( xx, x, ex, quad)
        goto (10,11,12,13) quad+1
 10	continue
        FScosd =   Sdcosapprox ( x, ex )
        return
 11	continue
	FScosd =  -Sdsinapprox ( x, ex )
	return
 12	continue
	FScosd =  -Sdcosapprox ( x, ex )
	return
 13	continue
	FScosd =   Sdsinapprox ( x, ex )
	return
	end	

	real*8 function FStand ( xx )
	real*8 xx

	real*8 x, Sdtanapprox, Sdcotanapprox
	real*8 ex
	integer quad

	if (abs(xx) .le. 	7.853981633974482790d-1		) then
                if (abs(xx) .le. 3.725290298461914062d-9	) then
                        FStand = xx
                        return
                else
			x = xx
       			ex = 0.0d0
			goto 10
		endif
	endif
	call Sdargred( xx, x, ex, quad)
        goto (10,11,12,13) quad+1
 10	continue
        FStand =    Sdtanapprox ( x, ex )
        return
 11	continue
	FStand = -Sdcotanapprox ( x, ex )
	return
 12	continue
	FStand =    Sdtanapprox ( x, ex )
	return
 13	continue
	FStand = -Sdcotanapprox ( x, ex )
	return
	end	
