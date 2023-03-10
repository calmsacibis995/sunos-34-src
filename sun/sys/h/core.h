/*	@(#)core.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef _CORE_
#define _CORE_

#ifdef KERNEL
#include "../h/exec.h"
#include "../machine/reg.h"
#else
#include <sys/exec.h>
#include <machine/reg.h>
#endif

#define CORE_MAGIC	0x080456
#define CORE_NAMELEN	16		/* Related to MAXCOMLEN in user.h */

/*
 * The size of struct fpa_regs is changed from 141 ints in 3.0 to
 * 77 ints in 3.x.  A pad of this size difference is added to struct core.
 */
#define CORE_PADLEN	64

/*
 * Format of the beginning of a `new' core file.
 * The `old' core file consisted of dumping the u area.
 * In the `new' core format, this structure is followed
 * copies of the data and  stack segments.  Finally the user
 * struct is dumped at the end of the core file for programs
 * which really need to know this kind of stuff.  The length
 * of this struct in the core file can be found in the
 * c_len field.  When struct core is changed, c_fpstatus
 * and c_fparegs should start at long word boundaries (to
 * make the floating pointing signal handler run more efficiently).
 */
struct core {
	int	c_magic;		/* Corefile magic number */
	int	c_len;			/* Sizeof (struct core) */
	struct	regs c_regs;		/* General purpose registers */
	struct 	exec c_aouthdr;		/* A.out header */
	int	c_signo;		/* Killing signal, if any */
	int	c_tsize;		/* Text size (bytes) */
	int	c_dsize;		/* Data size (bytes) */
	int	c_ssize;		/* Stack size (bytes) */
	char	c_cmdname[CORE_NAMELEN + 1]; /* Command name */
#ifdef sun
	struct 	fp_status c_fpstatus;	/* External FPP state, if any */
	struct  fpa_regs c_fparegs;	/* FPA registers, if any */
	int	c_pad[CORE_PADLEN];	/* see comment above */
#else sun
	int	c_ucode;		/* Exception no. from u_code */
#endif sun
};
#endif !_CORE_
