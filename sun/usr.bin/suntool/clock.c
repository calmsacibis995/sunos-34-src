#ifndef lint
static	char sccsid[] = "@(#)clock.c 1.4 87/01/07";
#endif

/*
 * Sun Microsystems, Inc.
 */

/*
 *	Clock:	Display time as text when open and icon when closed.
 */

#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <suntool/tool_hs.h>
#include <suntool/msgsw.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

extern	char *asctime();
extern	int errno;
void	notify_set_signal_check();

static char *optionsptr;
static struct pixfont *pfont;

#include "clockhands.h"

static	u_short icon_data[300] = {
#include <images/clock.icon>
};
mpr_static(clock_default_mpr, 64, 75, 1, icon_data);

static u_short icon_rom_data[300] = {
#include <images/clock.rom.icon>
};
mpr_static(clock_default_rom_mpr, 64, 75, 1, icon_rom_data);

static	struct pixrect *ic_mpr, *base_mpr;
static	struct icon clockicon = {64, 64, (struct pixrect *)0,
	    {0, 0, 64, 64}, 0,
	    {0, 0, 0, 0}, (char *)0, (struct pixfont *)0, 0};
static	struct icon clockicon_date = {64, 75, (struct pixrect *)0,
	    {0, 0, 64, 75}, 0,
	    {0, 0, 0, 0}, (char *)0, (struct pixfont *)0, 0};

static	Tool *tool;
static	Notify_value clock_tool_event();
static	Notify_value clock_itimer_expired();
static	int clock_client;

static	struct tm *tmp;

static	unsigned show_seconds;
static	unsigned testing;
static	unsigned roman;
static	unsigned show_date;
static	unsigned face;

static	Msgsw *msgsw;
static	Notify_value clock_msgsw_event();
static	Notify_value clock_tool_destroy();

static	int last_hour;

#ifdef STANDALONE
main(argc, argv)
#else
int clocktool_main(argc, argv)
#endif
	int argc;
	char **argv;
{
	char	*name = "clock";
	char	*tool_name = argv[0];
        char	**tool_attrs = NULL;
	struct	inputmask im;
	struct	pixrect *cmd_line_mpr;

	optionsptr = "";
	last_hour = -1;
	/*
	 * Create and customize tool
	 */
	argc--;   argv++;		/*	skip over program name	*/
        if (tool_parse_all(&argc, argv, &tool_attrs, tool_name) == -1) {
                (void)tool_usage(tool_name);
                EXIT(1);
        }
        while (argc-- > 0) {
                switch ((*argv)[1]) {
		  case 'S':
		  case 's': show_seconds = TRUE;
			    break;
		  case 'T':
		  case 't': testing = TRUE;
			    break;
		  case 'r': roman = TRUE;
			    break;
		  case 'd': show_date = TRUE;
			    argv++;
			    argc--;
			    if (argc < 0) {
				    (void)fprintf(stderr,
				    	"invalid -d option to clock\n");
				    break;
			    }
			    optionsptr = *argv;
			    break;
		  case 'f': face = TRUE;
		  	    break;
		  default:  (void)fprintf(stderr,
				"clock doesn't recognize '%s' argument\n",
				    *argv);
                }
                argv++;
        }
	pfont = pf_open("/usr/lib/fonts/fixedwidthfonts/screen.r.7");
	if (pfont == NULL) {
		(void)fprintf(stderr, "can't find screen.r.7\n");
		EXIT(1);
		/* pfont = pf_default(); */
	}
	/*
	 * Set up clock icon
	 */
	if (show_date == TRUE) {
		ic_mpr = mem_create(64, 75, 1);
		base_mpr = mem_create(64, 75, 1);
	} else {
		ic_mpr = mem_create(64, 64, 1);
		base_mpr = mem_create(64, 64, 1);
	}
	(void)pr_rop(base_mpr, 0, 0, 64, 64, PIX_SRC, 
	    (roman == TRUE)? &clock_default_rom_mpr: &clock_default_mpr, 0, 0);
	if (show_date == TRUE) {
		(void)pr_rop(base_mpr, 0, 62, 64, 13, PIX_SET, (Pixrect *)0, 0, 0);
		(void)pr_rop(base_mpr, 0+2, 62+2, 64-4, 13-4, PIX_CLR, (Pixrect *)0, 0, 0);
	}
	/* See if user supplied image from command line */
	if (tool_find_attribute(tool_attrs, (int)WIN_ICON_IMAGE, 
		(char **)(LINT_CAST(&cmd_line_mpr)))) {
		(void)pr_rop(base_mpr, 0, 0, cmd_line_mpr->pr_width,
		    cmd_line_mpr->pr_height, PIX_SRC, cmd_line_mpr, 0, 0);
		(void)tool_free_attribute((int)WIN_ICON_IMAGE, 
			(char *)(LINT_CAST(cmd_line_mpr)));
	}
        tool = tool_begin(
            WIN_LABEL,          name,
            WIN_COLUMNS,	26,
            WIN_LINES,		1,
            WIN_NAME_STRIPE,    1,
            WIN_ICONIC,		1,
            WIN_ICON,         show_date == TRUE ? &clockicon_date : &clockicon,
            WIN_ICON_IMAGE,	ic_mpr,
            WIN_ATTR_LIST,      tool_attrs,
            0);
        if (tool == (struct tool *)NULL)
            EXIT(1);
        (void)tool_free_attribute_list(tool_attrs);
	/*
	 * Note when clock changes state so can update appropriate time data
	 */
	(void) notify_interpose_event_func((Notify_client)(LINT_CAST(tool)), 
		clock_tool_event, NOTIFY_SAFE);
	/*
	 * Create and customize msg subwindow
	 */
	msgsw = msgsw_create(tool, "msgsw", TOOL_SWEXTENDTOEDGE,
	    TOOL_SWEXTENDTOEDGE, "", (struct pixfont *)0);
	if (msgsw == MSGSW_NULL)
		EXIT(1);
	/*
	 * Catch type-in to msgsw
	 */
	(void) notify_interpose_event_func((Notify_client)(LINT_CAST(msgsw)),
	    clock_msgsw_event, NOTIFY_SAFE);
	(void)input_imnull(&im);
	im.im_flags |= IM_ASCII;
	(void)win_setinputmask(msgsw->msg_windowfd, &im, (struct inputmask *)0,
	    WIN_NULLLINK);
	/*
	 * Notice when tool dies so that can remove interval timer.
	 */
	(void) notify_interpose_destroy_func((Notify_client)(LINT_CAST(tool)), 
		clock_tool_destroy);
	/*
	 * Install tool
	 */
	(void)tool_install(tool);
	/*
	 * Simulate a interval timer expiration to prime image.
	 */
	(void) clock_itimer_expired((Notify_client)(LINT_CAST(&clock_client)), 
		ITIMER_REAL);
	/*
	 * Main loop
	 */
	(void) notify_start();
	/*
	 * Cleanup
	 */
	(void)pf_close(pfont);
	(void)pr_destroy(ic_mpr);
	(void)pr_destroy(base_mpr);
	
	EXIT(0);
}

