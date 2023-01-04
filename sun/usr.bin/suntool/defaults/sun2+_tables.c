#ifndef lint
static char sccsid[] = "@(#)sun2+_tables.c 1.3 87/01/07 Copyright 1986 Sun Micro";
#endif
/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 *
 * sun2+_tables.c:    keytables for the Sun-2 and Sun-3 keyboards
 */


#include "setkey_tables.h"

static Key_info	 sun2_standard = {
    0, 0, 0, 0
};

static Pair	sun2_standard_noarrow_downs[] = {
		    {69, RF( 8)},
    { 91, RF(10)},		  {93, RF(12)},
		   {113, RF(14)}
};

static Key_info	 sun2_standard_noarrow = {
    sun2_standard_noarrow_downs,
    sizeof(sun2_standard_noarrow_downs) / sizeof(Pair),
    0, 0
};

static Pair	sun2_lefty_downs[] = {
    {  1, BUCKYBITS + SYSTEMBIT},		/* Left function pad */
		    { 2, LF(11)}, { 3, RF( 1)},
    { 25, RF( 6)},  {26, RF( 4)}, {27, LF(12)},
    { 49, RF( 9)},  {50, LF(13)}, {51, RF( 7)},
    { 72, RF(12)},  {73, RF(10)}, {74, LF(14)},
    { 95, RF(15)},  {96, LF(15)}, {97, RF(13)},				
						/* Right function pad */
    { 21, LF( 2)},  {22, STRING+LEFTARROW},
				  {23, BUCKYBITS + SYSTEMBIT},
    { 45, LF( 4)},  {46, STRING+RIGHTARROW},
				  {47, LF( 3)},
    { 68, LF( 6)},  {69, STRING+UPARROW},
				  {70, LF( 5)},
    { 91, LF( 8)},  {92, RF(11)}, {93, LF( 7)},
    {112, LF(10)}, {113, STRING+DOWNARROW},
				  {114, LF(9)}
};

static Pair	sun2_lefty_ups[] = {
    {111, SHIFTKEYS+RIGHTCTRL}			/* "Line Feed" => CTRL	*/

};

static Key_info	 sun2_lefty = {
    sun2_lefty_downs, sizeof(sun2_lefty_downs) / sizeof(Pair),
    sun2_lefty_ups, sizeof(sun2_lefty_ups) / sizeof(Pair)
};

static Pair	sun2_left_noarrow_downs[] = {
    {  1, BUCKYBITS + SYSTEMBIT},		/* Left function pad */
		    { 2, LF(11)}, { 3, RF( 1)},
    { 25, RF( 6)},  {26, RF( 4)}, {27, LF(12)},
    { 49, RF( 9)},  {50, LF(13)}, {51, RF( 7)},
    { 72, RF(12)},  {73, RF(10)}, {74, LF(14)},
    { 95, RF(15)},  {96, LF(15)}, {97, RF(13)},				
						/* Right function pad */
    { 21, LF( 2)},  {22, RF( 2)}, {23, BUCKYBITS + SYSTEMBIT},
    { 45, LF( 4)},  {46, RF( 5)}, {47, LF( 3)},
    { 68, LF( 6)},  {69, RF( 8)}, {70, LF( 5)},
    { 91, LF( 8)},  {92, RF(11)}, {93, LF( 7)},
    {112, LF(10)}, {113, RF(14)}, {114, LF(9)},
};

static Pair	sun2_left_noarrow_ups[] = {
    {111, SHIFTKEYS+RIGHTCTRL}			/* "Line Feed" => CTRL	*/

};

static Key_info	 sun2_left_noarrow = {
    sun2_left_noarrow_downs, sizeof(sun2_left_noarrow_downs) / sizeof(Pair),
    sun2_left_noarrow_ups, sizeof(sun2_left_noarrow_ups) / sizeof(Pair)
};

Keybd_info	ktbl_sun2_info = {
    &sun2_standard, &sun2_standard_noarrow, &sun2_lefty, &sun2_left_noarrow
};

Keybd_info	ktbl_sun3_info = {
    &sun2_standard, &sun2_standard_noarrow, &sun2_lefty, &sun2_left_noarrow
};

