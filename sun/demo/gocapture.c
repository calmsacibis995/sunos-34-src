#ifndef lint
static char sccsid[]= "@(#)gocapture.c 1.3 87/01/11 Copyr 1986 Sun Micro";
#endif

/* Copyright (c) Sun MicroSystems, 1986.  This code may be used and modified
    for not-for-profit use.  This notice must be retained.
*/

/* capture.c
    Counts of liberties here may count a single liberty twice if two
    stones touch it.
*/

#include "goban.h"


GroupType *UniteGroups ();


CheckCaptures (Game, Color, i, j)
    GameType *Game;
    Stone Color;
    int i, j;
/* Find all unique groups of Color with no liberties.
    Color is the opposite color to the moving player.
    Returns number of stones that would be captured.
*/
{
int NumGroups, NumCaptures, k, kk, kkk;
GroupType *Captures [4];
    NumGroups= 0;
    if ((i > 0) && (Game->Board [i-1][j] == Color) &&
	(Game->Groups [i-1][j]->NumLibs <= 1))
	Captures [NumGroups++]= Game->Groups [i-1][j];
    if ((j > 0) && (Game->Board [i][j-1] == Color) &&
	(Game->Groups [i][j-1]->NumLibs <= 1))
	Captures [NumGroups++]= Game->Groups [i][j-1];
    if ((i < (Game->Grid-1)) && (Game->Board [i+1][j] == Color) &&
	(Game->Groups [i+1][j]->NumLibs <= 1))
	Captures [NumGroups++]= Game->Groups [i+1][j];
    if ((j < (Game->Grid-1)) && (Game->Board [i][j+1] == Color) &&
	(Game->Groups [i][j+1]->NumLibs <= 1))
	Captures [NumGroups++]= Game->Groups [i][j+1];
    for (k= 1; k < NumGroups; k++) {
	for (kk= 0; kk < k; kk++) {
	    if (Captures [k] == Captures [kk]) {
		for (kkk= k+1; kkk < NumGroups; kkk++) {
		    Captures [kkk-1]= Captures [kkk];
		}
		NumGroups--;
	    }
	}
    }
    for (k= 0, NumCaptures= 0; k < NumGroups; k++)
	NumCaptures += Captures [k]->NumStones;
    return NumCaptures;
} /* CheckCaptures */


CountGrpLibs (Game, i, j, Match, g, RecursionLevel)
    GameType *Game;
    int i, j;
    Stone Match;
    GroupType *g;
    int RecursionLevel;	/* Zero on first call. */
{

#define CountGrpLibs0(a,b,c) \
    if ((a) && (Checked [b][c] < CheckNum)) { \
	Checked [b][c]= CheckNum; \
	Color= Game->Board [b][c]; \
	if (Color == Match) CountGrpLibs (Game, b, c, Match, g, 1); \
	else if (Color == Empty) g->NumLibs++; \
    }

Stone Color;
    if (RecursionLevel == 0) {
	g->NumLibs= 0;
	Checked [i][j]= ++CheckNum;
    }
    CountGrpLibs0 ((i > 0), i-1, j);
    CountGrpLibs0 ((j > 0), i, j-1);
    CountGrpLibs0 ((i < Game->Grid-1), i+1, j);
    CountGrpLibs0 ((j < Game->Grid-1), i, j+1);
} /* CountGrpLibs */


    int
CountLibs (Game, i, j)
    GameType *Game;
    int i, j;
/* Count liberties of a single isolated stone.
    Assumes stone is already on board.
*/
{
int NumLibs, LastLine;
    LastLine= Game->Grid - 1;
    NumLibs= 0;
    if ((i > 0) && (Game->Board [i-1][j] == Empty)) {
	NumLibs++;
    }
    if ((j > 0) && (Game->Board [i][j-1] == Empty)) {
	NumLibs++;
    }
    if ((i < LastLine) && (Game->Board [i+1][j] == Empty)) {
	NumLibs++;
    }
    if ((j < LastLine) && (Game->Board [i][j+1] == Empty)) {
	NumLibs++;
    }
    return NumLibs;
} /* CountLibs */


