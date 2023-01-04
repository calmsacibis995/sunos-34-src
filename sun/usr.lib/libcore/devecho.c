#ifndef lint
static char sccsid[] = "@(#)devecho.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"
#define	NULL	0

char *gcvt();
static int ndig = 8;
static char buf[80];
static double val;
static porttype ndc = {0,0,0,0,0,0};
/*---------------------------------------------------------------------*/
_core_devecho( class, dev, x, y, string, value)
int class, dev; int x, y; unsigned char *string;  float value;
{
    ipt_type ip1, ip2;
    viewsurf *surfp;

    ip2.x = x;  ip2.y = y;
    ip2.z = 0;  ip2.w = 1;	/* x,y is in NDC  */
    ip1 = ip2;
    switch (class) {
    case PICK:
	switch (_core_pick[dev].subpick.echo) {
	case 0: break;				/* no echo */
	case1:					/* shape at locator */
	case2:		/* Now done by window sys routines */
		break;
	default:
		break;
	}
	break;
    case KEYBOARD:
	switch (_core_keybord[dev].subkey.echo) {
	case 0: break;				/* no echo */
	case 1:					/* string at ref pt */
	    ip1.x = _core_keybord[dev].subkey.echopos[0];
	    ip1.y = _core_keybord[dev].subkey.echopos[1];
	    echotext( ip1.x, ip1.y, string, (int)value,
			_core_keybord[dev].subkey.echosurfp);
	default:
	    break;
	}
	break;
    case BUTTON:
	break;
    case LOCATOR:
	surfp = _core_locator[dev].subloc.echosurfp; /* set vwsurface driver*/
	ip1.x = _core_locator[dev].subloc.echopos[0];
	ip1.y = _core_locator[dev].subloc.echopos[1];
	switch (_core_locator[dev].subloc.echo) {
	case 0: break;					/* no echo */
	case 1:						/* shape at locator */
			/* Now done by window sys routines */
	    break;
	case 2:						/* rubber line */
	    echovec( ip1.x, ip1.y, ip2.x, ip2.y, surfp); break;
	case 3:						/* rubber horiz line */
	    echovec( ip1.x, ip1.y, ip2.x, ip1.y, surfp); break;
	case 4:						/* rubber vert line */
	    echovec( ip1.x, ip1.y, ip1.x, ip2.y, surfp); break;
	case 5:						/* rubber longest xy */
	    if (abs(ip2.x-ip1.x) > abs(ip2.y-ip1.y))
		echovec( ip1.x, ip1.y, ip2.x, ip1.y, surfp);
	    else 
		echovec( ip1.x, ip1.y, ip1.x, ip2.y, surfp);
	    break;
	case 6:						/* rubber box */
	    echobox( ip1.x, ip1.y, ip2.x, ip2.y, surfp); break;
	default:
	    break;
	}
	break;
    case STROKE:
	switch (_core_stroker[dev].substroke.echo) {
	case 0: break;					/* no echo */
	case 1:						/* shape at locator */
			/* Now done by window sys routines */
	    break;
	default:
	    break;
	}
	break;
    case VALUATOR:
	switch (_core_valuatr[dev].subval.echo) {
	case 0: break;					/* no echo */
	case 1:						/* + at locator */
	    ip1.x = _core_valuatr[dev].subval.echopos[0];
	    ip1.y = _core_valuatr[dev].subval.echopos[1];
	    val = value;
	    echotext( ip1.x, ip1.y, gcvt(val,ndig,buf), 0,
			_core_valuatr[dev].subval.echosurfp);
	    break;
	default:
	    break;
	}
    default: break;
    }
}
   
