/*	@(#)fault.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Where to go on fault in kernel mode
 * Zero means fault was unexpected
 */
label_t	*nofault;	/* longjmp vector */
