#ifndef lint
static char sccsid[]= "@(#)goservice.c 1.3 87/01/11 Copyr 1986 Sun Micro";
#endif

/* Copyright (c) Sun MicroSystems, 1986.  This code may be used and modified
    for not-for-profit use.  This notice must be retained.
*/

/* goservice.c
*/

#include "goban.h"
#include <suntool/panel.h>

MoveType	*TrashMoves;
CaptureType	*TrashCaptures;
GroupType	*TrashGroups;

extern Panel_item	CurrMoveSlider, HandicapSlider, NameButton;


    CaptureType *
AllocCapture (i, j)
    int i, j;
{
CaptureType *c;
    if (TrashCaptures != NULL) {
	c= TrashCaptures;
	TrashCaptures= TrashCaptures->Next;
    } else {
	c= Malloc (CaptureType);
    }
    c->Next= NULL;
    c->i= i;
    c->j= j;
    return c;
} /* AllocCapture */


    GroupType *
AllocGroup (Color, Libs, NumStones)
    int Color, Libs, NumStones;
{
GroupType *g;
    if (TrashGroups != NULL) {
	g= TrashGroups;
	TrashGroups= TrashGroups->Next;
    } else {
	g= Malloc (GroupType);
    }
    g->Color= Color;
    g->NumLibs= Libs;
    g->NumStones= NumStones;
    g->Next= NULL;
    return g;
} /* AllocGroup */


    MoveType *
AllocMove (i, j, CurrMove)
    int i, j;
    MoveType *CurrMove;
{
MoveType *m;
    if (TrashMoves != NULL) {
	m= TrashMoves;
	TrashMoves= TrashMoves->Next;
    } else {
	m= Malloc (MoveType);
    }
    m->Next= NULL;
    m->Prev= CurrMove;
    m->AltNext= m;
    m->AltPrev= m;
    m->Label [0]= ' ';
    m->Label [1]= '\0';
    m->Note= NULL;
    if (CurrMove != NULL) CurrMove->Next= m;
    m->Captures= NULL;
    m->i= i;
    m->j= j;
    return m;
} /* AllocMove */


Cafard (s)	/* French for cockroach */
    char *s;
{
    fprintf (stderr, s);
    exit (1);
} /* Cafard */


FreeCaptures (c)
    CaptureType *c;
/* Free capture c and all succeeding captures in its list by moving into
    TrashCaptures.
*/
{
CaptureType *c1;
    if (c == NULL) return;
    for (c1= c; c->Next != NULL; c= c->Next);	/* Go to end of list. */
    c->Next= TrashCaptures;
    TrashCaptures= c1;
} /* FreeCaptures */


FreeGroup (g)
    GroupType *g;
{
GroupType *t;
    if (g == NULL) return;
    for (t= TrashGroups; t != NULL; t= t->Next)
	if (g == t) return;
    g->Next= TrashGroups;
    TrashGroups= g;
} /* FreeGroup */


FreeMoves (m)
    MoveType *m;
/* Free move m and all succeeding moves in its list by moving into TrashMoves.
    Trim off and free all side trees descending from m.
*/
{
MoveType *m1, *m2;
    if (m == NULL) return;
    FreeCaptures (m->Captures);
    if (m->Note != NULL) {
	free (m->Note);
	m->Note= NULL;
    }
    for (m1= m->Next; m1 != NULL; m1= m2) {
	m2= m1->AltNext;
	FreeMoves (m1);
	if (m1->AltPrev != NULL) m1->AltPrev->AltNext= NULL;
	if (m1->AltNext != NULL) m1->AltNext->AltPrev= NULL;
	if (m2 == m->Next) break;
    }
    m->Next= TrashMoves;
    TrashMoves= m;
} /* FreeMoves */


GetComment (F, String, 	ReturnedString)
    FILE	*F;
    char	*String, **ReturnedString;
{
#define MaxBuf	1024
int	NumChars, Quit;
char	Buf [MaxBuf], *s, *b;
    s= String + 2;
    while (*s == ' ') s++;
    b= Buf;
    Buf [0]= '\0';
    Quit= 0;
    while (*s != '\0') {
	if ((*s == '*') && (*(s+1) == '/')) {
	    *b++= '\0';
	    Quit= 1;
	    break;
	}
	*b++= *s++;
    }
    if (!Quit) {
	while ((b - Buf) < MaxBuf) {
	    *b= getc (F);
	    if (*b == '/') {
		if (*(b-1) == '*') {
		    *(b-1)= '\0';
		    b--;
		    break;
		}
	    }
	    b++;
	}
    }
    while ((b >= Buf) && (*b == ' ')) {
	*b--= '\0';
    }
    if (Buf [0] != '\0') {
	NumChars= b- Buf;
	s= *ReturnedString= (char *) malloc (NumChars + 1);
	for (b= Buf; NumChars > 0; NumChars--) *s++= *b++;
	*s= '\0';
    }
    if (EOF == fscanf (F, "%s", String)) return 1;
    else return 0;
#undef MaxBuf
} /* GetComment */


