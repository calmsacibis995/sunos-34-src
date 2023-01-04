#ifndef lint
static char sccsid[]= "@(#)goboard.c 1.3 87/01/11 Copyr 1986 Sun Micro";
#endif

/* Copyright (c) Sun MicroSystems, 1986.  This code may be used and modified
    for not-for-profit use.  This notice must be retained.
*/

/* goboard.c
*/

#include "goban.h"
#include <suntool/canvas.h>
#include <suntool/panel.h>
#include <suntool/textsw.h>


#define GridX(i)	(5 + Radius + ((i) + 2) * 2 * Radius)
#define GridY(j)	(5 + Radius + (Game->Grid - (j) - 1 + 2) * 2 * Radius)

#define DrawBlack(i,j,r) \
	pw_rop (BoardPW, GridX (i) - r, GridY (j) - r \
	    , 2 * r + 1, 2 * r + 1, PIX_SRC, BlackStone, 0,0)
#define DrawWhite(i,j,r) \
	pw_rop (BoardPW, GridX (i) - r, GridY (j) - r \
	    , 2 * r + 1, 2 * r + 1, PIX_SRC, WhiteStone, 0,0)

extern Canvas		BoardSW;
extern Panel_item	CurrMoveSlider, LabelButton;
extern Textsw		TextSW;
extern int		UserTextMods, Verbose;


Backup (Game, Draw)
    GameType	*Game;
    int		Draw;
{
int	i, j;
Stone	s;	/* Other player. */
char	*String;
CaptureType	*Cap;
    if (Game->Placing) return;
    if (Game->MoveNum <= 0) return;
    if (Game->CurrMove == NULL) return;
    if (Draw && Game->CurrMove) PanelToLabel (Game->CurrMove->Next);
    Game->MoveNum--;
    i= Game->CurrMove->i;
    j= Game->CurrMove->j;
    if (Draw) {
	if (i >= 0) DrawEmpty (Game, i, j);
	if (Game->CurrMove->Next)
	    DrawAltsEmpty (Game, Game->CurrMove);
    }
    if (i >= 0) RemoveStone (Game, i, j);
    if (Game->MoveNum & 1) s= Game->FirstPlayer;
    else s= !Game->FirstPlayer;
    if (Draw) SetFirstPlayerButton (!s, PANEL_CLEAR);
    for (Cap= Game->CurrMove->Captures; Cap != NULL; Cap= Cap->Next) {
	Game->Board [Cap->i] [Cap->j]= s;
	Game->Prisoners [s]--;
	NewStoneToGroups (Game, (int) Cap->i, (int) Cap->j);
	if (Draw) DrawMove (Game, NULL, Cap->i, Cap->j);
    }
    Game->CurrMove= Game->CurrMove->Prev;
    if (Draw) {
	Status (Game);
	StdMsg ();
	if (Game->CurrMove) {
	    if (Verbose)
		HighlightMove (Game, Game->CurrMove, -1, -1);
	    if (Game->CurrMove->Next)
		DrawAlts (Game, Game->CurrMove);
	    NewText (Game->CurrMove->Note);
	    LabelToPanel (Game->CurrMove->Next);
	} else {
	    NewText (Game->Note);
	}
    }
    if (DebugLevel == 1) DebugGroups (Game->Groups);
} /* Backup */


DrawAlpha (Game, i, j, c)
    GameType *Game;
    int i, j;
    char *c;
{
    pw_text (BoardPW, GridX (i) - Radius / 2 + 1
	, GridY (j) + Radius / 2
	, PIX_SRC, Font, c);
} /* DrawAlpha */


DrawAlts (Game, Move)
    GameType *Game;
    MoveType *Move;
{
MoveType *m;
    if (!Verbose) return;
    for (m= Move->Next; m != NULL; ) {
	pw_rop (BoardPW, GridX (m->i) - Radius / 2
	    , GridY (m->j) - Radius / 2
	    , Radius, Radius, PIX_SRC | PIX_COLOR (0), NULL, 0, 0);
	if (m->Label [0] != '\0') DrawAlpha (Game, m->i, m->j, m->Label);
	m= m->AltNext;
	if (m == Move->Next) break;
    }
} /* DrawAlts */


DrawAltsEmpty (Game, Move)
    GameType *Game;
    MoveType *Move;
{
MoveType	*m;
    for (m= Move->Next; ; ) {
	DrawEmpty (Game, m->i, m->j);
	m= m->AltNext;
	if (m == Move->Next) break;
    }
} /* DrawAltsEmpty */


