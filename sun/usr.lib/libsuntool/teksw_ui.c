#ifndef lint
static	char sccsid[] = "@(#)teksw_ui.c 1.3 87/01/07 Copyr 1984 Sun Micro";
#endif

/*
 * User interface for teksw and tek
 *
 * Author: Steve Kleiman
 */

#define COPY

#include <sys/types.h>
#include <sys/time.h>

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <rasterfile.h>

#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_cursor.h>
#include <suntool/menu.h>

#include "tek.h"
#include "teksw_imp.h"

#define abs(x)		((x) >= 0? (x): -(x))
/*
* constants
*/
#define USAGE		"usage: tektool [-f fontdir] [-s [lcdeg[ce]] [-[cr] commands]\n"
#define MAXFONTNAMELEN	128			/* font name buffer size */
#define DEFAULTSTRAPS	(DELLOY)		/* default tek straps */
#define DEFAULTFONTDIR	"/usr/lib/fonts/tekfonts" /* default font directory */

/* default window sizes */
#define WXSIZE		800
#define WYSIZE		600

/* max and min addresses leaving room for character overflow and a border */
#define BORDER		4
#define WXSPACE		((2 * BORDER) + tekfont[0]->pf_defaultsize.x)
#define WYSPACE		((2 * BORDER) + tekfont[0]->pf_defaultsize.y)
#define WXMIN(TP)	(BORDER)
#define WYMIN(TP)	(BORDER+tekfont[0]->pf_defaultsize.y)
#define WXMAX(TP)	((TP)->winsize.x + WXMIN(TP))
#define WYMAX(TP)	((TP)->winsize.y + WYMIN(TP))

/*******************
*
* global data
*
*******************/

int dflag = 0;			/* debug flag */
FILE *debug;

static struct pr_size teksize = {	/* tektronix 4014 screen size */
	TXSIZE, TYSIZE
};

/* null cursor */
mpr_static(nullcur_mpr, 0, 0, 1, NULL);

struct cursor nullcursor = { 0, 0, PIX_SRC^PIX_DST, &nullcur_mpr};

/* saved cursor */
static short old_cursorimage[CUR_MAXIMAGEWORDS];

mpr_static(oldcur_mpr, 16, CUR_MAXIMAGEWORDS, 1, old_cursorimage);

static struct cursor oldcursor = { 0, 0, PIX_SRC^PIX_DST, &oldcur_mpr};

/* wait cursor */
static short hg_data[] = {
#include <images/hglass.cursor>
};

mpr_static(hglass_cursor_mpr, 16, 16, 1, hg_data);

static struct cursor waitcursor = {
	8, 8,
	PIX_SRC,
	&hglass_cursor_mpr
};

static struct pixfont *tekfont[NFONT];

static struct inputevent tekiepage = { KEY_TOP(1), 0, 0, 0, 0, {0, 0} };
#ifdef COPY
static struct inputevent tekiecopy = { KEY_TOP(4), 0, 0, 0, 0, {0, 0} };
#endif
static struct inputevent tekielocal = { KEY_TOP(2), 0, 0, 0, 0, {0, 0} };
static struct inputevent tekieonline = { KEY_TOP(3), 0, 0, 0, 0, {0, 0} };
static struct inputevent tekiereset = { KEY_TOP(1), 0, SHIFTMASK, 0, 0, {0, 0}};

static struct menuitem tekmenuitems[] = {
	{ MENU_IMAGESTRING, "Page", (caddr_t)&tekiepage },
#ifdef COPY
	{ MENU_IMAGESTRING, "Copy", (caddr_t)&tekiecopy },
#endif
	{ MENU_IMAGESTRING, "Local", (caddr_t)&tekielocal },
	{ MENU_IMAGESTRING, "Reset", (caddr_t)&tekiereset }
};

#ifdef COPY
#define LINEITEM	2		/* menu location of on/off line item */
#else
#define LINEITEM	1		/* menu location of on/off line item */
#endif

static struct menu tekmenu = {
	MENU_IMAGESTRING,
	"Tektool",
	sizeof(tekmenuitems)/sizeof(tekmenuitems[0]),
	tekmenuitems,
	0, 0
};

static struct menuitem teklocal = {
	MENU_IMAGESTRING, "Local", (caddr_t)&tekielocal
};
static struct menuitem tekonline = {
	MENU_IMAGESTRING, "Online", (caddr_t)&tekieonline
};

