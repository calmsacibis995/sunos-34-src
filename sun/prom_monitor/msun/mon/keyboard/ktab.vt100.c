/*
 * @(#)ktab.vt100.c 2.12 84/02/08 Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * ktab.vt100.c
 *
 * This module contains the translation tables for the up-down encoded
 * Sun VT100-style keyboard.
 *
 * A fuller explanation of the entries is in keyboard.h.
 *
 * Note that the "break" key is defined as a meta key; it turns on the
 * 0x80 bit of the returned character.  The "Setup" key turns on the 
 * 0x100 bit, which is ignored by most callers since they expect only a
 * byte.  NOTE that Setup-A, pressed in that order with nothing else
 * in between, aborts whatever program is running, and never passes on
 * the A to the user program.  All the other keys function as labeled.
 */

#include "../h/keyboard.h"

/* handy way to define control characters in the tables */
#define	c(char)	(char&0x1F)
#define ESC 0x1B

/* Unshifted keyboard table for "VT100 style" */

static struct keymap keytab_vt_lc[1] = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*  8 */	HOLE, 	HOLE, 	STRING+UPARROW,
					STRING+DOWNARROW,
						STRING+LEFTARROW,
							STRING+RIGHTARROW,
								OOPS,	STRING+PF1,
/* 16 */	STRING+PF2,
		 	STRING+PF3,
			 	STRING+PF4,
					c('['),	'1',	'2',	'3',	'4',
/* 24 */	'5', 	'6', 	'7',	'8',	'9',	'0',	'-',	'=',
/* 32 */	'`',	c('H'),	BUCKYBITS+METABIT,
					'7',	'8',	'9',	'-',	'\t',
/* 40 */	'q',	'w',	'e',	'r',	't',	'y',	'u',	'i',
/* 48 */	'o',	'p',	'[',	']',	0x7F,	'4',	'5',	'6',
/* 56 */	',',	SHIFTKEYS+LEFTCTRL,
				SHIFTKEYS+CAPSLOCK,
					'a',	's',	'd',	'f',	'g',
/* 64 */	'h',	'j',	'k',	'l',	';',	'\'',	'\r',	'\\',
/* 72 */	'1',	'2',	'3',	NOP,	NOSCROLL,
							SHIFTKEYS+LEFTSHIFT,
							 	'z',	'x',
/* 80 */	'c',	'v',	'b',	'n',	'm',	',',	'.',	'/',
/* 88 */	SHIFTKEYS+RIGHTSHIFT,
			'\n',	'0',	HOLE,	'.',	'\r',	HOLE,	HOLE,
/* 96 */	HOLE,	HOLE,	' ',	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*104 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*112 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	IDLE,
};


/* Shifted keyboard table for "VT100 style" */

static struct keymap keytab_vt_uc[1] = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*  8 */	HOLE, 	HOLE, 	STRING+UPARROW,
					STRING+DOWNARROW,
						STRING+LEFTARROW,
							STRING+RIGHTARROW,
								OOPS,	STRING+PF1,
/* 16 */	STRING+PF2,
		 	STRING+PF3,
			 	STRING+PF4,
					c('['),	'!',	'@',	'#',	'$',
/* 24 */	'%', 	'^', 	'&',	'*',	'(',	')',	'_',	'+',
/* 32 */	'~',	c('H'),	BUCKYBITS+METABIT,
					'7',	'8',	'9',	'-',	'\t',
/* 40 */	'Q',	'W',	'E',	'R',	'T',	'Y',	'U',	'I',
/* 48 */	'O',	'P',	'{',	'}',	0x7F,	'4',	'5',	'6',
/* 56 */	',',	SHIFTKEYS+LEFTCTRL,
				SHIFTKEYS+CAPSLOCK,
					'A',	'S',	'D',	'F',	'G',
/* 64 */	'H',	'J',	'K',	'L',	':',	'"',	'\r',	'|',
/* 72 */	'1',	'2',	'3',	NOP,	NOSCROLL,
							SHIFTKEYS+LEFTSHIFT,
							 	'Z',	'X',
/* 80 */	'C',	'V',	'B',	'N',	'M',	'<',	'>',	'?',
/* 88 */	SHIFTKEYS+RIGHTSHIFT,
			'\n',	'0',	HOLE,	'.',	'\r',	HOLE,	HOLE,
/* 96 */	HOLE,	HOLE,	' ',	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*104 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*112 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	IDLE,
};


/* Index to keymaps for "VT100 style" keyboard */
static struct keyboard keyindex_vt [1] = {
	keytab_vt_lc, keytab_vt_uc, 0, 0, 0,
	CAPSMASK+CTLSMASK, 	/* Shift keys that stay on at idle keyboard */
	0x0000,		/* Bucky bits that stay on at idle keyboard */
};

/***************************************************************************/
/*   Index table for the whole shebang					   */
/***************************************************************************/
struct keyboard *keytables[] = { keyindex_vt };
