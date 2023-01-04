#ifndef lint
static	char sccsid[] = "@(#)keytables.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (C) 1983 by Sun Microsystems, Inc.
 */

/*
 * keytables.c
 *
 * This module contains the translation tables for the up-down encoded
 * Sun keyboards.
 */
#include "../sundev/kbd.h"

/* handy way to define control characters in the tables */
#define	c(char)	(char&0x1F)
#define ESC 0x1B


/* Unshifted keyboard table for Micro Switch 103SD32-2 */

static struct keymap keytab_ms_lc = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				LF(2),	LF(3),	HOLE,	TF(1),	TF(2),	TF(3),
/*  8 */	TF(4), 	TF(5), 	TF(6),	TF(7),	TF(8),	TF(9),	TF(10),	TF(11),
/* 16 */	TF(12), TF(13), TF(14),	c('['),	HOLE,	RF(1),	'+',	'-',
/* 24 */	HOLE, 	LF(4), 	'\f',	LF(6),	HOLE,	SHIFTKEYS+CAPSLOCK,
								'1',	'2',
/* 32 */	'3',	'4',	'5',	'6',	'7',	'8',	'9',	'0',
/* 40 */	'-',	'~',	'`',	'\b',	HOLE,	'7',	'8',	'9',
/* 48 */	HOLE,	LF(7),	STRING+UPARROW,
					LF(9),	HOLE,	'\t',	'q',	'w',
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
			LF(15),	HOLE,	HOLE,	SHIFTKEYS+LEFTSHIFT,
							'z',	'x',	'c',
/*104 */	'v',	'b',	'n',	'm',	',',	'.',	'/',	SHIFTKEYS+RIGHTSHIFT,
/*112 */	NOP,	0x7F,	'0',	NOP,	'.',	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
					' ',	SHIFTKEYS+RIGHTCTRL,
							HOLE,	HOLE,	IDLE,
};

/* Shifted keyboard table for Micro Switch 103SD32-2 */

static struct keymap keytab_ms_uc = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				LF(2),	LF(3),	HOLE,	TF(1),	TF(2),	TF(3),
/*  8 */	TF(4), 	TF(5), 	TF(6),	TF(7),	TF(8),	TF(9),	TF(10),	TF(11),
/* 16 */	TF(12), TF(13), TF(14),	c('['),	HOLE,	RF(1),	'+',	'-',
/* 24 */	HOLE, 	LF(4), 	'\f',	LF(6),	HOLE,	SHIFTKEYS+CAPSLOCK,
								'!',	'"',
/* 32 */	'#',	'$',	'%',	'&',	'\'',	'(',	')',	'0',
/* 40 */	'=',	'^',	'@',	'\b',	HOLE,	'7',	'8',	'9',
/* 48 */	HOLE,	LF(7),	STRING+UPARROW,
					LF(9),	HOLE,	'\t',	'Q',	'W',
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
			LF(15),	HOLE,	HOLE,	SHIFTKEYS+LEFTSHIFT,
							'Z',	'X',	'C',
/*104 */	'V',	'B',	'N',	'M',	'<',	'>',	'?',	SHIFTKEYS+RIGHTSHIFT,
/*112 */	NOP,	0x7F,	'0',	NOP,	'.',	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
					' ',	SHIFTKEYS+RIGHTCTRL,
							HOLE,	HOLE,	IDLE,
};


/* Caps Locked keyboard table for Micro Switch 103SD32-2 */

static struct keymap keytab_ms_cl = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				LF(2),	LF(3),	HOLE,	TF(1),	TF(2),	TF(3),
/*  8 */	TF(4), 	TF(5), 	TF(6),	TF(7),	TF(8),	TF(9),	TF(10),	TF(11),
/* 16 */	TF(12), TF(13), TF(14),	c('['),	HOLE,	RF(1),	'+',	'-',
/* 24 */	HOLE, 	LF(4), 	'\f',	LF(6),	HOLE,	SHIFTKEYS+CAPSLOCK,
								'1',	'2',
/* 32 */	'3',	'4',	'5',	'6',	'7',	'8',	'9',	'0',
/* 40 */	'-',	'~',	'`',	'\b',	HOLE,	'7',	'8',	'9',
/* 48 */	HOLE,	LF(7),	STRING+UPARROW,
	 				LF(9),	HOLE,	'\t',	'Q',	'W',
