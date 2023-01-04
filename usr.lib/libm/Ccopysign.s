 	.data
|	.asciz	"@(#)Ccopysign.s 1.1 86/09/25 Copyr 1986 Sun Micro"
 	.even
 	.text
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

	ENTRY(copysign)
       moveml   PARAM,d0/d1 		| d0/d1 := x.
       lea   	PARAM3,a0               | Address of y.
       tstb    	a0@
       bmis    	1f
       bclr    	#31,d0
       bras    	2f
1:
        bset    #31,d0
2:
       RET


