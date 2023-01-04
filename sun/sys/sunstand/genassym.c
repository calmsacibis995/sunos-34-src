#ifndef lint
static  char sccsid[] = "@(#)genassym.c 1.6 87/04/02";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "../mon/sunromvec.h"
#include "../mon/keyboard.h"

main()
{
	printf("#define\tCACR_CLEAR 0x%x\n", 0x8);
	printf("#define\tCACR_ENABLE 0x%x\n", 0x1);
	printf("#define\tV_TRANSLATION 0x%x\n", &romp->v_translation);
	exit(0);
}