/* 56 */	'E',	'R',	'T',	'Y',	'U',	'I',	'O',	'P',
/* 64 */	'{',	'}',	'_',	HOLE,	'4',	'5',	'6',	HOLE,
/* 72 */	STRING+LEFTARROW,
			STRING+HOMEARROW,
				STRING+RIGHTARROW,
					HOLE,	SHIFTKEYS+SHIFTLOCK,
							'A', 	'S',	'D',
/* 80 */	'F',	'G',	'H',	'J',	'K',	'L',	';',	':',
/* 88 */	'|',	'\r',	HOLE,	'1',	'2',	'3',	HOLE,	NOSCROLL,
/* 96 */	STRING+DOWNARROW,
			LF(15),	HOLE,	HOLE,	SHIFTKEYS+LEFTSHIFT,
							'Z',	'X',	'C',
/*104 */	'V',	'B',	'N',	'M',	',',	'.',	'/',	SHIFTKEYS+RIGHTSHIFT,
/*112 */	NOP,	0x7F,	'0',	NOP,	'.',	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
					' ',	SHIFTKEYS+RIGHTCTRL,
							HOLE,	HOLE,	IDLE,
};

/* Controlled keyboard table for Micro Switch 103SD32-2 */

static struct keymap keytab_ms_ct = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				LF(2),	LF(3),	HOLE,	TF(1),	TF(2),	TF(3),
/*  8 */	TF(4), 	TF(5), 	TF(6),	TF(7),	TF(8),	TF(9),	TF(10),	TF(11),
/* 16 */	TF(12), TF(13), TF(14),	c('['),	HOLE,	RF(1),	OOPS, OOPS,
/* 24 */	HOLE, 	LF(4), 	'\f',	LF(6),	HOLE,	SHIFTKEYS+CAPSLOCK,
								OOPS,	OOPS,
/* 32 */	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,
/* 40 */	OOPS,	c('^'),	c('@'),	'\b',	HOLE,	OOPS,	OOPS,	OOPS,
/* 48 */	HOLE,	LF(7),	STRING+UPARROW,
					LF(9),	HOLE,	'\t',	CTRLQ,	c('W'),
/* 56 */	c('E'),	c('R'),	c('T'),	c('Y'),	c('U'),	c('I'),	c('O'),	c('P'),
/* 64 */	c('['),	c(']'),	c('_'),	HOLE,	OOPS,	OOPS,	OOPS,	HOLE,
/* 72 */	STRING+LEFTARROW,
			STRING+HOMEARROW,
				STRING+RIGHTARROW,
					HOLE,	SHIFTKEYS+SHIFTLOCK,
							c('A'),	CTRLS,	c('D'),
/* 80 */	c('F'),	c('G'),	c('H'),	c('J'),	c('K'),	c('L'),	OOPS,	OOPS,
/* 88 */	c('\\'),
			'\r',	HOLE,	OOPS,	OOPS,	OOPS,	HOLE,	NOSCROLL,
/* 96 */	STRING+DOWNARROW,
			LF(15),	HOLE,	HOLE,	SHIFTKEYS+LEFTSHIFT,
							c('Z'),	c('X'),	c('C'),
/*104 */	c('V'),	c('B'),	c('N'),	c('M'),	OOPS,	OOPS,	OOPS,	SHIFTKEYS+RIGHTSHIFT,
/*112 */	NOP,	0x7F,	OOPS,	NOP,	OOPS,	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
					'\0',	SHIFTKEYS+RIGHTCTRL,
							HOLE,	HOLE,	IDLE,
};


/* "Key Up" keyboard table for Micro Switch 103SD32-2 */

static struct keymap keytab_ms_up = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				OOPS,	OOPS,	HOLE,	OOPS,	OOPS,	OOPS,
/*  8 */	OOPS, 	OOPS, 	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,
/* 16 */	OOPS, 	OOPS, 	OOPS,	NOP,	HOLE,	OOPS,	NOP,	NOP,
/* 24 */	HOLE, 	OOPS, 	NOP,	OOPS,	HOLE,	SHIFTKEYS+CAPSLOCK,
								NOP,	NOP,
/* 32 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 40 */	NOP,	NOP,	NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,
/* 48 */	HOLE,	OOPS,	NOP,	OOPS,	HOLE,	NOP,	NOP,	NOP,
/* 56 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 64 */	NOP,	NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,	HOLE,
/* 72 */	NOP,	NOP,	NOP,	HOLE,	SHIFTKEYS+SHIFTLOCK,
							NOP, 	NOP,	NOP,
