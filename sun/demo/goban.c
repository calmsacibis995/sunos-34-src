#ifndef lint
static char sccsid[]= "@(#)goban.c 1.3 87/01/11 Copyr 1986 Sun Micro";
#endif

/* Copyright (c) Sun MicroSystems, 1986.  This code may be used and modified
    for not-for-profit use.  This notice must be retained.
*/

/* goban.c
    "Go ban" is the Japanese phrase for a go board.  goban acts
as an automatic go board.  It removes captured stones, saves
and retrieves games, and steps forwards and backwards through a
game.  See also Readme.goban.
*/

#include "goban.h"
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <suntool/textsw.h>


static short IconBits [] = {
#include "goban.icon"
};
DEFINE_ICON_FROM_IMAGE (GobanIcon, IconBits);


int		RootFD;
Frame		GoFrame;
Canvas		BoardSW;
Pixwin		*BoardPW;
struct pixrect	*BlackStone, *WhiteStone, *BlackFirstImage, *WhiteFirstImage;
struct pixfont	*Font, *Fonts [4];
Panel		MainPanel;
Panel		MsgSW, StatusSW;
Panel_item	CurrMoveSlider, HandicapSlider, FirstPlayerButton;
Panel_item	PlaceMoveButton, NameButton, LabelButton;
Panel_item	MsgPanelItem, StatusPanelItem;
Textsw		TextSW;
int		(*CachedTextSWProc) ();
int		UserTextMods, Verbose;

int RemBSWWidth, RemBSWHeight;
int Radius;
int LineWidth;
int DebugLevel;
int Checked [MaxGrid] [MaxGrid];	/* Initialized to zero. */
int CheckNum;				/* Initialized to 1. */

char	GameName [80] = " ";


/* Forward declarations. */
BSWEventProc ();
BSWRepaintProc ();
BSWResizeProc ();
MoveType	*IsAltOfNextMove ();
TextSWProc ();

extern SigPipe ();


main (argc, argv)
    int argc;
    char *argv [];
{
GameType	*Game;
int i, j;
    for (i= 0; i < MaxGrid; i++)
	for (j= 0; j < MaxGrid; j++)
	    Checked [i][j]= 0;
    CheckNum= 1;
    Game= Malloc (GameType);
    Hello (&argc, argv, Game);
    if (GameName [0] == ' ') {
	DfltGame (GameName, Game, PANEL_CLEAR);
    } else {
	if (ReadGame (GameName, Game, PANEL_CLEAR)) {
	    fprintf (stderr, "Unable to read \"%s\".\n", GameName);
	    ZeroGame (Game, PANEL_CLEAR);
	    HandicapBoard (Game);
	}
    }
    window_main_loop (GoFrame);
} /* main */


BSWEventProc (canvas, event)
    Canvas	canvas;
    Event	*event;
{
int		i, j, k;
GameType	*Game;
MoveType	*Move;
    if (event_is_up (event)) return;
    Game= (GameType *) window_get (BoardSW, WIN_CLIENT_DATA);
    if (UserTextMods) SaveText (Game);
    switch (event_id (event)) {
    case '\003':
	exit (0); break;
    case MS_LEFT:
	i= (event_x (event) - 5 - 4 * Radius) / (2 * Radius);
	j= Game->Grid - 1 - (event_y (event) - 5 - 4 * Radius) / (2 * Radius);
	if (event_ctrl_is_down (event))
	    EnterMove (i, j, Game, 1, MoveAction);
	else if (Move= IsAltOfNextMove (Game->CurrMove, i, j)) {
	    if (Game->CurrMove->Next != Move) {
		PanelToLabel (Game->CurrMove->Next);
		Game->CurrMove->Next= Move;
		for (k= Game->MoveNum; Move->Next != NULL; Move= Move->Next)
		    k++;
		panel_set (CurrMoveSlider, PANEL_MAX_VALUE, k, 0);
		LabelToPanel (Game->CurrMove->Next);
	    }
	    Step (Game, 1);
	} else EnterMove (i, j, Game, 1, AltMoveAction);
	break;
    case MS_MIDDLE:
	if (Game->Placing) {
	    i= (event_x (event) - 5 - 4 * Radius) / (2 * Radius);
	    j= Game->Grid - 1 - (event_y(event) - 5 - 4 * Radius) / (2*Radius);
	    Game->Board [i][j]= Game->InitialBoard [i][j]= Empty;
	    DrawEmpty (Game, i, j);
	} else
	    Backup (Game, 1);
	break;
    case MS_RIGHT:
	if (event_ctrl_is_down (event)) Sideways (Game, 1);
	else Step (Game, 1);
	break;
    }
} /* BSWEventProc */


