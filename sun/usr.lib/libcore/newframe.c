#ifndef lint
static char sccsid[] = "@(#)newframe.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
#include "coretypes.h"
#include "corevars.h"

new_frame()
{
    int index;
    _core_critflag++;
    /*
     * set all currently selected view surfaces' repaint flags
     */
    for (index = 0; index < MAXVSURF; index++) {
	 _core_surface[index].nwframnd = _core_surface[index].selected;
	}
    _core_repaint(TRUE);
    if ( --_core_critflag ==0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle)();
    return(0);
    }

