/*	@(#)enable.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * The System Enable register controls overall
 * operation of the Sun-2 CPU subsystem.  When the system is
 * externally reset (other than by a RESET
 * instruction), the Enable register is cleared,
 * putting us in boot state, disabling interrupts,
 * etc.
 *
 * The enable register is addressed in FC_MAP space.
 */

/*
 * The bits in the enable register...
 */
#define	ENA_PAR_GEN	0x01		/* enable parity generation */
#define	ENA_SOFT_INT_1	0x02		/* software interrupt on level 1 */
#define	ENA_SOFT_INT_2	0x04		/* software interrupt on level 2 */
#define	ENA_SOFT_INT_3	0x08		/* software interrupt on level 3 */
#define	ENA_PAR_CHECK	0x10		/* enable parity checking and errors */
#define	ENA_SDVMA	0x20		/* enable DVMA */
#define	ENA_INTS	0x40		/* enable interrupts */
#define	ENA_NOTBOOT	0x80		/* non-boot state */

/*
 * Where to find it
 */
#define	ENABLEREG	0x0E		/* addr of enable reg in FC_MAP space */
