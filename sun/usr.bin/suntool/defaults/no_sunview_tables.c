#ifndef lint
static char sccsid[] = "@(#)no_sunview_tables.c 1.1 86/09/25";
#endif
/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 *
 * no_sunview_tables.c:    keytables for the Sun-2 / 3 keyboard without
 *				SunView function keys
 */

#include "setkey_tables.h"

static Pair		no_sunview_downs[] = {
    {  1, BUCKYBITS + SYSTEMBIT},		/* Left function pad */
		    { 3, TF(11)},
    { 25, LF(12)},  {26, TF(12)},
    { 49, LF(13)},  {51, TF(13)},
    { 72, LF(14)},  {73, TF(14)},
    { 95, LF(15)},  {97, TF(15)},
    
    {  5, LF(11)}				/* F1 (Caps)		*/
};

static Key_info		no_sunview_tables = {
    no_sunview_downs, sizeof(no_sunview_downs) / sizeof(Pair), 0, 0
};

Keybd_info		ktbl_no_sunview_info = {
    &no_sunview_tables, 0, 0, 0
};
