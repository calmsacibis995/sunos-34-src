/*
 * @(#)banner.c 1.16 84/11/29 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * banner.c
 *
 * Prints banner at power-up or for "kb" command.
 */

#include "../h/sunmon.h"
#include "../h/globram.h"
#include "../h/diag.h"
#include "../h/idprom.h"
#include "../h/s2addrs.h"
#include "../h/video.h"

#ifdef S2COLOR
#include "../../sys/sun/fbio.h"
#endif S2COLOR

extern char monrev[];
int	cpudelay = 5;

/*
 * Table of error messages for damage control bits in diag_state.
 */
struct damage_table {
	short	dt_bit;
	char	*dt_message;
} damage_table[] = {
	CXBIT, ", context registers",
	SEGBIT, ", segment map",
	PMBIT, ", page map",
	MEMBIT, ", memory",
	MCONBIT, " (constant data)",
	MADDRBIT, " (address independence)",
	MTRANBIT, " (transient error)",
	DESBIT, ", DES chip",
	0, 0,
};


banner()
{
	register struct damage_table *dammsg;
	struct idprom id;
	unsigned megs;

	/* Check whether diagnostics completed properly or not */
	if (gp->g_diag_state.ds_message == 0 &&
	    gp->g_diag_state.ds_errcount == 0 &&
	    gp->g_diag_state.ds_tranerrs == 0
	    ) {
		printf("Self Test completed successfully.\n\n");
	} else {
		printf("Self Test found a problem");
		if (gp->g_diag_state.ds_message == 0) {
			printf(".\n");
		} else {
			printf(" in %s\n\n", gp->g_diag_state.ds_message);
			printf("Wrote %x at address %x, but read %x.\n",
				gp->g_diag_state.ds_wrote,
				gp->g_diag_state.ds_addr,
				gp->g_diag_state.ds_read);
		}
		printf("\n%d total error", gp->g_diag_state.ds_errcount);
		if (gp->g_diag_state.ds_errcount != 1)
		putchar ('s');
		if (gp->g_diag_state.ds_tranerrs != 0)
			printf(" (%d transient)", gp->g_diag_state.ds_tranerrs);
		printf(".\nDamage found");

		for (dammsg = damage_table;
		     dammsg->dt_message;
		     dammsg++) {
			if (! ((gp->g_diag_state.ds_damages >> dammsg->dt_bit)
				& 1)) {
				printf(dammsg->dt_message);
			}
		}

		printf("\n\n\
--> Give the above information to your service-person.\n\n");
	}

	if (IDFORM_1 != idprom(IDFORM_1, &id) ||
	    id.id_machine != 
#ifdef VME
			     IDM_CPU_VME
#else  VME
			     IDM_CPU_MULTI
#endif VME
					  ) {
		printf("ID PROM INVALID\n\n");
	}

#ifdef LOGO
	if (gp->g_outsink == OUTSCREEN) printf("\t");
#endif LOGO

#ifdef KEYBVT
printf ("Sun Workstation, Model Sun-1/100U or Sun-1/150U, VT100 keyboard\n");
#else
#ifdef KEYBKL
printf ("Sun Workstation, Model Sun-1/100U or Sun-1/150U, Two-tone keyboard\n");
#else
#ifdef KEYBS2
#ifdef VME
#ifdef S2COLOR
printf ("Sun Workstation, Model %sSun-2/160, Sun-2 keyboard\n",
	gp->g_fbtype == FBTYPE_SUN2COLOR? "": "Sun-2/50 or ");
#else  S2COLOR
#ifdef FB1024
printf ("Sun Workstation, Model Sun-2/50 (Square), Sun-2 keyboard\n");
#else  FB1024
printf ("Sun Workstation, Model Sun-2/50, Sun-2 keyboard\n");
#endif FB1024
#endif S2COLOR
#else  VME
#ifdef FB1024
printf ("Sun Workstation, Model Sun-2/120 or Sun-2/170 (Square), Sun-2 keyboard\n");
#else  FB1024
#ifdef S2FRAMEBUF
printf ("Sun Workstation, Model Sun-2/120 or Sun-2/170, Sun-2 keyboard\n");
#else  S2FRAMEBUF
printf ("Sun Workstation, Model Sun-2/110, Sun-2 keyboard\n");
#endif S2FRAMEBUF
#endif FB1024
#endif VME
#else
	.?> Keyboard Not Specified KEYBKL or KEYBVT or KEYBS2.
#endif
#endif
#endif

#ifdef LOGO
	if (gp->g_outsink == OUTSCREEN) printf("\t");
#endif LOGO
	printf("ROM %s, ",  monrev);
	megs = (unsigned)(gp->g_memorysize)/(1024*1024);
	if (megs * (1024*1024) == gp->g_memorysize)
		printf ("%dM", megs);
	else
		printf ("%dK", (unsigned)(gp->g_memorysize)/1024);
	printf ("B memory installed\n");

#ifdef LOGO
	if (gp->g_outsink == OUTSCREEN) printf("\t");
#endif LOGO
	printf("Serial #%d, Ethernet address %x:%x:%x:%x:%x:%x\n",
		id.id_serial,
		id.id_ether[0], id.id_ether[1], id.id_ether[2],
		id.id_ether[3], id.id_ether[4], id.id_ether[5]);

#ifdef LOGO
	/*
	 * Now back up a few lines and produce the logo.
	 */
	if (gp->g_outsink == OUTSCREEN && gp->g_ay >= 3) sunlogo(gp->g_ay - 3);
	printf("\n");
#endif LOGO

	showconfig();
}


