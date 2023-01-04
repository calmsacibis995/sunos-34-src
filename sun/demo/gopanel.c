#ifndef lint
static char sccsid[]= "@(#)gopanel.c 1.3 87/01/11 Copyr 1986 Sun Micro";
#endif

/* Copyright (c) Sun MicroSystems, 1986.  This code may be used and modified
    for not-for-profit use.  This notice must be retained.
*/

/* gopanel.c
    Main control panel for goban.
*/

#include "goban.h"
#include <suntool/canvas.h>
#include <suntool/panel.h>
#include <suntool/textsw.h>
#include <suntool/fullscreen.h>


struct pixrect	*MovingImage, *PlacingImage;
char StdMessage [] =
"Left:  new branch.  Ctrl left:  place.  Center:  Backup.  Right:  Step.";

extern Canvas		BoardSW;
extern Panel_item	CurrMoveSlider;
extern Panel_item	FirstPlayerButton;
extern Panel_item	HandicapSlider;
extern Panel_item	LabelButton;
extern Panel		MainPanel;
extern Panel_item	MsgPanelItem;
extern Panel_item	NameButton;
extern Panel_item	PlaceMoveButton;
extern Panel_item	StatusPanelItem;
extern Textsw		TextSW;
extern int		UserTextMods;
extern int		Verbose;

extern	SaveText ();

/* Forward */
AltProc ();
CloseProc ();
FirstPlayProc ();
HandicapProc ();
LeafwardNotify ();
Panel_setting	MoveLabelNotify ();
PassProc ();
PlaceMoveProc ();
PrintProc ();
PruneProc ();
Quit ();
ReadGameProc ();
ReplayProc ();
ResetProc ();
RootwardNotify ();
SetCurrMove ();
VerboseNotify ();
WriteBoardProc ();
WriteGameProc ();


AltProc (PI, GoEvent)
    Panel_item PI;
    struct inputevent *GoEvent;
{
GameType	*Game;
MoveType	*m;
    Game= (GameType *) panel_get (PI, PANEL_CLIENT_DATA);
    if ((Game->CurrMove != NULL) && (Game->CurrMove->AltNext != NULL)) {
	Sideways (Game, 1);
    }
} /* AltProc */


CloseProc (PI, GoEvent)
    Panel_item PI;
    struct inputevent *GoEvent;
/* Close tool to icon state. */
{
    wmgr_close (window_get (GoFrame, WIN_FD), RootFD);
} /* CloseProc */


    int
Confirm ()
{
struct fullscreen	*FullScreen;
struct inputmask	im;
struct inputevent	ie;
int			result;
int			FD;
    Msg ("Confirm:  right mouse button.  Deny: left & center buttons.");
    FD= (int) window_get (MainPanel, WIN_FD);
    FullScreen= (struct fullscreen *) fullscreen_init (FD);
    input_imnull (&im);
    win_setinputcodebit (&im, MS_LEFT);
    win_setinputcodebit (&im, MS_MIDDLE);
    win_setinputcodebit (&im, MS_RIGHT);
    win_setinputmask (FD, &im, (struct inputmask *) NULL, WIN_NULLLINK);
    for (;;) {	/* improve this do loop to eliminate for (;;). */
	if (input_readevent (FD, &ie) == -1)  {
	    perror("Confirm input failed.");
	    abort ();
	}
	switch (ie.ie_code)  {
	 case MS_LEFT:         result = FALSE; break;
	 case MS_MIDDLE:       result = FALSE; break;
	 case MS_RIGHT:        result = TRUE;  break;
	 default:              continue;
	}
	break;
    }
    fullscreen_destroy (FullScreen);
    StdMsg ();
    return result;
} /* Confirm */


FirstPlayProc (PI, GoEvent)
    Panel_item PI;
    struct inputevent *GoEvent;
/* Toggles first player of game. */
{
GameType *Game;
    Game= (GameType *) panel_get (PI, PANEL_CLIENT_DATA);
    if (Game->NumMoves == 0) SetFirst (Game, !Game->FirstPlayer, PANEL_CLEAR);
} /* FirstPlayProc */


    struct pixrect *