/*--------------------------------------*/
static echotext( x0, y0, string, pos, surfp)
int x0, y0, pos; char *string; viewsurf *surfp;
{
    int i, j;
    ddargtype ddstruct;

    if (surfp == NULL)
	return(1);
    ddstruct.instance = surfp->vsurf.instance;
    ddstruct.opcode = ECHOMOVE;			/* move to echopos */
    ddstruct.int1 = x0;  ddstruct.int2 = y0;
    (*surfp->vsurf.dd)(&ddstruct);
    ddstruct.opcode = TEXT;			/* write using raster font */
    ddstruct.ptr1 = string;
    ndc.xmax = _core_ndcspace[0];
    ndc.ymax = _core_ndcspace[1];
    ndc.zmax = _core_ndcspace[2];
    ddstruct.ptr2 = (char*)&ndc;
    ddstruct.int1 = 1;	
    ddstruct.int2 = pos;	
    (*surfp->vsurf.dd)(&ddstruct);

    _core_cpchang = TRUE;

    return(0);
}
/*--------------------------------------*/
static echovec( x0, y0, x1, y1, surfp)
int x0, y0, x1, y1; viewsurf *surfp;
{
    int i, j;
    ddargtype ddstruct;

    if (surfp == NULL)
	return(1);
    ddstruct.instance = surfp->vsurf.instance;
    ddstruct.opcode = SETLCOL;			/* set xor on */
    ddstruct.int1 = 255;
    ddstruct.int2 = XORROP;
    ddstruct.int3 = FALSE;
    (*surfp->vsurf.dd)(&ddstruct);
    ddstruct.opcode = ECHOMOVE;
    ddstruct.int1 = x0;
    ddstruct.int2 = y0;
    (*surfp->vsurf.dd)(&ddstruct);
    ddstruct.opcode = ECHOLINE;
    ddstruct.int1 = x1;
    ddstruct.int2 = y1;
    (*surfp->vsurf.dd)(&ddstruct);

    _core_cpchang = TRUE;			/* restore ddcp to original */
    ddstruct.opcode = SETLCOL;			/* restore xor to original */
    ddstruct.int1 = _core_current.lineindx;
    ddstruct.int2 = (_core_xorflag)?XORROP:_core_current.rasterop;
    ddstruct.int3 = FALSE;
    (*surfp->vsurf.dd)(&ddstruct);

    return(0);
}
/*--------------------------------------*/
static echobox( x0, y0, x1, y1, surfp)
int x0, y0, x1, y1; viewsurf *surfp;
{
    int i, j;
    ddargtype ddstruct;

    if (surfp == NULL)
	return(1);
    ddstruct.instance = surfp->vsurf.instance;
    ddstruct.opcode = SETLCOL;			/* set xor on */
    ddstruct.int1 = 255;
    ddstruct.int2 = XORROP;
    (*surfp->vsurf.dd)(&ddstruct);
    ddstruct.opcode = ECHOMOVE;			/* xor the box */
    ddstruct.int1 = x0;
    ddstruct.int2 = y0;
    (*surfp->vsurf.dd)(&ddstruct);
    ddstruct.opcode = ECHOLINE;
    ddstruct.int1 = x0;
    ddstruct.int2 = y1;
    (*surfp->vsurf.dd)(&ddstruct);
    ddstruct.opcode = ECHOLINE;
    ddstruct.int1 = x1;
    ddstruct.int2 = y1;
    (*surfp->vsurf.dd)(&ddstruct);
    ddstruct.opcode = ECHOLINE;
    ddstruct.int1 = x1;
    ddstruct.int2 = y0;
    (*surfp->vsurf.dd)(&ddstruct);
    ddstruct.opcode = ECHOLINE;
    ddstruct.int1 = x0;
    ddstruct.int2 = y0;
    (*surfp->vsurf.dd)(&ddstruct);

    _core_cpchang = TRUE;			/* restore ddcp to original */
    ddstruct.opcode = SETLCOL;			/* restore xor to original */
    ddstruct.int1 = _core_current.lineindx;
    ddstruct.int2 = (_core_xorflag)?XORROP:_core_current.rasterop;
    ddstruct.int3 = FALSE;
    (*surfp->vsurf.dd)(&ddstruct);

    return(0);
}