DebugGroups (Groups)
    GroupType *Groups [MaxGrid] [MaxGrid];
{
GroupType *g;
int i, j;
    for (j= 9; j >= 3; j--) {
	for (i= 13; i < 19; i++) {
	    g= Groups [i] [j];
	    printf ("  %7lx", g);
	    if (g != NULL) printf (" %c%d", g->Color ? 'o' : '*', g->NumLibs);
	    else printf ("   ");
	}
	printf ("\n");
    }
    printf ("\n");
} /* DebugGroups */


    int
FindEnemies (i, j, Game, Enemies)
    int i, j;
    GameType *Game;
    GroupType *Enemies [4];
/* Call by reference variable side effect:  Enemies. 
    Returns number of neighboring stones found of opposing color.
    Watch out for stone touching a group twice.
*/
{
int Num, k, c;	/* c == color. */
    c= !Game->Board [i][j];
    Num= 0;
    if ((i > 0) && (Game->Board [i-1][j] == c))
	Enemies [Num++]= Game->Groups [i-1][j];
    if ((j > 0) && (Game->Board [i][j-1] == c))
	Enemies [Num++]= Game->Groups [i][j-1];
    if ((i < (Game->Grid - 1)) && (Game->Board [i+1][j] == c))
	Enemies [Num++]= Game->Groups [i+1][j];
    if ((j < (Game->Grid - 1)) && (Game->Board [i][j+1] == c))
	Enemies [Num++]= Game->Groups [i][j+1];
    /* Check for Duplicates. */
    for (i= 0; i < (Num-1); i++) {
	for (j= i+1; j < Num; j++) {
	    if (Enemies [i] == Enemies [j]) {
		for (k= j; k < (Num-1); k++) Enemies [k]= Enemies [k+1];
		Num--;
		j--;	/* Since a new neighbor is at j position. */
	    }
	}
    }
    return Num;
} /* FindEnemies */


    int
FindFriends (i, j, Game, c, Neighbors)
    int i, j;
    GameType *Game;
    int c;	/* c == color. */
    GroupType *Neighbors [4];
/* Call by reference variable side effect:  Neighbors. 
    Returns number of neighboring stones found of same color.
    Watch out for stone touching a group twice.
*/
{
int Num, k;
    Num= 0;
    if ((i > 0) && (Game->Board [i-1][j] == c))
	Neighbors [Num++]= Game->Groups [i-1][j];
    if ((j > 0) && (Game->Board [i][j-1] == c))
	Neighbors [Num++]= Game->Groups [i][j-1];
    if ((i < (Game->Grid - 1)) && (Game->Board [i+1][j] == c))
	Neighbors [Num++]= Game->Groups [i+1][j];
    if ((j < (Game->Grid - 1)) && (Game->Board [i][j+1] == c))
	Neighbors [Num++]= Game->Groups [i][j+1];
    /* Eliminate Duplicates. */
    for (i= 0; i < (Num-1); i++) {
	for (j= i+1; j < Num; j++) {
	    if (Neighbors [i] == Neighbors [j]) {
		for (k= j; k < (Num-1); k++) Neighbors [k]= Neighbors [k+1];
		Num--;
		j--;	/* Since a new neighbor is at j position. */
	    }
	}
    }
    return Num;
} /* FindFriends */


    int 
GenCaptures (Game, Color, Move)
    GameType *Game;
    int Color;
    MoveType *Move;
/* Find all groups of Color with no liberties and removes them, putting
    the captures into the Move.  Color is the opposite color to the
    moving player.
*/
{
int i, j, k, NumCaptures;
GroupType **g, *Grp, *Neighbors [4];
CaptureType *c;
Stone *s;
    if (Move->Captures != NULL) {
	FreeCaptures (Move->Captures);
	Move->Captures= NULL;
    }
    for (i= 0, NumCaptures= 0; i < Game->Grid; i++) {
	g= Game->Groups [i];
	s= Game->Board [i];
	for (j= 0; j < Game->Grid; j++, s++, g++) {
	    Grp= *g;
	    if ((Grp != NULL) && (Grp->Color == Color)) {
		if (Grp->NumLibs == 0) {
		    NumCaptures++;
		    k= FindEnemies (i, j, Game, Neighbors) - 1;
		    for (; k >= 0; k--) {
			Neighbors [k]->NumLibs++;
		    }
		    *s= Empty;
		    Game->Prisoners [Color]++;
		    c= AllocCapture (i, j);
		    c->Next= Move->Captures;
		    Move->Captures= c;
		    Grp->NumStones--;
		    if (Grp->NumStones <= 0)
			FreeGroup (Grp);
		    *g= NULL;
		}
	    }
	}
    }
    return NumCaptures;
} /* GenCaptures */