static int getopts();
static int parsestraps();
static void tek_size();
static void tek_usrinput();
static void alphacursoron();
static void alphacursoroff();
static void drawalphacursor();
static void removealphacursor();
static void gfxcursoron();
static void gfxcursoroff();
static void drawgfxcursor();
static void removegfxcursor();
static void tek_blinkscreen();
static void drawgfxcursor();
static void removegfxcursor();
static void wintotek();
static void tektowin();
static void screencopy();

int
tek_init(tsp, argv)
register struct teksw *tsp;
register char *argv[];
{
	struct inputmask im;
	char fontname[MAXFONTNAMELEN];
	int straps;
	extern struct pixfont *pf_open();

	/*
	* open the window
	*/
	if((tsp->pwp = pw_open_monochrome(tsp->windowfd)) == (struct pixwin *)0)
		return(0);
	/*
	* get the straps and directory for fonts
	*/
	if(getopts(argv, &straps, fontname) < 0){
		fprintf(stderr, USAGE);
		return(0);
	}
	if(tekfont[0] == (struct pixfont *)0) {
		register int i;
		register char *fnp;

		strcat(fontname, "/tekfont0");
		fnp = &fontname[strlen(fontname) - 1];
		for(i = 0; i < NFONT; i++){
			*fnp = i + '0';
			if((tekfont[i] = pf_open(fontname)) == NULL){
				fprintf(stderr, "cannot open font %s\n", fontname);
				return(0);
			}
		}
	}
	if(dflag){
		debug = fopen("tekdebug", "w");
		setbuf(debug, NULL);
	}
	/*
	* save the current cursor image
	*/
	win_getcursor(tsp->windowfd, &oldcursor);
	/*
	* compute window size taking into account the border
	*/
	win_getsize(tsp->windowfd, &tsp->rect);
	tsp->pwp->pw_prretained = mem_create(
	    tsp->rect.r_width, tsp->rect.r_height, 1/*Only 1 deep image*/);
	tsp->winsize.x = tsp->rect.r_width - WXSPACE;
	tsp->winsize.y = tsp->rect.r_height - WYSPACE;
	if(tsp->winsize.y > ((tsp->winsize.x * (long)TYSIZE)/TXSIZE)){
		tsp->winsize.y = ((tsp->winsize.x * (long)TYSIZE)/TXSIZE);
	} else {
		tsp->winsize.x = ((tsp->winsize.y * (long)TXSIZE)/TYSIZE);
	}
	/*
	* initialize variables
	*/
	tsp->uiflags = 0;
	tsp->ptyorp = tsp->ptyowp = tsp->ptyobuf;
	tsp->curfont = tekfont[0];
	tsp->curpos.x = WXMIN(tsp);
	tsp->curpos.y = WYMIN(tsp);
	/*
	* open the emulator
	*/
	if((tsp->temu_data = tek_open((caddr_t)tsp, straps)) == NULL) {
		return(0);
	}
	/*
	* set input masks to get keys and button hits
	*/
	win_getinputmask(tsp->windowfd, &im, (int *)0);
	im.im_flags |= IM_ASCII;
	win_setinputcodebit(&im, MS_LEFT);	/* GIN select */
	win_setinputcodebit(&im, MS_RIGHT);	/* menu */
	win_setinputcodebit(&im, KEY_TOP(1));	/* page */
	win_setinputcodebit(&im, KEY_TOP(2));	/* reset */
	win_setinputcodebit(&im, KEY_TOP(3));	/* local */
	win_setinputcodebit(&im, KEY_TOP(4));	/* remote */
	win_setinputmask(tsp->windowfd,&im,(struct imputmask *)0,WIN_NULLLINK);
	return(1);
}

static int
getopts(argv, sp, fp)
register char **argv;
int *sp;
char *fp;
{
	register char *cp;
	register int straps;
	register char *fontdirp;
	extern char *getenv();

	straps = 0;
	fontdirp = NULL;
	while(cp = *argv++){
		if(*cp++ == '-') {
			if(*cp == 's') {
				cp++;
				if(*cp == NULL)
					cp = *argv;
				if((straps = parsestraps(cp)) < 0)
					return(-1);
			} else if(*cp == 'f') {
				cp++;
				if(*cp == NULL)
					cp = *argv;
				fontdirp = cp;
			} else if(*cp == 'd') {
				dflag = 1;
			}
		}
	}
	if(straps == 0){
		if((cp = getenv("TEKSTRAPS")) != 0) {
			if((straps = parsestraps(cp)) < 0)
				return(-1);
		} else {
			straps = DEFAULTSTRAPS;
		}
	}
	if(fontdirp == NULL){
		if((fontdirp = getenv("TEKFONTS")) == NULL)
			fontdirp = DEFAULTFONTDIR;
	}
	strncpy(fp, fontdirp, MAXFONTNAMELEN - 10);
	*sp = straps;
	return(0);
}

