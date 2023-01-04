/* @(#)sdreg.h	1.1 87/02/24	Copyr 1987 Sun Micro */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * SCSI disk controller specific stuff.
 */

/*
 * Error code fields for MD21 extended sense struct.
 * Not exhaustive - just those we treat specially.
 */

#define	SC_ERR_READ_RECOV_RETRIES 0x17	/* ctlr corrected read error w/retry */
#define	SC_ERR_READ_RECOV_ECC     0x18	/* ctlr corrected read error w/ECC */
