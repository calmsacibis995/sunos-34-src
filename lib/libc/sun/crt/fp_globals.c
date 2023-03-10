#ifdef sccsid
static	char sccsid[] = "@(#)fp_globals.c 1.1 86/09/24 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "fpcrtdefs.h"
#include "fpcrttypes.h"

/*			C GLOBAL INITIALIZATION		*/

enum fp_switch_type 	/* Current floating point variety. */
	fp_switch		= fp_unspecified ; 

enum fp_state_type	/* State of various floating point units. */
	fp_state_software = fp_unknown,	/* Floating point software. */
	fp_state_skyffp = fp_unknown,	/* Sky FFP. */
	fp_state_mc68881 = fp_unknown,	/* Motorola 68881. */	
	fp_state_sunfpa = fp_unknown;	/* Sun FPA. */

char *			
	_skybase = 0 ; 		/* sky access address */ 
				/* 0 if fp_state_skyffp != fp_enabled */ 
			        /* 1 if fp_state_skyffp == fp_enabled */

int
	Fmode = ROUNDTODOUBLE,	/* Software floating point modes. */
	Fstatus = 0 ;		/* Software floating point status. */
