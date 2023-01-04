/*      @(#)diag.h 1.1 86/09/25 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * The diagnostic register drvies an 8-bit LED display.
 * This register is addressed in FC_MAP space and is write only.
 * A "0" bit written will cause the corresponding LED to light up,
 * a "1" bit to be dark.
 */

#define	DIAGREG		0x70000000	/* addr of diag reg in FC_MAP space */