GenStoneImage (p, Color)
    struct pixrect *p;
    int Color;
{
int Width, Height, LineWid;
int i;
    if (p != NULL) free (p);
    Width= 25;
    Height= 25;
    LineWid= 2;
    p= mem_create (Width, Height, 1);
    for (i= 0; i < LineWid; i++) {
	pr_vector (p, 0, Height/2 + i, Width, Height/2 + i, PIX_SRC, 1);
	pr_vector (p, Width/2 + i, 0, Width/2 +i, Height, PIX_SRC, 1);
    }
    DiskPR (p, Width / 3, Width / 2, Width / 2, 1);
    if (Color == White) DiskPR (p, Width/3 - LineWid, Width/2, Width/2, 0);
    return p;
} /* GenStoneImage */


HandicapProc (PI, Value, GoEvent)
    Panel_item PI;
    unsigned int Value;
    struct inputevent *GoEvent;
/* Sets handicap for game. */
{
GameType *Game;
    Game= (GameType *) panel_get (PI, PANEL_CLIENT_DATA);
    SetHandicap (Game, (int) Value);
} /* HandicapProc */


InitPanel (Game, GameName, MsgSW, StatusSW)
    GameType	*Game;
    char	*GameName;
    Panel	MsgSW, StatusSW;
{
Panel_item	Junk;
#define NullFont	(struct pixfont *) NULL
    BlackFirstImage= GenStoneImage (BlackFirstImage, Black);
    WhiteFirstImage= GenStoneImage (WhiteFirstImage, White);
    FirstPlayerButton= panel_create_item (MainPanel, PANEL_CHOICE
	, PANEL_DISPLAY_LEVEL,	PANEL_CURRENT
	, PANEL_CLIENT_DATA,	(caddr_t) Game
	, PANEL_NOTIFY_PROC,	FirstPlayProc
	, PANEL_LABEL_STRING,	"Next: "
	, PANEL_CHOICE_IMAGES,	BlackFirstImage, WhiteFirstImage, 0
	, PANEL_PAINT,		PANEL_NONE
	, 0);	/* Remove PANEL_PAINT line in 3.0. */
    MovingImage= panel_button_image (MainPanel, "Moving", 8, NullFont);
    PlacingImage= panel_button_image (MainPanel, "Placing", 9, NullFont);
    PlaceMoveButton= panel_create_item (MainPanel, PANEL_CHOICE
	, PANEL_DISPLAY_LEVEL,	PANEL_CURRENT
	, PANEL_CLIENT_DATA,	(caddr_t) Game
	, PANEL_NOTIFY_PROC,	PlaceMoveProc
	, PANEL_CHOICE_IMAGES,	PlacingImage, MovingImage, 0
	, PANEL_VALUE,		1
	, 0);
    Junk= panel_create_item (MainPanel, PANEL_BUTTON
	, PANEL_CLIENT_DATA,	(caddr_t) Game
	, PANEL_NOTIFY_PROC,	PassProc
	, PANEL_LABEL_IMAGE, panel_button_image (MainPanel, "Pass", 6, NullFont)
	, 0);
    Junk= panel_create_item (MainPanel, PANEL_BUTTON
	, PANEL_CLIENT_DATA,	(caddr_t) Game
	, PANEL_NOTIFY_PROC,	ReplayProc
	, PANEL_LABEL_IMAGE
	    , panel_button_image (MainPanel, "Replay", 8, NullFont)
	, 0);
/*    Junk= panel_create_item (MainPanel, PANEL_BUTTON
	, PANEL_CLIENT_DATA,	(caddr_t) Game
	, PANEL_NOTIFY_PROC,	AltProc
	, PANEL_LABEL_IMAGE, panel_button_image (MainPanel, "Alt", 5, NullFont)
	, 0);
*/
    Junk= panel_create_item (MainPanel, PANEL_BUTTON
	, PANEL_CLIENT_DATA,	(caddr_t) Game
	, PANEL_NOTIFY_PROC,	PruneProc
	, PANEL_LABEL_IMAGE
	    , panel_button_image (MainPanel, "Prune", 7, NullFont)
	, 0);
#ifdef LABEL_BUTTON
    LabelButton= panel_create_item (MainPanel, PANEL_TEXT
	, PANEL_CLIENT_DATA,	(caddr_t) Game
	, PANEL_VALUE,		NULL
	, PANEL_VALUE_DISPLAY_LENGTH,	MaxMvLblChars-1
	, PANEL_LABEL_STRING,	"Label: "
	, PANEL_NOTIFY_LEVEL,	PANEL_ALL
	, PANEL_NOTIFY_PROC,	MoveLabelNotify
	, 0);
#endif LABEL_BUTTON
    Junk= panel_create_item (MainPanel, PANEL_BUTTON
	, PANEL_CLIENT_DATA,	(caddr_t) Game
	, PANEL_VALUE,		GameName
	, PANEL_NOTIFY_PROC,	RootwardNotify
	, PANEL_LABEL_IMAGE
	    , panel_button_image (MainPanel, "Rootward", 10, NullFont)
	, 0);
    Junk= panel_create_item (MainPanel, PANEL_BUTTON
	, PANEL_CLIENT_DATA,	(caddr_t) Game
	, PANEL_VALUE,		GameName
	, PANEL_NOTIFY_PROC,	LeafwardNotify
	, PANEL_LABEL_IMAGE
	    , panel_button_image (MainPanel, "Leafward", 10, NullFont)
	, 0);
    CurrMoveSlider= panel_create_item (MainPanel, PANEL_SLIDER
	, PANEL_CLIENT_DATA,	(caddr_t) Game
	, PANEL_NOTIFY_PROC,	SetCurrMove
	, PANEL_MAX_VALUE,	MaxMoves
	, PANEL_VALUE,		Game->NumMoves
	, PANEL_LAYOUT,		PANEL_VERTICAL
	, PANEL_SLIDER_WIDTH,	250
	, PANEL_LABEL_STRING,	"Current Move: "
	, 0);
    HandicapSlider= panel_create_item (MainPanel, PANEL_SLIDER
	, PANEL_CLIENT_DATA,	(caddr_t) Game
	, PANEL_NOTIFY_PROC,	HandicapProc
	, PANEL_MAX_VALUE,	MaxHandicap
	, PANEL_VALUE,		Game->Handicap
	, PANEL_LAYOUT,		PANEL_VERTICAL
	, PANEL_LABEL_STRING,	"Handicap: "
	, PANEL_PAINT,		PANEL_NONE
	, 0);	/* Remove PANEL_PAINT line in 3.0. */
    NameButton= panel_create_item (MainPanel, PANEL_TEXT
	, PANEL_CLIENT_DATA,	(caddr_t) Game
	, PANEL_VALUE,		GameName
	, PANEL_VALUE_DISPLAY_LENGTH,	16
	, PANEL_LABEL_STRING,	"Game: "
	, 0);
    Junk= panel_create_item (MainPanel, PANEL_BUTTON
	, PANEL_CLIENT_DATA,	(caddr_t) NameButton
	, PANEL_NOTIFY_PROC,	ReadGameProc
	, PANEL_VALUE,		GameName
	, PANEL_LABEL_IMAGE,
	    panel_button_image (MainPanel, "Read Game", 11, NullFont)
	, 0);
    Junk= panel_create_item (MainPanel, PANEL_BUTTON
	, PANEL_CLIENT_DATA,	(caddr_t) NameButton
	, PANEL_NOTIFY_PROC,	PrintProc
	, PANEL_VALUE,		GameName
	, PANEL_LABEL_IMAGE
	    , panel_button_image (MainPanel, "Print", 7, NullFont)
	, 0);
    Junk= panel_create_item (MainPanel, PANEL_BUTTON
	, PANEL_CLIENT_DATA,	(caddr_t) NameButton
	, PANEL_VALUE,		GameName
	, PANEL_NOTIFY_PROC,	WriteGameProc
	, PANEL_LABEL_IMAGE,
	    panel_button_image (MainPanel, "Write Game", 12, NullFont)
	, 0);
    Junk= panel_create_item (MainPanel, PANEL_BUTTON
	, PANEL_CLIENT_DATA,	(caddr_t) NameButton
	, PANEL_VALUE,		GameName
	, PANEL_NOTIFY_PROC,	WriteBoardProc
	, PANEL_LABEL_IMAGE,
	    panel_button_image (MainPanel, "Write Board", 13, NullFont)
	, 0);
    Junk= panel_create_item (MainPanel, PANEL_CHOICE
	, PANEL_CLIENT_DATA,	(caddr_t) Game
	, PANEL_NOTIFY_PROC,	VerboseNotify
	, PANEL_CHOICE_STRINGS,	"Verbose", "Silent", 0
	, PANEL_VALUE,		Verbose
	, PANEL_DISPLAY_LEVEL,	PANEL_CURRENT
	, 0);
    Junk= panel_create_item (MainPanel, PANEL_BUTTON
	, PANEL_CLIENT_DATA,	(caddr_t) Game
	, PANEL_NOTIFY_PROC,	CloseProc
	, PANEL_LABEL_IMAGE
	    , panel_button_image (MainPanel, "Close", 7, NullFont)
	, 0);
    Junk= panel_create_item (MainPanel, PANEL_BUTTON
	, PANEL_CLIENT_DATA,	(caddr_t) Game
	, PANEL_NOTIFY_PROC,	Quit
	, PANEL_LABEL_IMAGE, panel_button_image (MainPanel, "Quit", 6, NullFont)
	, 0);
    Junk= panel_create_item (MainPanel, PANEL_BUTTON
	, PANEL_CLIENT_DATA,	(caddr_t) Game
	, PANEL_NOTIFY_PROC,	ResetProc
	, PANEL_LABEL_IMAGE
	    , panel_button_image (MainPanel, "Reset", 7, NullFont)
	, 0);
    MsgPanelItem= panel_create_item (MsgSW, PANEL_MESSAGE
	, PANEL_LABEL_STRING, StdMessage, 0);
    StatusPanelItem= panel_create_item (StatusSW, PANEL_MESSAGE, 0);
} /* InitPanel */


