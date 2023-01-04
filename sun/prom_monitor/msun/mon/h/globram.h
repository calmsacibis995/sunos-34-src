/*	@(#)globram.h 2.23 84/08/15 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * globram.h
 *
 * Sun ROM Monitor global data
 *
 * This defines a data structure that lives in low RAM and is
 * used by the ROM monitor for its own purposes.
 *
 * Elements of the structure are accessed by referring to "gp->xxx".
 * "gp" is defined as a small integer coerced to a struct pointer.
 * The compiler and assembler generate short absolute addresses from this.
 */
#ifndef GLOBRAM
#define	GLOBRAM

/* FIXME, this whole nested include stuff needs cleanup. */
#include "sunmon.h"
#include "buserr.h"
#include "asyncbuf.h"
#include "dpy.h"
#include "s2map.h"
#include "diag.h"
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include "enable.h"

/* Define "keybuf_t" for asynchronous bufferring between nmi routine
   and keyboard handler. */
DEFBUF (keybuf_t, unsigned char, KEYBUFSIZE);

struct globram {

/* Bus error information */
	struct BusErrInfo	g_bestack;	/* structured data from 68010 */
	long			g_beregs[17];	/* Regs & usp from last b.e. */
	struct buserrorreg	g_because;	/* What caused the bus error? */

/* Fake copies of hardware registers that can't be read or written from C */
/* Each one has a corrsponding routine for setting the real hardware reg. */
	struct enablereg	g_enable;	/* Sys Enable Reg, set_enable */
	unsigned char		g_leds;		/* LED register, set_leds */

/* Miscellaneous services */
	int			g_nmiclock;	/* pseudo-clock counter (msec, 
					 	   bumped by nmi routine) */
	int			g_memorysize;	/* size of memory in bytes */
	long			*g_busbuf;	/* setbus() save area ptr */
	struct diag_state	g_diag_state;	/* Info from powerup diags */

/* Command line input for monitor */
	unsigned char		g_linebuf[BUFSIZE+2];	/* data input buffer */
	unsigned char		*g_lineptr;	/* next "unseen" char */
	int			g_linesize;	/* size of input line */

/* keyboard handling */
	struct zscc_device	*g_keybzscc;	/* UART address */
	unsigned char		g_key;		/* Last up/down code read */
	unsigned char		g_prevkey;	/* The previous reading */
	unsigned char		g_keystate;	/* State machine */
	unsigned char		g_keybid;	/* Id byte supplied */
	struct keybuf_t		g_keybuf[1];	/* Raw up/down code buf */
	unsigned int		g_shiftmask;	/* "shift-like" keys' posns */
	unsigned int		g_translation;	/* Current translation */
	int			g_keyrinit;	/* Rpt timeout (eg 500 ms) */
	int			g_keyrtime;	/* Time of next repeat */
	unsigned char		g_keyrtick;	/* 2nd rpt timeout (eg 30 ms) */
	unsigned char		g_keyrkey; 	/* keycode to rpt, or IDLEKEY */

/* Console I/O vectoring and such */
	/* Possible values for g_insource and g_outsink defined in sunmon.h */
	struct zscc_device	*g_inzscc;	/* Input UART address */
	struct zscc_device	*g_outzscc;	/* Output UART address */
	unsigned char		g_insource;	/* Current input source */
	unsigned char		g_outsink;	/* Current output sink */
	unsigned char		g_debounce;	/* used to debounce BREAK key */
	unsigned char		g_echo;		/* input should be echoed? */
	unsigned char		g_fbthere;	/* frame buffer plugged in? */

/* Watchdog resets */
	long			g_resetaddr;	/* watchdog jump address */
	long			g_resetaddrcomp;	/* ~g_resetaddr */
	long			g_resetmap;	/* Map entry for g_resetaddr */
	/* g_resetmap is really (struct pgmapent) but you can't do ~ on them. */
	long			g_resetmapcomp;	/* ~g_resetmap */

/* Trace and breakpoint */
	short			g_breakval;	/* instruc rplcd by bkpt */
	short			*g_breakaddr;	/* address of current bkpt */
	int			g_breaking;	/* we are resuming at bkpt? */
	long			g_tracevec;	/* User's value for Tr ivec */
	unsigned char		*g_tracecmds;	/* string of commands */
	long			g_breakvec;	/* User's value for T1 ivec */
	long			g_breaktrvec;	/* User's value for Tr ivec */

/* Terminal emulator */
	struct pixrect		g_fbpr;		/* pixrect for frame buffer */
	struct mpr_data		g_fbdata;	/* mem_raster	''	*/
	struct pr_prpos		g_fbpos;	/* Cur posn in frame buffer */
	struct pixrect		g_charpr;	/* pixrect for char to paint */
	struct mpr_data		g_chardata;	/* mem_raster 	''	*/
	struct pr_prpos		g_charpos;	/* Posit info	''	*/
	unsigned short		(*g_font)[CHRSHORTS-1];	/* Font table address */
	int			g_ax;		/* Alpha cursor x posn, chars */
	int			g_ay;		/* Alpha cursor y posn, chars */
	unsigned short		*g_dcax;
	unsigned short		*g_dcay; 	/* '' in device coordinates */
	short			g_dcok;		/* device coords current? */
	int			g_escape;	/* next char an escape cmd? */
	int			g_chrfunc;	/* POX func to draw chars */
	int			g_fillfunc;	/* POX func to clear areas */
	int			g_state;	/* ALPHA, SKIPPING, etc */
	int			g_cursor;	/* NOCURSOR or BLOCKCURSOR */
	int			g_ac;		/* last arg in ESCBRKT seq's */
	int			g_ac0;		/* ditto, next-to-last arg */
	int			g_acinit;	/* g_ac with 0 left alone */
	int			g_scrlins;	/* # lines to scroll */
	int			g_gfxmemsize;	/* g_memorysize after shadow */
	int			g_gfxmemsizecomp;	/* ~g_gfxmemsize */
	unsigned 		g_scrwidth;	/* Total width in pixels */
	unsigned		g_scrheight;	/* Total height in pixels */
	unsigned		g_wintop;	/* Top   margin of screen */
	unsigned		g_winleft;	/* Left  margin of screen */
	int			g_fbtype;	/* Type, see <sun/fbio.h> */

/* 4014 vector drawing globals */
	int			g_gx;
	int			g_gy;		/* graph mode coordinates */
	int			g_gxhi;
	int			g_gxlo;		/* graph mode 5-bit registers */
	int			g_gyhi;
	int			g_gylo;		/* ditto */
	int			g_vectfunc; 	/* POX func to draw vectors */
	int			g_pendown;	/* vector "pen" is writing?*/
	int			g_yloseen;	/* 0=xlo, 1=ylo seen last */
	short			g_bresw[BRESIZ];	/* Bresenham code */

};

#define				gp	((struct globram *)STRTDATA)

#endif
