|
|	@(#)bres.s 2.3 83/09/16 Copyright (c) 1983 by Sun Microsystems, Inc.
|

| TITLE
| Bresenham routine

| HISTORY
|	created May 31, 1982		V. Pratt

|	bugfix dbge -> dbgt fixes 2-point vectors which are plotted on
|	a diagonal instead of straight.  Made by JCGilmore on VRPratt's advice.
|	(For Rev D, 11August 1982)

| REMARKS
| Compilation of this routine is completed at run time
| Hence it must be executed in writable storage
| The constants BRESIZ (number of shorts in this code), BRESMINOR (location of
| the minor command relative to the start, in shorts), and BRESMAJOR (ditto
| for the major command), must be kept up to date in dpy.h

	.text
	.globl	_bres, minor, major
_bres:
minor:	.word	0		| step 1 in minor direction
major:	.word	0		| plot 1 in major direction
	addw	d6,d4		| count up minor
	dbgt	d5,major	| if no overflow, keep plotting
	subw	d7,d4		| on overflow, reset counter
	tstw	d5		| catch d5 underflow
	dblt	d5,minor	| and do both minor and major
	rts			| if exhausted, exit