static int
parsestraps(cp)
register char *cp;
{
	register int straps;

	straps = 0;
	while(*cp) {
		switch(*cp){
		case 'l':
			straps |= LFCR;
			break;	
		case 'c':
			straps |= CRLF;
			break;	
		case 'd':
			straps |= DELLOY;
			break;	
		case 'e':
			straps |= AECHO;
			break;
		case 'g':
			cp++;
			if(*cp == 'c')
				straps |= GINCR;
			else if(*cp == 'e')
				straps |= GINCRE; 
			else
				return(-1);
			break;
		default:
			return(-1);
		}
		cp++;
	}
	return(straps);
}

void
tek_done(tsp)
register struct teksw *tsp;
{
	register int i;

	pw_close(tsp->pwp);
	tek_close(tsp->temu_data);
	for(i = 0; i < NFONT; i++){
		pf_close(tekfont[i]);
	}
}

/*
 * Main I/O processing.
 */
int
teksw_selected(tsd, ibits, obits, ebits, timer)
struct teksubwindow *tsd;
register int *ibits, *obits, *ebits;
struct timeval **timer;
{
	register struct teksw *tsp;
	int ptyicc;
	char ptyibuf[128], *pbp;
	int noptyinput = 0;
	int ptyoutputwaiting = 0;
	extern int errno;

	tsp = (struct teksw *)tsd;
	/*
	 * Read input from window
	 */
	if (*ibits & (1 << tsp->windowfd)) {
		struct inputevent uie;

		pw_lock(tsp->pwp, &tsp->rect);
		while(input_readevent(tsp->windowfd, &uie) != -1){
			tek_usrinput(tsp, &uie);
		}
		pw_unlock(tsp->pwp);
	}
	/*
	 * Read pty's input (which is output from program)
	 */
	if (*ibits & (1 << tsp->pty)) {
		if((ptyicc = read(tsp->pty, ptyibuf, sizeof (ptyibuf))) > 0)  {
			pbp = ptyibuf;
			if (ptyibuf[0] == 0)
				pbp++, ptyicc--;
			else {
#ifdef notdef
				if (ptyibuf[0]&TIOCPKT_NOSTOP)
					ioctl(tsp->windowfd, TIOCSTART, 0);
				else if (ptyibuf[0]&TIOCPKT_DOSTOP)
					ioctl(tsp->windowfd, TIOCSTOP, 0);
#endif
				ptyicc = 0;
			}
			if(dflag){
				fwrite(pbp, 1, ptyicc, debug);
			}
			pw_lock(tsp->pwp, &tsp->rect);
			while(ptyicc-- > 0){
				tek_ttyinput(tsp->temu_data, *pbp++);
			}
			pw_unlock(tsp->pwp);
		}
		else if(ptyicc == 0 || (ptyicc < 0 && errno == EWOULDBLOCK)){
			noptyinput++;
		}
	}
	if(tsp->ptyowp > tsp->ptyorp) {
		int cc;

		cc = write(tsp->pty, tsp->ptyorp, tsp->ptyowp - tsp->ptyorp);
		if(cc > 0){
			tsp->ptyorp += cc;
		}
		if(tsp->ptyorp == tsp->ptyowp){
			tsp->ptyorp = tsp->ptyowp = tsp->ptyobuf;
		} else if((cc >= 0) || (errno == EWOULDBLOCK)) {
			ptyoutputwaiting++;
		}
	}
	/*
	 * Setup masks again
	 */
	*ibits = 0;
	*obits = 0;
	*ebits = 0;
	if(ptyoutputwaiting) {
		*obits = (1 << tsp->pty);
	} else {
		*ibits |= (1 << tsp->windowfd);
		if(!noptyinput)
			*ibits |= (1 << tsp->pty);
	}
}

/*
 * Window changed signal handler
 */
teksw_handlesigwinch(tsd)
struct	teksubwindow *tsd;
{
	register struct teksw *tsp;
	register struct pixrect *prsavedp;
	register struct rectlist *rlp;

