#ifndef lint
static char sccsid[]= "@(#)gomaster.c 1.1 87/01/11 Copyr 1986 Sun Micro";
#endif

/* Copyright (c) Sun MicroSystems, 1986.  This code may be used and modified
    for not-for-profit use.  This notice must be retained.
*/

/* gomaster.c
    Default Game from "Master of Go" by Yasunari Kawabata.
*/

#include "goban.h"
#include <suntool/panel.h>

extern Panel_item	CurrMoveSlider, HandicapSlider, NameButton;


BookMove (i, j, Move, Color, Game)
    int i, j;
    MoveType *Move;
    Stone *Color;
    GameType *Game;
{
int NumCaptures;
Stone c;
    c= *Color;
    AllocMove (i, j, Move);
    Game->Board [i] [j]= c;
    NewStoneToGroups (Game, i, j);
    NumCaptures= GenCaptures (Game, !c, Move);
    Game->NumMoves++;
    if (c == Black) *Color= White;
    else *Color= Black;
} /* BookMove */


DfltGame (GameName, Game, ShouldPaint)
    char *GameName;    GameType *Game;
    Panel_setting ShouldPaint;
/* Fills in a Game structure with the game from the book "Master of Go"
    by Yasunari Kawabata.
*/
{
MoveType *Move;
Stone Color;
int i, j, NumCaptures;
    strcpy (GameName, "masterofgo");
    panel_set (NameButton, PANEL_VALUE, GameName, 0);
    Game->Grid= 19;
    Game->Handicap= 0;
    ZeroGame (Game, ShouldPaint);
    strncpy (Game->Title, GameName, MaxTitleChars-1);
    for (i= 0; i < Game->Grid; i++)
	for (j= 0; j < Game->Grid; j++) {
	    Game->Board [i] [j]= Empty;
	}
    Game->FirstPlayer= Color= Black;
    Game->MoveList= Move= AllocMove (i=16, j=15, (MoveType *) NULL);
    Game->Board [i][j]= Color;
    NewStoneToGroups (Game, i, j);
    NumCaptures= GenCaptures (Game, !Color, Move);
    Game->NumMoves++;
    Color= White;
#define BM(i,j)	BookMove (i, j, Move, &Color, Game); Move= Move->Next
    BM (16, 3);
    BM (3, 2);
    BM (2, 4);
    BM (4, 3);
    BM (14, 15);
    BM (14, 16);
    BM (13, 16);
    BM (15, 16);
    BM (12, 15);
    BM (13, 14);
    BM (14, 14);
    BM (13, 17);
    BM (13, 13);
    BM (12, 17);
    BM (13, 2);
    BM (15, 13);
    BM (2, 14);
    BM (2, 16);
    BM (4, 15);
    BM (4, 16);
    BM (5, 16);
    BM (5, 15);
    BM (6, 15);
    BM (5, 14);
    BM (4, 14);
    BM (6, 16);
    BM (5, 17);
    BM (4, 17);
    BM (6, 17);
    BM (5, 13);
    BM (7, 16);
    BM (4, 13);
    BM (2, 15);
    BM (1, 16);
    BM (2, 11);
    BM (3, 10);
    BM (2, 10);
    BM (3, 9);
    BM (2, 9);
    BM (3, 8);
    BM (2, 8);
    BM (3, 7);
    BM (2, 6);
    BM (3, 5);
    BM (2, 5);
    BM (3, 6);
    BM (9, 3);
    BM (9, 16);
    BM (9, 14);
    BM (8, 14);
    BM (9, 15);
    BM (8, 15);
    BM (8, 16);
    BM (10, 16);
    BM (8, 13);
    BM (7, 14);
    BM (9, 13);
    BM (7, 13);
    BM (9, 17);
    BM (10, 17);
    BM (8, 18);
    BM (13, 4);
    BM (13, 3);
    BM (11, 4);
    BM (11, 3);
    BM (11, 14);
    BM (11, 13);
    BM (10, 13);
    BM (12, 14);
    BM (11, 12);
    BM (12, 13);
    BM (10, 15);
    BM (10, 12);
    BM (10, 14);
    BM (11, 11);
    BM (9, 12);
    BM (12, 12);
    BM (15, 11);
    BM (9, 11);
    BM (8, 12);
    BM (16, 7);
    BM (16, 9);
    BM (17, 9);
    BM (17, 10);
    BM (14, 7);
    BM (17, 8);
    BM (10, 4);
    BM (7, 2);
    BM (7, 3);
    BM (6, 3);
    BM (6, 2);
    BM (7, 1);
    BM (8, 2);
    BM (7, 4);
    BM (8, 3);
    BM (6, 1);
    BM (10, 7);
    BM (10, 10);
    BM (10, 11);
    BM (17, 6);
    BM (17, 7);
    BM (18, 7);
    BM (16, 6);
    BM (17, 5);
    BM (16, 4);
    BM (1, 3);
    BM (3, 12);
    BM (1, 15);
    BM (1, 14);
    BM (3, 16);
    BM (6, 4);
    BM (5, 2);
    BM (7, 5);
    BM (14, 9);
    BM (13, 9);
    BM (13, 10);
    BM (12, 9);
    BM (14, 8);
    BM (13, 8);
    BM (7, 15);
    BM (7, 17);
    BM (16, 5);
    BM (15, 5);
    BM (18, 3);
    BM (17, 2);
    BM (12, 10);
    BM (11, 10);
    BM (13, 7);
    BM (17, 4);
    BM (13, 6);
    BM (14, 6);
    BM (11, 9);
    BM (12, 8);
    BM (11, 8);
    BM (12, 7);
    BM (11, 7);
    BM (12, 6);
    BM (11, 6);
    BM (12, 5);
    BM (11, 5);
    BM (12, 4);
    BM (9, 9);
    BM (8, 1);
    BM (18, 4);
    BM (18, 2);
    BM (18, 6);
    BM (1, 4);
    BM (1, 2);
    BM (8, 8);
    BM (8, 9);
    BM (7, 9);
    BM (8, 10);
    BM (9, 8);
    BM (7, 7);
    BM (7, 8);
    BM (5, 4);
    BM (6, 5);
    BM (6, 7);
    BM (8, 7);
    BM (2, 7);
    BM (1, 7);
    BM (13, 11);
    BM (12, 11);
    BM (16, 8);
    BM (14, 12);
    BM (15, 12);
    BM (4, 18);
    BM (3, 18);
    BM (5, 18);
    BM (1, 18);
    BM (7, 6);
    BM (8, 0);
    BM (9, 0);
    BM (7, 0);
    BM (9, 1);
    BM (0, 14);
    BM (0, 13);
    BM (0, 15);
    BM (2, 13);
    BM (15, 7);
    BM (15, 6);
    BM (15, 8);
    BM (13, 5);
    BM (5, 6);
    BM (5, 5);
    BM (4, 5);
    BM (0, 4);
    BM (10, 6);
    BM (9, 6);
    BM (5, 8);
    BM (3, 4);
    BM (4, 4);
    BM (7, 10);
    BM (8, 11);
    BM (5, 9);
    BM (3, 15);
    BM (4, 9);
    BM (4, 8);
    BM (3, 14);
    BM (1, 12);
    BM (1, 13);
    BM (2, 12);
    BM (1, 11);
    BM (3, 13);
    BM (0, 12);
    BM (3, 11);
    BM (5, 11);
    BM (4, 11);
    BM (2, 12);
    BM (5, 10);
    BM (6, 10);
    BM (4, 10);
    BM (10, 8);
    BM (10, 9);
    BM (6, 12);
    BM (10, 18);
    BM (9, 18);
    BM (10, 5);
    BM (9, 5);
    BM (6, 9);
    BM (6, 8);
    BM (5, 9);
    BM (6, 6);
    BM (5, 7);
    BM (0, 3);
    BM (0, 2);
    BM (2, 3);
    BM (2, 2);
    BM (14, 11);
    BM (14, 10);
    BM (15, 14);
    BM (16, 14);
    BM (13, 15);
    BM (18, 8);
    BM (11, 16);
    BM (11, 17);
    SetFirstPlayerButton (Color, ShouldPaint);
    Game->MoveNum= Game->NumMoves;
    panel_set (CurrMoveSlider
	, PANEL_MAX_VALUE, Game->NumMoves
	, PANEL_VALUE, Game->MoveNum
	, PANEL_PAINT, ShouldPaint, 0);
    panel_set (HandicapSlider, PANEL_VALUE, Game->Handicap
	, PANEL_PAINT, ShouldPaint, 0);
    if (Game->MoveList != NULL) Game->CurrMove= Move;
    return 0;
} /* DfltGame */
