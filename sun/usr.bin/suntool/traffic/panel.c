#ifndef lint
static  char sccsid[] = "@(#)panel.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <netdb.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <suntool/menu.h>		/* prompt stuff */
#include <suntool/tool_struct.h>	/* TOOL_BORDERWIDTH */
#include <sys/socket.h>
#include <netinet/in.h>
#include <rpcsvc/ether.h>
#include "traffic.h"

#define HASHNAMESIZE 256

struct hnamemem {
	int h_addr;
	char *h_name;
	struct hnamemem *h_nxt;
};

/* 
 *  global variables
 */
static struct hnamemem	*htable[HASHNAMESIZE];
static int		oldaddrs[MAXSPLIT][NUMNAMES];
static char *protoname[NPROTOS] = {
		"nd",
		"icmp",
		"udp",
		"tcp",
		"arp",
		"other"
};

/* 
 *  procedure variables
 */
char *getname();

/* 
 *  pixrects
 */
DEFINE_CURSOR(leftbutton, 7, 7, PIX_SRC | PIX_DST,
	0x1FF8,0x3FFC,0x336C,0x336C,0x336C,0x336C,0x336C,0x336C,
	0x3FFC,0x3FFC,0x3FFC,0x3FFC,0x3FFC,0x3E3C,0x3FFC,0x1FF8);
