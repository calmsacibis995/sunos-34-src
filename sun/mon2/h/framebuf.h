/*	@(#)framebuf.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * framebuf.h - constants for the Sun graphics board version 1
 */

# define GXUnit0Base	(0x100000)	/* device 0 base adress */
# define GXUnit1Base	(0x120000)	/* device 1 base adress */
# define GXUnit2Base	(0x140000)	/* device 2 base adress */
# define GXUnit3Base	(0x160000)	/* device 3 base adress */
# define GXUnit4Base	(0x180000)	/* device 4 base adress */
# define GXUnit5Base	(0x1A0000)	/* device 5 base adress */
# define GXUnit6Base	(0x1C0000)	/* device 6 base adress */
/* Note: no GXUnit7Base, this would overlap Multibus I/O Space. */
#define	GXDefaultBase	GXUnit6Base	/* default Frame buffer */

/*
 * The low order 11 bits consist of the X or Y address times 2.
 * The lowest order bit is ignored, so word addressing works efficiently.
 */

# define GXselectX (0<<11)	/* the address is loaded into an X register */
# define GXselectx (0<<11)	/* the address is loaded into an X register */
# define GXselectY (1<<11)	/* the address is loaded into an Y register */
# define GXselecty (1<<11)	/* the address is loaded into an Y register */

/*
 * The screen is of size 1024x800 pixels.  It includes the left and top edges
 * and excludes the right and bottom edges of the following square.
 * NOTE: This assumes a portrait monitor.
 */

# define GXleft 0
# define GXright 800
# define GXtop 0
# define GXbottom 1024

/*
 * There are four sets of X and Y register pairs, selected by the following bits
 */

# define GXaddressSet0  (0<<12)
# define GXaddressSet1  (1<<12)
# define GXaddressSet2  (2<<12)
# define GXaddressSet3  (3<<12)
# define GXset0  (0<<12)
# define GXset1  (1<<12)
# define GXset2  (2<<12)
# define GXset3  (3<<12)

/*
 * The following bits indicate which registers are to be loaded
 */

# define GXnone    (0<<14)
# define GXothers  (1<<14)
# define GXsource  (2<<14)
# define GXmask    (3<<14)
# define GXpat     (3<<14)

# define GXupdate (1<<16)	/* actually update the frame buffer */


/*
 * These registers can appear on the left of an assignment statement.
 * Note they clobber X register 3.
 */

# define GXfunction *(short *)(GXBase+GXset3+GXothers+(0<<1) )
# define GXwidth    *(short *)(GXBase+GXset3+GXothers+(1<<1) )
# define GXcontrol  *(short *)(GXBase+GXset3+GXothers+(2<<1) )
# define GXintClear *(short *)(GXBase+GXset3+GXothers+(3<<1) )

# define GXsetMask    *(short *)(GXBase+GXset3+GXmask )
# define GXsetSource  *(short *)(GXBase+GXset3+GXsource )
# define GXpattern    *(short *)(GXBase+GXset3+GXpat )

/*
 * The following bits are written into the GX control register.
 * It is reset to zero on hardware reset and power-up.
 * The high order three bits determine the Interrupt level (0-7)
 */

# define GXintEnable   (1<<8)
# define GXvideoEnable (1<<9)
# define GXintLevel0	(0<<13)
# define GXintLevel1	(1<<13)
# define GXintLevel2	(2<<13)
# define GXintLevel3	(3<<13)
# define GXintLevel4	(4<<13)
# define GXintLevel5	(5<<13)
# define GXintLevel6	(6<<13)
# define GXintLevel7	(7<<13)

/*
 * The following are "function" encodings loaded into the function register
 */

# define GXnoop			0xAAAA
# define GXinvert		0x5555
# define GXcopy        		0xCCCC
# define GXcopyInverted 	0x3333
# define GXclear		0x0000
# define GXset			0xFFFF
# define GXpaint		0xEEEE
# define GXpaintInverted 	0x2222
# define GXand			0x8888
# define GXxor			0x6666

/*
 * The following three constants permit explicit construction of function
 * encodings, as used in SETGXFUNC(~(GXSOURCE^GXDEST)&GXMASK)
 */

# define GXMASK 0xF0F0
# define GXSOURCE 0xCCCC
# define GXDEST 0xAAAA

/*
 * These may appear in statement contexts to just
 * set the X and Y registers of set number zero to the given values.
 */

# define GXsetX(X)	*(short *)(GXBase				 \
/* Dumb compiler adds zero due to this:  +GXselectX+GXset0 */		 \
							  +((X)<<1)) = 1;
# define GXsetY(Y)	*(short *)(GXBase+GXselectY+GXset0+((Y)<<1)) = 1;