LabelToPanel (m)
    MoveType *m;
{
char *c;
    if (m == NULL) c= " ";
    else c= m->Label;
    panel_set (LabelButton, PANEL_VALUE, c, 0);
} /* LabelToPanel */


LeafwardNotify (PI, event)
    Panel_item	PI;
    Event	*event;
{
GameType *Game;
MoveType *Move;
    Game= (GameType *) panel_get (PI, PANEL_CLIENT_DATA);
    if (UserTextMods) SaveText (Game);
    Step (Game, 0);
    if (Game->CurrMove == NULL) return;
    for (Move= Game->CurrMove->Next; Move != NULL; Move= Move->Next) {
	if (Move->AltNext != Move) break;
	Step (Game, 0);
    }
    Replay (Game, 1);
} /* LeafwardNotify */


    Panel_setting
MoveLabelNotify (PI, GoEvent)
    Panel_item	PI;
    Event	*GoEvent;
{
GameType *Game;
char	c, s [2];
    Game= (GameType *) panel_get (PI, PANEL_CLIENT_DATA);
    if (Game->CurrMove != NULL) {
	c= event_id (GoEvent);
	s[0]= c;
	s[1]= '\0';
	if ((c == '\127') || (c == '\010')) c= '\0'; /* del or back-space */
	if ((c == '\0')
	 || ((c >= 'a') && (c <= 'z'))
	 || ((c >= 'A') && (c <= 'Z')))
	    panel_set_value (PI, s);
	PanelToLabel (Game->CurrMove->Next);
	DrawAlts (Game, Game->CurrMove);
    }
    return panel_text_notify (PI, GoEvent);
} /* MoveLabelNotify */


