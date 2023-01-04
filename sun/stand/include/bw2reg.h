/*	@(#) bw2reg.h 1.1 86/09/25	SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


/*
 * Sun-2 black and white display hardware definitions
 */

/*
 * The Sun-2 video lives in P2 memory space (BW2MB) or is onboard
 * (BW2OB); we can access it through
 *	the identical virtual addresses established by the monitor
 *	the physical address appropriate to the type.
 * The video memory is just memory, although it can also copy data that
 * is written to other locations.
 * The A-side of the MB_ZSCC connects to the keyboard.
 * The B-side of the MB_ZSCC connects to the mouse.
 */

#define	BW2_FBSIZE		(128*1024)	/* size of frame buffer */

#define	BW2MB_FB	(char *)0x700000	/* frame buffer */
#define	BW2MB_ZSCC	(char *)0x780000	/* UARTS */
#define BW2MB_CR	(char *)0x781800	/* video control register */
#define BW2MB_PGT		PGT_OBMEM

#define BW2VME_FB	(char *)0x000000
#define BW2VME_CR	(char *)0x020000
#define BW2VME_PGT		PGT_OBIO

/*
 * The video control register is arranged as shown below.
 *
 * Vc_copybase specifies the base (physical) address in main memory
 * where, if a write is done, the write is also done to the frame buffer.
 * Note that copying only works on 128K boundaries even though the base
 * address is specified in 64K units, since the low order bit is ignored.
 */
struct	bw2cr {
	unsigned vc_video_en:1;		/* Video enable */
	unsigned vc_copy_en:1;		/* Copy enable */
	unsigned vc_int_en:1;		/* Interrupt enable */
	unsigned vc_int:1;		/* Int active - r/o */
	unsigned vc_b_jumper:1;		/* Config jumper, 0=default */
					/* FIXME: 1=manufacturing burnin */
					/* This is a 'temporary' kludge */
	unsigned vc_a_jumper:1;		/* Config jumper, 0=default */
	unsigned vc_color_jumper:1;	/* Config jumper, 0=default */
					/* 1=use S-2 color as console */
	unsigned vc_1024_jumper:1;	/* Config jumper, 0=default */
					/* 1=screen is 1024*1024 */
	/* NOTE: vc_copybase & 0x81 == aberrant bits. Don't depend on 'em! */
#define	weird	1
	unsigned vc_copybase:weird+6+weird;	/* Base addr of copy memory */
#undef	weird
};
#define BW2_COPYSHIFT	16		/* bits to shift base address */

#define BW2_USECOPYMEM		0x1	/* config flag to use copy memory */

#define BW2_VIDEOENABLEMASK     0x8000  /* Video enable */
#define BW2_COPYENABLEMASK      0x4000  /* Copy enable */
#define BW2_INTENABLEMASK       0x2000  /* Interrupt enable */
#define BW2_INTACTIVEMASK       0x1000  /* Interrupt active */
#define BW2_COPYBASEMASK        0x007E  /* Copy base */

/*
 * The device itself, as we remap it
 */
struct	bw2dev {
	u_char	image[BW2_FBSIZE];
#ifdef sun2
	struct bw2cr bw2cr;
	u_char	filler[NBPG - sizeof (struct bw2cr)];
#endif
};