HandicapBoard (Game)
    GameType *Game;
/* Set Handicap points on board to black. */
{
int i, j;
    switch (Game->Handicap) {
    default:  printf ("Handicap (%d) larger than 17.\n", Game->Handicap);
	Game->Handicap= 17;
    case 17:  Game->InitialBoard [ 6] [12]= Black;
    case 16:  Game->InitialBoard [12] [ 6]= Black;
    case 15:  Game->InitialBoard [ 6] [ 6]= Black;
    case 14:  Game->InitialBoard [12] [12]= Black;
    case 13:  Game->InitialBoard [ 2] [16]= Black;
    case 12:  Game->InitialBoard [16] [ 2]= Black;
    case 11:  Game->InitialBoard [ 2] [ 2]= Black;
    case 10:  Game->InitialBoard [16] [16]= Black;
    case 9:   Game->InitialBoard [ 9] [ 9]= Black;
    case 8:   Game->InitialBoard [ 9] [15]= Black;
    case 7:  if (Game->Handicap == 7) Game->InitialBoard [9] [9]= Black;
	    else Game->InitialBoard [9] [3]= Black;
    case 6:  Game->InitialBoard [3] [9]= Black;
    case 5:  if (Game->Handicap == 5) Game->InitialBoard [9] [9]= Black;
	    else Game->InitialBoard [15] [9]= Black;
    case 4:  Game->InitialBoard [ 3] [15]= Black;
    case 3:  Game->InitialBoard [15] [ 3]= Black;
    case 2:  Game->InitialBoard [ 3] [ 3]= Black;
    case 1:  Game->InitialBoard [15] [15]= Black;
    case 0:  break;
    } /*switch*/
    for (i= 0; i < MaxGrid; i++)
	for (j= 0; j < MaxGrid; j++) {
	    Game->Board [i] [j]= Game->InitialBoard [i] [j];
	    if (Game->InitialBoard [i] [j] != Empty) {
		if (Game->Groups [i][j] != NULL)
		    FreeGroup (Game->Groups [i][j]);
		Game->Groups [i] [j]=
		    AllocGroup (Game->InitialBoard [i] [j], 4, 1);
	    }
	}
} /* HandicapBoard */


    char *
PositionToString (i, j, String, Depth)
    int i, j;
    char String [];
    int Depth;	/* level of tree. */
/* Write into a char array a string consisting of
    <depth>."pass" or
    <depth>.<letter><one or two digits><newline>.
    If Depth == 0 ignore it.
*/
{
    if (Depth == 0) {
	if (i >= 0)
	    sprintf (String, "%c%d", 'A' + ((i > 7) ? (i + 1) : i), j + 1);
	else sprintf (String, "%s", "pass");
    } else {
	if (i >= 0)
	    sprintf (String, "%d.%c%d"
		, Depth, 'A' + ((i > 7) ? (i + 1) : i), j + 1);
	else sprintf (String, "%d.%s", Depth, "pass");
    }
    return String;
} /* PositionToString */