#ifdef DIAGLOOP
/*
 * Check the loopback configuration to see whether we are in the
 * manufacturing burnin area.  Return a result indicating what action
 * the mainline reset code should take:
 *
 *	0	We are a normal power-up, proceed with Unix boot.
 *	1	This is Manufacturing, but no errors were found.
 *		Loop back into the diagnostics.
 *	2	This is Manufacturing, and errors were found.
 *		Delay a moment so they can read the screen if they
 *		want, then loop back into the diagnostics.
 *
#ifdef notdef
 * We define ourselves to be in manufacturing loopback if:
 *	RTS on port A is connected to CTS on port B, and
 *	RTS on port B is connected to CTS on port A
 * I fervently hope that no customer wants to wire up their system
 * this way...
#else notdef
 * FIXME:  For now we just use a jumper from the video jumpers.
 * This is easy to do and not too bad for mfg for the first 50 units.
#endif notdef
 */
int
diagloop()
{
#ifdef notdef
	register char bits;

	/*
	 * We depend on initialization to set RTS on both ports.
	 * So skip out totally if we don't see it coming in.
	 */
	bits = (SERIAL0_BASE+1)->zscc_control;	/* Read and toss */
	bits = (SERIAL0_BASE+1)->zscc_control;	/* Check the A-side */
	if ((bits & ZSRR0_CTS) == 0)
		return 0;
	bits = (SERIAL0_BASE+0)->zscc_control;	/* Read and toss */
	bits = (SERIAL0_BASE+0)->zscc_control;	/* Check the B side */
	if ((bits & ZSRR0_CTS) == 0)
		return 0;

	(SERIAL0_BASE+1)->zscc_control = 5;

	gp->g_inzscc = SERIAL0_BASE;
	gp->g_outzscc = SERIAL0_BASE;
	if (gp->g_outzscc[2-gp->g_outsink].zscc_control & ZSRR0_TX_READY) {
		gp->g_outzscc[2-gp->g_outsink].zscc_data = x;
		return 0;
	}
#else notdef
	/* KISS, FIXME later for uart loopback */
	if (VIDEOCTL_BASE->vc_b_jumper) {
		if (gp->g_diag_state.ds_message == 0 &&
		    gp->g_diag_state.ds_errcount == 0 &&
		    gp->g_diag_state.ds_tranerrs == 0
		    ) {
			return 1;		/* Mfg, no errs */
		} else {
			return 2;		/* Mfg, errors */
		}	
	}
	return 0;			/* Production use */
#endif notdef
}
#endif DIAGLOOP