/* 80 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 88 */	NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,	HOLE,	NOP,
/* 96 */	NOP,	OOPS,	HOLE,	HOLE,	SHIFTKEYS+LEFTSHIFT,
							NOP,	NOP,	NOP,
/*104 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	SHIFTKEYS+RIGHTSHIFT,
/*112 */	NOP,	NOP,	NOP,	NOP,	NOP,	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
					NOP,	SHIFTKEYS+RIGHTCTRL,
							HOLE,	HOLE,	RESET,
};


/* Index to keymaps for Micro Switch 103SD32-2 */
static struct keyboard keyindex_ms = {
	&keytab_ms_lc,
	&keytab_ms_uc,
	&keytab_ms_cl,
	&keytab_ms_ct,
	&keytab_ms_up,
	CTLSMASK,	/* Shift bits which stay on with idle keyboard */
	0x0000,		/* Bucky bits which stay on with idle keyboard */
	1,	77,	/* abort keys */
	0x0000,		/* Shift bits which toggle on down event */
};

/* Unshifted keyboard table for Sun-2 keyboard */

static struct keymap keytab_s2_lc = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				LF(11),	LF(2),	HOLE,	TF(1),	TF(2),	TF(11),
/*  8 */	TF(3), 	TF(12),	TF(4),	TF(13),	TF(5),	TF(14),	TF(6),	TF(15),
/* 16 */	TF(7),	TF(8),	TF(9),	TF(10),	HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	LF(12),	HOLE,	c('['),	'1',	'2',
/* 32 */	'3',	'4',	'5',	'6',	'7',	'8',	'9',	'0',
/* 40 */	'-',	'=',	'`',	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	HOLE,	LF(5),	LF(13),	LF(6),	HOLE,	'\t',	'q',	'w',
/* 56 */	'e',	'r',	't',	'y',	'u',	'i',	'o',	'p',
/* 64 */	'[',	']',	0x7F,	HOLE,	RF(7),	STRING+UPARROW,
								RF(9),	HOLE,
/* 72 */	LF(7),	LF(8),	LF(14),	HOLE,	SHIFTKEYS+LEFTCTRL,
							'a', 	's',	'd',
/* 80 */	'f',	'g',	'h',	'j',	'k',	'l',	';',	'\'',
/* 88 */	'\\',	'\r',	HOLE,	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								HOLE,	LF(9),
/* 96 */	LF(15),	LF(10),	HOLE,	SHIFTKEYS+LEFTSHIFT,
						'z',	'x',	'c',	'v',
/*104 */	'b',	'n',	'm',	',',	'.',	'/',	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	BUCKYBITS+METABIT,
			' ',	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	ERROR,	IDLE,
};

/* Shifted keyboard table for Sun-2 keyboard */

static struct keymap keytab_s2_uc = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				LF(11),	LF(2),	HOLE,	TF(1),	TF(2),	TF(11),
/*  8 */	TF(3), 	TF(12),	TF(4),	TF(13),	TF(5),	TF(14),	TF(6),	TF(15),
/* 16 */	TF(7),	TF(8),	TF(9),	TF(10),	HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	LF(12),	HOLE,	c('['),	'!',    '@',
/* 32 */	'#',	'$',	'%',	'^',	'&',	'*',	'(',	')',
/* 40 */	'_',	'+',	'~',	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	HOLE,	LF(5),	LF(13),	LF(6),	HOLE,	'\t',	'Q',	'W',
/* 56 */	'E',	'R',	'T',	'Y',	'U',	'I',	'O',	'P',
/* 64 */	'{',	'}',	0x7F,	HOLE,	RF(7),	STRING+UPARROW,
								RF(9),	HOLE,
/* 72 */	LF(7),	LF(8),	LF(14),	HOLE,	SHIFTKEYS+LEFTCTRL,
							'A', 	'S',	'D',
/* 80 */	'F',	'G',	'H',	'J',	'K',	'L',	':',	'"',
/* 88 */	'|',	'\r',	HOLE,	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								HOLE,	LF(9),
/* 96 */	LF(15),	LF(10),	HOLE,	SHIFTKEYS+LEFTSHIFT,
						'Z',	'X',	'C',	'V',