ReadGame (GameName, Game, ShouldPaint)
    char *GameName;
    GameType *Game;
    Panel_setting ShouldPaint;
{
FILE *F;
int		i, j, NumCaptures, Depth;
char		String [40];
char		Err [80];
Stone		Color;
MoveType	*Move, *m;
    if (NULL == (F= fopen (GameName, "r", 0))) return 1;
    ZeroGame (Game, ShouldPaint);
    strncpy (Game->Title, GameName, MaxTitleChars-1);
    fscanf (F, "%s", String);	/* Size of board. */
    if ((String [0] == '/') && (String [1] == '*')) {
	GetComment (F, String, &(Game->Note));
    }
    Game->Grid= atoi (String);
    fscanf (F, "%s", String);	/* Handicap. */
    Game->Handicap= atoi (String);
    if (Game->Handicap == 0) {	/* Initial board position from file. */
	for (;;) {
	    fscanf (F, "%s", String);
	    if (String [0] == '.') break;
	    StringToPosition (String, &i, &j, &Depth);
	    Game->InitialBoard [i] [j]= Black;
	}
	for (;;) {
	    fscanf (F, "%s", String);
	    if (String [0] == '.') break;
	    StringToPosition (String, &i, &j, &Depth);
	    Game->InitialBoard [i] [j]= White;
	}
	for (i= 0; i < Game->Grid; i++)
	    for (j= 0; j < Game->Grid; j++) {
		Game->Board [i] [j]= Game->InitialBoard [i] [j];
	    }
    } else {	/* Initial board position automatic. */
	HandicapBoard (Game);
    }
    if (EOF == fscanf (F, "%s", String))
	Cafard ("Unexpected end of file after handicap or initial board.\n");
    if (String [0] == 'B') Game->FirstPlayer= Black;
    else if (String [0] == 'W') Game->FirstPlayer= White;
    for (Game->MoveNum= 0; ;) {
	if (EOF == fscanf (F, "%s", String)) break;
	if (String [0] == '|') {
	    SetLabel (Move, String + 1);
	    if (EOF == fscanf (F, "%s", String)) break;
	}
	if ((String [0] == '/') && (String [1] == '*')) {
	    if (GetComment (F, String, &(Move->Note))) break;
	}
	StringToPosition (String, &i, &j, &Depth);
	Depth--;
	if ((Depth >= 0) && (Game->MoveNum != Depth)) {
	    if (Depth > Game->MoveNum) {
		sprintf (Err,"Moves out of order:  too deep. \"%s\".\n",String);
		Cafard (Err);
	    }
	    while (Depth < Game->MoveNum) {
		Move= Move->Prev;
		Backup (Game, 0);
	    }
	    /* Install as alternate branch. */
	    if (Move != NULL) {
		m= AllocMove (i, j, (MoveType *) NULL);
		m->Prev= Move;
		m->AltPrev= Move->Next->AltPrev;
		Move->Next->AltPrev= m;
		m->AltNext= Move->Next;
		m->AltPrev->AltNext= m;
		Move= m;
	    } else {
		m= AllocMove (i, j, (MoveType *) NULL);
		m->AltNext= Game->MoveList;
		m->AltPrev= Game->MoveList->AltPrev;
		m->AltPrev->AltNext= m;
		Game->MoveList->AltPrev= m;
		Move= m;
	    }
	} else {
	    if (Game->MoveList == NULL) {
		Game->MoveList= Move= AllocMove (i, j, (MoveType *) NULL);
	    } else {
		Move->Next= AllocMove (i, j, Move);
		Move= Move->Next;
	    }
	}
	Game->CurrMove= Move;
	if (i >= 0) {
	    Game->Board [i] [j]= Color=
		(Game->MoveNum & 1) ? !Game->FirstPlayer : Game->FirstPlayer;
	    NewStoneToGroups (Game, i, j);
	    NumCaptures= GenCaptures (Game, !Color, Move);
	}
	if (++Game->MoveNum > Game->NumMoves) Game->NumMoves= Game->MoveNum;
    }
    for (Move= Game->MoveList, Game->MoveNum= 0; Move != NULL; Move=Move->Next){
	Game->MoveNum++;
    }
    Color= (Game->MoveNum & 1) ? !Game->FirstPlayer : Game->FirstPlayer;
    SetFirstPlayerButton (Color, ShouldPaint);
    panel_set (CurrMoveSlider
	, PANEL_MAX_VALUE, Game->NumMoves
	, PANEL_VALUE, Game->MoveNum
	, PANEL_PAINT, ShouldPaint, 0);
    panel_set (HandicapSlider, PANEL_VALUE, Game->Handicap
	, PANEL_PAINT, ShouldPaint, 0);
    if (Game->MoveList != NULL) Game->CurrMove= Move;
    return 0;
} /* ReadGame */


SetFirst (Game, s, ShouldPaint)
    GameType *Game;
    Stone s;
    Panel_setting ShouldPaint;
/* Set first player of game. */
{
    Game->FirstPlayer= s;
    SetFirstPlayerButton (s, ShouldPaint);
} /* SetFirst */


SetLabel (m, s)
    MoveType	*m;
    char	*s;
{
    m->Label [0]= *s;
    m->Label [1]= '\0';
} /* SetLabel */



StringToPosition (String, i, j, Depth)
    char String [];
    int *i, *j, *Depth;