	tsp = (struct teksw *)tsd;
	win_getsize(tsp->windowfd, &tsp->rect);
	prsavedp = tsp->pwp->pw_prretained;
	pw_damaged(tsp->pwp);
	rlp = &tsp->pwp->pw_clipdata->pwcd_clipping;
	tsp->pwp->pw_prretained = (struct pixrect *)0;
	pw_write(tsp->pwp,
		rlp->rl_bound.r_left, rlp->rl_bound.r_top,
		rlp->rl_bound.r_width, rlp->rl_bound.r_height,
		PIX_SRC,
		prsavedp, rlp->rl_bound.r_left, rlp->rl_bound.r_top);
	tsp->pwp->pw_prretained = prsavedp;
	pw_donedamaged(tsp->pwp);
}

static void
tek_usrinput(tsp, ep)
register struct teksw *tsp;
register struct inputevent *ep;
{
	struct pr_pos tpos;
	struct menu *mp;
	struct menuitem *ip;

	if(ep->ie_code <= ASCII_LAST){
		tek_kbdinput(tsp->temu_data, ep->ie_code);
	} else switch (ep->ie_code) {
	case KEY_TOP(1):
		if(ep->ie_shiftmask & SHIFTMASK)
			tek_kbdinput(tsp->temu_data, RESET);
		else
			tek_kbdinput(tsp->temu_data, PAGE);
		break;
	case KEY_TOP(2):
		tek_kbdinput(tsp->temu_data, LOCAL);
		tekmenuitems[LINEITEM] = tekonline;
		break;
	case KEY_TOP(3):
		tek_kbdinput(tsp->temu_data, REMOTE);
		tekmenuitems[LINEITEM] = teklocal;
		break;
	case KEY_TOP(4):
		screencopy(tsp);
		break;
	case MS_LEFT:
		tpos.x = ep->ie_locx;
		tpos.y = ep->ie_locy;
		wintotek(tsp, &tpos);
		tek_posinput(tsp->temu_data, tpos.x, tpos.y);
		tek_kbdinput(tsp->temu_data, ' ');
		break;
	case MS_RIGHT:
		mp = &tekmenu;
		pw_unlock(tsp->pwp);
		ip = menu_display(&mp, ep, tsp->windowfd);
		pw_lock(tsp->pwp, &tsp->rect);
		if (ip != NULL) {
			tek_usrinput(tsp, (struct inputevent *)ip->mi_data);
		}
		break;
	case LOC_MOVE:
	case LOC_STILL:
		tpos.x = ep->ie_locx;
		tpos.y = ep->ie_locy;
		wintotek(tsp, &tpos);
		tek_posinput(tsp->temu_data, tpos.x, tpos.y);
		break;
	}
}

/*******************
*
* tek emulator interface routines
*
*******************/

void
tek_move(td, x, y)
caddr_t td;
int x, y;
{
	register struct teksw *tsp;

	tsp = (struct teksw *)td;
	tsp->curpos.x = x;
	tsp->curpos.y = y;
	tektowin(tsp, &tsp->curpos);
	if(tsp->uiflags & GCURSORON){
		removegfxcursor(tsp);
		drawgfxcursor(tsp);
	}
}

void
tek_draw(td, x, y)
caddr_t td;
int x, y;
{
	register struct teksw *tsp;
	struct pr_pos tpos;

	tsp = (struct teksw *)td;
	tpos.x = x;
	tpos.y = y;
	tektowin(tsp, &tpos);
	if((tpos.x == tsp->curpos.x) && (tpos.y == tsp->curpos.y)){
		pw_put(tsp->pwp,
			tpos.x, tpos. y,
			1);
	} else if(tsp->type == VT_WRITETHRU){
		tek_line(tsp->pwp,
			tsp->curpos.x, tsp->curpos.y,
			tpos.x, tpos. y,
			(PIX_SRC ^ PIX_DST),
			1,
			tsp->style);
		tek_line(tsp->pwp,
			tsp->curpos.x, tsp->curpos.y,
			tpos.x, tpos. y,
			(PIX_SRC ^ PIX_DST),
			1,
			tsp->style);
	} else {
		int width;

		if(tsp->type == VT_DEFOCUSED)
			width = 2;
		else
			width = 1;
		tek_line(tsp->pwp,
			tsp->curpos.x, tsp->curpos.y,
			tpos.x, tpos. y,
			(PIX_SRC | PIX_DST),
			width,
			tsp->style);
	}
	tsp->curpos = tpos;
}