Msg (MsgString)
    char *MsgString;
{
    panel_set (MsgPanelItem, PANEL_LABEL_STRING, MsgString, 0);
} /* MsgString */


PanelToLabel (m)
    MoveType *m;
{
#ifdef LABEL_BUTTON
char *s;
    if (m == NULL) return;
    s= (char *) panel_get (LabelButton, PANEL_VALUE);
/*    printf ("PL %s:  %s\n", s, m->Note);
*/
    m->Label [0]= *s;
    m->Label [1]= '\0';
#endif LABEL_BUTTON
} /* PanelToLabel */


PassProc (PI, GoEvent)
    Panel_item PI;
    struct inputevent *GoEvent;
/* Allows a player to pass on a move. */
{
GameType *Game;
    Game= (GameType *) panel_get (PI, PANEL_CLIENT_DATA);
    if (EnterMove (-1, -1, Game, 1, PassAction)) StatusMsg ("Unable to pass.");
} /* PassProc */


PlaceMoveProc (PI, GoEvent)
    Panel_item PI;
    struct inputevent *GoEvent;
/* Toggles Placing or Moving state and Panel button. */
{
GameType *Game;
    Game= (GameType *) panel_get (PI, PANEL_CLIENT_DATA);
    if (Game->Placing == 0) {
	if (Game->MoveNum == 0) {
	    Game->Placing= 1;
	    panel_set_value (PlaceMoveButton, 0);
	}
    } else {
	Game->Placing= 0;
	panel_set_value (PlaceMoveButton, 1);
    }
} /* PlaceMoveProc */