static
Notify_value
clock_msgsw_event(msgsw_local, event, arg, type)
	struct	msgsubwindow *msgsw_local;
	Event	*event;
	Notify_arg arg;
	Notify_event_type type;
{
	switch (event_id(event)) {
	  case 's':	/* toggle second */
	  case 'S':	/*    hand	*/
		show_seconds = !show_seconds;
		/* Simulate a interval timer expiration to prime image */
		(void) clock_itimer_expired((Notify_client)(LINT_CAST(
			&clock_client)), ITIMER_REAL);
		break;
	  case 't':	/* toggle testing */
	  case 'T':
		testing = !testing;
		/* Simulate a interval timer expiration to prime image */
		(void) clock_itimer_expired((Notify_client)(LINT_CAST(
			&clock_client)), ITIMER_REAL);
		break;
	  default: /*  pass on the rest */
		return(notify_next_event_func((Notify_client)(LINT_CAST(
			msgsw_local)), (Notify_event)(LINT_CAST(event)), 
			arg, type));
	}
	return(NOTIFY_DONE);
} 

static
Notify_value
clock_tool_event(tool_local, event, arg, type)
	Tool	*tool_local;
	Event	*event;
	Notify_arg arg;
	Notify_event_type type;
{
	short iconic_start = (tool_local->tl_flags & TOOL_ICONIC);
	Notify_value value = notify_next_event_func((Notify_client)(LINT_CAST(
		tool_local)), (Notify_event)(LINT_CAST(event)), arg, type);

	if (iconic_start != (tool_local->tl_flags & TOOL_ICONIC))
		/*
		 * Simulate interval timer expiration so that the correct
		 * time is shown after changing state.
		 */
		(void) clock_itimer_expired((Notify_client)(LINT_CAST(
			&clock_client)), ITIMER_REAL);
	return(value);
}

static
Notify_value
clock_tool_destroy(tool_local, status)
	Tool	*tool_local;
	Destroy_status status;
{
	Notify_value value = notify_next_destroy_func((Notify_client)(LINT_CAST(
		tool_local)), status);

	if (((status == DESTROY_CLEANUP) || (status == DESTROY_PROCESS_DEATH))&&
	    (value == NOTIFY_DONE))
		/*
		 * Remove clock related conditions (e.g., interval timer)
		 * if tool going away.  Could just let it expire & ignore it
		 * but then the tool's process would be a long time terminating
		 * (it would be waiting to the timeout).
		 */
		(void)notify_remove((Notify_client)(LINT_CAST(&clock_client)));
	return(value);
} 