void
tek_char(td, c)
caddr_t td;
register unsigned char c;
{
	register struct teksw *tsp;
	register struct pixchar *pcp;

	tsp = (struct teksw *)td;
	c &= 0xff;
	pcp = &tsp->curfont->pf_char[c];
	if(tsp->type == VT_WRITETHRU){
		pw_char(tsp->pwp,
			tsp->curpos.x,
			tsp->curpos.y,
			(PIX_SRC ^ PIX_DST),
			tsp->curfont,
			c);
		pw_char(tsp->pwp,
			tsp->curpos.x,
			tsp->curpos.y,
			(PIX_SRC ^ PIX_DST),
			tsp->curfont,
			c);
	} else {
		pw_char(tsp->pwp,
			tsp->curpos.x,
			tsp->curpos.y,
			(PIX_SRC | PIX_DST),
			tsp->curfont,
			c);
	}
}

void
tek_ttyoutput(td, c)
caddr_t td;
char c;
{
	register struct teksw *tsp;

	tsp = (struct teksw *)td;
	*tsp->ptyowp++ = c;
	if(tsp->ptyowp >= &tsp->ptyobuf[sizeof(tsp->ptyobuf)]){
		tsp->ptyowp--;
	}
}

void
tek_displaymode(td, style, type)
caddr_t td;
enum vstyle style;
enum vtype type;
{
	register struct teksw *tsp;

	tsp = (struct teksw *)td;
	tsp->style = style;
	tsp->type = type;
}

void
tek_chfont(td, f)
caddr_t td;
int f;
{
	register struct teksw *tsp;

	tsp = (struct teksw *)td;
	tsp->curfont = tekfont[f];
	if(tsp->uiflags & ACURSORON) {
		removealphacursor(tsp);
		drawalphacursor(tsp);
	}
}

void
tek_cursormode(td, cmode)
caddr_t td;
register enum cursormode cmode;
{
	register struct teksw *tsp;

	tsp = (struct teksw *)td;
	switch(cmode){
	case NOCURSOR:
		alphacursoroff(tsp);
		gfxcursoroff(tsp);
		break;
	case ALPHACURSOR:
		gfxcursoroff(tsp);
		alphacursoron(tsp);
		break;
	case GFXCURSOR:
		alphacursoroff(tsp);
		gfxcursoron(tsp);
		break;
	}
}

void
tek_clearscreen(td)
caddr_t td;
{
	register struct teksw *tsp;

	tsp = (struct teksw *)td;
	pw_writebackground(tsp->pwp,
		0, 0,
		tsp->pwp->pw_prretained->pr_size.x,
		tsp->pwp->pw_prretained->pr_size.y,
		PIX_CLR);
	if(tsp->uiflags & ACURSORON)
		drawalphacursor(tsp);
	else if(tsp->uiflags & GCURSORON)
		drawgfxcursor(tsp);
}

void
tek_bell(td)
caddr_t td;
{
	register struct teksw *tsp;

	tsp = (struct teksw *)td;
	tek_blinkscreen(tsp);
}

static void
tek_blinkscreen(tsp)
register struct teksw *tsp;
{
	int i = 10000;

	pw_writebackground(tsp->pwp,
		0, 0,
		tsp->winsize.x, tsp->winsize.y,
		PIX_NOT(PIX_DST));
	while(i--)
		;
	pw_writebackground(tsp->pwp,
		0, 0,
		tsp->winsize.x, tsp->winsize.y,
		PIX_NOT(PIX_DST));
}

static void
alphacursoron(tsp)
register struct teksw *tsp;
{
	if(!(tsp->uiflags & ACURSORON)) {
		tsp->uiflags |= ACURSORON;
		drawalphacursor(tsp);
	}
}

static void
alphacursoroff(tsp)
register struct teksw *tsp;
{
	if(tsp->uiflags & ACURSORON) {
		tsp->uiflags &= ~ACURSORON;
		removealphacursor(tsp);
	}
}

static void
drawalphacursor(tsp)
register struct teksw *tsp;
{
	tsp->alphacursorpos = tsp->curpos;
	tsp->alphacursorsize = tsp->curfont->pf_defaultsize;
	tsp->alphacursorpos.y += tsp->curfont->pf_char['A'].pc_home.y;
	pw_writebackground(tsp->pwp,
		tsp->alphacursorpos.x, tsp->alphacursorpos.y,
		tsp->alphacursorsize.x, tsp->alphacursorsize.y,
		PIX_NOT(PIX_DST));
}

