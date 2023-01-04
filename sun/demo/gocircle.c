#ifndef lint
static char sccsid[]= "@(#)gocircle.c 1.3 87/01/11 Copyr 1986 Sun Micro";
#endif

/* Copyright (c) Sun MicroSystems, 1986.  This code may be used and modified
    for not-for-profit use.  This notice must be retained.
*/

/* Draw circles (disks) to represent go stones.
*/

#include <sunwindow/window_hs.h>


Disk (pw, Radius, CtrX, CtrY, Value)
    struct pixwin *pw;
    int Radius, CtrX, CtrY, Value;
{
int x, y, d;
    x= 0;
    y= Radius;
    d= 3 - 2 * Radius;
    while (x < y) {
	DiskPoints (pw, CtrX, CtrY, x, y, Value);
	if (d < 0) d= d + (4 * x) + 6;
	else {
	    d= d + (4 * (x - y)) + 10;
	    y--;
	}
	x++;
    }
    if (x == y) DiskPoints (pw, CtrX, CtrY, x, y, Value);
} /* Disk */


DiskPoints (pw, CtrX, CtrY, x, y, Color)
    struct pixwin *pw;
    int CtrX, CtrY, x, y, Color;
{
    pw_vector (pw, CtrX - x, CtrY - y, CtrX + x, CtrY - y, PIX_SRC, Color);
    pw_vector (pw, CtrX - x, CtrY + y, CtrX + x, CtrY + y, PIX_SRC, Color);
    pw_vector (pw, CtrX - y, CtrY - x, CtrX + y, CtrY - x, PIX_SRC, Color);
    pw_vector (pw, CtrX - y, CtrY + x, CtrX + y, CtrY + x, PIX_SRC, Color);
} /* DiskPoints */


DiskPR (pr, Radius, CtrX, CtrY, Value)
    struct pixrect *pr;
    int Radius, CtrX, CtrY, Value;
{
int x, y, d;
    x= 0;
    y= Radius;
    d= 3 - 2 * Radius;
    while (x < y) {
	DiskPointsPR (pr, CtrX, CtrY, x, y, Value);
	if (d < 0) d= d + (4 * x) + 6;
	else {
	    d= d + (4 * (x - y)) + 10;
	    y--;
	}
	x++;
    }
    if (x == y) DiskPointsPR (pr, CtrX, CtrY, x, y, Value);
} /* DiskPR */


DiskPointsPR (pr, CtrX, CtrY, x, y, Color)
    struct pixrect *pr;
    int CtrX, CtrY, x, y, Color;
{
    pr_vector (pr, CtrX - x, CtrY - y, CtrX + x, CtrY - y, PIX_SRC, Color);
    pr_vector (pr, CtrX - x, CtrY + y, CtrX + x, CtrY + y, PIX_SRC, Color);
    pr_vector (pr, CtrX - y, CtrY - x, CtrX + y, CtrY - x, PIX_SRC, Color);
    pr_vector (pr, CtrX - y, CtrY + x, CtrX + y, CtrY + x, PIX_SRC, Color);
} /* DiskPointsPR */