DrawEmpty (Game, i, j)
    GameType *Game;
    int i, j;
{
    pw_rop (BoardPW, GridX (i) - Radius
	, GridY (j) - Radius, 2 * Radius + 1, 2 * Radius + 1
	, PIX_SRC | PIX_COLOR (0), NULL, 0, 0);
    if (i > 0)
	pw_rop (BoardPW, GridX (i) - Radius, GridY (j) - 1
	    , Radius + 1, LineWidth, PIX_SRC | PIX_COLOR (1), NULL, 0, 0);
    if (i < (Game->Grid - 1))
	pw_rop (BoardPW, GridX (i), GridY (j) - 1
	    , Radius + 1, LineWidth, PIX_SRC | PIX_COLOR (1), NULL, 0, 0);
    if (j > 0)
	pw_rop (BoardPW, GridX (i) - 1, GridY (j)
	    , LineWidth, Radius + 1, PIX_SRC | PIX_COLOR (1), NULL, 0, 0);
    if (j < (Game->Grid - 1))
	pw_rop (BoardPW, GridX (i) - 1, GridY (j) - Radius
	    , LineWidth, Radius + 1, PIX_SRC | PIX_COLOR (1), NULL, 0, 0);
    if (((i == 3) || (i == Game->Grid / 2) || (i == Game->Grid - 4))
	&& ((j == 3) || (j == Game->Grid / 2) || (j == Game->Grid - 4)))
	    Disk (BoardPW, 2 * LineWidth, GridX (i), GridY (j), 1);
} /* DrawEmpty */


DrawGame (Game, ReCompBoard)
    GameType *Game;
{
int i, j, k;
MoveType *Move;
CaptureType *c;
Stone s;
char String [3];
    for (i= 0; i < Game->Grid; i++) {	/* Draw horizontals, then verticals. */
	pw_rop (BoardPW, GridX (i) - 1, GridX (0) - 1
	    , LineWidth, (Game->Grid-1) * 2 * Radius
	    , PIX_SRC | PIX_COLOR (1), NULL, 0, 0);
	sprintf (String, "%c", (i < 8) ? ('A' + i) : ('B' + i));
	pw_text (BoardPW, GridX (i) -1, GridX (-1) - 10, PIX_SRC, Font, String);
	pw_text (BoardPW, GridX (i) -1, GridX (Game->Grid + 2) - 10, PIX_SRC
	    , Font, String);
	sprintf (String, "%2d", i + 1);
	pw_text (BoardPW, GridX (-1) - 20, GridY (i) +2, PIX_SRC, Font, String);
	pw_text (BoardPW, GridX (Game->Grid + 2) - 20, GridY (i) +2, PIX_SRC
	    , Font, String);
	pw_rop (BoardPW, GridX (0) - 1, GridX (i) - 1
	    , (Game->Grid-1) * 2 * Radius, LineWidth
	    , PIX_SRC | PIX_COLOR (1), NULL, 0, 0);
    }
    /* Draw 9 handicap points (seimoku). */
    Disk (BoardPW, 2 * LineWidth, GridX (3), GridY (3), 1);
    Disk (BoardPW, 2 * LineWidth, GridX (3), GridY (Game->Grid / 2), 1);
    Disk (BoardPW, 2 * LineWidth, GridX (3), GridY (Game->Grid - 4), 1);
    Disk (BoardPW, 2 * LineWidth, GridX (Game->Grid / 2), GridY (3), 1);
    Disk (BoardPW, 2 * LineWidth, GridX (Game->Grid / 2)
	, GridY (Game->Grid / 2), 1);
    Disk (BoardPW, 2 * LineWidth, GridX (Game->Grid / 2)
	, GridY (Game->Grid - 4), 1);
    Disk (BoardPW, 2 * LineWidth, GridX (Game->Grid - 4), GridY (3), 1);
    Disk (BoardPW, 2 * LineWidth, GridX (Game->Grid - 4)
	, GridY (Game->Grid / 2), 1);
    Disk (BoardPW, 2 * LineWidth, GridX (Game->Grid - 4)
	, GridY (Game->Grid - 4), 1);
    /* Place stones on board according to Initial position. */
    for (i= 0; i < Game->Grid; i++) {
	for (j= 0; j < Game->Grid; j++) {
	    if (Game->InitialBoard [i] [j] == Black) DrawBlack (i, j, Radius);
	    else if (Game->InitialBoard [i] [j] == White)
		DrawWhite (i, j, Radius);
	    if (ReCompBoard) Game->Board [i] [j]= Game->InitialBoard [i] [j];
	}
    }
    /* Place stones on board according to moves. */
    if (ReCompBoard) {
	s= Game->FirstPlayer;
	NullGroups (Game);
	Game->Prisoners [0]= Game->Prisoners [1]= 0;
    }
    for (k= 0, Move= Game->MoveList; (k < Game->MoveNum) && (Move!=NULL); k++){
	i= Move->i;
	j= Move->j;
	if (ReCompBoard) {
	    if (i >= 0) {
		Game->Board [i] [j]= s;
		NewStoneToGroups (Game, i, j);
		if (GenCaptures (Game, !s, Move)) {
		    for (c= Move->Captures; c != NULL; c= c->Next) {
			DrawEmpty (Game, c->i, c->j);
		    }
		}
	    }
	    s= !s;
	}
	if (i >= 0) {
	    if (Game->Board [i] [j] == Black) DrawBlack (i, j, Radius);
	    else DrawWhite (i, j, Radius);
	}
	Game->CurrMove= Move;
	Move= Move->Next;
    }
    HighlightMove (Game, Game->CurrMove, -1, -1);
    panel_set (CurrMoveSlider, PANEL_VALUE, Game->MoveNum, 0);
    for ( ; Move != NULL; k++, Move= Move->Next);
	    /* Find last move of this path. */
    panel_set (CurrMoveSlider, PANEL_MAX_VALUE, k, 0);
    if (Game->CurrMove) {
	LabelToPanel (Game->CurrMove->Next);
	NewText (Game->CurrMove->Note);
	if (Game->CurrMove->Next) DrawAlts (Game, Game->CurrMove);
    } else {
	NewText (Game->Note);
    }
} /* DrawGame */


