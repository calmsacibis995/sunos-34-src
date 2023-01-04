/*	@(#)globram.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
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
#include "../../sys/h/types.h"
#include "sunromvec.h"
#include "sunmon.h"
#include "buserr.h"
#ifdef SUN2
#include "../sun2/cpu.buserr.h"
#endif SUN2
#ifdef SUN3
#include "../../sys/sun3/buserr.h"
#endif SUN3
#include "asyncbuf.h"
#include "dpy.h"
#include "cpu.map.h"
#include "diag.h"
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>

/* Define "keybuf_t" for asynchronous bufferring between nmi routine
   and keyboard handler. */
DEFBUF (keybuf_t, unsigned char, KEYBUFSIZE);

struct globram {

/* Bus error information */
#ifdef M68020
	struct buserr_stack_20_common g_bestack;/* Stacked data from 68020 */
	struct buserr_stack_20_extension g_beext;/* More data from 68020 */
#else  M68020
	struct buserr_stack_10	g_bestack;	/* Stacked data from 68010 */
#endif M68020
	long			g_beregs[17];	/* Regs & usp from last b.e. */
	struct buserrorreg	g_because;	/* What caused the bus error? */

/* Fake copies of hardware registers that can't be read or written from C */
	unsigned char		g_leds;		/* LED register, set_leds */

/* Miscellaneous services */
	int			g_nmiclock;	/* pseudo-clock counter (msec, 
					 	   bumped by nmi routine) */
	unsigned		g_memorysize;	/* Phys size of mem in bytes */
	unsigned		g_memoryavail;	/* Memory avail to Unix, not
						   including Monitor pages */
	unsigned char		*g_memorybitmap;/* 0 or Ptr to pageavail map */
	unsigned		g_memoryworking;/* Memory that works, including
						   Monitor pages */
	char			*g_nextrawvirt;	/* Next for resalloc() */
	char			*g_nextdmaaddr;	/* Next for resalloc() */
	struct pgmapent		g_nextmainmap;	/* Next physmem to resalloc() */
	long			*g_busbuf;	/* setbus() save area ptr */
	struct diag_state	g_diag_state;	/* Info from powerup diags */
	void			(*g_vector_cmd)(); /* Addr of vector cmd */

/* Command line input for monitor */
	unsigned char		g_linebuf[BUFSIZE+2];	/* data input buffer */
	unsigned char		*g_lineptr;	/* next "unseen" char */
	int			g_linesize;	/* size of input line */
	struct bootparam	g_bootparam;	/* Bootstrap parameter block */

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
	/* Values for g_insource and g_outsink are defined in sunromvec.h */
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
	int			(*g_tracevec)();/* User's Trace vector */
	unsigned char		*g_tracecmds;	/* string of commands */
	int			(*g_breakvec)();/* User's Trap #1 vector */
	int			(*g_breaktrvec)();/* User's Trace vector */

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

};

#define				gp	((struct globram *)STRTDATA)

#endif