/* Parses string of form <letter><one or two digits> and leaves
     corresponding position in i and j.  The string may be prepended
     with <depth><period>.
 */
{
char Err [80];
char *s;
    *Depth= 0;
    *i= -1;
    *j= -1;
    s= String;
    if ((*s >= '0') && (*s <= '9')) {
	while ((*s >= '0') && (*s <= '9')) { /* Initial depth? */
	    *Depth= *Depth * 10 + *s++ - '0';
	}
	if (*s++ != '.') *Depth= -1;	/* Alert error routine. */
    }
    if ((0 == strncmp (s, "pass", 4)) && (*(s + 4) == '\0')) {
	return 0;	/* OK */
    }
    if (*s > 'I') (*s)--;
    *i= *s - 'A';
    if (*(s+2) != '\0') *j= 10 + *(s+2) - '1';
    else *j= *(s+1) - '1';
    if ((*Depth<0) || (*i<0) || (*i >=MaxGrid) || (*j < 0) || (*j >= MaxGrid)) {
	sprintf (Err, "Bad position %s (%d %d %d).\n", String, *i, *j, *Depth);
	Cafard (Err);
    }
} /* StringToPosition */


WriteBoard (GameName, Game)
    char *GameName;
    GameType *Game;
{
FILE *F;
int i, j, k, Count, MoveNum, NumRows;
MoveType *m;
char String [80];
char Moves [MaxMoves] [2];
    sprintf (String, "%s.brd", GameName);
    if (NULL == (F= fopen (String, "w", 0))) return 1;
    if (Game->Handicap != 0) fprintf (F, "Handicap:  %d.\n", Game->Handicap);
    for (i= 0, Count= 0; i < Game->Grid; i++)
	for (j= 0; j < Game->Grid; j++)
	    if (Game->InitialBoard [i] [j] != Empty) Count++;
    if (Count > 0) {
	fprintf (F, "Black at:  ");
	for (i= 0; i < Game->Grid; i++)
	    for (j= 0; j < Game->Grid; j++) {
		if (Game->InitialBoard [i] [j] == Black) {
		    fprintf (F, " %s", PositionToString (i, j, String, 0));
		} /*if*/
	    }
	fprintf (F, ".\nWhite at:  ");
	for (i= 0; i < Game->Grid; i++)
	    for (j= 0; j < Game->Grid; j++) {
		if (Game->InitialBoard [i] [j] == White) {
		    fprintf (F, " %s", PositionToString (i, j, String, 0));
		} /*if*/
	    }
	fprintf (F, ".\n");
    }
    MoveNum= 0;
    m= Game->MoveList;
    fprintf (F, "   A B C D E F G H J K L M N O P Q R S T");
    if ((Game->FirstPlayer == White) && (m != NULL)) {
	fprintf (F, "\t  1:\t\t%s.\n", PositionToString (m->i, m->j, String,0));
	MoveNum= 1;
	m= m->Next;
    } else {
	fprintf (F, "\n");
    }
    for (j= Game->Grid - 1; j >= 0; j--) {
	fprintf (F, "%2d", j + 1);
	for (i= 0; i < Game->Grid; i++) {
	    switch (Game->Board [i] [j]) {
	    case Empty:
		if (((i == 3) || (i == (Game->Grid-4)) || (i == (Game->Grid/2)))
		&& ((j == 3) || (j == (Game->Grid-4)) || (j == (Game->Grid/2))))
		    fprintf (F, " +");
		else fprintf (F, " .");
		break;
	    case Black:  fprintf (F, " *"); break;
	    case White:  fprintf (F, " O"); break;
	    }
	}
	fprintf (F, " %2d", j + 1);
	if (m == NULL) fprintf (F, "\n");
	else if (MoveNum < Game->MoveNum) {
	    fprintf (F, "\t%3d:\t%s", MoveNum+1
		, PositionToString (m->i, m->j, String, 0));
	    m= m->Next;
	    if ((++MoveNum < Game->MoveNum) && (m != NULL)) {
		fprintf (F, "\t%s.\n", PositionToString (m->i, m->j, String,0));
		m= m->Next;
	    } else {
		fprintf (F, "\n");
	    }
	    MoveNum++;
	} else {
	    fprintf (F, "\n");
	}
    }
    fprintf (F, "   A B C D E F G H J K L M N O P Q R S T");
    if (m == NULL) fprintf (F, "\n");
    else if (MoveNum < Game->MoveNum) {
	fprintf (F, "\t%3d:\t%s", MoveNum+1
	    , PositionToString (m->i, m->j, String, 0));
	m= m->Next;
	if ((++MoveNum < Game->MoveNum) && (m != NULL))
	    fprintf (F, "\t%s.\n\n", PositionToString (m->i, m->j, String, 0));
	else {
	    fprintf (F, "\n");
	}
	if (m != NULL) {
	    m= m->Next;
	    MoveNum++;
	}
    }
    for (; MoveNum < Game->MoveNum; MoveNum++) {
	if (m == NULL) break;
	Moves [MoveNum-40] [0]= (char) m->i;
	Moves [MoveNum-40] [1]= (char) m->j;
	m= m->Next;
    }
    NumRows= 1 + ((Game->MoveNum - 41) / 6);
    for (i= 0, j= NumRows * 2, k= NumRows * 4; i < NumRows - 1; i++) {
	fprintf (F, "%3d:  %3s  %3s\t\t%3d:  %3s  %3s\t\t%3d:  %3s  %3s\n",
	    i + 41,
	    PositionToString (Moves [i] [0],   Moves [i] [1],   String, 0),
	    PositionToString (Moves [i+1] [0], Moves [i+1] [1], String, 0),
	    j + 41,
	    PositionToString (Moves [j] [0],   Moves [j] [1],   String, 0),
	    PositionToString (Moves [j+1] [0], Moves [j+1] [1], String, 0),
	    k + 41,
	    PositionToString (Moves [k] [0],   Moves [k] [1],   String, 0),
	    PositionToString (Moves [k+1] [0], Moves [k+1] [1], String, 0));
    }
    fprintf ("\n");
    fprintf (F, "\nWhite stones captured:  %d.  Black:  %d.\n"
	, Game->Prisoners [White], Game->Prisoners [Black]);
    fclose (F);
    return 0;
} /* WriteBoard */


