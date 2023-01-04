/*	@(#)frame.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Definition of the sun stack frame
 */
struct frame {
	struct frame	*fr_savfp;	/* saved frame pointer */
	int	fr_savpc;		/* saved program counter */
	int	fr_arg[1];		/* array of arguments */
};