static void
removealphacursor(tsp)
register struct teksw *tsp;
{
	pw_writebackground(tsp->pwp,
		tsp->alphacursorpos.x, tsp->alphacursorpos.y,
		tsp->alphacursorsize.x, tsp->alphacursorsize.y,
		PIX_NOT(PIX_DST));
}

static void
gfxcursoron(tsp)
register struct teksw *tsp;
{
	struct inputmask im;

	if(!(tsp->uiflags & GCURSORON)) {
		tsp->uiflags |= GCURSORON;
		win_setcursor(tsp->windowfd, &nullcursor);
		win_getinputmask(tsp->windowfd, &im, (int *)0);
		win_setinputcodebit(&im, LOC_MOVE);
		win_setinputcodebit(&im, LOC_STILL);
		win_setinputmask(tsp->windowfd,&im,(struct imputmask *)0,WIN_NULLLINK);
		drawgfxcursor(tsp);
	}
}

static void
gfxcursoroff(tsp)
register struct teksw *tsp;
{
	struct inputmask im;

	if(tsp->uiflags & GCURSORON) {
		tsp->uiflags &= ~GCURSORON;
		win_getinputmask(tsp->windowfd, &im, (int *)0);
		win_unsetinputcodebit(&im, LOC_MOVE);
		win_unsetinputcodebit(&im, LOC_STILL);
		win_setinputmask(tsp->windowfd,&im,(struct imputmask *)0,WIN_NULLLINK);
		win_setcursor(tsp->windowfd, &oldcursor);
		removegfxcursor(tsp);
	}
}

static void
drawgfxcursor(tsp)
register struct teksw *tsp;
{
	tsp->gfxcursorpos = tsp->curpos;
	pw_vector(tsp->pwp,
		WXMIN(tsp), tsp->gfxcursorpos.y,
		WXMAX(tsp), tsp->gfxcursorpos.y,
		(PIX_SRC ^ PIX_DST), 1);
	pw_vector(tsp->pwp,
		tsp->gfxcursorpos.x, WYMIN(tsp),
		tsp->gfxcursorpos.x, WYMAX(tsp),
		(PIX_SRC ^ PIX_DST), 1);
}

static void
removegfxcursor(tsp)
register struct teksw *tsp;
{
	pw_vector(tsp->pwp,
		WXMIN(tsp), tsp->gfxcursorpos.y,
		WXMAX(tsp), tsp->gfxcursorpos.y,
		(PIX_SRC ^ PIX_DST), 1);
	pw_vector(tsp->pwp,
		tsp->gfxcursorpos.x, WYMIN(tsp),
		tsp->gfxcursorpos.x, WYMAX(tsp),
		(PIX_SRC ^ PIX_DST), 1);
}

#define HALF		(1 << 9)		/* 1/2 in out fixed point */
#define FIX(X)		((X) << 10)		/* int to fixed pt */
#define UNFIX(X)	(((X) + HALF) >> 10)	/* fixed pt to int */

/*
 * vector styles (out of the 4014 hardware manual)
 */
#define DOT	3			/* line style dot in tekpts (guess) */
/*
 * One pixel is subtracted from the on pattern beacause vector drawing includes
 * the end pixel. The off pattern is incremented for the same reason.
 */
#define ON(X)		FIX(((X) * DOT) - 1)
#define OFF(X)		FIX(((X) * DOT) + 1)
static long pattern[5][4] = {
	0, 0, 0, 0,				/* solid  */
	ON(1), OFF(1), ON(1), OFF(1),		/* dotted */
	ON(5), OFF(1), ON(1), OFF(1),		/* dash dot*/
	ON(3), OFF(1), ON(3), OFF(1),		/* short dash */
	ON(6), OFF(2), ON(6), OFF(2)		/* long dash */
};
#undef DOT
#undef ON
#undef OFF

static int pdx[4], pdy[4];			/* pattern length vector */    
static int rop;
static struct pixwin *destpw;

static void plotline();
static void circle();
static void circ_pts();
static int length();
static int isqrt();

