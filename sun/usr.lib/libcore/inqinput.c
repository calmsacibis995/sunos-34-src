#ifndef lint
static char sccsid[] = "@(#)inqinput.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
#include "coretypes.h"
#include "corevars.h"
extern struct vwsurf _core_nullvs;

device *_core_check_pick();
keybstr *_core_check_keyboard();
strokstr *_core_check_stroke();
locatstr *_core_check_locator();
valstr *_core_check_valuator();
butnstr *_core_check_button();

/*-----------------------------------------------*/
/*----------Inquire Echoing Parameters-----------*/
/*-----------------------------------------------*/
inquire_echo( devclass, devnum, echotype)
    int devclass, devnum, *echotype;
{
    static char funcname[] = "inquire_echo";
    device *pickptr;
    keybstr *keybptr;
    strokstr *strokptr;
    locatstr *locatptr;
    valstr *valptr;
    butnstr *butnptr;

    switch (devclass) {
	case PICK:
	    if ((pickptr = _core_check_pick(funcname, devnum, TRUE)) == 0)
		return(1);
	    *echotype = pickptr->echo;
 	    break;
	case KEYBOARD:
	    if ((keybptr = _core_check_keyboard(funcname, devnum, TRUE)) == 0)
		return(1);
            *echotype = keybptr->subkey.echo;
	    break;
	case BUTTON:
	    if ((butnptr = _core_check_button(funcname, devnum, TRUE)) == 0)
		return(1);
	    *echotype = butnptr->subbut.echo;
	    break;
	case LOCATOR:
	    if ((locatptr = _core_check_locator(funcname, devnum, TRUE)) == 0)
		return(1);
            *echotype = locatptr->subloc.echo;
	    break;
	case VALUATOR:
	    if ((valptr = _core_check_valuator(funcname, devnum, TRUE)) == 0)
		return(1);
	    *echotype = valptr->subval.echo;
	    break;
	case STROKE:
	    if ((strokptr = _core_check_stroke(funcname, devnum, TRUE)) == 0)
		return(1);
	    *echotype = strokptr->substroke.echo;
	    break;
	default:
	    break;
	}
    }
/*-----------------------------------------------*/
inquire_echo_position( devclass, devnum, x, y)
    int devclass, devnum; float *x, *y;
{
    static char funcname[] = "inquire_echo_position";
    device *pickptr;
    keybstr *keybptr;
    strokstr *strokptr;
    locatstr *locatptr;
    valstr *valptr;
    butnstr *butnptr;

    switch (devclass) {
	case PICK:
	    if ((pickptr = _core_check_pick(funcname, devnum, TRUE)) == 0)
		return(1);
	    *x = pickptr->echopos[0];
	    *y = pickptr->echopos[1];
	    break;
	case KEYBOARD:
	    if ((keybptr = _core_check_keyboard(funcname, devnum, TRUE)) == 0)
		return(1);
	    *x = keybptr->subkey.echopos[0];
	    *y = keybptr->subkey.echopos[1];
	    break;
	case BUTTON:
	    if ((butnptr = _core_check_button(funcname, devnum, TRUE)) == 0)
		return(1);
	    *x = butnptr->subbut.echopos[0];
	    *y = butnptr->subbut.echopos[1];
	    break;
	case LOCATOR:
	    if ((locatptr = _core_check_locator(funcname, devnum, TRUE)) == 0)
		return(1);
	    *x = locatptr->subloc.echopos[0];
	    *y = locatptr->subloc.echopos[1];
	    break;
	case VALUATOR:
	    if ((valptr = _core_check_valuator(funcname, devnum, TRUE)) == 0)
		return(1);
	    *x = valptr->subval.echopos[0];
	    *y = valptr->subval.echopos[1];
	    break;
	case STROKE:
	    if ((strokptr = _core_check_stroke(funcname, devnum, TRUE)) == 0)
		return(1);
	    *x = strokptr->substroke.echopos[0];
	    *y = strokptr->substroke.echopos[1];
	    break;
	default:
	    break;
	}
    *x /= (float) MAX_NDC_COORD;	/* users NDC is 0..1.0 */
    *y /= (float) MAX_NDC_COORD;
    }