PrintProc (PI, GoEvent)
    Panel_item PI;
    struct inputevent *GoEvent;
{
GameType *Game;
    PI= panel_get (PI, PANEL_CLIENT_DATA);
    Game= (GameType *) panel_get (PI, PANEL_CLIENT_DATA);
    PrintBoard (Game);
} /* PrintProc */


PruneProc (PI, GoEvent)
    Panel_item PI;
    struct inputevent *GoEvent;
{
GameType *Game;
MoveType *m, *m1;
int Count;
    Game= (GameType *) panel_get (PI, PANEL_CLIENT_DATA);
    m= Game->CurrMove;
    if (m == NULL) return;
    if (m->AltNext == m) {
	if (m->Prev) m->Prev->Next= NULL;
	else Game->MoveList= NULL;
    } else {
	if (!m->Prev) {
	    Game->MoveList= m->AltNext;
	} else {
	    m->Prev->Next= m->AltNext;
	}
	if (m->AltNext->AltNext == m) {
	    m->AltNext->Label [0]= ' ';
	} else {
	    for (m1= m->AltNext, Count= 0; m1 != m; m1= m1->AltNext)
		m1->Label [0]= 'a' + Count++;
	}
	m->AltNext->AltPrev= m->AltPrev;
	m->AltPrev->AltNext= m->AltNext;
    }
    m->AltNext= NULL;
    m->AltPrev= NULL;
    Backup (Game, 1);
    FreeMoves (m);
    Step (Game, 1);
} /* PruneProc */


Quit (PI, GoEvent)
    Panel_item PI;
    struct inputevent *GoEvent;
{
    if (Confirm ()) exit (0);
} /* Quit */


ReadGameProc (PI, GoEvent)
    Panel_item PI;
    struct inputevent *GoEvent;
/* Notified by panel. */
{
GameType *Game;
    if (Confirm ()) {
	PI= panel_get (PI, PANEL_CLIENT_DATA);
	Game= (GameType *) panel_get (PI, PANEL_CLIENT_DATA);
	if (ReadGame (panel_get_value (PI), Game, PANEL_CLEAR)) {
	    StatusMsg ("Can't read file.");
	    return;
	}
	Replay (Game, 1);
	Status (Game);
    }
} /* ReadGameProc */


ReplayProc (PI, GoEvent)
    Panel_item PI;
    struct inputevent *GoEvent;
{
GameType *Game;
    Game= (GameType *) panel_get (PI, PANEL_CLIENT_DATA);
    Replay (Game, 1);	/* Recompute board. */
} /* ReplayProc */


ResetProc (PI, GoEvent)
    Panel_item PI;
    struct inputevent *GoEvent;
/* Resets (clears) game. */
{
struct rect *r;
GameType *Game;
    if (Confirm ()) {
	Game= (GameType *) panel_get (PI, PANEL_CLIENT_DATA);
	r= (struct rect *) window_get (BoardSW, WIN_RECT);
	pw_writebackground (BoardPW, 0, 0, r->r_width, r->r_height, PIX_CLR);
	ZeroGame (Game, PANEL_CLEAR);
	HandicapBoard (Game);
/*
	DrawGameInit (Game, r->r_width, r->r_height);
*/
	DrawGame (Game, 0);
    }
} /* ResetProc */


RootwardNotify (PI, event)
    Panel_item	PI;
    Event	*event;
{
GameType *Game;
MoveType *Move;
    Game= (GameType *) panel_get (PI, PANEL_CLIENT_DATA);
    if (UserTextMods) SaveText (Game);
    Backup (Game, 0);
    if (Game->CurrMove == NULL) return;
    for (Move= Game->CurrMove; Move != NULL; Move= Move->Prev) {
	if (Move->Next->AltNext != Move->Next) break;
	Backup (Game, 0);
    }
    Replay (Game, 1);
} /* RootwardNotify */