/*--------------------------------------------------------------------------*/
tek_line(pwp, x0, y0 , x1, y1, op, width, style)
struct pixwin *pwp;
register int x0, y0, x1, y1;		/* window coords  */
int op, width, style;			/* window width   */
{
	register int dx, dy;		/* vector components */
	register short ldx,ldy; 	/* normal vec cmpnts left and right */    
	register short incx, incy;
	register short p0lx, p0ly, p0rx, p0ry, p1lx, p1ly, p1rx, p1ry;
	register short frac, i;

	rop = op;
	destpw = pwp;
	dx = x1 - x0;
	dy = y1 - y0;

	if ((dx == 0) && (dy == 0)) {
		circle( x0, y0, width/2);
		return(0);
	}
	if (style) {			/* if not solid fat line */
		register long vecl;	/* vector length */
		register long pl;	/* piece length */

		vecl = length(dx, dy);
		/* make four part pattern cycle */
		for (i=0; i<4; i++) {
			pl = pattern[style][i];
			pdx[i] = (dx * pl) / vecl;
			pdy[i] = (dy * pl) / vecl;
		}
	} else {
		pdx[0] = dx<<10;
		pdy[0] = dy<<10;
	}
	if (width <= 1) {
		plotline(x0, y0, x1, y1);
		return(0);
	}
	incx = (dy < 0) ? 1 : -1;
        incy = (dx < 0) ? -1 : 1;
	ldx = abs(dy)<<1;		/* fraction for end vectors */
	ldy = abs(dx)<<1;
	p0lx = p0rx = x0;		/* width vec at each end of orig vec */
	p0ly = p0ry = y0;		/* bresenham for subsequent endpts */
	p1lx = p1rx = x1;		/* of each line of a fat line */
	p1ly = p1ry = y1;
	plotline( p0lx,p0ly,p1lx,p1ly);		/* draw center edge */
	frac = (ldy < ldx) ? ldx/2 : -ldy/2;	/* init frac = 1/2 major */
	for (i=0; i<width/2; i++) {
		if (frac < 0) {
                        p0ly += incy; p1ly += incy;     /* bump each line end */
			plotline( p0lx,p0ly,p1lx,p1ly);	/* draw left edge */
                        p0ry -= incy; p1ry -= incy;     /* bump each line end */
			plotline( p0rx,p0ry,p1rx,p1ry);	/* draw right edge */
			frac += ldx;
		} else {
                        p0lx += incx; p1lx += incx;     /* bump each line end */
                        plotline( p0lx,p0ly,p1lx,p1ly); /* draw left edge */
                        p0rx -= incx; p1rx -= incx;     /* bump each line end */
			plotline( p0rx,p0ry,p1rx,p1ry);	/* draw right edge */
			frac -= ldy; 
		}
	}
	circle( x0, y0, width/2);
	circle( x1, y1, width/2);
	return(0);
}

/*-----------------------------------*/
static void
plotline(x0, y0, x1, y1)
short x0,y0,x1,y1;
{
	register long int xb, yb, xe, ye;	/* begin and end pts */
	register long int dx, dy;
	register int i;

	xb = xe = FIX(x0);
	yb = ye = FIX(y0);
	dx = FIX(abs(x1 - x0));
	dy = FIX(abs(y1 - y0));
	i = 0;
	while(dx || dy){
		if(((dx -= abs(pdx[i])) < 0) || ((dy -= abs(pdy[i])) < 0)){
			pw_vector(destpw, UNFIX(xe), UNFIX(ye), x1, y1, rop, 1);
			break;
		}
		xe += pdx[i];
		ye += pdy[i];
		if((i&1) == 0){
			pw_vector(destpw,
			    UNFIX(xb), UNFIX(yb), UNFIX(xe), UNFIX(ye), rop, 1);
		}
		i++;
		i &= 3;				/* four part pattern cycle */
		xb = xe;
		yb = ye;
	}
}

/*-------------------------------*/
static void
circle(x0, y0, rad)
register short x0, y0, rad;
{
	register int x,y,d;

	if(rad == 0){
		pw_vector(destpw, x0, y0, x0, y0, rop, 1);
		return;
	}
	x = 0;  y = rad;
	d = 3 - 2 * rad;
	while(x < y) {
	    circ_pts(x0, y0, x, y);
	    if (d < 0) {
	        d = d + 4*x + 6;
	    } else {
		d = d + 4 *(x-y) + 10;
		y = y - 1;
	    }
	    x += 1;
	}
	if(x=y)
	    circ_pts(x0, y0, x, y);
}

/*------------------------------------------------------------------------*/
static void
circ_pts(x0, y0, x, y)
register short x0, y0, x, y;
{	/* draw symmetry points of circle */
	register short a1,a2,a3,a4,b1,b2,b3,b4;

	a1 = x0+x; a2 = x0+y; a3 = x0-x; a4 = x0-y;
	b1 = y0+x; b2 = y0+y; b3 = y0-x; b4 = y0-y;
        pw_vector( destpw, a4, b1, a2, b1, rop, 1);
        pw_vector( destpw, a3, b2, a1, b2, rop, 1);
        pw_vector( destpw, a4, b3, a2, b3, rop, 1);
        pw_vector( destpw, a3, b4, a1, b4, rop, 1);
}

