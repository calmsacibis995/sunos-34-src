/*	@(#)corewinvars.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

	int _core_maskhaschanged;		/* using new input mask */
	int _core_curshaschanged;		/* using new cursor */
	struct cursor _core_oldcursor;		/* saved old cursor */
	struct inputmask _core_oldim;		/* saved old input mask */
	struct sigvec _core_sigwin, _core_sigint;	/* sigwinch, sigint */