DEFINE_CURSOR(oldcursor, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

/* ARGSUSED */
slider_cycle(item, value, event)
	Panel_item item;
	struct inputevent *event;
{
	int j, v;
	char buf[20];

	j = getpanelnum(horizpanel, item);
	if (value == 0) {
		panel_set(speed_item1[j], PANEL_SHOW_ITEM, 0, 0);
		panel_set(speed_item[j], PANEL_SHOW_ITEM, 1, 0);
		v = (int)panel_get_value(speed_item[j]);
		sprintf(buf, "  %d.%01d secs", v/10, v%10);
		panel_set(speedvalue_item[j], PANEL_LABEL_STRING, buf, 0);
	}
	else {
		panel_set(speed_item[j], PANEL_SHOW_ITEM, 0, 0);
		panel_set(speed_item1[j], PANEL_SHOW_ITEM, 1, 0);
		sprintf(buf, "  %d secs", panel_get_value(speed_item1[j]));
		panel_set(speedvalue_item[j], PANEL_LABEL_STRING, buf, 0);
	}
}

/* ARGSUSED */
scale_cycle(item, value, event)
	Panel_item item;
	struct inputevent *event;
{
	int j;

	j = getpanelnum(horizpanel, item);
	traf_absolute[j] = value;
	traf_board_init(j);
}

/* ARGSUSED */
speed_slider(item, speed, event)
	Panel_item item;
	int speed;
	struct inputevent *event;
{
	int j;
	char buf[20];
	
	j = getpanelnum(horizpanel, item);
	if (win_inputnegevent(event)) {
		timeout[j].it_interval.tv_sec = speed/10;
		timeout[j].it_interval.tv_usec = (speed%10) * 100000;
		timeout[j].it_value.tv_sec = speed/10;
		timeout[j].it_value.tv_usec = (speed%10) * 100000;
		notify_set_itimer_func(canvas[j], timeout_notify, ITIMER_REAL,
		    &timeout[j], 0);
	}
	sprintf(buf, "  %d.%01d secs", speed/10, speed%10);
	panel_set(speedvalue_item[j], PANEL_LABEL_STRING, buf, 0);
}

/* ARGSUSED */
speed1_slider(item, speed, event)
	Panel_item item;
	int speed;
	struct inputevent *event;
{
	int j;
	char buf[20];
	
	j = getpanelnum(horizpanel, item);
	if (win_inputnegevent(event))
		timeout1[j] = speed;
	sprintf(buf, "  %d secs", speed);
	panel_set(speedvalue_item[j], PANEL_LABEL_STRING, buf, 0);
}

/* ARGSUSED */
mode_choice(item, value, event)
	Panel_item item;
	int value;
	struct inputevent *event;
{
	int j;
	
	j = getpanelnum(vertpanel, item);
	if (mode[j] == value)
		return;
	mode[j] = value;
	traf_board_init(j);
}

/* ARGSUSED */
grid_toggle(item, value, event)
	Panel_item item;
	struct inputevent *event;
{
	int j;
	
	j = getpanelnum(horizpanel, item);
	gridon[j] ^= 1;
#if 0
	if (mode[j] == DISPLAY_LOAD && gridon[j] == 0)
		traf_board_init(j);
	else
#endif
		drawgrid(j);
}

/* ARGSUSED */
quit_button(item, event)
	Panel_item item;
	struct inputevent *event;
{
	int x, y;
	struct inputevent ie;
	
	x = (int)panel_get(item, PANEL_ITEM_X) - 100;
	y = (int)panel_get(item, PANEL_ITEM_Y) + 50;
	traf_box("click left button to confirm exit", x, y, &ie);
	if (ie.ie_code == MS_LEFT && win_inputposevent(&ie))
		quit();
}

traf_box(string, x, y, iep)
	char *string;
	struct inputevent *iep;
{
	struct prompt p;
	int fd, wd, ht;
	
	fd = (int)window_get(frame, WIN_FD);
	wd = fonts[1]->pf_defaultsize.x;
	ht = fonts[1]->pf_defaultsize.y;
	rect_construct(&p.prt_rect, x, y, (strlen(string) + 4) * wd, 2 * ht);
	p.prt_font = fonts[1];
	p.prt_text = string;
	
 	win_getcursor(fd, &oldcursor);	/* save cursor */
	win_setcursor(fd, &leftbutton);	/* change cursor */
	menu_prompt(&p, iep, fd);
	win_setcursor(fd, &oldcursor);  /* restore cursor */
}

quit()
{
	deinitdevice();
	exit(0);
}

/* ARGSUSED */
split_button(item, event)
	Panel_item item;
	struct inputevent *event;
{
	int i;
	
	if (splitcnt < MAXSPLIT-1) {
		splitcnt++;
		if (vertpanel[splitcnt] == NULL)
			makesubwindows(splitcnt);
		/* 
		 * initialize timeout to something?
		 */
		/*tsw[splitcnt]->ts_io.tio_timer = &timeout[splitcnt]; */
		placesubwindows();
		for (i = 0; i < (splitcnt+1); i++)
			traf_board_init(i);
	}
}

/* ARGSUSED */
delete_button(item, event)
	Panel_item item;
	struct inputevent *event;
{
	int i, j;
	
	j = getpanelnum(horizpanel, item);
	for (i = j+1; i < MAXSPLIT; i++) {
		if (vertpanel[i]) {
			gridon[i-1] = gridon[i];
			mode[i-1] = mode[i];
			timeout[i-1] =timeout[i];
			timeout1[i-1] =timeout1[i];
			panel_set_value(speed_item[i-1],
			    panel_get_value(speed_item[i]));
			panel_set_value(grid_item[i-1],
			    panel_get_value(grid_item[i]));
			panel_set_value(mode_item[i-1],
			    panel_get_value(mode_item[i]));
		}
	}
	if (splitcnt > 0) {
		notify_set_itimer_func(canvas[splitcnt], timeout_notify,
		    ITIMER_REAL, NULL, 0);
		splitcnt--;
		placesubwindows();
		for (i = 0; i < (splitcnt+1); i++)
			traf_board_init(i);
	}
}

lock(i)
{
        struct  rect    rect;
        
	rect.r_top = 0;
	rect.r_left = 0;
	rect.r_width = (int)window_get(canvas[i], CANVAS_WIDTH);
	rect.r_height = (int)window_get(canvas[i], CANVAS_HEIGHT);
	pw_lock(pixwin[i], &rect);
}

unlock(i)
{
	pw_unlock(pixwin[i]);
}

/* 
 * draw names on bottom margin, and also copy rank1[] to rank[]
 * in correct order
 */
drawnames(k, rank, rank1)
	struct rank rank[];
	struct rank rank1[];
{
	int wd, charwd, charht, i, j, l, off, left;
	int put[NUMNAMES];	/* put[i] is where to put rank[i] */
	int taken[NUMNAMES];	/* taken[i] true if put points to it */
	struct pr_size sz;
	char *p;
	char save;
	int maxlen;
	
	left = LEFTMARGIN*marginfontwd;
	wd = (tswrect[k].r_width - left)/NUMNAMES;
	sz = pf_textwidth(1, fonts[0], "A");
	charwd = sz.x;
	charht = sz.y;
	maxlen = wd/charwd;

	bzero(taken, sizeof(taken));
	for (i = 0; i < NUMNAMES; i++) {
		for (j = 0; j < NUMNAMES; j++) {
			if (rank1[i].addr == oldaddrs[k][j]) {
				put[i] = j;
				taken[j] = 1;
				break;
			}
		}
		if (j == NUMNAMES)
			put[i] = -1;
	}

	j = 0;
	for (i = 0; i < NUMNAMES; i++) {
		if ((l = put[i]) != -1) {
			rank[l] = rank1[i];
			continue;
		}
		while(taken[j])
			j++;
		p = getname(rank1[i].addr);
		off = (wd - strlen(p)*charwd)/2;
		if (off < 0)
			off = 0;
		pw_writebackground(pixwin[k], left + j*wd,
		    tswrect[k].r_height - 3 - charht,
		    wd, charht+1, PIX_CLR);
		/* 
		 * truncate p to avoid overlap
		 */
		save = p[maxlen];
		p[maxlen] = 0;
		pw_text(pixwin[k], left + j*wd + off, tswrect[k].r_height - 3,
		    PIX_SRC, fonts[0], p);
		p[maxlen] = save;
		oldaddrs[k][j] = rank1[i].addr;
		rank[j] = rank1[i];
		j++;
	}
}

zerooldaddrs(i)
{
	bzero(oldaddrs[i], sizeof(oldaddrs[i]));
}
	
drawleftmargin(k)
{
	int ht, i, j, delta;
	char buf[4];
	
	if (traf_absolute[k]) {
		delta = maxabs[k]/10;
		j = maxabs[k];
	}
	else{
		delta = 10;
		j = 100;
	}
	for (i = 0; i < 11; i++) {
		sprintf(buf, "%3d", j);
		if ((ht = TOPGAP + marginfontht/2 + (i*curht[k])/10) >=
		    tswrect[k].r_height)
			break;
		pw_text(pixwin[k], marginfontwd/2, ht, PIX_SRC, fonts[0], buf);
		j -= delta;
	}
}

drawprototext(k)
{
	int wd, charwd, i, j, off, left;
	struct pr_size sz;
	
	left = LEFTMARGIN*marginfontwd;
	wd = (tswrect[k].r_width - left)/NPROTOS;
	for (i = NFONTS-1; i >= 0; i--) {
		sz = pf_textwidth(1, fonts[i], "A");
		charwd = sz.x;
		for (j = 0; j < NPROTOS; j++) {
			if (charwd*(strlen(protoname[j])+2) > wd)
				break;
		}
		if (j == NPROTOS)
			break;
	}
	if (i == -1)
		i = 0;
	for (j = 0; j < NPROTOS; j++) {
		off = (wd - strlen(protoname[j])*charwd)/2;
		pw_text(pixwin[k], left + j*wd + off, tswrect[k].r_height - 3,
		    PIX_SRC, fonts[i], protoname[j]);
	}
}

drawsizetext(k)
{
	int wd, charwd, i, j, off, left;
	struct pr_size sz;
	char buf[100];
	
	left = LEFTMARGIN*marginfontwd;
	wd = (tswrect[k].r_width - left)/(NBUCKETS/2);
	for (i = NFONTS-1; i >= 0; i--) {
		sz = pf_textwidth(1, fonts[i], "A");
		charwd = sz.x;
		for (j = 0; j < NBUCKETS/2; j++) {
			sprintf(buf, "%d-%d", j*2*BUCKETLNTH + 60,
			    (j+1)*2*BUCKETLNTH + 60 - 1);
			if (charwd*(strlen(buf)+2) > wd)
				break;
		}
		if (j == NBUCKETS/2)
			break;
	}
	if (i == -1)
		i = 0;
	for (j = 0; j < NBUCKETS/2; j++) {
		sprintf(buf, "%d-%d", j*2*BUCKETLNTH + 60,
		    (j==NBUCKETS/2-1) ? MAXPACKETLEN:(j+1)*2*BUCKETLNTH+60-1);
		off = (wd - strlen(buf)*charwd)/2;
		pw_text(pixwin[k], left + j*wd + off, tswrect[k].r_height - 3,
		    PIX_SRC, fonts[i], buf);
	}
}

drawgrid(k)
{
	int i, j, ht, wd, spacing, left;
	
	ht = curht[k];
	wd = tswrect[k].r_width;
	spacing = ht/10;
	lock(k);
	left = LEFTMARGIN*marginfontwd;
	for (i = 0, j = TOPGAP + ht; i <= 10; i++, j-=spacing)
		pw_vector(pixwin[k], left, j, wd, j, PIX_SRC^PIX_DST, 1);
	unlock(k);
}
/* 
 * XXX this routine should go away
 */
drawgridstub(k, step)
{
	int i, j, ht, wd, spacing;
	
	ht = curht[k];
	wd = tswrect[k].r_width;
	spacing = ht/10;
	lock(k);
	for (i = 0, j = TOPGAP + ht; i <= 10; i++, j-=spacing)
		pw_vector(pixwin[k], wd-step, j, wd, j, PIX_SRC^PIX_DST, 1);
	unlock(k);
}

placesubwindows()
{
	int	vertwd, horizht, toolwd;
	int	perwindowht, i;
	
	/* Get the numbers computed by fit_height() */
	vertwd = (int)window_get(vertpanel[0], WIN_WIDTH);
	horizht = (int)window_get(horizpanel[0], WIN_HEIGHT);

	toolwd = (int)window_get(frame, WIN_WIDTH);
	perwindowht = ((int)window_get(frame, WIN_HEIGHT)
	    - (int)window_get(frame, WIN_TOP_MARGIN)
	    - (int)window_get(toppanel, WIN_HEIGHT)
	    - (splitcnt+1)*TOOL_BORDERWIDTH)/(splitcnt+1);

	for (i = 0; i < splitcnt+1; i++) {
		window_set(horizpanel[i],
		    WIN_SHOW, 1,
		    WIN_BELOW, i ? canvas[i-1] : toppanel,
		    WIN_X, 0,
		    WIN_HEIGHT, horizht,
		    WIN_WIDTH, toolwd - vertwd - 3*TOOL_BORDERWIDTH, 0);
		window_set(vertpanel[i],
		    WIN_SHOW, 1,
		    WIN_BELOW, i ? vertpanel[i-1] : toppanel,
		    WIN_RIGHT_OF, horizpanel[i], 
		    WIN_HEIGHT, i == splitcnt ? -1 : perwindowht,
		    WIN_WIDTH, vertwd, 0);
		window_set(canvas[i], WIN_BELOW, horizpanel[i],
		    WIN_SHOW, 1,
		    WIN_X, 0,
		    WIN_HEIGHT, i == splitcnt ? -1 :
			perwindowht - horizht - TOOL_BORDERWIDTH,
		    WIN_WIDTH, toolwd - vertwd - 3*TOOL_BORDERWIDTH, 0);
		/* shouldn't cache here */
		tswrect[i] = *(struct rect *)window_get(canvas[i], WIN_RECT);
	}

	for(i = splitcnt + 1; i < MAXSPLIT; i++) {
		if (vertpanel[i]) {
			window_set(vertpanel[i], WIN_SHOW, 0, 0);
			window_set(horizpanel[i], WIN_SHOW, 0, 0);
			window_set(canvas[i], WIN_SHOW, 0, 0);
		}
	}
}

getpanelnum(panelarr, item)
	Panel panelarr[];
	Panel_item item;
{
	Panel panel;
	int j;
	
	panel = (Panel) panel_get(item, PANEL_PARENT_PANEL);
	for (j = 0; j < MAXSPLIT; j++)
		if (panelarr[j] == panel)
			break;
	if (j == MAXSPLIT) {
		fprintf(stderr, "can't find panel\n");
		exit(1);
	}
	return (j);
}

char *
getname(addr)
{
	int x;
	struct hostent *hp;
	struct hnamemem *p;
	char buf[20];
	
	x = addr & 0xff;
	for (p = htable[x]; p != NULL; p = p->h_nxt) {
		if (p->h_addr == addr)
			return (p->h_name);
	}
	p = (struct hnamemem *)malloc(sizeof(struct hnamemem));
	p->h_addr = addr;
	p->h_nxt = htable[x];
	htable[x] = p;

	if (inet_lnaof(addr) == INADDR_ANY) {
		p->h_name = "broadcast";
		return (p->h_name);
	}
	hp = gethostbyaddr(&addr, sizeof(int), AF_INET);
	if (hp) {
		p->h_name = (char *)malloc(strlen(hp->h_name) + 1);
		strcpy(p->h_name, hp->h_name);
	}
	else {
		sprintf(buf, "0x%x", addr);
		p->h_name = (char *)malloc((strlen(buf)) + 1);
		strcpy(p->h_name, buf);
	}
	return (p->h_name);
}

traf_compar(a,b)
	struct rank *a, *b;
{
	return (a->cnt < b->cnt);
}