/*-------------------------------------------------------------------------*/
/* length and integer square root */

static int
length(dx, dy)
register int dx, dy;
{
	if(dx == 0)
		return(abs(dy));
	if(dy == 0)
		return(abs(dx));
	return(isqrt((dx*dx) + (dy*dy)));
}

static int
isqrt(n)
register unsigned n;
{
	register unsigned q,r,x2,x;
	register unsigned t;

	if (n > 0xFFFE0000)
		return(0xFFF);		/* algorithm doesn't cover this case */
	if (n == 0xFFFE0000)
		return(0xFFFE);		/* or this case */
	if (n < 2)
		return(n);		/* or this case */
	t = x = n;
	while (t >>= 2)
		x >>= 1;
	x++;
	while(1) {
		q = n/x;
		r = n%x;
		if (x <= q) {
			x2 = x+2;
			if(q < x2 || q == x2 && r == 0)
				break;
		}
		x = (x + q)>>1;
	}
	return(x);
}

#undef HALF
#undef FIX
#undef UNFIX

#define scale(pp, fs, ts)	(pp)->x = (((long)(pp)->x*(long)ts.x)+((long)fs.x/2))\
						/ (long)fs.x; \
				(pp)->y = (((long)(pp)->y*(long)ts.y)+((long)fs.y/2))\
						/ (long)fs.y;

static void
wintotek(tsp, posp)
register struct teksw *tsp;
struct pr_pos *posp;
{
	posp->x -= WXMIN(tsp);
	if(posp->x < 0)
		posp->x = 0;
	if(posp->x > tsp->winsize.x)
		posp->x = tsp->winsize.x;
	posp->y -= WYMIN(tsp);
	if(posp->y < 0)
		posp->y = 0;
	if(posp->y > tsp->winsize.y)
		posp->y = tsp->winsize.y;
	scale(posp, tsp->winsize, teksize);
	posp->y = teksize.y - posp->y;
}

static void
tektowin(tsp, posp)
register struct teksw *tsp;
struct pr_pos *posp;
{
	posp->y = teksize.y - posp->y;
	scale(posp, teksize, tsp->winsize);
	posp->x += WXMIN(tsp);
	posp->y += WYMIN(tsp);
}

static void
screencopy(tsp)
register struct teksw *tsp;
{
#ifdef COPY
	char *cmd;
	int ostdin;
	int ostdout;
	struct rect rect;
	int status;
	int w;
	register int (*istat)(), (*qstat)(), (*pstat)();
	int fds[2];
	int pid;

	pw_unlock(tsp->pwp);
	if((cmd = getenv("TEKCOPY")) == 0){
		fprintf(stderr, "cannot find copy command\n");
		return;
	}
	fflush(stdout);
	if((ostdin = dup(0)) < 0){
		perror("tektool,copy");
		return;
	}
	if((ostdout = dup(1)) < 0){
		perror("tektool,copy");
		close(ostdin);
		return;
	}
	close(0);
	close(1);
	pipe(fds);
	pid = vfork();
	if(pid < 0){
		perror("tektool,copy");
		dup2(ostdin, 0);
		close(ostdin);
		dup2(ostdout, 1);
		close(ostdout);
		return;
	}
	if(pid > 0){
		dup2(ostdin, 0);
		close(ostdin);
		istat = signal(SIGINT, SIG_IGN);
		qstat = signal(SIGQUIT, SIG_IGN);
		pstat = signal(SIGPIPE, SIG_IGN);
		win_setcursor(tsp->windowfd, &waitcursor);
		(void) pr_dump(tsp->pwp->pw_prretained, stdout, NULL,
				RT_BYTE_ENCODED, 1);
		fflush(stdout);
		dup2(ostdout, 1);
		close(ostdout);
		signal(SIGPIPE, pstat);
		while ((w = wait(&status)) != pid && w != -1)
			;
		win_setcursor(tsp->windowfd, &oldcursor);
		signal(SIGINT, istat);
		signal(SIGQUIT, qstat);
	} else {
		dup2(ostdout, 1);
		close(ostdout);
		close(ostdin);
		execl("/bin/sh", "/bin/sh", "-c", cmd, 0);
		_exit(1);
	}
	pw_lock(tsp->pwp, &tsp->rect);
#endif
}
