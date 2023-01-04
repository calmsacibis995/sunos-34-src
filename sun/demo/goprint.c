#ifndef lint
static char sccsid[]= "@(#)goprint.c 1.3 87/01/11 Copyr 1986 Sun Micro";
#endif

/* Copyright (c) Sun MicroSystems, 1985.  This code may be used and modified
    for not-for-profit use.  This notice must be retained.
*/

/* goprint.c
    Prints using laser-writer and PostScript.
*/

#include "goban.h"
#include <setjmp.h>

jmp_buf LWEnvironment;


PrintBoard (Game)
    GameType *Game;
/* Print board large. */
{
FILE *f;
int i, j, k, LastLine;
MoveType *Move;
int Diam;	/* Diameter of a stone. */
int Units;	/* Units per inch. */
int FontPts;
int BrdMargin, Margin, PageWidth;
Stone s;
    LastLine= Game->Grid - 1;
    Units= 72;	/* Pts. per inch. */
    FontPts= 12;
    BrdMargin= (int) (1.5 * Units);
    Margin= Units;
    PageWidth= (int) (8.5 * Units);
    Diam= ((int) (Units * 5.5)) / LastLine;
    if (setjmp (LWEnvironment)) {
	StatusMsg ("SigPipe: Couldn't find laser printer (lpr -Plw).\n");
	return 1;
    } else {
	f= popen ("lpr -Plw", "w");
	if (f == (FILE *) NULL) {
	    StatusMsg ("popen: Couldn't find laser printer (lpr -Plw).\n");
	    return 1;
	}
	fprintf (f, "%%!\n");	/* Tell lpr to pass commands to postscript. */
	fprintf (f
	 , "/cshow { dup stringwidth exch 2 div neg exch rmoveto show } def\n");
	fprintf (f, "(Times-Roman) findfont %d scalefont setfont\n", FontPts);
	for (i= 0; i < Game->Grid; i++) {
	    fprintf (f, "%d %d moveto %d %d lineto stroke\n"
		, BrdMargin + i * Diam, BrdMargin
		, BrdMargin + i * Diam, BrdMargin + LastLine * Diam);
	    fprintf (f, "%d %d moveto %d %d lineto stroke\n"
		, BrdMargin, BrdMargin + i * Diam
		, BrdMargin + LastLine * Diam, BrdMargin + i * Diam);
	    fprintf (f, "%d %d moveto (%c) show\n"
		, BrdMargin + i * Diam - FontPts / 2, Margin
		, (i < 8) ? ('A' + i) : ('B' + i));
	    fprintf (f, "%d %d moveto (%c) show\n"
		, BrdMargin + i * Diam - FontPts / 2
		, PageWidth - Margin
		, (i < 8) ? ('A' + i) : ('B' + i));
	    fprintf (f, "%d %d moveto (%d) show\n"
		, Margin, BrdMargin + i * Diam - FontPts / 2, i + 1);
	    fprintf (f, "%d %d moveto (%d) show\n", PageWidth - Margin
		, BrdMargin + i * Diam - FontPts / 2, i + 1);
	}
	if (Game->Grid == 19) {
	    for (i= 0; i < 3; i++) {
		for (j= 0; j < 3; j++) {
		    fprintf (f, "newpath %d %d %d 0 360 arc fill\n"
			, BrdMargin + (3 + i * 6) * Diam
			, BrdMargin + (3 + j * 6) * Diam, 3);
		}
	    }
	}
	fprintf (f, "newpath\n");
	for (i= 0; i < Game->Grid; i++) {
	    for (j= 0; j < Game->Grid; j++) {
		if (Game->InitialBoard [i][j] != Empty) {
		    fprintf (f, "%d %d %d 0 360 arc fill\n", BrdMargin + i *Diam
			, BrdMargin + j * Diam, Diam / 2 - Units / 72);
		    if (Game->InitialBoard [i][j] == White) {
			fprintf (f, "1 setgray %d %d %d 0 360 arc fill\n"
			    , BrdMargin + i * Diam, BrdMargin + j * Diam
			    , Diam / 2 - Units / 36);
			fprintf (f, "0 setgray\n");
		    }
		}
	    }
	}
	if (Game->MoveNum < 10) FontPts= 12;
	else if (Game->MoveNum < 100) FontPts= 10;
	else FontPts= 8;
	fprintf (f, "(Times-Roman) findfont %d scalefont setfont\n", FontPts);
	FontPts= FontPts / 3;
	s= Game->FirstPlayer;
	for (k= 0, Move= Game->MoveList; (k < Game->MoveNum) && (Move!=NULL)
		; k++){
	    i= Move->i;
	    j= Move->j;
	    if (i >= 0) {
		if (Game->Board [i] [j] == Empty) {
		    fprintf (f, "newpath 1 setgray %d %d %d 0 360 arc fill\n"
			, BrdMargin + i * Diam, BrdMargin + j * Diam
			, Diam / 2 - Units / 36);
		    fprintf (f, "0 setgray\n");
		    fprintf (f, "%d %d moveto (%d) cshow\n"
			, BrdMargin + i * Diam, BrdMargin + j * Diam - FontPts
			, k + 1);
		} else {
		    fprintf (f, "newpath %d %d %d 0 360 arc fill\n"
			, BrdMargin + i * Diam, BrdMargin + j * Diam
			, Diam / 2 - Units / 72);
		    if (Game->Board [i] [j] == White) {
			fprintf (f, "1 setgray %d %d %d 0 360 arc fill\n"
			    , BrdMargin + i * Diam, BrdMargin + j * Diam
			    , Diam / 2 - Units / 36);
			fprintf (f, "0 setgray\n");
			fprintf (f, "%d %d moveto (%d) cshow\n"
			    , BrdMargin + i * Diam
			    , BrdMargin + j * Diam - FontPts
			    , k + 1);
		    } else {
			fprintf (f, "1 setgray %d %d moveto (%d) cshow\n"
			    , BrdMargin + i * Diam
			    , BrdMargin + j * Diam - FontPts
			    , k + 1);
			fprintf (f, "0 setgray\n");
		    }
		}
	    }
	    s= !s;
	    Move= Move->Next;
	}
    }
    fprintf (f, "(Times-Roman) findfont %d scalefont setfont\n", 18);
    fprintf (f, "%d %d moveto (%s) show\n", Margin, PageWidth + Margin
	, Game->Title);
    if (Game->MoveNum & 1) s= !Game->FirstPlayer;
    else s= Game->FirstPlayer;
    fprintf (f, "%d %d moveto (%s to move.) show\n"
	, Margin, PageWidth + Margin/2, (s == Black) ? "Black" : "White");
    fprintf (f, "%d %d moveto\n", Margin, PageWidth);
    fprintf (f, "(White stones captured:  %d,  Black stones captured:  %d.)"
	, Game->Prisoners [White], Game->Prisoners [Black]);
    fprintf (f, " show\n");
    fprintf (f, "showpage\n");	/* Tell postscript to print. */
#ifdef DEBUG
#else
    pclose (f);
#endif
    return 0;
} /* PrintBoard */


SigPipe ()
/* Take care of broken pipe from lpr -Plw. */
{
    longjmp (LWEnvironment, 1);
} /* SigPipe */
