/*	@(#)teksw_imp.h 1.3 87/01/07 SMI	*/

/*
 * A tek subwindow structure
 */

struct teksw {
	int windowfd;
	int tty;
	int pty;
	int pgrp;
	int cachedttyslot;
	char ptyobuf[1024];
	char *ptyowp;
	char *ptyorp;
	caddr_t temu_data;
	int uiflags;
#define ACURSORON	0x01		/* alpha cursor is on the screen */
#define GCURSORON	0x02		/* graphics cursor is on the screen */
	struct pixwin *pwp;
	struct rect rect;
	struct pr_size winsize;
	struct pr_pos curpos;
	struct pixfont *curfont;
	enum vstyle style;
	enum vtype type;
	struct pr_pos alphacursorpos;
	struct pr_size alphacursorsize;
	struct pr_pos gfxcursorpos;
};

/*
* external tek_ui routines
*/
extern int tek_init();
extern void tek_done();
