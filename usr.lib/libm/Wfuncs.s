        .data
        .asciz  "@(#)Wfuncs.s 1.1 86/09/25 Copyr 1986 Sun Micro"
        .even
        .text

|       Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Wdefs.h"

RTENTRY(Wexps)
	fpmoves	d0,fpa0
	fpetoxs	fpa0,fpa0
	fpmoves	fpa0,d0
	RET
RTENTRY(Wexp1s)
	fpmoves	d0,fpa0
	fpetoxm1s	fpa0,fpa0
	fpmoves	fpa0,d0
	RET
RTENTRY(Wlogs)
	fpmoves	d0,fpa0
	fplogns	fpa0,fpa0
	fpmoves	fpa0,d0
	RET
RTENTRY(Wlog1s)
	fpmoves	d0,fpa0
	fplognp1s	fpa0,fpa0
	fpmoves	fpa0,d0
	RET
RTENTRY(Wsins)
	fpmoves	d0,fpa0
	fpsins	fpa0,fpa0
	fpmoves	fpa0,d0
	RET
RTENTRY(Wcoss)
	fpmoves	d0,fpa0
	fpcoss	fpa0,fpa0
	fpmoves	fpa0,d0
	RET
RTENTRY(Watans)
	fpmoves	d0,fpa0
	fpatans	fpa0,fpa0
	fpmoves	fpa0,d0
	RET
RTENTRY(Wpow2s)
	ftwotoxs	d0,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Wpow10s)
	ftentoxs	d0,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Wlog2s)
	flog2s	d0,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Wlog10s)
	flog10s	d0,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Wtans)
	ftans	d0,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Wasins)
	fasins	d0,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Wacoss)
	facoss	d0,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Wsinhs)
	fsinhs	d0,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Wcoshs)
	fcoshs	d0,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Wtanhs)
	ftanhs	d0,fp0
	fmoves	fp0,d0
	RET
RTENTRY(Wpows)
	jsr	Mpows
	RET