/*104 */	'B',	'N',	'M',	'<',	'>',	'?',	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	BUCKYBITS+METABIT,
			' ',	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	ERROR,	IDLE,
};


/* Caps Locked keyboard table for Sun-2 keyboard */

static struct keymap keytab_s2_cl = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				LF(11),	LF(2),	HOLE,	TF(1),	TF(2),	TF(11),
/*  8 */	TF(3), 	TF(12),	TF(4),	TF(13),	TF(5),	TF(14),	TF(6),	TF(15),
/* 16 */	TF(7),	TF(8),	TF(9),	TF(10),	HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	LF(12),	HOLE,	c('['),	'1',	'2',
/* 32 */	'3',	'4',	'5',	'6',	'7',	'8',	'9',	'0',
/* 40 */	'-',	'=',	'`',	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	HOLE,	LF(5),	LF(13),	LF(6),	HOLE,	'\t',	'Q',	'W',
/* 56 */	'E',	'R',	'T',	'Y',	'U',	'I',	'O',	'P',
/* 64 */	'[',	']',	0x7F,	HOLE,	RF(7),	STRING+UPARROW,
								RF(9),	HOLE,
/* 72 */	LF(7),	LF(8),	LF(14),	HOLE,	SHIFTKEYS+LEFTCTRL,
							'A', 	'S',	'D',
/* 80 */	'F',	'G',	'H',	'J',	'K',	'L',	';',	'\'',
/* 88 */	'\\',	'\r',	HOLE,	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								HOLE,	LF(9),
/* 96 */	LF(15),	LF(10),	HOLE,	SHIFTKEYS+LEFTSHIFT,
						'Z',	'X',	'C',	'V',
/*104 */	'B',	'N',	'M',	',',	'.',	'/',	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	BUCKYBITS+METABIT,
			' ',	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	ERROR,	IDLE,
};

/* Controlled keyboard table for Sun-2 keyboard */

static struct keymap keytab_s2_ct = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				LF(11),	LF(2),	HOLE,	TF(1),	TF(2),	TF(11),
/*  8 */	TF(3), 	TF(12),	TF(4),	TF(13),	TF(5),	TF(14),	TF(6),	TF(15),
/* 16 */	TF(7),	TF(8),	TF(9),	TF(10),	HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	LF(12),	HOLE,	c('['),	'1',	c('@'),
/* 32 */	'3',	'4',	'5',	c('^'),	'7',	'8',	'9',	'0',
/* 40 */	c('_'),	'=',	c('^'),	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	HOLE,	LF(5),	LF(13),	LF(6),	HOLE,	'\t',   c('q'),	c('w'),
/* 56 */	c('e'),	c('r'),	c('t'),	c('y'),	c('u'),	c('i'),	c('o'),	c('p'),
/* 64 */	c('['),	c(']'),	0x7F,	HOLE,	RF(7),	STRING+UPARROW,
								RF(9),	HOLE,
/* 72 */	LF(7),	LF(8),	LF(14),	HOLE,	SHIFTKEYS+LEFTCTRL,
							c('a'),	c('s'),	c('d'),
/* 80 */	c('f'),	c('g'),	c('h'),	c('j'),	c('k'),	c('l'),	';',	'\'',
/* 88 */	c('\\'),
			'\r',	HOLE,	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								HOLE,	LF(9),
/* 96 */	LF(15),	LF(10),	HOLE,	SHIFTKEYS+LEFTSHIFT,
						c('z'),	c('x'),	c('c'),	c('v'),
/*104 */	c('b'),	c('n'),	c('m'),	',',	'.',	c('_'),	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	BUCKYBITS+METABIT,
			c(' '),	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	ERROR,	IDLE,
};



/* "Key Up" keyboard table for Sun-2 keyboard */

static struct keymap keytab_s2_up = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				OOPS,	OOPS,	HOLE,	OOPS,	OOPS,	OOPS,
/*  8 */	OOPS, 	OOPS, 	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,
/* 16 */	OOPS, 	OOPS, 	OOPS,	OOPS,	HOLE,	OOPS,	OOPS,	NOP,
/* 24 */	HOLE, 	OOPS, 	OOPS,	OOPS,	HOLE,	NOP,	NOP,	NOP,
/* 32 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 40 */	NOP,	NOP,	NOP,	NOP,	HOLE,	OOPS,	OOPS,	NOP,
/* 48 */	HOLE,	OOPS,	OOPS,	OOPS,	HOLE,	NOP,	NOP,	NOP,
/* 56 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 64 */	NOP,	NOP,	NOP,	HOLE,	OOPS,	OOPS,	NOP,	HOLE,
/* 72 */	OOPS,	OOPS,	OOPS,	HOLE,	SHIFTKEYS+LEFTCTRL,
							NOP, 	NOP,	NOP,