NewStoneToGroups (Game, i, j)
    GameType *Game;
    int i, j;
/* Adjusts group map to include presence of new stone at i, j.
*/
{
GroupType	*g, *Neighbors [4];
    /* First stores Neighbors, then Enemies. */
int 		NumNeighbors, NumEnemies;
int		k;
    if (Game->Board [i][j] == Empty) {
	fprintf (stderr, "NewStonetoGroups (%d, %d, Game) is Empty.\n", i, j);
    }
    NumNeighbors= FindFriends (i, j, Game, Game->Board [i][j], Neighbors);
    switch (NumNeighbors) {
    case 0:	/* New Group */
	Game->Groups [i][j]= g= AllocGroup (Game->Board [i][j], 0, 1);
	g->NumLibs = CountLibs (Game, i, j);
	break;
    case 1:
	g= Game->Groups [i][j]= Neighbors [0];
	g->NumStones++;
	CountGrpLibs (Game, i, j, Game->Board [i][j], g, 0);
	break;
    default:
	g= UniteGroups (i, j, Neighbors, NumNeighbors, Game);
    } /* switch */
    NumEnemies= FindEnemies (i, j, Game, Neighbors);
    for (k= 0; k < NumEnemies; k++) Neighbors [k]->NumLibs--;
    if (DebugLevel == 1) {
	printf ("NewStonetoGroups %d %d\n", i, j);
	DebugGroups (Game->Groups);
    }
} /* NewStoneToGroups */


NullGroups (Game)
    GameType *Game;
/* Set Group map entries to NULL. */
{
int i, j;
    for (i= 0; i < MaxGrid; i++)
	for (j= 0; j < MaxGrid; j++)
	    if (Game->Groups [i][j] != NULL) {
		FreeGroup (Game->Groups [i][j]);
		Game->Groups [i][j]= NULL;
	    }
    for (i= 0; i < MaxGrid; i++)
	for (j= 0; j < MaxGrid; j++)
	    if (Game->InitialBoard [i][j] != Empty) {
		Game->Groups [i][j]= 
		    AllocGroup (Game->InitialBoard [i][j], 4, 1);
	    }
} /* NullGroups */


    int
RemGroup (Game, i, j, c, Dir, LastLine)
    GameType *Game;
    int i, j;
    Stone c;
    int Dir, LastLine;
	/* Dir is Direction this call is going. */
{
int s = 0;
GroupType *g;
    if (Dir == NoDir) g= Game->Groups [i][j];
    Game->Groups [i][j]= NULL;
    if ((Dir != East) && (i > 0) && (Game->Board [i-1][j] == c)
	&& (Game->Groups [i-1][j] != NULL)) {
	s |= West;
	RemGroup (Game, i-1, j, c, West, LastLine);
    }
    if ((Dir != North) && (j > 0) && (Game->Board [i][j-1] == c)
	&& (Game->Groups [i][j-1] != NULL)) {
	s |= South;
	RemGroup (Game, i, j-1, c, South, LastLine);
    }
    if ((Dir != West) && (i < LastLine) && (Game->Board [i+1][j] == c)
	&& (Game->Groups [i+1][j] != NULL)) {
	s |= East;
	RemGroup (Game, i+1, j, c, East, LastLine);
    }
    if ((Dir != South) && (j < LastLine) && (Game->Board [i][j+1] == c)
	&& (Game->Groups [i][j+1] != NULL)) {
	s |= North;
	RemGroup (Game, i, j+1, c, North, LastLine);
    }
    if (Dir == NoDir) FreeGroup (g);
    return s;
} /* RemGroup */


RemoveStone (Game, i, j)
    GameType *Game;
    int i, j;
