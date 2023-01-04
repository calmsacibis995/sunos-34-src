c       .data
c       .asciz  "@(#)Fpow10is.f 1.1 86/09/25 Copyr 1986 Sun Micro"
c       .even
c       .text

c       Copyright (c) 1986 by Sun Microsystems, Inc.

	real function Fpow10is( i )
	integer i

c	computes 10.0e0**i for 0 <= i.

	real smallpowers(0:39)
	data smallpowers/
&	1.0e0,
&	1.0e1,
&	1.0e2,
&	1.0e3,
&	1.0e4,
&	1.0e5,
&	1.0e6,
&	1.0e7,
&	1.0e8,
&	1.0e9,
&	1.0e10,
&	1.0e11,
&	1.0e12,
&	1.0e13,
&	1.0e14,
&	1.0e15,
&	1.0e16,
&	1.0e17,
&	1.0e18,
&	1.0e19,
&	1.0e20,
&	1.0e21,
&	1.0e22,
&	1.0e23,
&	1.0e24,
&	1.0e25,
&	1.0e26,
&	1.0e27,
&	1.0e28,
&	1.0e29,
&	1.0e30,
&	1.0e31,
&	1.0e32,
&	1.0e33,
&	1.0e34,
&	1.0e35,
&	1.0e36,
&	1.0e37,
&	1.0e38,
&	1.0e39/
		Fpow10is = smallpowers(i)
	end
