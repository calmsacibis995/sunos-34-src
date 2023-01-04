/*
 *	@(#)scutils.c 1.8 84/11/29 Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "structconst.h"
#include "../h/s2addrs.h"
#include "../h/s2map.h"
#include "../h/pginit.h"
#define	u_short	unsigned short
#define	u_char	unsigned char
#include "../../usr.lib/libpixrect/memreg.h"
#include "../../usr.lib/libpixrect/cg2reg.h"

#define	TEMP_PAGE ((unsigned short *)(VIDEOMEM_BASE))

struct pginit colorzap[] = {
	{VIDEOMEM_BASE, 1,		/* Video memory (if we have any) */
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				 VPM_VME0, 0, 0, 0x400000 >> BYTES_PG_SHIFT}},
/* FIXME, if you change this value you also have to change kernel/genassym.c */

	{(char *)TIMER_BASE, PGINITEND,	/* Trailer for table end */
		{1, PMP_NONE, MPM_MEMORY, 0, 0, 0}},
};

struct cg2statusreg zero_statreg = {0};

/* ======================================================================
   Author : Peter Costello & John Gilmore
   Date   : April 21, 1982 & August 6, 1984
   Purpose : This routine initializes the color board. It clears the 
	frame buffer to color 0, enables all video planes, loads the 
	default color map, and enables video.
   Error Handling: Returns -1 if no color board.
   Returns: 1 if color board config is 1024x1024, 0 if 1152x900.
	   -1 if no color board.
   ====================================================================== */

int
init_scolor()
{
	register unsigned short *cmp, *endp;
	register unsigned short *reg = TEMP_PAGE;
	register struct cg2statusreg *statreg =
		(struct cg2statusreg *)TEMP_PAGE;
	int usercontext;
	long oldpgmap;
	int result;

	/*
	 * Make sure we're modifying the supervisor's page maps;
	 * save old context and page map entry.
	 */
	usercontext = getusercontext();
	setusercontext(getsupcontext());
	oldpgmap = getpgmap(reg);

	/* 
	 * Initialize status register just in case, and check for
	 * whether the board really exists.
	 */
	setpgmap(reg, PME_COLOR_STAT);
	if (poke(reg, 0)) {
		/* Bus error on status reg access, no color board. */
		setpgmap(reg, oldpgmap);		/* Restore temp page */
		result = -1;
	} else {
		/* 
		 * Set Color Maps to default values
		 */
		setpgmap(reg, PME_COLOR_MAPS);

		cmp = (unsigned short *)reg;
		endp = cmp + 256*3;
		while (cmp < endp) {
			*cmp++ = -1;
			*cmp++ = 0;
		}

		/* Load the shadow color map into the real color map */
		setpgmap(statreg, PME_COLOR_STAT);
		statreg->update_cmap = 1;
		while ( (statreg->retrace));		/* See retrace */
		while (!(statreg->retrace));		/* End of retrace */
		while ( (statreg->retrace));		/* Start of next one */
		statreg->update_cmap = 0;
		
		setpgmap(reg, PME_COLOR_ZOOM);
		*reg = 0;
		setpgmap(reg, PME_COLOR_WPAN);
		*reg = 0;
		setpgmap(reg, PME_COLOR_PPAN);
		*reg = 0;
		setpgmap(reg, PME_COLOR_VZOOM);
		*reg = 0xFF;

		setpgmap(statreg, PME_COLOR_STAT);
		*statreg = zero_statreg;	/* Clear status register */
		statreg->video_enab = 1;	/* Enable video */
		result = 0;			/* Tell caller if 1024x1024 */
		if (statreg->resolution)
			result = 1;

		/* Map in the colorbuf memory where the b&w usually goes */
		setupmap(colorzap);
	}

	setusercontext(usercontext);		/* Restore user context */

	return result;
}