/* 80 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 88 */	NOP,	NOP,	HOLE,	OOPS,	OOPS,	NOP,	HOLE,	OOPS,
/* 96 */	OOPS,	OOPS,	HOLE,	SHIFTKEYS+LEFTSHIFT,
						NOP,	NOP,	NOP,	NOP,
/*104 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	SHIFTKEYS+RIGHTSHIFT,
									NOP,
/*112 */	OOPS,	OOPS,	NOP,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	BUCKYBITS+METABIT,
			NOP,	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	HOLE,	RESET,
};

/* Index to keymaps for Sun-2 keyboard */
static struct keyboard keyindex_s2 = {
	&keytab_s2_lc,
	&keytab_s2_uc,
	&keytab_s2_cl,
	&keytab_s2_ct,
	&keytab_s2_up,
	CAPSMASK,	/* Shift bits which stay on with idle keyboard */
	0x0000,		/* Bucky bits which stay on with idle keyboard */
	1,	77,	/* abort keys */
	0x0000,		/* Shift bits which toggle on down event */
};

/* Unshifted keyboard table for "VT100 style" */

static struct keymap keytab_vt_lc = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*  8 */	HOLE, 	HOLE, 	STRING+UPARROW,
					STRING+DOWNARROW,
						STRING+LEFTARROW,
							STRING+RIGHTARROW,
								HOLE,	TF(1),
/* 16 */	TF(2),	TF(3),	TF(4),	c('['),	'1',	'2',	'3',	'4',
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

static struct keymap keytab_vt_uc = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*  8 */	HOLE, 	HOLE, 	STRING+UPARROW,
					STRING+DOWNARROW,
						STRING+LEFTARROW,
							STRING+RIGHTARROW,
								HOLE,	TF(1),
/* 16 */	TF(2),	TF(3),	TF(4),	c('['),	'!',	'@',	'#',	'$',
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

/* Caps Locked keyboard table for "VT100 style" */

static struct keymap keytab_vt_cl = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*  8 */	HOLE, 	HOLE, 	STRING+UPARROW,
					STRING+DOWNARROW,
						STRING+LEFTARROW,
							STRING+RIGHTARROW,
								HOLE,	TF(1),
/* 16 */	TF(2),	TF(3),	TF(4),	c('['),	'1',	'2',	'3',	'4',
/* 24 */	'5', 	'6', 	'7',	'8',	'9',	'0',	'-',	'=',
/* 32 */	'`',	c('H'),	BUCKYBITS+METABIT,
					'7',	'8',	'9',	'-',	'\t',
/* 40 */	'Q',	'W',	'E',	'R',	'T',	'Y',	'U',	'I',
/* 48 */	'O',	'P',	'[',	']',	0x7F,	'4',	'5',	'6',
/* 56 */	',',	SHIFTKEYS+LEFTCTRL,
				SHIFTKEYS+CAPSLOCK,
					'A',	'S',	'D',	'F',	'G',
/* 64 */	'H',	'J',	'K',	'L',	';',	'\'',	'\r',	'\\',
/* 72 */	'1',	'2',	'3',	NOP,	NOSCROLL,
							SHIFTKEYS+LEFTSHIFT,
							 	'Z',	'X',
/* 80 */	'C',	'V',	'B',	'N',	'M',	',',	'.',	'/',
/* 88 */	SHIFTKEYS+RIGHTSHIFT,
			'\n',	'0',	HOLE,	'.',	'\r',	HOLE,	HOLE,
/* 96 */	HOLE,	HOLE,	' ',	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*104 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*112 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	IDLE,
};

/* Controlled keyboard table for "VT100 style" */

static struct keymap keytab_vt_ct = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*  8 */	HOLE, 	HOLE, 	STRING+UPARROW,
					STRING+DOWNARROW,
						STRING+LEFTARROW,
							STRING+RIGHTARROW,
								HOLE,	TF(1),