/* ARGSUSED */
static
Notify_value
clock_itimer_expired(clock, which)
	Notify_client	clock;
	int		which;
{
	struct	itimerval itimer;
	struct	timeval c_tv;

	clock_update_tmp();
	if (tool->tl_flags & TOOL_ICONIC)  {
		if (roman) {
			clock_rom_sethands();
		} else {
			clock_sethands();
		}
	}  else
		(void)msgsw_setstring(msgsw, asctime(tmp));
	/* Reset itimer everytime */
	if (testing)
		/* Poll at top speed */
		itimer = NOTIFY_POLLING_ITIMER;
	else {
		/* Compute next timeout from current time */
		itimer.it_value.tv_usec = 0;
		itimer.it_value.tv_sec = ((show_seconds) ? 1: 60 - tmp->tm_sec);
	}
	/* Don't utilize subsequent interval */
	itimer.it_interval.tv_usec = 0;
	itimer.it_interval.tv_sec = 0;
	/* Utilize notifier hack to avoid stopped clock bug */
	c_tv = itimer.it_value;
	c_tv.tv_sec *= 2;
	notify_set_signal_check(c_tv);
	/* Set itimer event handler */
	(void) notify_set_itimer_func((Notify_client)(LINT_CAST(&clock_client)), 
		clock_itimer_expired, ITIMER_REAL, &itimer,(struct itimerval *)0);
	return(NOTIFY_DONE);
}

/* Call when tmp needs to be up-to-date */
static
clock_update_tmp()
{
	int	clock;

	if (testing) {
		if (!tmp) {
			/* Get local time from kernel */
			clock = time((time_t *)0);
			tmp = localtime (&clock);
		}
		/* Increment local time */
		if (++(tmp->tm_min) == 60) {
			tmp->tm_min = 0;
			if (++(tmp->tm_hour) == 24)
				tmp->tm_hour = 0;
		}
	} else {
		/* Get local time from kernel */
		clock = time((time_t *)0);
		tmp = localtime (&clock);
	}
}

static
clock_sethands()
{
	struct hands   *hand;

	if (show_date == TRUE)
		clock_show_date();
	(void)pr_rop(ic_mpr, 0, 0, 64, (show_date == TRUE)? 75: 64,
	    PIX_SRC, base_mpr, 0, 0);
	hand = &hand_points[(tmp->tm_hour*5 + (tmp->tm_min + 6)/12) % 60];
	(void)pr_vector(ic_mpr,
		  hand->x1, hand->y1,
		  hand->hour_x, hand->hour_y,
		  PIX_SET, 1);
	(void)pr_vector(ic_mpr,
		  hand->x2, hand->y2,
		  hand->hour_x, hand->hour_y,
		  PIX_SET, 1);

	hand = &hand_points[tmp->tm_min];
	(void)pr_vector(ic_mpr,
		  hand->x1, hand->y1,
		  hand->min_x, hand->min_y,
		  PIX_SET, 1);
	(void)pr_vector(ic_mpr,
		  hand->x2, hand->y2,
		  hand->min_x, hand->min_y,
		  PIX_SET, 1);

	if (show_seconds) {
		hand = &hand_points[tmp->tm_sec];
		(void)pr_vector(ic_mpr,
			  hand->sec_x, hand->sec_y,
			  hand->min_x, hand->min_y,
			  PIX_SET, 1);
	}
	if (face)
		paintface(ic_mpr);
	/*
	tool_set_attributes(tool, WIN_ICON_IMAGE, ic_mpr, 0);
	*/
	tool->tl_icon->ic_mpr = ic_mpr;
	(void)tool_display(tool);
}

static struct endpoints min_box[4] = {
	0, 0,
	0, 1,
	1, 1,
	1, 0,
};