DrawGameInit (Game, Width, Height)
    GameType *Game;
    int Width, Height;
{
    Radius= Width;
    if (Height < Width) Radius= Height;
    if (Radius > 650) {
	LineWidth= 3;
	Font= Fonts [3];
    } else if (Radius > 400) {
	LineWidth= 2;
	Font= Fonts [2];
    } else if (Radius > 300) {
	LineWidth= 1;
	Font= Fonts [1];
    } else {
	LineWidth= 1;
	Font= Fonts [0];
    }
    Radius= Radius - 10;
    Radius= Radius / (Game->Grid + 4);
    Radius= Radius / 2;
    if (BlackStone != NULL) pr_destroy (BlackStone);
    if (WhiteStone != NULL) pr_destroy (WhiteStone);
    BlackStone= mem_create (2 * Radius + 1, 2 * Radius + 1, 1);
    WhiteStone= mem_create (2 * Radius + 1, 2 * Radius + 1, 1);
    /* Create stone pixrects. */
    DiskPR (BlackStone, Radius, Radius, Radius, 1);
    DiskPR (WhiteStone, Radius, Radius, Radius, 1);
    DiskPR (WhiteStone, Radius - LineWidth, Radius, Radius, 0);
} /* DrawGameInit */


DrawMove (Game, m, i, j)
    GameType *Game;
    MoveType *m;
    int i, j;
{
/* Assume move is already on board. */
    if (Game->Board [i] [j] == Black) DrawBlack (i, j, Radius);
    else DrawWhite (i, j, Radius);
    if (Verbose && m) HighlightMove (Game, m, -1, -1);
} /* DrawMove */