BSWRepaintProc (canvas, pw, RepaintRects)
    Canvas	canvas;
    Pixwin	*pw;
    Rectlist	RepaintRects;
{
GameType	*Game;
int		Width, Height;
    Width= (int) window_get (canvas, CANVAS_WIDTH);
    Height= (int) window_get (canvas, CANVAS_HEIGHT);
    Game= (GameType *) window_get (BoardSW, WIN_CLIENT_DATA);
    pw_writebackground (BoardPW, 0, 0, Width, Height, PIX_CLR);
    DrawGame (Game, 1);
} /* BSWRepaintProc */


BSWResizeProc (canvas, Width, Height)
    Canvas canvas;
    int Width, Height;
{
struct rect *r;
GameType *Game= (GameType *) window_get (BoardSW, WIN_CLIENT_DATA);
    DrawGameInit (Game, Width, Height);
} /* BSWResizeProc */


Hello (argc, argv, Game)
    int *argc;
    char *argv [];
    GameType *Game;
{
char	FrameName [80];
char	*c, *g;
    Verbose= 1;
    strncpy (FrameName, argv [0], 75);
    strcat (FrameName, ".3.2");
    GoFrame= window_create (NULL, FRAME,
	FRAME_ICON,		&GobanIcon,
	FRAME_ARGC_PTR_ARGV,	argc, argv,
	FRAME_LABEL,		FrameName,
	WIN_HEIGHT,		900,
	WIN_Y,			0,
	0);
    while (--(*argc) > 0) {
        if ('-' == (*++argv) [0]) {
            switch ((*argv) [1]) {
	    case 'd':
                c= &((*argv) [2]);
		if (*c == '\0') {
		    --(*argc);
		    c= &((*++argv) [0]);
		}
		DebugLevel= atoi (c);
		break;
            case 'g':
                break;
            } /* switch */
        } else {
	    c= &((*argv) [0]);
	    g= GameName;
	    do {
		*g++= *c;
	    } while (*c++ != '\0');
	} /* else */
    } /* while */
    Font= pw_pfsysopen ();
    InitSW (GoFrame, Game);
    Fonts [0]= pf_open ("/usr/lib/fonts/fixedwidthfonts/screen.r.7");
    if (Fonts [0] == NULL)
	fprintf (stderr
	    , "Can't open \"/usr/lib/fonts/fixedwidthfonts/screen.r.7\".\n");
    Fonts [1]= pf_open ("/usr/lib/fonts/fixedwidthfonts/screen.r.11");
    if (Fonts [1] == NULL)
	fprintf (stderr
	    , "Can't open \"/usr/lib/fonts/fixedwidthfonts/screen.r.11\".\n");
    Fonts [2]= pf_open ("/usr/lib/fonts/fixedwidthfonts/screen.r.14");
    if (Fonts [2] == NULL)
	fprintf (stderr
	    , "Can't open \"/usr/lib/fonts/fixedwidthfonts/screen.r.14\".\n");
    Fonts [3]= pf_open ("/usr/lib/fonts/fixedwidthfonts/gallant.r.19");
    if (Fonts [3] == NULL)
	fprintf (stderr
	    , "Can't open \"/usr/lib/fonts/fixedwidthfonts/gallant.r.19\".\n");
    RootFD= (int) window_get (GoFrame, WIN_FD);
    signal (SIGPIPE, SigPipe);
} /* Hello */


InitSW (GoFrame, Game)
    Frame	GoFrame;
    GameType	*Game;
