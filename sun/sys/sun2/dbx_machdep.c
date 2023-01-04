#ifndef lint
static	char sccsid[] = "@(#)dbx_machdep.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * This file is optionally brought in by including a
 * "psuedo-device dbx" line in the config file.  It is
 * compiled using the "-g" flag to generate structure
 * information which is used by dbx with the -k flag.
 */

#include "../h/param.h"

#include "../sun2/buserr.h"
#include "../sun2/clock.h"
#include "../sun2/mmu.h"
#include "../sun2/cpu.h"
#include "../sun2/enable.h"
#include "../sun2/frame.h"