EnterMove (i, j, Game, Draw, Action)
    int i, j;
    GameType *Game;
    int Draw, Action;
{
int NumCaptures, Count;
MoveType *m, *m1;
Stone s;
GroupType *Neighbors [4], *g;
    if (Game->CurrMove != NULL) PanelToLabel (Game->CurrMove->Next);
    if (Action == PassAction) {
	i= -1;
	j= -1;
    } else {
	if ((i < 0) || (i >= Game->Grid) || (j < 0) || (j >= Game->Grid))
	    return 1;
    }
    if (Game->Placing) {
	if (Action != PassAction) {
	    Game->Board [i][j]= Game->InitialBoard [i][j]= Game->FirstPlayer;
	    DrawMove (Game, NULL, i, j);
	    return 0;
	} else return 0;
    }
    if (Action != PassAction) {
	if (Game->Board [i] [j] != Empty) return 1;
    }
    if (Game->MoveNum & 1) s= !Game->FirstPlayer;
    else s= Game->FirstPlayer;
    if (Action != PassAction) {		/* Check legality of move */
	g= AllocGroup (s, 0, 0);
	CountGrpLibs (Game, i, j, s, g, 0);
	if (g->NumLibs <= 0) {
	    NumCaptures= CheckCaptures (Game, !s, i, j);
	    if (FindFriends (i, j, Game, s, Neighbors) && (NumCaptures == 0)) { 
		/* Undo suicide. */
		FreeGroup (g);
		StatusMsg ("Illegal move (suicide).");
		return 1;
	    } else {	/* Detect ko or single suicide. */
		if (NumCaptures == 0) {
		    /* Undo suicide. */
		    FreeGroup (g);
		    StatusMsg ("Illegal move (suicide).");
		    return 1;
		} else if (NumCaptures == 1) {	/* Check for ko violation. */
		    if ((Game->CurrMove != NULL)
			&& (Game->CurrMove->Captures != NULL)
			&& (Game->CurrMove->Captures->Next == NULL)
			&& (Game->CurrMove->Captures->i == i)
			&& (Game->CurrMove->Captures->j == j)) {
			    /* ko violation. */
			FreeGroup (g);
			StatusMsg ("Illegal move (ko violation).");
			return 2;
		    }
		}
	    }
	}
	FreeGroup (g);
    }
    if ((Game->MoveList == NULL) || (Action == MoveAction)
	    || (Action == PassAction)) {
	/* Get rid of moves beyond current move. */
/*	if (Game->NumMoves != Game->MoveNum) { */
	    if (Game->CurrMove != NULL) {
		if (Draw && Verbose && Game->CurrMove->Next)
		    DrawEmpty (Game, Game->CurrMove->Next->i
			, Game->CurrMove->Next->j);
		FreeMoves (Game->CurrMove->Next);
		Game->CurrMove->Next= NULL;
	    } else {
		FreeMoves (Game->MoveList);
	    }
/*	} */
	m= AllocMove (i, j, Game->CurrMove);
	if (Game->MoveList == NULL) Game->MoveList= m;
	Game->NumMoves= Game->MoveNum + 1;
	panel_set (CurrMoveSlider, PANEL_MAX_VALUE, Game->NumMoves
	    , PANEL_PAINT, PANEL_CLEAR, 0);
    } else if (Action == AltMoveAction) {
#define NEW
#ifdef NEW
	m1= Game->CurrMove->Next;
	m= AllocMove (i, j, Game->CurrMove);
	Game->NumMoves= Game->MoveNum + 1;
	panel_set (CurrMoveSlider, PANEL_MAX_VALUE, Game->NumMoves
	    , PANEL_PAINT, PANEL_CLEAR, 0);
	if (m1 != NULL) {
	    if ((m1->Label [0] == ' ') || (m1->Label [0] == 0))
		m1->Label [0]= 'a';
	    m->AltNext= m1;
	    m->AltPrev= m1->AltPrev;
	    m1->AltPrev= m;
	    m->AltPrev->AltNext= m;
	    for (m1= m->AltNext, Count= 1; m1 != m; m1= m1->AltNext)
		m1->Label [0]= 'a' + Count++;
	    m->Label [0]= 'a';
	    LabelToPanel (m); /* Step will PanelToLabel(Game->CurrMove->Next) */
	}
#else
	if (Game->CurrMove != NULL) {
	    if ((Game->CurrMove->Label [0] == ' ')
		|| (Game->CurrMove->Label [0] == 0))
		Game->CurrMove->Label [0]= 'a';
	    LabelToPanel (Game->CurrMove->Next);
	    m= AllocMove (i, j, Game->CurrMove->Prev);
	    m->AltPrev= Game->CurrMove;
	    m->AltNext= Game->CurrMove->AltNext;
	    Game->CurrMove->AltNext= m;
	    m->AltNext->AltPrev= m;
	    if (Game->CurrMove->Prev == NULL) Game->MoveList= m;
	    else Game->CurrMove->Prev->Next= m;
	    for (m1= m->AltNext, Count= 0; m1 != m; m1= m1->AltNext) Count++;
	    m->Label [0]= 'a' + Count;
	    Backup (Game, Draw);
	} else Cafard ("AltMoveAction:  Game->CurrMove == NULL");
#endif
    }
    Step (Game, Draw);
    return 0;
} /* EnterMove */


HighlightMove (Game, m, ii, jj)
    GameType	*Game;
    MoveType	*m;
    int 	ii, jj;
/* If (m) use it for i and j; otherwise use ii and jj. */
{
int i, j;
int Color;
    if (m) {
	i= m->i;
	j= m->j;
    } else {
	i= ii;
	j= jj;
    }
    if (Game->Board [i][j] == Black) Color= 0; else Color= 1;
    if (1 || m->Label [0] < 'a')
	Disk (BoardPW, 2 * LineWidth, GridX (i), GridY (j), Color);
    else
	pw_text (BoardPW, GridX (i) - Radius / 2 + 1
	    , GridY (j) + Radius / 2
	    , PIX_SRC ^ PIX_DST, Font, m->Label);
} /* HighlightMove */