/* 16 */	TF(2),	TF(3),	TF(4),	c('['),	'1',	c('@'),	'3',	'4',
/* 24 */	'5',	c('^'),	'7',	'8',	'9',	'0',	c('_'),	'=',
/* 32 */	c('^'),	c('H'),	BUCKYBITS+METABIT,
					'7',	'8',	'9',	'-',	'\t',
/* 40 */	CTRLQ,	c('W'),	c('E'),	c('R'),	c('T'),	c('Y'),	c('U'),	c('I'),
/* 48 */	c('O'),	c('P'),	c('['),	c(']'),	0x7F,	'4',	'5',	'6',
/* 56 */	',',	SHIFTKEYS+LEFTCTRL,
				SHIFTKEYS+CAPSLOCK,
					c('A'),	CTRLS,	c('D'),	c('F'),	c('G'),
/* 64 */	c('H'),	c('J'),	c('K'),	c('L'),	':',	'"',	'\r',	c('\\'),
/* 72 */	'1',	'2',	'3',	NOP,	NOSCROLL,
							SHIFTKEYS+LEFTSHIFT,
							 	c('Z'),	c('X'),
/* 80 */	c('C'),	c('V'),	c('B'),	c('N'),	c('M'),	',',	'.',	c('_'),
/* 88 */	SHIFTKEYS+RIGHTSHIFT,
			'\n',	'0',	HOLE,	'.',	'\r',	HOLE,	HOLE,
/* 96 */	HOLE,	HOLE,	c(' '),	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*104 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*112 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	IDLE,
};


/* "Key Up" keyboard table for "VT100 style" */

static struct keymap keytab_vt_up = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*  8 */	HOLE, 	HOLE, 	NOP,	NOP,	NOP,	NOP,	HOLE,	OOPS,
/* 16 */	OOPS,	OOPS,	OOPS,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 24 */	NOP, 	NOP, 	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 32 */	NOP,	NOP,	BUCKYBITS+METABIT,
					NOP,	NOP,	NOP,	NOP,	NOP,
/* 40 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 48 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 56 */	NOP,	SHIFTKEYS+LEFTCTRL,
				SHIFTKEYS+CAPSLOCK,
					NOP,	NOP,	NOP,	NOP,	NOP,
/* 64 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 72 */	NOP,	NOP,	NOP,	NOP,	NOP,	SHIFTKEYS+LEFTSHIFT,
							 	NOP,	NOP,
/* 80 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 88 */	SHIFTKEYS+RIGHTSHIFT,
			NOP,	NOP,	HOLE,	NOP,	NOP,	HOLE,	HOLE,
/* 96 */	HOLE,	HOLE,	NOP,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*104 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*112 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	RESET,
};


/* Index to keymaps for "VT100 style" keyboard */
static struct keyboard keyindex_vt = {
	&keytab_vt_lc,
	&keytab_vt_uc,
	&keytab_vt_cl,
	&keytab_vt_ct,
	&keytab_vt_up,
	CAPSMASK+CTLSMASK, 	/* Shift keys that stay on at idle keyboard */
	0x0000,		/* Bucky bits that stay on at idle keyboard */
	1,	59,	/* abort keys */
	0x0000,		/* Shift bits which toggle on down event */
};

/* Unshifted keyboard table for Sun-3 keyboard */

static struct keymap keytab_s3_lc = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	LF(2),	HOLE,	TF(1),	TF(2),	HOLE,
/*  8 */	TF(3), 	HOLE,	TF(4),	HOLE,	TF(5),	HOLE,	TF(6),	HOLE,
/* 16 */	TF(7),	TF(8),	TF(9),	ALT,	HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	HOLE,	HOLE,	c('['),	'1',	'2',
/* 32 */	'3',	'4',	'5',	'6',	'7',	'8',	'9',	'0',
/* 40 */	'-',	'=',	'`',	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	HOLE,	LF(5),	HOLE,	LF(6),	HOLE,	'\t',	'q',	'w',
/* 56 */	'e',	'r',	't',	'y',	'u',	'i',	'o',	'p',
/* 64 */	'[',	']',	0x7F,	HOLE,	RF(7),	STRING+UPARROW,
								RF(9),	HOLE,
/* 72 */	LF(7),	LF(8),	LF(14),	HOLE,	SHIFTKEYS+LEFTCTRL,
							'a', 	's',	'd',
