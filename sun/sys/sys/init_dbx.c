#ifndef lint
static	char sccsid[] = "@(#)init_dbx.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * This file is compiled using the "-g" flag to generate the
 * minimum required structure information which is required by
 * dbx with the -k flag.
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../machine/pte.h"