/* Initialize board subwindow. */
{
struct inputmask Mask;
int Width;	/* Assume square. */
    MsgSW= (Panel) window_create (GoFrame, PANEL,
	WIN_HEIGHT,		MsgSWHeight,
	0);
    StatusSW= (Panel) window_create (GoFrame, PANEL,
	WIN_HEIGHT,		StatusSWHeight,
	0);
    MainPanel= window_create (GoFrame, PANEL, 0);
    InitPanel (Game, GameName, MsgSW, StatusSW);
    window_fit (MainPanel);
    Width= ((struct rect *) window_get (GoFrame, FRAME_OPEN_RECT))->r_width;
    BoardSW= window_create (GoFrame, CANVAS,
	WIN_WIDTH,		Width - 10,
	WIN_HEIGHT,		Width - 10,
	CANVAS_WIDTH,		Width - 10,
	CANVAS_HEIGHT,		Width - 10,
	WIN_BELOW,		MainPanel,
	WIN_X,			0,
	WIN_CLIENT_DATA,	Game,
	WIN_EVENT_PROC,		BSWEventProc,
	WIN_CONSUME_PICK_EVENTS, WIN_NO_EVENTS, WIN_MOUSE_BUTTONS, 0,
	WIN_CONSUME_KBD_EVENTS,	WIN_NO_EVENTS, WIN_ASCII_EVENTS, 0,
	CANVAS_RESIZE_PROC,	BSWResizeProc,
	CANVAS_REPAINT_PROC,	BSWRepaintProc,
	CANVAS_RETAINED,	FALSE,
	CANVAS_FIXED_IMAGE,	FALSE,
	0);
    if (BoardSW == NULL)
	Cafard ("Couldn't create BSW subwindow");
    BoardPW = (Pixwin *) window_get (BoardSW, WIN_PIXWIN);
    if (BoardPW == (struct pixwin *) NULL)
	Cafard ("Couldn't create BSW pixwin");
    TextSW= window_create (GoFrame, TEXTSW,
	WIN_BELOW,	BoardSW,
	WIN_X,		0,
	0);
    CachedTextSWProc= (int (*) ()) window_get (TextSW, TEXTSW_NOTIFY_PROC);
    window_set (TextSW, TEXTSW_NOTIFY_PROC, TextSWProc, 0);
    window_set (TextSW, TEXTSW_NOTIFY_LEVEL
	, TEXTSW_NOTIFY_STANDARD | TEXTSW_NOTIFY_EDIT, 0);
} /* InitSW */


    MoveType *
IsAltOfNextMove (m, i, j)
    MoveType *m;
    int	i, j;
{
MoveType *m1;
    if (m == NULL) return 0;
    for (m1= m->Next; m1 != NULL; ) {
	if ((m1->i == i) && (m1->j == j)) return m1;
	m1= m1->AltNext;
	if (m1 == m->Next) return 0;
    }
    return 0;
} /* IsAltOfNextMove */


SaveText (Game)
    GameType *Game;
{
    if (Game != NULL) {
	if (Game->CurrMove == NULL) {
	    TextSWToData (&(Game->Note));
	} else {
	    TextSWToData (&(Game->CurrMove->Note));
	}
	UserTextMods= 0;
    }
} /* SaveText */


TextSWProc (textsw, attributes)
    Textsw	textsw;
    Attr_avlist	attributes;
{
Attr_avlist	attrs;
    for (attrs= attributes; *attrs; attrs= attr_next (attrs)) {
	UserTextMods++;
    }
    CachedTextSWProc (textsw, attributes);
} /* TextSWProc */


TextSWToData (StringPtrPtr)
    char	**StringPtrPtr;
{
char	*s;
int Length;
    if (*StringPtrPtr != NULL) {
	free (*StringPtrPtr);
	*StringPtrPtr= NULL;
    }
    Length= (int) window_get (TextSW, TEXTSW_LENGTH);
    if (Length != 0) {
	s= *StringPtrPtr= (char *) malloc (Length);
	window_get (TextSW, TEXTSW_CONTENTS, 0, s, Length - 1);
	    /* Ignore added last newline character. */
	s [Length - 1]= '\0';
    }
} /* TextSWToData */