/* 80 */	'f',	'g',	'h',	'j',	'k',	'l',	';',	'\'',
/* 88 */	'\\',	'\r',	HOLE,	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								HOLE,	LF(9),
/* 96 */	LF(15),	LF(10),	HOLE,	SHIFTKEYS+LEFTSHIFT,
						'z',	'x',	'c',	'v',
/*104 */	'b',	'n',	'm',	',',	'.',	'/',	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	HOLE,	SHIFTKEYS+CAPSLOCK,
/*120 */	BUCKYBITS+METABIT,
			' ',	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	ERROR,	IDLE,
};

/* Shifted keyboard table for Sun-3 keyboard */

static struct keymap keytab_s3_uc = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	LF(2),	HOLE,	TF(1),	TF(2),	HOLE,
/*  8 */	TF(3), 	HOLE,	TF(4),	HOLE,	TF(5),	HOLE,	TF(6),	HOLE,
/* 16 */	TF(7),	TF(8),	TF(9),	ALT,	HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	HOLE,	HOLE,	c('['),	'!',    '@',
/* 32 */	'#',	'$',	'%',	'^',	'&',	'*',	'(',	')',
/* 40 */	'_',	'+',	'~',	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	HOLE,	LF(5),	HOLE,	LF(6),	HOLE,	'\t',	'Q',	'W',
/* 56 */	'E',	'R',	'T',	'Y',	'U',	'I',	'O',	'P',
/* 64 */	'{',	'}',	0x7F,	HOLE,	RF(7),	STRING+UPARROW,
								RF(9),	HOLE,
/* 72 */	LF(7),	LF(8),	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
							'A', 	'S',	'D',
/* 80 */	'F',	'G',	'H',	'J',	'K',	'L',	':',	'"',
/* 88 */	'|',	'\r',	HOLE,	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								HOLE,	LF(9),
/* 96 */	LF(15),	LF(10),	HOLE,	SHIFTKEYS+LEFTSHIFT,
						'Z',	'X',	'C',	'V',
/*104 */	'B',	'N',	'M',	'<',	'>',	'?',	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	HOLE,	SHIFTKEYS+CAPSLOCK,
/*120 */	BUCKYBITS+METABIT,
			' ',	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	ERROR,	IDLE,
};


/* Caps Locked keyboard table for Sun-3 keyboard */

static struct keymap keytab_s3_cl = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	LF(2),	HOLE,	TF(1),	TF(2),	HOLE,
/*  8 */	TF(3), 	HOLE,	TF(4),	HOLE,	TF(5),	HOLE,	TF(6),	HOLE,
/* 16 */	TF(7),	TF(8),	TF(9),	ALT,	HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	HOLE,	HOLE,	c('['),	'1',	'2',
/* 32 */	'3',	'4',	'5',	'6',	'7',	'8',	'9',	'0',
/* 40 */	'-',	'=',	'`',	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	HOLE,	LF(5),	HOLE,	LF(6),	HOLE,	'\t',	'Q',	'W',
/* 56 */	'E',	'R',	'T',	'Y',	'U',	'I',	'O',	'P',
/* 64 */	'[',	']',	0x7F,	HOLE,	RF(7),	STRING+UPARROW,
								RF(9),	HOLE,
/* 72 */	LF(7),	LF(8),	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
							'A', 	'S',	'D',
/* 80 */	'F',	'G',	'H',	'J',	'K',	'L',	';',	'\'',
/* 88 */	'\\',	'\r',	HOLE,	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								HOLE,	LF(9),
/* 96 */	LF(15),	LF(10),	HOLE,	SHIFTKEYS+LEFTSHIFT,
						'Z',	'X',	'C',	'V',
/*104 */	'B',	'N',	'M',	',',	'.',	'/',	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	HOLE,	SHIFTKEYS+CAPSLOCK,
/*120 */	BUCKYBITS+METABIT,
			' ',	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	ERROR,	IDLE,
};

/* Controlled keyboard table for Sun-3 keyboard */

