	.data
|	.asciz	"@(#)fpa_handler.s 1.1 86/09/24 Copyr 1986 Sun Micro"
	.even
	.text

|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

/*
	int fpa_handler(scp) ;
	struct sig_context *scp ;

*/

RTENTRY(_fpa_handler)
	movel	PARAM,d0		| d0 gets scp.
	fmovemx	fp0/fp1,sp@-		| Save 68881 scratch registers.
	fmovel	fpcr,sp@-
	movel	d0,sp@-			| Push scp.
	jsr 	_fpa_handle
	addql	#4,sp			| Remove parameter from stack.
	fmovel	sp@+,fpcr
	fmovemx	sp@+,fp0/fp1
	RET
