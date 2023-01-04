/*	@(#)sunromvec.h 2.19 84/02/08 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * sunromvec.h
 *
 * This defines the structure of the vector table at the first of the boot rom.
 *
 * This vector table is the only knowledge the outside world has of this prom.
 * It is referenced by hardware (Reset SSP, PC), and
 * programs & operating systems that run under the monitor.  Once located,
 * no entry can be re-located unless you change the world that needs it.
 * 
 * The entries start as the very first thing in the Prom.
 * The easiest way to reference elements of this vector is to say:
 *
 *	*romp->xxx
 *
 * as in:
 *
 *	(*romp->v_putchar) (c);
 *
 * This is pretty cheap as it only generates   movl NNNNNN:l,a0;   jsr a0@ .
 * That's 2 bytes longer, and 4 cycles longer, than a simple subroutine call.
 */

struct sunromvec {
	char		*v_initsp;		/* Initial SSP for hardware */
	int		(*v_startmon)();	/* Initial PC  for hardware */

/* Monitor and hardware revision and identification */
	int		*v_dummy1a;		/* Was: v_mon_id. To be: berr */
	struct bootparam	**v_bootparam;	/* Info for bootstrapped pgm */
 	unsigned	*v_memorysize;		/* Usable memory in bytes */

/* Single-character input and output */
	unsigned char	(*v_getchar)();		/* Get char from input source */
	int		(*v_putchar)();		/* Put char to output sink */
	int		(*v_mayget)();		/* Maybe get char, or -1 */
	int		(*v_mayput)();		/* Maybe put char, or -1 */
	unsigned char	*v_echo;		/* Should getchar echo? */
	unsigned char	*v_insource;		/* Input source selector */
	unsigned char	*v_outsink;		/* Output sink selector */

/* Keyboard input (scanned by monitor nmi routine) */
	int		(*v_getkey)();		/* Get next key if one exists */
	int		(*v_initgetkey)();	/* Like it says */
	unsigned int	*v_translation;		/* Kbd translation selector */
	unsigned char	*v_keybid;		/* Keyboard ID byte */
	int		dummy0k;
	int		dummy1k;
	struct keybuf	*v_keybuf;		/* Up/down keycode buffer */

	char		*v_mon_id;		/* Revision level of monitor */

/* Frame buffer output and terminal emulation */
	int		(*v_fwritechar)();	/* Write a character to FB */
	int		*v_fbaddr;		/* Address of frame buffer */
	char		**v_font;		/* font table for FB */
	int		(*v_fwritestr)();	/* Quickly write string to FB */

/* Reboot interface routine -- resets and reboots system.  No return. */
	int		(*v_boot_me)();		/* eg boot_me("xy()vmunix") */

/* Line input and parsing */
	unsigned char	*v_linebuf;		/* The line input buffer */
	unsigned char	**v_lineptr;		/* Cur pointer into linebuf */
	int		*v_linesize;		/* length of line in linebuf */
	int		(*v_getline)();		/* Get line from user */
	unsigned char	(*v_getone)();		/* Get next char from linebuf */
	unsigned char	(*v_peekchar)();	/* Peek at next char */
	int		*v_fbthere;		/* =1 if frame buffer there */
	int		(*v_getnum)();		/* Grab hex num from line */

/* Phrase output to current output sink */
	int		(*v_printf)();		/* Similar to "Kernel printf" */
	int		(*v_printhex)();	/* Format N digits in hex */

	unsigned char	*v_leds;		/* RAM copy of LED register */
	int		(*v_set_leds)();	/* Sets LED's and RAM copy */

/* nmi related information */
	int		(*v_nmi)();		/* Addr for level 7 vector */
	int		(*v_abortent)();	/* entry for keyboard abort */
	int		*v_nmiclock;		/* counts up in msec */

	int		*v_fbtype;		/* FB type: see <sun/fbio.h> */

/* Assorted other things */
	int		dummy2k;
	struct globram	*v_gp;			/* monitor global variables */
	struct zscc_device 	*v_keybzscc;	/* Addr of keyboard in use */

	int		*v_keyrinit;		/* ms before kbd repeat */
	unsigned char	*v_keyrtick; 		/* ms between repetitions */
	int		dummy3k;
	long		*v_resetaddr;		/* where to jump on a reset */
	long		*v_resetmap;		/* pgmap entry for resetaddr */
						/* Really struct pgmapent *  */
	int		(*v_exit_to_mon)();	/* Exit from user program */
};

/*
 * THE FOLLOWING CONSTANTS MUST BE CHANGED ANYTIME THE VALUE OF "PROM_BASE"
 * IN s2addrs.h IS CHANGED.  THEY ARE CONSTANTS HERE SO THAT EVERY MODULE WHICH
 * NEEDS AN ADDRESS OUT OF SUNROMVEC DOESN'T HAVE TO INCLUDE s2addrs.h.
 */
#ifdef SUN1
#define	romp	((struct sunromvec *)0x200000)
#else  SUN1
#define	romp	((struct sunromvec *)0xEF0000)
#endif SUN1

/*
 * The following are included for compatability with previous versions
 * of this header file.  Names containing capital letters have been
 * changed to conform with "Bill Joy Normal Form".  This section provides
 * the translation between the old and new names.  It can go away once
 * Sun-1 applications have been converted over.
 */
#define	RomVecPtr	romp
#define	v_SunRev	v_mon_id
#define	v_MemorySize	v_memorysize
#define	v_EchoOn	v_echo
#define	v_InSource	v_insource
#define	v_OutSink	v_outsink
#define	v_InitGetkey	v_initgetkey
#define	v_KeybId	v_keybid
#define	v_Keybuf	v_keybuf
#define	v_FBAddr	v_fbaddr
#define	v_FontTable	v_font
#define	v_message	v_printf
#define	v_KeyFrsh	v_nmi
#define	AbortEnt	v_abortent
#define	v_RefrCnt	v_nmiclock
#define	v_GlobPtr	v_gp
#define	v_KRptInitial	v_keyrinit
#define	v_KRptTick	v_keyrtick
#define	v_ExitOp	v_exit_to_mon
#define	v_fwrstr	v_fwritestr
#define	v_linbuf	v_linebuf
