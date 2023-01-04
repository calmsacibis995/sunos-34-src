#ifndef lint
static char sccsid[] = "@(#)inqtextent.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */
#include "coretypes.h"
#include "corevars.h"

/************************************************************************/
/*									*/
/*	Function: inquire_text_extent_3					*/
/*									*/
/*	Purpose: find the current text extent in world coords		*/
/************************************************************************/

inquire_text_extent_3( s, dx, dy, dz) char *s; float *dx,*dy,*dz;
{
    short xmin, xmax;			/* char extent in char coords */
    char ch;
    short npts, font;
    pt_type p2;

    font = _core_current.font;
    p2 = _core_current.chrpath;		/* scale path vec by width of chars */
    _core_scalept(_core_current.charsize.width / 16., &p2);

    *dx = *dy = *dz = 0.0;
    while (ch = *s++) {			/* for all chars in string */
	if (font) _core_scribextent(font-1, ch, &xmin, &xmax, &npts);
	else _core_sfontextent(0, ch, &xmin, &xmax, &npts);
	*dx += (xmax-xmin) * p2.x + _core_current.chrspace.x;
	*dy += (xmax-xmin) * p2.y + _core_current.chrspace.y;
	*dz += (xmax-xmin) * p2.z + _core_current.chrspace.z;
    }
    return(0);
}

/************************************************************************/
/*									*/
/*	Function: inquire_text_extent_2					*/
/*									*/
/*	Purpose: find the current text extent in world coords		*/
/************************************************************************/

inquire_text_extent_2( s, dx, dy) char *s; float *dx,*dy;
{
    short xmin, xmax;
    char ch;
    short npts, font;
    pt_type p2;

    font = _core_current.font;
    p2 = _core_current.chrpath;		/* scale path vec by width of chars */
    _core_scalept(_core_current.charsize.width / 16, &p2);

    *dx = *dy = 0.0;
    while (ch = *s++) {			/* for all chars in string */
	if (font) _core_scribextent(font-1, ch, &xmin, &xmax, &npts);
	else _core_sfontextent( 0, ch, &xmin, &xmax, &npts);
	*dx += (xmax-xmin) * p2.x + _core_current.chrspace.x;
	*dy += (xmax-xmin) * p2.y + _core_current.chrspace.y;
    }
    return(0);
}

