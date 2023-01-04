/* @(#)goban.h 1.3 87/01/11 Copyr 1986 Sun Micro
    Copyright (c) Sun MicroSystems, 1986.  This code may be used and modified
    for not-for-profit use.  This notice must be retained.
*/

/* goban.h
    Include file for goban program.  See goban.c.
*/

#include <suntool/sunview.h>
#include <stdio.h>
#include <sys/file.h>

#define Black	0
#define White	1
    /* White == !Black. */
#define Empty	2

/* Actions */
#define MoveAction	1
#define	PassAction	2
#define AltMoveAction	3

#define NoDir	0
#define North	(1 << 1)
#define East	(1 << 2)
#define South	(1 << 3)
#define West	(1 << 4)

#define MsgSWHeight	 25
#define StatusSWHeight	 25

#define MaxGrid		21
#define MaxHandicap	17
#define MaxMoves	((MaxGrid * MaxGrid * 3) / 2)
#define MaxTitleChars	40
#define MaxMvLblChars	2

#define Malloc(a)	(a *) malloc (sizeof (a))

typedef short Stone;

typedef struct MoveType {
    char i, j;			/* i == -1 means pass. */
    struct CaptureType *Captures;
    struct MoveType *Next, *Prev, *AltNext, *AltPrev;
    char *Note;
    char Label [MaxMvLblChars];
} MoveType;

typedef struct CaptureType {
    unsigned char i, j;
    struct CaptureType *Next;
} CaptureType;

typedef struct GroupType {
    short Color;
    short NumLibs;
    short NumStones;
    struct GroupType *Next;	/* for Trash list only. */
} GroupType;

typedef struct GameType {
    char Title [40];	/* keyword of filenames Title.brd and Title.game. */
    char *Note;
    Stone Board [MaxGrid] [MaxGrid];
    Stone InitialBoard [MaxGrid] [MaxGrid];
    GroupType *Groups [MaxGrid] [MaxGrid];
    MoveType *MoveList, *CurrMove;
    short NumMoves;		/* Gets set to MoveNum if player changes play.*/
    short MoveNum;		/* Initially NumMoves. */
    short Handicap;
    short Prisoners [2];	/* Prisoner stones colored with given color. */
    Stone FirstPlayer;		/* Black or White. */
    short Placing;		/* 1 if user placing stones (not yet moving)*/
    short Grid;
	/* Size of board:  9 (neophyte), 13 (novice), 17 (historical),
	    19 (standard), 21 (experimental). */
} GameType;

extern MoveType		*TrashMoves;
extern CaptureType	*TrashCaptures;
extern GroupType	*TrashGroups;

extern struct screen	Screen;
extern int		RootFD;
extern Frame		GoFrame;
extern Pixwin		*BoardPW;
extern struct pixrect	*BlackStone, *WhiteStone;
extern struct pixrect	*BlackFirstImage, *WhiteFirstImage;
extern struct pixfont	*Font, *Fonts [4];

extern int RemBSWWidth, RemBSWHeight;
extern int Radius, LineWidth;
extern int DebugLevel;
extern int Checked [MaxGrid][MaxGrid];
extern int CheckNum;

extern MoveType 	*AllocMove ();
extern CaptureType	*AllocCapture ();
extern GroupType	*AllocGroup ();