NewText (Msg)
    char *Msg;
{
char Junk [3];
    if (Msg == NULL) {
	Junk [0]= '\n';
	Junk [1]= '\n';
	Junk [2]= '\0';
	Msg= Junk;
    }
    textsw_replace (TextSW, 0, TEXTSW_INFINITY, Msg, strlen (Msg));
    window_set (TextSW, TEXTSW_INSERTION_POINT, TEXTSW_INFINITY, 0);
    textsw_insert (TextSW, "\n", 1);
    window_set (TextSW, TEXTSW_INSERTION_POINT, 0, 0);
    UserTextMods= 0;
    if (Verbose) textsw_normalize_view (TextSW, 0);
    else textsw_normalize_view (TextSW, TEXTSW_INFINITY);
/*    textsw_display (TextSW);*/
} /* NewText */


Replay (Game, ReCompBoard)
    GameType *Game;
    int ReCompBoard;
{
struct rect *r;
    r= (struct rect *) window_get (BoardSW, WIN_RECT);
    pw_writebackground (BoardPW, 0, 0, r->r_width, r->r_height, PIX_CLR);
/*
    DrawGameInit (Game, r->r_width, r->r_height);
*/
    DrawGame (Game, ReCompBoard);
} /* Replay */


SetHandicap (Game, Value)
    GameType *Game;
    int Value;
/* Set handicap value and set first-player accordingly. */
{
    if (Value < 2) Value= 0;
    else if (Value > MaxHandicap) Value= MaxHandicap;
    if (Value == 0) {
	Game->Handicap= 0;
	Game->FirstPlayer= Black;
    } else {
	Game->Handicap= Value;
	Game->FirstPlayer= White;
    }
} /* SetHandicap */


Sideways (Game, Draw)
    GameType *Game;
    int Draw;
{
MoveType *m;
    m= Game->CurrMove->AltNext;
    if (Game->CurrMove->Prev != NULL) {
	Backup (Game, Draw);
	Game->CurrMove->Next= m;
	Step (Game, Draw);
    }
} /* Sideways */


Step (Game, Draw)
    GameType *Game;
    int Draw;
{
int	i, j;
Stone	s;
CaptureType	*c;
    if (Game->Placing) return;
    if (Game->CurrMove == NULL) {
	Game->MoveNum= 0;
	if (Game->MoveList == NULL) return;
	Game->CurrMove= Game->MoveList;
    } else {
	if (Draw) {
	    PanelToLabel (Game->CurrMove->Next);
	    if (Game->CurrMove->Next)
		DrawAltsEmpty (Game, Game->CurrMove);
	}
	if (Game->CurrMove->Next == NULL) return;
	Game->CurrMove= Game->CurrMove->Next;
    }
    Game->MoveNum++;
    if (Game->MoveNum & 1) s= Game->FirstPlayer;
    else s= !Game->FirstPlayer;
    if (Draw) SetFirstPlayerButton (!s, PANEL_CLEAR);
    i= Game->CurrMove->i;
    if (i >= 0) {
	Game->Board [i] [j= Game->CurrMove->j]= s;
	if (Draw) DrawMove (Game, Game->CurrMove, i, j);
	NewStoneToGroups (Game, i, j);
	if (GenCaptures (Game, !s, Game->CurrMove)) {
	    if (Draw) {
		for (c= Game->CurrMove->Captures; c != NULL; c= c->Next) {
		    DrawEmpty (Game, c->i, c->j);
		}
	    }
	}
    }
    if (Draw) {
	Status (Game);
	StdMsg ();
	if ((Game->CurrMove) && (Game->CurrMove->Prev)) {
	    i= Game->CurrMove->Prev->i;
	    j= Game->CurrMove->Prev->j;
	    UnHighlightMove (Game, i, j);
	}
	if (Game->CurrMove) {
	    if (Game->CurrMove->Next) DrawAlts (Game, Game->CurrMove);
	    NewText (Game->CurrMove->Note);
	    LabelToPanel (Game->CurrMove->Next);
	} else {
	    NewText (Game->CurrMove->Note);
	}
    }
} /* Step */


UnHighlightMove (Game, i, j)
    GameType	*Game;
    int 	i, j;
{
int Color;
    if (Game->Board [i][j] == Black) Color= 1;
    else if (Game->Board [i][j] == White) Color= 0;
    else return;
    Disk (BoardPW, 2 * LineWidth, GridX (i), GridY (j), Color);
} /* UnHighlightMove */
