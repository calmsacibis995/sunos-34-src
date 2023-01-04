/*
 * @(#)ktab.klunk.c 2.7 84/02/08 Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * ktab.klunk.c
 *
 * This module contains the translation tables for the up-down encoded
 * Sun keyboard.
 *
 * A fuller explanation of the entries is in keyboard.h.
 */

#include "../h/keyboard.h"

/* handy way to define control characters in the tables */
#define	c(char)	(char&0x1F)
#define ESC 0x1B


/* Unshifted keyboard table for Micro Switch 103SD32-2 */

static struct keymap keytab_ms_lc[1] = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				OOPS,	OOPS,	HOLE,	OOPS,	OOPS,	OOPS,
/*  8 */	OOPS, 	OOPS, 	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,
/* 16 */	OOPS, 	OOPS, 	OOPS,	c('['),	HOLE,	OOPS,	'+',	'-',
/* 24 */	HOLE, 	OOPS, 	'\f',	OOPS,	HOLE,	SHIFTKEYS+CAPSLOCK,
								'1',	'2',
/* 32 */	'3',	'4',	'5',	'6',	'7',	'8',	'9',	'0',
/* 40 */	'-',	'~',	'`',	'\b',	HOLE,	'7',	'8',	'9',
/* 48 */	HOLE,	OOPS,	STRING+UPARROW,
					OOPS,	HOLE,	'\t',	'q',	'w',
/* 56 */	'e',	'r',	't',	'y',	'u',	'i',	'o',	'p',
/* 64 */	'{',	'}',	'_',	HOLE,	'4',	'5',	'6',	HOLE,
/* 72 */	STRING+LEFTARROW,
			STRING+HOMEARROW,
				STRING+RIGHTARROW,
					HOLE,	SHIFTKEYS+SHIFTLOCK,
							'a', 	's',	'd',
/* 80 */	'f',	'g',	'h',	'j',	'k',	'l',	';',	':',
/* 88 */	'|',	'\r',	HOLE,	'1',	'2',	'3',	HOLE,	NOSCROLL,
/* 96 */	STRING+DOWNARROW,
			OOPS,	HOLE,	HOLE,	SHIFTKEYS+LEFTSHIFT,
							'z',	'x',	'c',
/*104 */	'v',	'b',	'n',	'm',	',',	'.',	'/',	SHIFTKEYS+RIGHTSHIFT,
/*112 */	NOP,	0x7F,	'0',	NOP,	'.',	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
					' ',	SHIFTKEYS+RIGHTCTRL,
							HOLE,	HOLE,	IDLE,
};

/* Shifted keyboard table for Micro Switch 103SD32-2 */

static struct keymap keytab_ms_uc [1] = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				OOPS,	OOPS,	HOLE,	OOPS,	OOPS,	OOPS,
/*  8 */	OOPS, 	OOPS, 	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,
/* 16 */	OOPS, 	OOPS, 	OOPS,	c('['),	HOLE,	OOPS,	'+',	'-',
/* 24 */	HOLE, 	OOPS, 	'\f',	OOPS,	HOLE,	SHIFTKEYS+CAPSLOCK,
								'!',	'"',
/* 32 */	'#',	'$',	'%',	'&',	'\'',	'(',	')',	'0',
/* 40 */	'=',	'^',	'@',	'\b',	HOLE,	'7',	'8',	'9',
/* 48 */	HOLE,	OOPS,	STRING+UPARROW,
					OOPS,	HOLE,	'\t',	'Q',	'W',
/* 56 */	'E',	'R',	'T',	'Y',	'U',	'I',	'O',	'P',
/* 64 */	'[',	']',	'_',	HOLE,	'4',	'5',	'6',	HOLE,
/* 72 */	STRING+LEFTARROW,
			STRING+HOMEARROW,
				STRING+RIGHTARROW,
					HOLE,	SHIFTKEYS+SHIFTLOCK,
							'A', 	'S',	'D',
/* 80 */	'F',	'G',	'H',	'J',	'K',	'L',	'+',	'*',
/* 88 */	'\\',	'\r',	HOLE,	'1',	'2',	'3',	HOLE,	NOSCROLL,
/* 96 */	STRING+DOWNARROW,
			OOPS,	HOLE,	HOLE,	SHIFTKEYS+LEFTSHIFT,
							'Z',	'X',	'C',
/*104 */	'V',	'B',	'N',	'M',	'<',	'>',	'?',	SHIFTKEYS+RIGHTSHIFT,
/*112 */	NOP,	0x7F,	'0',	NOP,	'.',	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
					' ',	SHIFTKEYS+RIGHTCTRL,
							HOLE,	HOLE,	IDLE,
};

/* Index to keymaps for Micro Switch 103SD32-2 */
static struct keyboard keyindex_ms [1] = {
	keytab_ms_lc, keytab_ms_uc, 0, 0, 0,
	CTLSMASK,	/* Shift bits which stay on with idle keyboard */
	0x0000,		/* Bucky bits which stay on with idle keyboard */
};

/***************************************************************************/
/*   Index table for the whole shebang					   */
/***************************************************************************/
struct keyboard *keytables[] = { keyindex_ms };