/* When splitting the group that the stone i,j is in, be sure to
    watch for a group that comes around and touches twice.
*/
{
#define NullGrp	(GroupType *) NULL
int LastLine, Dirs, k;
Stone c;
GroupType *Neighbors [4];
    for (k= FindEnemies (i, j, Game, Neighbors) - 1; k >= 0; k--) {
	Neighbors [k]->NumLibs++;
    }
    c= Game->Board [i][j];
    Game->Board [i][j]= Empty;
    LastLine= Game->Grid - 1;
    if (Dirs= RemGroup (Game, i, j, c, NoDir, LastLine)) {
	if (Dirs & West) {
	    RestoreGroup (Game, i-1, j, c, West, LastLine, NullGrp);
	    CountGrpLibs (Game, i-1, j, c, Game->Groups [i-1][j], 0);
	}
	if (Dirs & South) {
	    RestoreGroup (Game, i, j-1, c, South, LastLine, NullGrp);
	    CountGrpLibs (Game, i, j-1, c, Game->Groups [i][j-1], 0);
	}
	if (Dirs & East) {
	    RestoreGroup (Game, i+1, j, c, East, LastLine, NullGrp);
	    CountGrpLibs (Game, i+1, j, c, Game->Groups [i+1][j], 0);
	}
	if (Dirs & North) {
	    RestoreGroup (Game, i, j+1, c, North, LastLine, NullGrp);
	    CountGrpLibs (Game, i, j+1, c, Game->Groups [i][j+1], 0);
	}
    }
} /* RemoveStone */


RestoreGroup (Game, i, j, c, Dir, LastLine, g)
    GameType *Game;
    int i, j;
    Stone c;
    int Dir, LastLine;
    GroupType *g;
/* Seeks out stones and fills in group map.  Used for each branch when
    removing a single stone splits a group.  Since all contiguous stones
    have just prior been removed, don't have to worry about uniting groups.
*/
{
    if (g == NULL) {
	g= AllocGroup (c, 0, 1);
	Game->Groups [i][j]= g;
    } else {
	Game->Groups [i][j]= g;
	g->NumStones++;
    }
    if ((Dir != East) && (i > 0) && (Game->Board [i-1][j] == c)
	&& (Game->Groups [i-1] [j] != g)) {
	RestoreGroup (Game, i-1, j, c, West, LastLine, g);
    }
    if ((Dir != North) && (j > 0) && (Game->Board [i][j-1] == c)
	&& (Game->Groups [i] [j-1] != g)) {
	RestoreGroup (Game, i, j-1, c, South, LastLine, g);
    }
    if ((Dir != West) && (i < LastLine) && (Game->Board [i+1][j] == c)
	&& (Game->Groups [i+1] [j] != g)) {
	RestoreGroup (Game, i+1, j, c, East, LastLine, g);
    }
    if ((Dir != South) && (j < LastLine) && (Game->Board [i][j+1] == c)
	&& (Game->Groups [i] [j+1] != g)) {
	RestoreGroup (Game, i, j+1, c, North, LastLine, g);
    }
} /* RestoreGroup */


    GroupType *
UniteGroups (ii, jj, Neighbors, NumNeighbors, Game)
    int ii, jj;
    GroupType *Neighbors [4];
    int NumNeighbors;
    GameType *Game;
/* Unite Group of Stone ii, jj with list of Neighbors.  Group of ii, jj
    is subsumed by last Neighbor which subsumes the other neighbor groups.
    Free Neighbor groups.
*/
{
GroupType *g;
int i, j, k;
Stone c;
    c= Game->Board [ii][jj];
    g= Neighbors [--NumNeighbors];
    Game->Groups [ii][jj]= g;
    g->NumStones++;	/* for stone at ii, jj. */
    if (NumNeighbors <= 0) return 0;
    for (i= 0; i < Game->Grid; i++) {
	for (j= 0; j < Game->Grid; j++) {
	    for (k= 0; k < NumNeighbors; k++)
		if (Game->Groups [i][j] == Neighbors [k]) {
		    Game->Groups [i][j]= g;
		    g->NumStones++;
		}
	}
    }
    for (k= 0; k < NumNeighbors; k++) {
	FreeGroup (Neighbors [k]);
    }
    CountGrpLibs (Game, ii, jj, c, g, 0);
    return g;
} /* UniteGroups */