static	char *monthstr[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static	char *daystr[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

static
clock_rom_sethands()
{
	register struct endpoints *hand, *handorg;
	register int i;

	/*
	 * Copy the icon image to ic_mpr.
	 */
	if (show_date == TRUE)
		clock_show_date();
	(void)pr_rop(ic_mpr, 0, 0, 64, (show_date == TRUE)? 75: 64,
	    PIX_SRC, base_mpr, 0, 0);
	/*
	 * Hour hand.
	 */
	hand = &ep_hr[(tmp->tm_hour*5 + (tmp->tm_min + 6)/12) % 60];
	for (i = 0; i < 4; i++) {
		(void)pr_vector(ic_mpr, 31 + min_box[i].x , 31 + min_box[i].y,
			hand->x + min_box[i].x , hand->y + min_box[i].y,
			PIX_SET, 1);
	}
	/*
	 * Minute hand.
	 */
	hand = &ep_min[tmp->tm_min];
	for (i = 0; i < 4; i++) {
		(void)pr_vector(ic_mpr, 31 + min_box[i].x , 31 + min_box[i].y,
			hand->x + min_box[i].x , hand->y + min_box[i].y,
			PIX_SET, 1);
	}
	/*
	 * Second hand.
	 */
	if (show_seconds) {
		hand = &ep_sec[tmp->tm_sec];
		handorg = &ep_secorg[(tmp->tm_sec + 30) % 60];
		/* cop out */
		(void)pr_vector(ic_mpr, handorg->x, handorg->y, hand->x, hand->y,
			PIX_SET, 1);
	}
	/*
	 * Install the new icon.
	 */
	if (face)
		paintface(ic_mpr);
	/*
	tool_set_attributes(tool, WIN_ICON_IMAGE, ic_mpr, 0);
	*/
	tool->tl_icon->ic_mpr = ic_mpr;
	(void)tool_display(tool);
}

static
clock_show_date()
{
	char datestr[20];
	register char *datep, *optp;
	struct pr_prpos where;

	/*
	 * Update the date field once an hour.
	 */
	if (last_hour != tmp->tm_hour) {
		last_hour = tmp->tm_wday;
		datep = datestr;
		optp = optionsptr;
		while (*optp) {
			switch (*optp++) {
			case 'm':		/* Month */
				(void)strcpy(datep, monthstr[tmp->tm_mon]);
				datep += 3;
				break;
			case 'w':		/* day of Week */
				(void)strcpy(datep, daystr[tmp->tm_wday]);
				datep += 3;
				break;
			case 'd':		/* Day of month */
				if (tmp->tm_mday >= 10) {
					*datep++ = (tmp->tm_mday / 10 + '0');
				}
				*datep++ = tmp->tm_mday % 10 + '0';
				break;
			case 'y':		/* Year */
				*datep++ = tmp->tm_year / 10 + '0';
				*datep++ = tmp->tm_year % 10 + '0';
				break;
			case 'a':		/* Am/pm */
				*datep++ = tmp->tm_hour > 11 ? 'P' : 'A';
				*datep++ = 'M';
				break;
			}
			*datep++ = ' ';
		}
		*--datep = '\0';
		where.pr = base_mpr;
		where.pos.y = 71;
		where.pos.x = 2;
		(void)pf_text(where, PIX_SRC, pfont, "          "); /* clear */
		where.pos.x = 31 - 3 * strlen(datestr);
		if (where.pos.x < 2) {
			where.pos.x = 2;
		}
		(void)pf_text(where, PIX_SRC, pfont, datestr);
	}
}
	
static char *date[] = {"", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10",
	"11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
	"21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31"};
	
#define TOP   0
#define LEFT  1
#define BOT   2
#define RIGHT 3

static struct pr_pos pos[] = {{(64-3*6)/2, 24},	/* top */
			  {8, 32+3},		/* left */
			  {(64-2*6)/2, 64-24+6},  /* bottom */
			  {32+8, 32 +3}};	/* right */

static int minq[]  =  { TOP, TOP, TOP, TOP, TOP, TOP, TOP, TOP,
			RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT,
			     RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT,
			BOT, BOT, BOT, BOT, BOT, BOT, BOT, BOT,BOT,
			     BOT, BOT, BOT, BOT, BOT, BOT,
			LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT,
			     LEFT, LEFT, LEFT, LEFT, LEFT, LEFT,
			TOP, TOP, TOP, TOP, TOP, TOP, TOP};

static
paintface(mpr)
	struct pixrect *mpr;
{
	static struct pr_prpos where;
	int arr[4], z;
	
	where.pr = mpr;
	arr[0] = arr[1] = arr[2] = arr[3] = 0;

	z = 60*tmp->tm_hour + tmp->tm_min;
	if (z > 720)
		z -= 720;
	if (90 <= z && z <= 270)
		arr[RIGHT] = 1;
	else if (270 <= z && z <= 450)
		arr[BOT] = 1;
	else if (450 <= z && z <= 630)
		arr[LEFT] = 1;
	else
		arr[TOP] = 1;
	arr[minq[tmp->tm_min]] = 1;
	if (arr[LEFT] == 0 && arr[RIGHT] == 0 && !roman) {
		where.pos = pos[LEFT];
		(void)pf_text(where, PIX_SRC, pfont, daystr[tmp->tm_wday]);
		where.pos = pos[RIGHT];
		(void)pf_text(where, PIX_SRC, pfont, date[tmp->tm_mday]);
	}
	else {
		where.pos = pos[TOP];
		(void)pf_text(where, PIX_SRC, pfont, daystr[tmp->tm_wday]);
		where.pos = pos[BOT];
		(void)pf_text(where, PIX_SRC, pfont, date[tmp->tm_mday]);
	}
}
