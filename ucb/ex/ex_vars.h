/*	@(#)ex_vars.h 1.1 86/09/25 SMI; from UCB 7.5 8/29/85	*/

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#define AUTOINDENT      0
#define AUTOPRINT       1
#define AUTOWRITE       2
#define BEAUTIFY        3
#define DIRECTORY       4
#define EDCOMPATIBLE    5
#define ERRORBELLS      6
#define HARDTABS        7
#define IGNORECASE      8
#define LISP            9
#define LIST            10
#define MAGIC           11
#define MESG            12
#define MODELINE        13
#define NUMBER          14
#define OPEN            15
#define OPTIMIZE        16
#define PARAGRAPHS      17
#define PROMPT          18
#define READONLY        19
#define REDRAW          20
#define REMAP           21
#define REPORT          22
#define SCROLL          23
#define SECTIONS        24
#define SHELL           25
#define SHIFTWIDTH      26
#define SHOWMATCH       27
#define SLOWOPEN        28
#define SOURCEANY       29
#define TABSTOP         30
#define TAGLENGTH       31
#define TAGS            32
#ifdef TAG_STACK
#define TAGSTACK        33
#define TERM            34
#define TERSE           35
#define TIMEOUT         36
#define TTYTYPE         37
#define WARN            38
#define WINDOW          39
#define WRAPSCAN        40
#define WRAPMARGIN      41
#define WRITEANY        42

#define	NOPTS	43
#else
#define TERM            33
#define TERSE           34
#define TIMEOUT         35
#define TTYTYPE         36
#define WARN            37
#define WINDOW          38
#define WRAPSCAN        39
#define WRAPMARGIN      40
#define WRITEANY        41

#define	NOPTS	42
#endif