SetCurrMove (PI, Value, GoEvent)
    Panel_item PI;
    unsigned int Value;
    struct inputevent *GoEvent;
{
MoveType *m;
int k;
GameType *Game;
    Game= (GameType *) panel_get (PI, PANEL_CLIENT_DATA);
    if (Value > Game->NumMoves) Game->MoveNum= Game->NumMoves;
    else Game->MoveNum= Value;
    for (m= Game->MoveList, k= 1; m != NULL; m= m->Next) {
	/* Find move corresponding to MoveNum. */
/*
	if (m->Next == NULL) break;
	if (k >= Game->MoveNum) break;
	k++;
*/
    }
    if (Game->MoveNum == 0) Game->CurrMove= NULL;
    else Game->CurrMove= m;
    Replay (Game, 1);
    Status (Game);
} /* SetCurrMove */


SetFirstPlayerButton (s, ShouldPaint)
    Stone s;
    Panel_setting ShouldPaint;
{
    if (s == Black)
	panel_set (FirstPlayerButton, PANEL_VALUE, 0
	    , PANEL_PAINT, ShouldPaint, 0);
    else
	panel_set (FirstPlayerButton, PANEL_VALUE, 1
	    , PANEL_PAINT, ShouldPaint, 0);
} /* SetFirstPlayerButton */


Status (Game)
    GameType *Game;
{
char	MsgString [80];
char	AltMsg [30];
int	i;
MoveType *m;
    panel_set (CurrMoveSlider, PANEL_VALUE, Game->MoveNum, 0);
    for (m= Game->CurrMove, i= 1; m != NULL; m= m->AltNext) {
	if (m->AltNext == Game->CurrMove) break;
	i++;
    }
    if (i > 1) sprintf (AltMsg, "%d Alternates.  ", i);
    else AltMsg [0]= '\0';
    sprintf (MsgString
	, "%sWhite stones captured %d,  Black stones captured %d.\n"
	, AltMsg
	, Game->Prisoners [White], Game->Prisoners [Black]);
    StatusMsg (MsgString);
} /* Status */


StatusMsg (MsgString)
    char *MsgString;
/* Status message in panel message subwindow. */
{
    panel_set (StatusPanelItem, PANEL_LABEL_STRING, MsgString, 0);
} /* StatusMsg */


StdMsg ()
/* Standard message in panel message subwindow. */
{
    Msg (StdMessage);
} /* StdMsg */


VerboseNotify (PI, Value, GoEvent)
    Panel_item	PI;
    int		Value;
    Event	*GoEvent;
{
GameType *Game;
int i, j;
    Game= (GameType *) panel_get (PI, PANEL_CLIENT_DATA);
    Verbose= Value;
    if (Verbose) {
	if (Game->CurrMove) {
	    HighlightMove (Game, Game->CurrMove, -1, -1);
	    if (Game->CurrMove->Next) DrawAlts (Game, Game->CurrMove);
	}
	textsw_normalize_view (TextSW, 0);
    } else {
	if (Game->CurrMove) {
	    if (Game->CurrMove->Next) DrawAltsEmpty (Game, Game->CurrMove);
	    i= Game->CurrMove->i;	/* Unhighlight currmove. */
	    j= Game->CurrMove->j;
	    UnHighlightMove (Game, i, j);
	}
	textsw_normalize_view (TextSW, TEXTSW_INFINITY);
    }
} /* VerboseNotify */


WriteBoardProc (PI, GoEvent)
    Panel_item PI;
    struct inputevent *GoEvent;
/* Notified by panel. */
{
GameType *Game;
    if (Confirm ()) {
	PI= panel_get (PI, PANEL_CLIENT_DATA);
	Game= (GameType *) panel_get (PI, PANEL_CLIENT_DATA);
	if (WriteBoard (panel_get_value (PI), Game))
	    StatusMsg ("Can't create file.");
    }
} /* WriteBoardProc */


WriteGameProc (PI, GoEvent)
    Panel_item PI;
    struct inputevent *GoEvent;
/* Notified by panel. */
{
GameType *Game;
    if (Confirm ()) {
	PI= panel_get (PI, PANEL_CLIENT_DATA);
	Game= (GameType *) panel_get (PI, PANEL_CLIENT_DATA);
	if (UserTextMods) SaveText (Game);
	if (WriteGame (panel_get_value (PI), Game))
	    StatusMsg ("Can't create file.");
    }
} /* WriteGameProc */
