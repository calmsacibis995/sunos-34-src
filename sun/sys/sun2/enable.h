/*	@(#)enable.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * The System Enable register controls overall
 * operation of the system.  When the system is
 * reset, the Enable register is cleared.  The
 * enable register is addressed in FC_MAP space.
 */
#ifndef LOCORE
struct enablereg {				/* Enables: */
	unsigned			:8;
	unsigned	ena_notboot	:1;	/* Normal (non-boot) state */
	unsigned	ena_ints	:1;	/* Interrupts */
	unsigned	ena_dvma	:1;	/* Direct Virtual Mem Access */
	unsigned	ena_par_check	:1;	/* Parity checking & errs */
	unsigned	ena_soft_int_3	:1;	/* Software int on level 3 */
	unsigned	ena_soft_int_2	:1;	/* Software int on level 2 */
	unsigned	ena_soft_int_1	:1;	/* Software int on level 1 */
	unsigned	ena_par_gen	:1;	/* Parity generation enabled */
};
#endif

/*
 * Equivalent bits
 */
#define	ENA_PAR_GEN	0x01		/* enable parity generation */
#define	ENA_SOFT_INT_1	0x02		/* software interrupt on level 1 */
#define	ENA_SOFT_INT_2	0x04		/* software interrupt on level 2 */
#define	ENA_SOFT_INT_3	0x08		/* software interrupt on level 3 */
#define	ENA_PAR_CHECK	0x10		/* enable parity checking and errors */
#define	ENA_DVMA	0x20		/* enable DVMA */
#define	ENA_INTS	0x40		/* enable interrupts */
#define	ENA_NOTBOOT	0x80		/* non-boot state */

#define	ENA_NORMAL	0xF1		/* normal state */
#define	ENA_NOINTR	0xD1		/* disable all interrupts */

#define	ENABLEREG	0x0E		/* addr of enable reg in FC_MAP space */