WriteGame (GameName, Game)
    char	*GameName;
    GameType	*Game;
{
FILE	*F;
int	i, j;
char	String [80];
MoveType *m;
    if (NULL == (F= fopen (GameName, "w", 0))) return 1;
    if (Game->Note) {
	fprintf (F, "/*%s*/\n", Game->Note);
    }
    fprintf (F, "%d\n", Game->Grid);
    fprintf (F, "%d\n", Game->Handicap);	/* Handicap */
    if (Game->Handicap == 0) {
	for (i= 0; i < Game->Grid; i++)
	    for (j= 0; j < Game->Grid; j++) {
		if (Game->InitialBoard [i] [j] == Black) {
		    fprintf (F, " %s", PositionToString (i, j, String, 0));
		} /*if*/
	    }
	fprintf (F, " .\n");
	for (i= 0; i < Game->Grid; i++)
	    for (j= 0; j < Game->Grid; j++) {
		if (Game->InitialBoard [i] [j] == White) {
		    fprintf (F, " %s", PositionToString (i, j, String, 0));
		} /*if*/
	    }
	fprintf (F, " .\n");
    } /* else Initial board position automatic. */
    if (Game->FirstPlayer == Black) fprintf (F, "Black\n");
    else fprintf (F, "White\n");
    for (m= Game->MoveList; m != NULL; m= m->AltNext) {
	WriteBranch (m, 1, F);
	if (m->AltNext == Game->MoveList) break;
    }
    fprintf (F, "\n");
    fclose (F);
    return 0;
} /* WriteGame */


WriteBranch (m, Depth, F)
    MoveType	*m;
    int		Depth;
    FILE	*F;
{
MoveType *m1;
char	String [20];
    fprintf (F, "%s\n", PositionToString (m->i, m->j, String, Depth));
    if ((m->Label [0] != ' ') && (m->Label [0] != '\0'))
	fprintf (F, "|%s\n", m->Label);
    if (m->Note) fprintf (F, "/*%s*/\n", m->Note);
    if (m->Next != NULL)
	for (m1= m->Next, Depth++; m1 != NULL; m1= m1->AltNext) {
	    WriteBranch (m1, Depth, F);
	    if (m1->AltNext == m->Next) break;
	}
} /* WriteBranch */


ZeroGame (Game, ShouldPaint)
    GameType *Game;
    Panel_setting ShouldPaint;
/* Initializes a game by clearing boards and data structures. */
{
int i, j;
    if (Game->Note) free (Game->Note);
    for (i= 0; i < MaxGrid; i++)
	for (j= 0; j < MaxGrid; j++) {
	    Game->InitialBoard [i] [j]= Empty;
	    Game->Groups [i] [j]= NULL;
	}
    if (Game->MoveList != NULL) FreeMoves (Game->MoveList);
    Game->MoveList= Game->CurrMove= NULL;
    Game->NumMoves= Game->MoveNum= 0;
    panel_set (CurrMoveSlider, PANEL_VALUE, Game->MoveNum
	, PANEL_PAINT, ShouldPaint, 0);
    if (Game->Handicap == 0) SetFirst (Game, Black, ShouldPaint);
    else SetFirst (Game, White, ShouldPaint);
    Game->Placing= 0;
    Game->Grid= 19;	/* Traditional. */
    Game->Prisoners [0]= Game->Prisoners [1]= 0;
} /* ZeroGame */