static struct keymap keytab_s3_ct = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	LF(2),	HOLE,	TF(1),	TF(2),	HOLE,
/*  8 */	TF(3), 	HOLE,	TF(4),	HOLE,	TF(5),	HOLE,	TF(6),	HOLE,
/* 16 */	TF(7),	TF(8),	TF(9),	ALT,	HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	HOLE,	HOLE,	c('['),	'1',	c('@'),
/* 32 */	'3',	'4',	'5',	c('^'),	'7',	'8',	'9',	'0',
/* 40 */	c('_'),	'=',	c('^'),	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	HOLE,	LF(5),	HOLE,	LF(6),	HOLE,	'\t',   c('q'),	c('w'),
/* 56 */	c('e'),	c('r'),	c('t'),	c('y'),	c('u'),	c('i'),	c('o'),	c('p'),
/* 64 */	c('['),	c(']'),	0x7F,	HOLE,	RF(7),	STRING+UPARROW,
								RF(9),	HOLE,
/* 72 */	LF(7),	LF(8),	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
							c('a'),	c('s'),	c('d'),
/* 80 */	c('f'),	c('g'),	c('h'),	c('j'),	c('k'),	c('l'),	';',	'\'',
/* 88 */	c('\\'),
			'\r',	HOLE,	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								HOLE,	LF(9),
/* 96 */	LF(15),	LF(10),	HOLE,	SHIFTKEYS+LEFTSHIFT,
						c('z'),	c('x'),	c('c'),	c('v'),
/*104 */	c('b'),	c('n'),	c('m'),	',',	'.',	c('_'),	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	HOLE,	SHIFTKEYS+CAPSLOCK,
/*120 */	BUCKYBITS+METABIT,
			c(' '),	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	ERROR,	IDLE,
};



/* "Key Up" keyboard table for Sun-3 keyboard */

static struct keymap keytab_s3_up = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	OOPS,	HOLE,	OOPS,	OOPS,	HOLE,
/*  8 */	OOPS, 	HOLE, 	OOPS,	HOLE,	OOPS,	HOLE,	OOPS,	HOLE,
/* 16 */	OOPS, 	OOPS, 	OOPS,	OOPS,	HOLE,	OOPS,	OOPS,	NOP,
/* 24 */	HOLE, 	OOPS, 	OOPS,	HOLE,	HOLE,	NOP,	NOP,	NOP,
/* 32 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 40 */	NOP,	NOP,	NOP,	NOP,	HOLE,	OOPS,	OOPS,	NOP,
/* 48 */	HOLE,	OOPS,	HOLE,	OOPS,	HOLE,	NOP,	NOP,	NOP,
/* 56 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 64 */	NOP,	NOP,	NOP,	HOLE,	OOPS,	OOPS,	NOP,	HOLE,
/* 72 */	OOPS,	OOPS,	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
							NOP, 	NOP,	NOP,
/* 80 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 88 */	NOP,	NOP,	HOLE,	OOPS,	OOPS,	NOP,	HOLE,	OOPS,
/* 96 */	OOPS,	OOPS,	HOLE,	SHIFTKEYS+LEFTSHIFT,
						NOP,	NOP,	NOP,	NOP,
/*104 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	SHIFTKEYS+RIGHTSHIFT,
									NOP,
/*112 */	OOPS,	OOPS,	NOP,	HOLE,	HOLE,	HOLE,	HOLE,	NOP,
/*120 */	BUCKYBITS+METABIT,
			NOP,	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	HOLE,	RESET,
};

/* Index to keymaps for Sun-3 keyboard */
static struct keyboard keyindex_s3 = {
	&keytab_s3_lc,
	&keytab_s3_uc,
	&keytab_s3_cl,
	&keytab_s3_ct,
	&keytab_s3_up,
	0x0000,		/* Shift bits which stay on with idle keyboard */
	0x0000,		/* Bucky bits which stay on with idle keyboard */
	1,	77,	/* abort keys */
	CAPSMASK,	/* Shift bits which toggle on down event */
};

/***************************************************************************/
/*   Index table for the whole shebang					   */
/***************************************************************************/
int nkeytables = 4;	/* max 16 */
struct keyboard *keytables[] = {
	&keyindex_ms,
	&keyindex_vt,
	&keyindex_s2,
	&keyindex_s3,
};

/* 
	Keyboard String Table

 	This defines the strings sent by various keys (as selected in the
	tables above).

	The first byte of each string is its length, the rest is data.
*/

#define	kstescinit(c)	{'\033', '[', 'c', '\0'}
char keystringtab[16][KTAB_STRLEN] = {
	kstescinit(H) /*home*/,
	kstescinit(A) /*up*/,
	kstescinit(B) /*down*/,
	kstescinit(D) /*left*/,
	kstescinit(C) /*right*/,
};
