#ifndef lint
static	char sccsid[] = "@(#)sc_conf.c 1.4 86/10/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "si.h"
#include "sc.h"
#include "sd.h"
#include "st.h"
#include "sf.h"

#if ((NSC > 0) || (NSI > 0))

#include "../h/types.h"
#include "../h/buf.h"
#include "../sun/dklabel.h"
#include "../sun/dkio.h"
#include "../sundev/screg.h"
#include "../sundev/sireg.h"
#include "../sundev/scsi.h"

/* generic scsi debug flag */
int scsi_debug = 0;

/* scsi disconnect/reconnect global enable flag */
int scsi_disre_enable = 1;

/*
 * If disks exist, declare unit structures for them.
 */
#if NSD > 0
struct	scsi_unit sdunits[NSD];
struct scsi_disk sdisk[NSD];
int nsdisk = NSD;
#endif NSD

/*
 * Same thing for tapes.
 */
#if NST > 0
struct	scsi_unit stunits[NST];
struct scsi_tape stape[NST];
int nstape = NST;
#endif NST

/*
 * Same thing for floppy disks.
 */
#if NSF > 0
struct	scsi_unit sfunits[NSF];
struct scsi_floppy sfloppy[NSF];
int nsfloppy = NSF;
#endif NSF

struct	mb_device *sdinfo[NSD + NST + NSF];


/*
 * Device specific subroutines.
 * Indexed by "flag" from the configuration file,
 * which is in mc_flag.
 * Disk is 0, tape is 1.
 */
#if NSD > 0
int	sdattach(), sdstart(), sdmkcdb(), sdintr(), sdunitptr();
#endif NSD > 0

#if NST > 0
int	stattach(), ststart(), stmkcdb(), stintr(), stunitptr();
#endif NST > 0

#if NSF > 0
int	sfattach(), sfstart(), sfmkcdb(), sfintr(), sfunitptr();
#endif NSF > 0

struct	scsi_unit_subr scsi_unit_subr[] = {
#if NSD > 0
	{ sdattach, sdstart, sdmkcdb, sdintr, sdunitptr, "sd", },
#else
	{ (int (*)())0, (int (*)())0, (int (*)())0, (int (*)())0, (int (*)())0,
	(char *)0},
#endif NSD > 0

#if NST > 0
	{ stattach, ststart, stmkcdb, stintr, stunitptr, "st", },
#else
	{ (int (*)())0, (int (*)())0, (int (*)())0, (int (*)())0, (int (*)())0,
	(char *)0},
#endif NST > 0

#if NSF > 0
	{ sfattach, sfstart, sfmkcdb, sfintr, sfunitptr, "sf", },
#else
	{ (int (*)())0, (int (*)())0, (int (*)())0, (int (*)())0, (int (*)())0,
	(char *)0},
#endif NSF > 0
};

/*
 * Number of SCSI device driver types.
 */
int scsi_ntype = sizeof scsi_unit_subr / sizeof (struct scsi_unit_subr);

#endif ((NSC > 0) || (NSI > 0))