/*-----------------------------------------------*/
inquire_echo_surface( devclass, devnum, surfname)
    int devclass, devnum;  struct vwsurf *surfname;
{
    viewsurf *surfp;
    static char funcname[] = "inquire_echo_surface";
    device *pickptr;
    keybstr *keybptr;
    strokstr *strokptr;
    locatstr *locatptr;
    valstr *valptr;
    butnstr *butnptr;

    surfp = (viewsurf *) 0;
    switch (devclass) {
	case PICK:
	    if ((pickptr = _core_check_pick(funcname, devnum, TRUE)) == 0)
		return(1);
            surfp = pickptr->echosurfp;
 	    break;
	case KEYBOARD:
	    if ((keybptr = _core_check_keyboard(funcname, devnum, TRUE)) == 0)
		return(1);
            surfp = keybptr->subkey.echosurfp;
	    break;
	case BUTTON:
	    if ((butnptr = _core_check_button(funcname, devnum, TRUE)) == 0)
		return(1);
	    surfp = butnptr->subbut.echosurfp;
	    break;
	case LOCATOR:
	    if ((locatptr = _core_check_locator(funcname, devnum, TRUE)) == 0)
		return(1);
            surfp = locatptr->subloc.echosurfp;
	    break;
	case VALUATOR:
	    if ((valptr = _core_check_valuator(funcname, devnum, TRUE)) == 0)
		return(1);
	    surfp = valptr->subval.echosurfp;
	    break;
	case STROKE:
	    if ((strokptr = _core_check_stroke(funcname, devnum, TRUE)) == 0)
		return(1);
	    surfp = strokptr->substroke.echosurfp;
	    break;
	default:
	    break;
	}
    if (surfp)
	*surfname = surfp->vsurf;
    else
	*surfname = _core_nullvs;
    }
/*-----------------------------------------------*/
inquire_locator_2( locnum, x, y) int locnum; float *x, *y;
{
    static char funcname[] = "inquire_locator_2";
    locatstr *locatptr;

    if ((locatptr = _core_check_locator(funcname, locnum, TRUE)) == 0)
	return(1);
    *x = (float) locatptr->setpos[0] / (float) MAX_NDC_COORD;
    *y = (float) locatptr->setpos[1] / (float) MAX_NDC_COORD;
}
/*-----------------------------------------------*/
inquire_valuator( valnum, init, low, high) int valnum; float *init, *low, *high;
{
    static char funcname[] = "inquire_valuator";
    valstr *valptr;

    if ((valptr = _core_check_valuator(funcname, valnum, TRUE)) == 0)
	return(1);
    *init = valptr->vlinit;
    *low = valptr->vlmin;
    *high = valptr->vlmax;
    return(0);
}
/*-----------------------------------------------*/
inquire_stroke( strokenum, bufsize, dist, time)
    int strokenum, *bufsize, *time; float *dist;
{
    static char funcname[] = "inquire_stroke";
    strokstr *strokptr;

    if ((strokptr = _core_check_stroke(funcname, strokenum, TRUE)) == 0)
	return(1);
    *bufsize = strokptr->bufsize;
    *dist = (float) strokptr->distance / (float) MAX_NDC_COORD;
    return(0);
}
/*-----------------------------------------------*/
inquire_keyboard( keynum, bufsize, istr, pos)
    int keynum, *bufsize, *pos; char *istr;
{
    static char funcname[] = "inquire_keyboard";
    keybstr *keybptr;
    char *sptr;

    if ((keybptr = _core_check_keyboard(funcname, keynum, TRUE)) == 0)
	return(1);
    *bufsize = keybptr->bufsize;
    sptr = keybptr->initstring;
    while (*istr++ = *sptr++);
    *pos = keybptr->initpos;
    return(0);
}
