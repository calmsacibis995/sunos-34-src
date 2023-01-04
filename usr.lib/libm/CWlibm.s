	.data
	.asciz	"@(#)CWlibm.s 1.1 86/09/25 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

RTENTRY(_CWexpd)
|	fpetoxd	PARAM,fpa0
|	fpmoved	fpa0,d0:d1
	RET

RTENTRY(_CWlogd)
|	fplognd	PARAM,fpa0
|	fpmoved	fpa0,d0:d1
	RET

RTENTRY(_CWatand)
|	fpatand	PARAM,fpa0
|	fpmoved	fpa0,d0:d1
	RET

