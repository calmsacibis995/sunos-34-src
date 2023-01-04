#ifndef lint
static	char sccsid[] = "@(#)pw_cms.c 1.4 87/01/07 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Pw_cms.c: Implement the colormap segment sharing aspect of
 *		the pixwin.h interface.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <errno.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/cms_mono.h>
#include <sunwindow/pixwin.h>
#include "pw_util.h"
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>
#include <strings.h>
extern char *sprintf();

static	void pw_set_plane_group();
static	void pw_set_planes_limited();

extern	int errno;

static	cmsid;
int	pwenforcecmstd;		/* Leave in case old program touching */
				/* Not used anymore */

#define	NO_PLANE_PTR	((int *)0)

struct	colormapseg pw_defaultcms;
u_char	pw_defaultred[256];
u_char	pw_defaultgreen[256];
u_char	pw_defaultblue[256];
struct	cms_map pw_defaultmap = {
	    pw_defaultred, pw_defaultgreen, pw_defaultblue};

static	int	pw_plane_group_preference_set;
static	int	pw_plane_group_preference;

static	int	pw_global_groups_available_set;
static	struct	win_plane_groups_available pw_global_groups_available;

static	void	pw_read_plane_group_preference();
static	void	pw_initialize_plane_group();
static	void	pw_initialize_invisible_pg();
static	int	pw_choose_prefered_plane_group();
static	int	fullscreen_pw_start_other_plane_group();
static	void	fullscreen_pw_finish_other_plane_group();
static	void	pw_set_planes_directly();

void	win_set_plane_group();
int	win_get_plane_group();
void	pw_use_fast_monochrome();

/*
 * Colormap segment manager operations.
 */
pw_setcmsname(pw, cmsname)
	struct	pixwin *pw;
	char	*cmsname;
{
	if (strncmp(pw->pw_cmsname, cmsname, CMS_NAMESIZE) != 0) {
		(void)win_clearcms(pw->pw_clipdata->pwcd_windowfd);
		(void)strncpy(pw->pw_cmsname, cmsname, CMS_NAMESIZE);
	}
}

pw_getcmsname(pw, cmsname)
	struct	pixwin *pw;
	char	*cmsname;
{
	(void)strncpy(cmsname, pw->pw_cmsname, CMS_NAMESIZE);
}

pwo_putattributes(pw, planes)
	struct	pixwin *pw;
	register int *planes;
{
	struct	colormapseg cms;
	int	myplanes;
	int	firstplanes;

	if (!planes)
		return(0);
	(void)pw_getcmsdata(pw, &cms, &firstplanes);
	/* Make sure that restricting planes only */
	if (*planes > cms.cms_size-1) {
	    (void)fprintf(stderr,
	     "can only restrict planes to <= cms_size-1 in pw_putattributes\n");
	    return(EINVAL);
	}
	/* Restrict planes */
	myplanes = *planes & cms.cms_size-1;
	/* Put planes in pixrect for protection */
#ifdef  planes_fully_implemented 
	pr_putattributes(pw->pw_pixrect, &myplanes);
#else
	(void)pw_full_putattributes(pw->pw_pixrect, &myplanes);
#endif  planes_fully_implemented
	/* Reset clipping to get changes to clipping pixrects */
	if (firstplanes != myplanes)
		(void)pw_getclipping(pw);
	return(0);
}

pwo_getattributes(pw, planes)
	struct	pixwin *pw;
	int	*planes;
{
	return(pr_getattributes(pw->pw_pixrect, planes));
}

/*
 * Will inherit screen inversion in single plane case.
 * Will initialize planes offset so that think are
 * writing to zero offset in multi-plane case.
 */
pwo_putcolormap(pw, idx, count, red, green, blue)
	register Pixwin *pw;
	int	idx, count;
	u_char	red[], green[], blue[];
{
	struct	colormapseg cms;
	struct	cms_map cmap;
	int	planes;
	int	new_cms, base_pixrect_changed = 0;

	/* Determine status of cms */
	(void)pw_getcmsdata(pw, &cms, NO_PLANE_PTR);
	/* See if changing cms */
	new_cms = (strncmp(cms.cms_name, pw->pw_cmsname, CMS_NAMESIZE) != 0);
	/*
	 * If pw_cmsname is not set then generate a unique name.
	 * Obviously, this cms will not be shared.
	 */
	if (pw->pw_cmsname[0] == '\0')
		(void)sprintf(pw->pw_cmsname, "cms%10D-%4D", getpid(), cmsid++);

	/*
	 * If changing colormap segments, maybe changing plane group.
	 */
	if (new_cms) {
		int original_plane_group, new_plane_group;

		/*
		 * Choose plane group based on new cms name, user
		 * preference and availability of plane groups.
		 */
		new_plane_group = pw_choose_prefered_plane_group(pw,
		    &original_plane_group);
		/* Tell kernel about this window's plane group */
		win_set_plane_group(pw->pw_windowfd, new_plane_group);
		/*
		 * Put plane group in pixrect (got original_plane_group
		 * from pixrect).
		 */
		if (original_plane_group != new_plane_group)
			pw_set_plane_group(pw, new_plane_group);
		/*
		 * new_cms flag will cause change to pixrect to
		 * be propagated to other pixrects in pixwin.
		 */
	}

	/*
	 * Set the cms in the kernel using cmsname.
	 * Kernel recognizes size<(original size) &| offset != 0 to be partial
	 * cmap updates.
	 */
	cms.cms_addr = idx;
	cms.cms_size = count;
	(void)strncpy(cms.cms_name, pw->pw_cmsname, CMS_NAMESIZE);
	cmap.cm_red = red;
	cmap.cm_green = green;
	cmap.cm_blue = blue;
	(void)win_setcms(pw->pw_clipdata->pwcd_windowfd, &cms, &cmap);

	/*
	 * Propagate actual color change.
	 */
	/* Clear shared locking inversion flag */
	pw->pw_clipdata->pwcd_flags &= ~PWCD_CURSOR_INVERTED;
	if (pw->pw_pixrect->pr_depth == 1) {
                u_char rbefore[2], g[2], b[2], rafter[2];
		struct screen screen;

                (void)pr_getcolormap(pw->pw_pixrect, 0, 2, rbefore, g, b);
                /*
                 * One tells monochrome displays about being reversed
                 * by doing a pr_putcolormap (although no colormap is modified).
                 * Bws also don't use the planes attribute.
                 */
                (void)pr_putcolormap(pw->pw_pixrect, idx, count, red, green, blue);
                (void)pr_getcolormap(pw->pw_pixrect, 0, 2, rafter, g, b);
		/*
		 * The zero entry of red controls the reverse mode on monochrome
		 * devices.  See if changed.
		 */
                if (rbefore[0] != rafter[0])
			base_pixrect_changed = 1;
		/* set the revers-relative-to-kernel bit for cursor rops */
		(void)win_screenget(pw->pw_windowfd, &screen);
		if (((screen.scr_background.red == rafter[0]) &&
		     (screen.scr_flags & SCR_SWITCHBKGRDFRGRD)) ||
		    ((screen.scr_background.red != rafter[0]) &&
		     (!(screen.scr_flags & SCR_SWITCHBKGRDFRGRD))))
			pw->pw_clipdata->pwcd_flags |= PWCD_CURSOR_INVERTED;
	} else {
		if (new_cms) {
			/* Put planes in pixrect for offset protection */
			planes = cms.cms_size-1;
#ifdef  planes_fully_implemented 
			pr_putattributes(pw->pw_pixrect, &planes);
#else
			(void)pw_full_putattributes(pw->pw_pixrect, &planes);
#endif  planes_fully_implemented
		} /* else used to AND with previous planes
		   * in case user has restricted planes already
		   * but this should be a no-op so removed this code.
		   */
	}

	/*
	 * If changing colormap size, retained image may need to be
	 * reallocated.
	 */
	if (new_cms && pw->pw_prretained)
		(void)pw_set_retain(pw, pw->pw_prretained->pr_width,
		    pw->pw_prretained->pr_height);

	/*
	 * Recompute clippers so that all accessing pixrects
	 * are the same if change base pixrect or have new cms.
	 */
	if (base_pixrect_changed || new_cms)
		(void)pw_getclipping(pw);

	/*
	 * Prepare surface of entire visible area of the pixwin.
	 */
	if (new_cms)
		(void)pw_preparesurface_full(pw, RECT_NULL, 1);
	return(0);
}

pwo_getcolormap(pw, idx, count, red, green, blue)
	struct	pixwin *pw;
	int	idx, count;
	u_char	red[], green[], blue[];
{
	struct	colormapseg cms;
	struct	cms_map cmap;

	/*
	 * If pw_cmsname is not set then error.
	 */
	if (pw->pw_cmsname[0] == '\0') {
		(void)fprintf(stderr, "pw_cmsname NULL in getcolormap\n");
		return(EINVAL);
	}
	/*
	 * Get cms from window system.
	 */
	cms.cms_addr = idx;
	cms.cms_size = count;
	(void)strncpy(cms.cms_name, pw->pw_cmsname, CMS_NAMESIZE);
	cmap.cm_red = red;
	cmap.cm_green = green;
	cmap.cm_blue = blue;
	(void)win_getcms(pw->pw_clipdata->pwcd_windowfd, &cms, &cmap);
	/*
	 * Internal check.
	 */
	if (strncmp(pw->pw_cmsname, cms.cms_name, CMS_NAMESIZE) != 0) {
		(void)fprintf(stderr, "cms names differ in pw_getcolormap\n");
		return(EINVAL);
	}
	return(0);
}

pw_preparesurface(pw, rect)
	register Pixwin *pw;
	struct	rect *rect;
{
	(void)pw_preparesurface_full(pw, rect, 0);
}

pw_preparesurface_full(pw, rect, full)
	register Pixwin *pw;
	struct	rect *rect;
	int full;
{
	struct	rect rdest;
	struct	colormapseg cms;
	char	*plane_groups_available =
		    pw->pw_clipdata->pwcd_plane_groups_available;
	int	plane_group;

	/* See if some other pixwin on this window changed the cms */
	(void)pw_getcmsdata(pw, &cms, NO_PLANE_PTR);
	if (strncmp(pw->pw_cmsname, cms.cms_name, CMS_NAMESIZE) != 0) {
		/* Get current cms/planes of window for this pixwin */
		(void)pw_initcms(pw);
		/* Update cms variable */
		(void)pw_getcmsdata(pw, &cms, NO_PLANE_PTR);
	}

	/* Supply rdest if rect not supplied.  Restrict to visible area. */
	if (rect)
		(void)rect_intersection(&pw->pw_clipdata->pwcd_clipping.rl_bound,
		    rect, &rdest);
	else
		rdest = pw->pw_clipdata->pwcd_clipping.rl_bound;
	/* See if anything to do */
	if (rect_isnull(&rdest))
		/* Assumes no side affects from calling this routine */
		return;

	/* See if some other pixwin on this window changed the plane group */
	if (plane_groups_available[PIXPG_OVERLAY_ENABLE]) {
	/* NOTE: ADD HERE AS NEW ENABLE PLANES BECOME AVAILABLE */
		/* Get this window's choice of planes from kernel */
		plane_group = win_get_plane_group(pw->pw_windowfd,
		    pw->pw_pixrect);
		if (plane_group != pw->pw_clipdata->pwcd_plane_group) {
			/* Get current cms/planes of window for this pixwin */
			(void)pw_initcms(pw);
			/* Update plane_group variable */
			plane_group = win_get_plane_group(pw->pw_windowfd,
			    pw->pw_pixrect);
		}
	}

	/* Remove cursor for duration of preparation */
	(void)pw_lock(pw, &rdest);
	/*
	 * Enable bits that program will be writing to.
	 * Do before enable planes to avoid "ghost" image flashes.
	 */
	if (pw->pw_pixrect->pr_depth > 1 ||
	    plane_groups_available[PIXPG_OVERLAY_ENABLE]) {
	    /* NOTE: ADD HERE AS NEW ENABLE PLANES BECOME AVAILABLE */
		/* Prepare depth > 1 pixwin's surface */
		pw_initialize_plane_group(pw,
		    pr_get_plane_group(pw->pw_pixrect), cms.cms_addr, &rdest);
	}
	/*
	 * else simple 1 deep pixwins clear own areas so don't prepare
	 * surface because pixwins are supposed to clear (or set) their
	 * own surfaces anyway.
	 */

	/*
	 * The enable plane and other invisible plane groups are initialized
	 * here.  Do the "other" invisible plane groups last so as to not
	 * slow down the preceived repaint speed so much. 
	 */
	if (plane_groups_available[PIXPG_OVERLAY_ENABLE]) {
	/* NOTE: ADD HERE AS NEW ENABLE PLANES BECOME AVAILABLE */
		/* See if need to set enable for color */
		if (plane_group == PIXPG_8BIT_COLOR) {
			pw_initialize_invisible_pg(pw,
			    PIXPG_OVERLAY_ENABLE, 0, &rdest, plane_group);
			if (plane_groups_available[PIXPG_OVERLAY] && full)
				pw_initialize_invisible_pg(pw,
				    PIXPG_OVERLAY, 0, &rdest, plane_group);
		/* See if need to set enable for overlay */
		} else if (plane_group == PIXPG_OVERLAY) {
			pw_initialize_invisible_pg(pw,
			    PIXPG_OVERLAY_ENABLE, 1, &rdest, plane_group);
			if (plane_groups_available[PIXPG_8BIT_COLOR] && full)
				pw_initialize_invisible_pg(pw,
				    PIXPG_8BIT_COLOR, 0, &rdest, plane_group);
		} else {
			/*
			 * Nop.  One can get into this situation if this
			 * routine gets called recursively during damage
			 * repair.  The affect of ignoring this is that
			 * surface preparation occasionally gets done twice,
			 * which is OK.
			 *
			 * More explicitly:
			 *	pw_preparesurface_full() here we are
			 *	pw_damaged()
			 *	pwco_lockstd()
			 *	pw_initialize_plane_group()
			 *	pw_initialize_invisible_pg()
			 *	pw_preparesurface_full() above here
			 *	pw_damaged()
			 * 	...
			 */
		}
	}
	/* Replace cursor */
	(void)pw_unlock(pw);

	return;
}

pw_blackonwhite( pw, first, last)
	struct pixwin *pw;
	int first, last;
{
	unsigned char black = 0, white = -1;

	(void)pw_setgrnd(pw, first, last, black, black, black, white, white, white);
}

pw_whiteonblack( pw, first, last)
	struct pixwin *pw;
	int first, last;
{
	unsigned char black = 0, white = -1;

	(void)pw_setgrnd(pw, first, last, white, white, white, black, black, black);
}

pw_reversevideo( pw, first, last)
	struct pixwin *pw;
	int first, last;
{
	struct	screen screen;

	(void)win_screenget(pw->pw_clipdata->pwcd_windowfd, &screen);
	(void)pw_setgrnd(pw, first, last,
	    screen.scr_background.red,
		screen.scr_background.green,
		screen.scr_background.blue,
	    screen.scr_foreground.red,
		screen.scr_foreground.green,
		screen.scr_foreground.blue);
}

pw_setdefaultcms(cms, map)
	struct	colormapseg *cms;
	struct	cms_map *map;
{
	if (cms->cms_size > 256)
		return(-1);
	pw_defaultcms = *cms;
	bcopy((caddr_t)map->cm_red, (caddr_t)pw_defaultmap.cm_red, cms->cms_size);
	bcopy((caddr_t)map->cm_green, (caddr_t)pw_defaultmap.cm_green, cms->cms_size);
	bcopy((caddr_t)map->cm_blue, (caddr_t)pw_defaultmap.cm_blue, cms->cms_size);
	return(0);
}

pw_getdefaultcms(cms, map)
	struct	colormapseg *cms;
	struct	cms_map *map;
{
	if (cms->cms_size < pw_defaultcms.cms_size ||
	    pw_defaultcms.cms_size == 0)
		return(-1);
	*cms = pw_defaultcms;
	bcopy((caddr_t)pw_defaultmap.cm_red, (caddr_t)map->cm_red, pw_defaultcms.cms_size);
	bcopy((caddr_t)pw_defaultmap.cm_green, (caddr_t)map->cm_green, pw_defaultcms.cms_size);
	bcopy((caddr_t)pw_defaultmap.cm_blue, (caddr_t)map->cm_blue, pw_defaultcms.cms_size);
	return(0);
}

/*
 * Newly Public routines:
 */

struct pixwin *
pw_open_monochrome(windowfd)
	int windowfd;
{
	struct pixwin *pw = pw_open(windowfd);
	
	if (pw)
		pw_use_fast_monochrome(pw);
	return (pw);
}

void
pw_use_fast_monochrome(pw)
	register Pixwin *pw;
{
	/* Use overlay plane for monochrome cms (if overlay is available) */
	if (pw->pw_clipdata->pwcd_plane_group == PIXPG_8BIT_COLOR &&
	    strncmp(pw->pw_cmsname, CMS_MONOCHROME, CMS_NAMESIZE) == 0 &&
	    (pw->pw_clipdata->pwcd_plane_groups_available[PIXPG_OVERLAY])) {
		win_set_plane_group(pw->pw_windowfd, PIXPG_OVERLAY);
		pw_set_planes_directly(pw, PIXPG_OVERLAY, PIX_ALL_PLANES);
		pw->pw_cmsname[0] = NULL; /* Forces call to pw_initcms */
		(void)pw_preparesurface_full(pw, RECT_NULL, 1);
	}
}

void
pw_set_plane_group_preference(plane_group)
	int plane_group;
{
	pw_plane_group_preference = plane_group;
	pw_plane_group_preference_set = 1;
}

int
pw_get_plane_group_preference()
{
	return (pw_plane_group_preference);
}

/*
 * Semi-public routines:
 */

pw_getcmsdata(pw, cms, planes)
	struct	pixwin *pw;
	struct	colormapseg *cms;
	int	*planes;
{
	struct	cms_map cmap;

	if (planes != NO_PLANE_PTR)
		(void)pr_getattributes(pw->pw_pixrect, planes);
	/*
	 * Get the truth from the kernel
	 */
	cmap.cm_red = cmap.cm_green = cmap.cm_blue = 0;
	(void)win_getcms(pw->pw_clipdata->pwcd_windowfd, cms, &cmap);
}

/*
 * Newly Semi-public routines:
 */

int
win_get_plane_group(windowfd, pr)
	int windowfd;
	struct pixrect *pr;
{
	int plane_group;

	/* Get this window's choice of planes from kernel */
	if (ioctl(windowfd, WINGETPLANEGROUP, &plane_group) == -1) {
		if (errno == ENOTTY)
			/*
			 * For backwards compatibility reasons,
			 * ignore fact that kernel not prepared for this call.
			 * If we are running on an old kernel, then we
			 * should not find ourselves running on a framebuffer
			 * with multiple plane groups.  This is because
			 * there was no notion of plane groups in 3.0.
			 * So, just use planes as full_planes.
			 */
			{}
		else
			(void)werror(-1, WINGETPLANEGROUP);
		plane_group = pr_get_plane_group(pr);
	}
	return (plane_group);
}

void
win_set_plane_group(windowfd, new_plane_group)
	int windowfd;
	int new_plane_group;
{
	if (ioctl(windowfd, WINSETPLANEGROUP, &new_plane_group) == -1) {
		if (errno == ENOTTY)
			/*
			 * For backwards compatibility reasons,
			 * ignore fact that kernel not prepared for this call.
			 * If we are running on an old kernel, then we
			 * should not find ourselves running on a framebuffer
			 * with multiple plane groups.  This is because
			 * there was no notion of plane groups in 3.0.
			 *
			 * For forwards compatibility reasons,
			 * the kernel knows that if it doesn't get this ioctl
			 * that the plane group must be what the framebuffer
			 * answered with to the FBIOTYPE call when suntools
			 * was first started.
			 */
			{}
		else
			(void)werror(-1, WINSETPLANEGROUP);
	}
}

void
win_get_plane_groups_available(windowfd, pr, available)
	int windowfd;
	struct pixrect *pr;
	struct win_plane_groups_available *available;
{
	/* Get this screen's choice of plane groups from kernel */
	if (ioctl(windowfd, WINGETAVAILPLANEGROUPS, available) == -1) {
		if (errno == ENOTTY)
			/*
			 * For backwards compatibility reasons,
			 * ignore fact that kernel not prepared for this call.
			 * If we are running on an old kernel, then we
			 * should not find ourselves running on a framebuffer
			 * with multiple plane groups.
			 */
			{}
		else
			(void)werror(-1, WINGETAVAILPLANEGROUPS);
		available->plane_groups_available[pr_get_plane_group(pr)] = 1;
	}
}

void
win_set_plane_groups_available(windowfd, available)
	int windowfd;
	struct win_plane_groups_available *available;
{
	if (ioctl(windowfd, WINSETAVAILPLANEGROUPS, available) == -1) {
		if (errno == ENOTTY)
			/*
			 * For backwards compatibility reasons,
			 * ignore fact that kernel not prepared for this call.
			 */
			{}
		else
			(void)werror(-1, WINSETAVAILPLANEGROUPS);
	}
}

/*
 * Private routines:
 */

static void
pw_initialize_plane_group(pw, plane_group, color, rdest)
	Pixwin *pw;
	int plane_group;
	int color;
	Rect *rdest;
{ 
	struct	rect rintersect;
	int planes_save, plane_group_save, full_planes = PIX_ALL_PLANES;
	struct	pixrect *pr = pw->pw_clipdata->pwcd_prmulti;

	/* Remember original planes */
	plane_group_save = pr_get_plane_group(pr);
	(void)pr_getattributes(pr, &planes_save);
	/*
	 * Set plane group and planes.  Set planes to enable writing offset,
	 * effectively clears.
	 */
	pw_set_planes_directly(pw, plane_group, full_planes);
	/* Lock entire bounding rect of access */
	(void)pw_lock(pw, rdest);
	/* Write background */
	pw_begincliploop(pw, rdest, &rintersect);
		(void)pr_rop(pr, rintersect.r_left, rintersect.r_top,
		    rintersect.r_width, rintersect.r_height,
		    PIX_COLOR(color)|PIX_SRC|PIX_DONTCLIP,
		    (Pixrect *)0, 0, 0);
	/* Terminate clipping loop */
	pw_endcliploop();
	/* Unlock */
	(void)pw_unlock(pw);
	/* Reset planes & plane group */
	pw_set_planes_directly(pw, plane_group_save, planes_save);
}

static	void
pw_initialize_invisible_pg(pw, invisible_pg, color, r, original_pg)
	Pixwin *pw;
	int invisible_pg;
	int color;
	Rect *r;
	int original_pg;
{
	(void) win_set_plane_group(pw->pw_windowfd, invisible_pg);
	pw_initialize_plane_group(pw, invisible_pg, color, r);
	(void) win_set_plane_group(pw->pw_windowfd, original_pg);
}

/* Would make static if weren't worried about breaking someones program */
pw_setgrnd( pw, first, last, frred, frgreen, frblue, bkred, bkgreen, bkblue)
	struct pixwin *pw;
	int first, last;
	unsigned char frred, frgreen, frblue, bkred, bkgreen, bkblue;
{
	unsigned char red[256], green[256], blue[256];
	struct	colormapseg cms;

	(void)pw_getcmsdata(pw, &cms, NO_PLANE_PTR);
	(void)pw_getcolormap( pw, 0, cms.cms_size, red, green, blue);
	red[first] = bkred; green[first] = bkgreen; blue[first] = bkblue;
	red[last] = frred; green[last] = frgreen; blue[last] = frblue;
	(void)pw_putcolormap( pw, 0, cms.cms_size, red, green, blue);
}

/* Referenced from pw_access.c */
pw_initcms(pw)
	register Pixwin *pw;
{
	extern	int pw_damaged();
	u_char	red[256], green[256], blue[256]; 
	struct	colormapseg cms;
	struct	cms_map map;
	int	new_planes;
	int	original_plane_group, new_plane_group;
	int	new_window;
	register int	i;

	/*
	 * Choose the initial colormap segment
	 */
	/* Set up default cms, but don't do anything to pw yet */
	(void)pw_initdefaultcms(pw);
	/* Determine current window cms state */
	(void)pw_getcmsdata(pw, &cms, NO_PLANE_PTR);
	/* The name indicates whether or not the cms has been set yet */
	if (cms.cms_name[0] == '\0') {
		/*
		 * The cms for this window hasn't been set yet.
		 * So, initialize it to the default cms for this process.
		 */
		map.cm_red = red; map.cm_green = green; map.cm_blue = blue;
		cms.cms_size = 256;
		(void)pw_getdefaultcms(&cms, &map);
		/* Set the name in the pixwin */
		(void)strncpy(pw->pw_cmsname, cms.cms_name, CMS_NAMESIZE);
		/*
		 * Will prepare surface because first pixwin on window.
		 * Is guaranteed to cover window because can't be region.
		 */
		new_window = 1;
	} else {
		/* Set the name in the pixwin (pw_getcolormap needs name set) */
		(void)strncpy(pw->pw_cmsname, cms.cms_name, CMS_NAMESIZE);
		/* Initialize pixwin with existing cms */
		(void)pw_getcolormap(pw, 0, cms.cms_size, red, green, blue);
		/* Wouldn't prepare surface because 1st pixwin did */
		new_window = 0;
	}

	/*
	 * Choose the initial plane group
	 */
	if (!pw_global_groups_available_set) {
		win_get_plane_groups_available(pw->pw_windowfd,
		    pw->pw_pixrect, &pw_global_groups_available);
		pw_global_groups_available_set = 1;
	}
	/* Find out 'user' preference if not explicitly set by program */
	if (!pw_plane_group_preference_set)
		pw_read_plane_group_preference(pw);
	/* Find out available planes from pixrect */
	(void) pr_available_plane_groups(pw->pw_pixrect, PIX_MAX_PLANE_GROUPS,
	    pw->pw_clipdata->pwcd_plane_groups_available);
	/* Restrict available planes */
	for (i = 0; i < WIN_MAX_PLANE_GROUPS; i++) {
		if (pw->pw_clipdata->pwcd_plane_groups_available[i] &&
		    !pw_global_groups_available.plane_groups_available[i])
			pw->pw_clipdata->pwcd_plane_groups_available[i] = 0;
	}
	/* Use existing plane group if already set */
	if (!new_window)
		/* Use existing plane group */
		original_plane_group = new_plane_group =
		    win_get_plane_group(pw->pw_windowfd, pw->pw_pixrect);
	else {
		/*
		 * The plane group for this window hasn't been set yet.
		 * So, initialize it based on default cms name, user
		 * preference and availability.
		 */
		new_plane_group = pw_choose_prefered_plane_group(pw,
		    &original_plane_group);
	}

	/*
	 * Set the initial plane group
	 */
	if (original_plane_group != new_plane_group)
		/* Tell kernel about this window's choice of plane group */
		win_set_plane_group(pw->pw_windowfd, new_plane_group);
	/* Put plane group in pixrect (this may be redundant) */
	pw_set_plane_group(pw, new_plane_group);

	/*
	 * Set the pixrect planes attribute.  We always choose the full
	 * planes available to a given cms size.  This is because the
	 * planes attribute is associated with a pixrect, not the entire
	 * window.
	 */
	/* Restrict planes */
	new_planes = cms.cms_size-1;
	/*
	 * Put planes in pixrect for enabling of used planes.
	 * This plane data will be propogated to rest of pixrect
	 * when do pw_exposed (below).
	 */
#ifdef  planes_fully_implemented 
	pr_putattributes(pw->pw_pixrect, &new_planes);
#else
	(void)pw_full_putattributes(pw->pw_pixrect, &new_planes);
#endif  planes_fully_implemented

	/*
	 * Set the initial colormap segment
	 */
	/* Set the cms (planes and colormap) */
	(void)pw_putcolormap(pw, 0, cms.cms_size, red, green, blue);

	/* See if reinitializing the cms */
	if (pw->pw_clipops->pwco_getclipping == pw_damaged) {
		/* Propagate plane related changes to all pixrects */
		pw_set_planes_directly(pw, new_plane_group, new_planes);
		/* Assume being called from pw_preparesurface */
		return;
	}

	/* Figure new clipping (initial getclipping op was nop) */
	(void)pw_exposed(pw);
	/*
	 * Prepare surface if first time opening pixwin on this window.
	 * Previous attempts to call pw_preparesurface have been no-ops
	 * because the clipping was null.  RECT_NULL forces entire
	 * visible area.
	 */
	if (new_window)
		(void)pw_preparesurface_full(pw, RECT_NULL, 1);
}

static	void
pw_read_plane_group_preference(pw)
	Pixwin *pw;
{
	register char *en_str;
	char *getenv();
	char *available_plane_groups =
	    pw_global_groups_available.plane_groups_available;

	en_str=getenv("PW_PLANES");
	if (en_str == NULL)
		goto Default;
	else if ((strcmp(en_str, "OVERLAY") == 0) ||
	    (strcmp(en_str, "BW") == 0))
		pw_plane_group_preference = PIXPG_OVERLAY;
	else if ((strcmp(en_str, "COLOR") == 0) ||
	    (strcmp(en_str, "8BIT") == 0))
		pw_plane_group_preference = PIXPG_8BIT_COLOR;
	else {
Default:
		/*
		 * Default based on available planes.  Change order here
		 * to affect entire systems color preference.
		 * Current setup prefers the overlay plane.
		 */
		if (available_plane_groups[PIXPG_8BIT_COLOR])
			pw_plane_group_preference = PIXPG_8BIT_COLOR;
		else if (available_plane_groups[PIXPG_OVERLAY])
			pw_plane_group_preference = PIXPG_OVERLAY;
		else
			if (pw->pw_pixrect->pr_depth == 1)
				pw_plane_group_preference = PIXPG_MONO;
			else
				pw_plane_group_preference = PIXPG_8BIT_COLOR;
		/* NOTE: ADD HERE WHEN HAVE NEW PLANE GROUP */
	}
	/* NOTE: ADD HERE WHEN HAVE NEW PLANE GROUP */
	pw_plane_group_preference_set = 1;
}

/*
 * The plane group for this window hasn't been set yet.
 * So, initialize it based on default cms name, user
 * preference and availability.
 */
static int
pw_choose_prefered_plane_group(pw, original_plane_group)
	Pixwin *pw;
	int *original_plane_group;
{
	char *plane_groups_available =
	    pw->pw_clipdata->pwcd_plane_groups_available;
	int new_plane_group;

	*original_plane_group = pr_get_plane_group(pw->pw_pixrect);

#ifdef	notdef
	/* Use overlay plane for monochrome cms (if overlay is available) */
	if ((strncmp(pw->pw_cmsname, CMS_MONOCHROME, CMS_NAMESIZE) == 0) &&
	    (plane_groups_available[PIXPG_OVERLAY]))
		new_plane_group = PIXPG_OVERLAY;
	/*
	 * NOTE: ADD HERE WHEN HAVE NEW PLANE GROUP THAT HAS A STRONG
	 * AFFINITY FOR A PARTICULAR CMS (like "monochrome").
	 */
	/* See if want to make plane group the prefered group */
	else
#endif	notdef
	if (plane_groups_available[pw_plane_group_preference])
		new_plane_group = pw_plane_group_preference;
	else
		new_plane_group = *original_plane_group;
	return (new_plane_group);
}

/* Would make static if weren't worried about breaking someones program */
pw_initdefaultcms(pw)
	struct	pixwin *pw;
{
	struct	screen screen;
	register struct	singlecolor *bkgnd;
	register struct	singlecolor *frgnd;
	struct	singlecolor *tmpgnd;

	if (pw_defaultcms.cms_size != 0)
		return;
	/* Initialize to monochrome cms. */
	pw_defaultcms.cms_size = CMS_MONOCHROMESIZE;
	pw_defaultcms.cms_addr = 0;
	(void)strncpy(pw_defaultcms.cms_name, CMS_MONOCHROME, CMS_NAMESIZE);
	/* Adjust notion of foreground and background to match screen's. */
	(void)win_screenget(pw->pw_clipdata->pwcd_windowfd, &screen);
	bkgnd = &screen.scr_background;
	frgnd = &screen.scr_foreground;
	if (screen.scr_flags & SCR_SWITCHBKGRDFRGRD) {
		tmpgnd = bkgnd;
		bkgnd = frgnd;
		frgnd = tmpgnd;
	}
	pw_defaultmap.cm_red[BLACK] = frgnd->red;
	pw_defaultmap.cm_green[BLACK] = frgnd->green;
	pw_defaultmap.cm_blue[BLACK] = frgnd->blue;
	pw_defaultmap.cm_red[WHITE] = bkgnd->red;
	pw_defaultmap.cm_green[WHITE] = bkgnd->green;
	pw_defaultmap.cm_blue[WHITE] = bkgnd->blue;
}

/*
 * Special fullscreen access routines:
 */

/*
 * Anyone calling these fullscreen_pw_* routines is assumed to have
 * done a fullscreen_init, is using the fullscreen pixwin during
 * this call, hasn't done any prepare surface under these bits,
 * and should be using a PIX_NOT(PIX_DST) operation.  Also, pw_lock
 * should not have been called before using these operations.
 */
fullscreen_pw_vector(pw, x0, y0, x1, y1, op, cms_index)
	register struct	pixwin *pw;
	int	op;
	register int	x0, y0, x1, y1;
	int	cms_index;
{
	int original_planes, original_plane_group;
	int left, top;

	/* Draw vector in current plane group */
	(void)pw_vector(pw, x0, y0, x1, y1, op, cms_index);
	/* Set up pw to draw in another plane group */
	left = min(x0, x1);
	top = min(y0, y1);
	if (fullscreen_pw_start_other_plane_group(pw,
	    left, top, max(x0, x1)+1-left, max(y0, y1)+1-top,
	    &original_planes, &original_plane_group)) {
		/* Draw vector in alternate plane group */
		(void)pw_vector(pw, x0, y0, x1, y1, op, cms_index);
		/* Undo what fullscreen_pw_other_plane_group did */
		fullscreen_pw_finish_other_plane_group(pw, original_planes,
		    original_plane_group);
	}
	return;
}

fullscreen_pw_write(pw, xw, yw, width, height, op, pr, xr, yr)
	register Pixwin *pw;
	int	op, xw, yw, width, height;
	struct	pixrect *pr;
	int	xr, yr;
{
	int original_planes, original_plane_group;

	/* Do rop in current plane group */
	(void)pw_write(pw, xw, yw, width, height, op, pr, xr, yr);
	/* Set up pw to draw in another plane group */
	if (fullscreen_pw_start_other_plane_group(pw, xw, yw, width, height,
	    &original_planes, &original_plane_group)) {
		/* Do rop in alternate plane group */
		(void)pw_write(pw, xw, yw, width, height, op, pr, xr, yr);
		/* Undo what fullscreen_pw_other_plane_group did */
		fullscreen_pw_finish_other_plane_group(pw, original_planes,
		    original_plane_group);
	}
	return;
}

static int
fullscreen_pw_start_other_plane_group(pw, left, top, width, height,
    original_planes, original_plane_group)
	register Pixwin *pw;
	int left, top, width, height;
	int *original_planes, *original_plane_group;
{
	char *available_plane_group =
	    pw->pw_clipdata->pwcd_plane_groups_available;
	Rect rdest;
	int new_plane_group;

	/* Adjust operation's offset because didn't go thru macro */
	left = PW_X_OFFSET(pw, left);
	top = PW_Y_OFFSET(pw, top);
	/* Remember current planes */
	(void)pr_getattributes(pw->pw_pixrect, original_planes);
	/* Remember current plane group */
	*original_plane_group = pw->pw_clipdata->pwcd_plane_group;
	/*
	 * If we are in the 8 bit color plane group now and the overlay plane
	 * is available then set things up to write in the overlay plane.
	 */
	if ((*original_plane_group == PIXPG_8BIT_COLOR) &&
	    (available_plane_group[PIXPG_OVERLAY]))
		new_plane_group = PIXPG_OVERLAY;
	/*
	 * If we are in the overlay plane group now and the
	 * 8 bit color plane group is available then set things
	 * up to write in the 8 bit color plane group.
	 */
	else if ((*original_plane_group == PIXPG_OVERLAY) &&
	    (available_plane_group[PIXPG_8BIT_COLOR]))
		new_plane_group = PIXPG_8BIT_COLOR;
	/* Else unknown plane group involved */
	else
		/* Return if no other plane groups to worry about */
		return (0);
	/*
	 * Set new plane group temporarily so that cursor lifting is
	 * done on the new plane group.
	 */
	win_set_plane_group(pw->pw_windowfd, new_plane_group);
	/*
	 * Lock pixwin so that clipping change doesn't blitz temporary
	 * plane group changes.
	 */
	PW_SETUP(pw, rdest, Continue, left, top, width, height);
Continue:
	/*
	 * Set all pixrects to operate in all planes of given plane group.
	 */
	pw_set_planes_directly(pw, new_plane_group, PIX_ALL_PLANES);
	return (1);
}

static	void
fullscreen_pw_finish_other_plane_group(pw, original_planes,
    original_plane_group)
	register Pixwin *pw;
	int original_planes, original_plane_group;
{
	/* Reset plane group accessing */
	pw_set_planes_directly(pw, original_plane_group, original_planes);
	/* Unlock pixwin (was locked in fullscreen_pw_other_plane_group) */
	(void)pw_unlock(pw);
	/*
	 * Set plane group back to original so that cursor lifting is
	 * done on the original plane group (was set to new plane
	 * group in fullscreen_pw_other_plane_group).
	 */
	win_set_plane_group(pw->pw_windowfd, original_plane_group);
}

static void
pw_set_planes_directly(pw, plane_group, planes)
	Pixwin *pw;
	int plane_group;
	int planes;
{
	register struct	pixwin_prlist *prl;
	register struct pixwin_clipdata *pwcd = pw->pw_clipdata;

	pw_set_planes_limited(pw, plane_group, planes);
	if (pwcd->pwcd_prmulti)
		pr_set_planes(pwcd->pwcd_prmulti, plane_group, planes);
	if (pwcd->pwcd_prsingle)
		pr_set_planes(pwcd->pwcd_prsingle, plane_group, planes);
	for (prl = pwcd->pwcd_prl;prl;prl = prl->prl_next)
		pr_set_planes(prl->prl_pixrect, plane_group, planes);
}

void
fullscreen_pw_copy(pw, xw, yw, width, height, op, pw_src, xr, yr)
	Pixwin	*pw;
	register int	op, xw, yw, width, height;
	Pixwin	*pw_src;
	int	xr, yr;
{
#define	do_copy(pg) \
		{ pr_set_planes(pr, (pg), PIX_ALL_PLANES); \
		(void)pr_rop(pr, xw, yw, width, height, op, pr, xr, yr); }

	int original_planes, original_plane_group;
	register char *plane_groups_available =
		    pw->pw_clipdata->pwcd_plane_groups_available;
	register struct	pixrect *pr;

	if (pw != pw_src)
		return;
	pr = pw->pw_pixrect;
	/* Remember current plane stuff */
	original_plane_group = pr_get_plane_group(pr);
	(void)pr_getattributes(pr, &original_planes);
	/*
	 * Copy plane group that belongs to the pixwin.
	 * Do before enable planes to avoid "ghost" image flashes.
	 */
	do_copy(original_plane_group)
	/*
	 * The enable plane and other invisible plane groups are copied
	 * here.  Do the "other" invisible plane groups last so as to not
	 * slow down the preceived repaint speed so much. 
	 */
	if (plane_groups_available[PIXPG_OVERLAY_ENABLE]) {
	/* NOTE: ADD HERE AS NEW ENABLE PLANES BECOME AVAILABLE */
		do_copy(PIXPG_OVERLAY_ENABLE)
		/* See if need to copy overlay */
		if (original_plane_group == PIXPG_8BIT_COLOR &&
		    plane_groups_available[PIXPG_OVERLAY])
			do_copy(PIXPG_OVERLAY)
		/* See if need to copy color plane group */
		if (original_plane_group == PIXPG_OVERLAY &&
		    plane_groups_available[PIXPG_8BIT_COLOR])
			do_copy(PIXPG_8BIT_COLOR)
	}
	/* Restore state */
	pr_set_planes(pr, original_plane_group, original_planes);
	return;
#undef	do_copy
}

Pw_pixel_cache *
pw_save_pixels(pw, r)
	register Pixwin *pw;
	register Rect  *r;
{
	register int pg;
	register struct pixrect *pr = pw->pw_pixrect;
	struct	pixrect *mpr;
	Pw_pixel_cache *pc;
	caddr_t calloc();
	int original_plane_group, original_planes;
	register struct pixwin_clipdata *pwcd = pw->pw_clipdata;

	/* Allocate pixel cache */
	pc = (Pw_pixel_cache *)calloc(sizeof(Pw_pixel_cache), 1);
	if (pc == PW_PIXEL_CACHE_NULL)
		goto NoMem;
	pc->r = *r;

	/* Cycle through available plane groups */
	for (pg = 0; pg < PIX_MAX_PLANE_GROUPS; pg++) {
		/*
		 * Only save image of current plane group and the enable
		 * plane, if available.
		 */
		if (pwcd->pwcd_plane_groups_available[pg] &&
		    (pwcd->pwcd_plane_group == pg ||
		    pg == PIXPG_OVERLAY_ENABLE)) {
			/* Remember original plane state */
			original_plane_group = pr_get_plane_group(pr);
			(void)pr_getattributes(pr, &original_planes);
			/* Set plane group to pg */
			pw_set_planes_limited(pw, pg, PIX_ALL_PLANES);
			/* Allocate a mpr */
			mpr = mem_create(r->r_width, r->r_height, pr->pr_depth);
			if (mpr == (struct pixrect *)0)
				goto NoMem;
			/*
			 * Save pixels.
			 *
			 * Subvert the pixwin interface,
			 * because pw_read requires a
			 * PIX_DONTCLIP|PIX_SRC to not clip to
			 * the src (the window) but the PIX_DONTCLIP
			 * flags makes the call unsafe if try to read
			 * from off the screen.
			 */
			(void)pw_lock(pw, r);
			(void)pr_rop(mpr, 0, 0, r->r_width, r->r_height, PIX_SRC, pr,
			    r->r_left + pwcd->pwcd_screen_x,
			    r->r_top + pwcd->pwcd_screen_y);
			(void)pw_unlock(pw);
			/* Set plane state to original */
			pw_set_planes_limited(pw, original_plane_group,
			    original_planes);
			/* Note in data structure */
			pc->plane_group[pg] = mpr;
		}
	}
	return (pc);
NoMem:
	(void)fprintf(stderr, "Couldn't allocate memory in pw_save_pixels\n");
	pw_restore_pixels(pw, pc);
	return (PW_PIXEL_CACHE_NULL);
}

void
pw_restore_pixels(pw, pc)
	register Pixwin *pw;
	Pw_pixel_cache *pc;
{
	register int pg;
	register struct pixrect *pr = pw->pw_pixrect;
	struct	pixrect *mpr;
	int original_plane_group, original_planes;
	register Rect *r = &pc->r;

	if (pc == PW_PIXEL_CACHE_NULL)
		return;

	/* Cycle through available plane groups */
	for (pg = 0; pg < PIX_MAX_PLANE_GROUPS; pg++) {
		/*
		 * Only save image of current plane group and the enable
		 * plane, if available.
		 */
		mpr = pc->plane_group[pg];
		if (mpr != (struct pixrect *)0) {
			/* Remember original plane state */
			original_plane_group = pr_get_plane_group(pr);
			(void)pr_getattributes(pr, &original_planes);
			/* Set plane group to pg */
			pw_set_planes_limited(pw, pg, PIX_ALL_PLANES);
			/* Restore pixels */
			(void)pw_lock(pw, r);
			(void)pr_rop(pr,
			    r->r_left + pw->pw_clipdata->pwcd_screen_x,
			    r->r_top + pw->pw_clipdata->pwcd_screen_y,
			    r->r_width, r->r_height, PIX_SRC, mpr, 0, 0);
			(void)pw_unlock(pw);
			/* Set plane state to original */
			pw_set_planes_limited(pw, original_plane_group,
			    original_planes);
			/* Free the mpr */
			mem_destroy(mpr);
		}
	}
	/* Free pixel cache */
	free((caddr_t)pc);
	return;
}

static	void
pw_set_planes_limited(pw, plane_group, planes)
	Pixwin *pw;
	int plane_group;
	int planes;
{
	pw->pw_clipdata->pwcd_plane_group = plane_group;
	pr_set_planes(pw->pw_pixrect, plane_group, planes);
}

static	void
pw_set_plane_group(pw, plane_group)
	Pixwin *pw;
	int plane_group;
{
	pw->pw_clipdata->pwcd_plane_group = plane_group;
	pr_set_plane_group(pw->pw_pixrect, plane_group);
}

#ifndef  planes_fully_implemented 
pw_full_putattributes(pr, planes)
	struct pixrect *pr;
	int *planes;
{
	pr_set_planes(pr, pr_get_plane_group(pr), *planes);
}
#endif  planes_fully_implemented

#ifdef	using_pixwin_attr_interface
#define	PIXWIN_ATTR(type, ordinal)	ATTR(ATTR_PKG_PIXWIN, type, ordinal)

typedef enum {

    /* boolean attributes */
    PIXWIN_IS_COLOR_AVAILABLE 		= PIXWIN_ATTR(ATTR_BOOLEAN, 1),
    PIXWIN_IS_FAST_MONO_AVAILABLE 	= PIXWIN_ATTR(ATTR_BOOLEAN, 2),

    /* integer attributes */
    PIXWIN_DEPTH_RETAIN			= PIXWIN_ATTR(ATTR_INT, 20),
    PIXWIN_DEPTH_AVAILABLE 		= PIXWIN_ATTR(ATTR_INT, 21),
    PIXWIN_DEPTH_UNDER			= PIXWIN_ATTR(ATTR_INT, 22),

} Pixwin_attribute;

extern caddr_t	pixwin_get();
extern int	pixwin_set();

/* pixwin_get returns the current value of which_attr. */
caddr_t
pixwin_get(pixwin, which_attr)
	register struct pixwin		*pixwin;
	Pixwin_attribute		which_attr;
{

    switch (which_attr) {
	case PIXWIN_IS_COLOR_AVAILABLE:
	    return (caddr_t) ((int) pixwin_get(pixwin,
		PIXWIN_DEPTH_AVAILABLE) > 1);

	case PIXWIN_DEPTH_RETAIN: {
	    struct	colormapseg cms;

	    (void)pw_getcmsdata(pixwin, &cms, NO_PLANE_PTR);
	    return (caddr_t) ((cms.cms_size == 2)? 1:
		pixwin->pw_pixrect->pr_depth);
	    }

	case PIXWIN_DEPTH_AVAILABLE: {
	    int depth;
	    char *plane_groups_available =
		pixwin->pw_clipdata->pwcd_plane_groups_available;

	    if (plane_groups_available[PIXPG_8BIT_COLOR])
		depth = 8;
	    else
		depth = 1;
	    /* NOTE: ADD HERE WHEN INVENTING NEW PLANE GROUP */
	    return (caddr_t) depth;
	    }

	case PIXWIN_DEPTH_UNDER:
	    return (caddr_t) pixwin->pw_pixrect->pr_depth;

      default:
	 return (caddr_t) 0;
   }

} /* pixwin_get */

int
pixwin_set(pixwin, arg1)
struct pixwin	*pixwin;
caddr_t 	arg1;
{
    caddr_t 		avlist[ATTR_STANDARD_SIZE];
    int	pixwin_set_attr();

    attr_make(avlist, ATTR_STANDARD_SIZE, &arg1);
 
    return pixwin_set_attr(pixwin, avlist);
}

/* pixwin_set_attr sets the attributes mentioned in avlist. */
static int
pixwin_set_attr(pixwin, avlist)
	register struct pixwin	*pixwin;
	register Attr_avlist	avlist;
{
	register Pixwin_attribute	which_attr;

    while (which_attr = (Pixwin_attribute) *avlist++) {
	switch (which_attr) {
	    case PIXWIN_IS_COLOR_AVAILABLE:
	    case PIXWIN_DEPTH_RETAIN:
	    case PIXWIN_DEPTH_AVAILABLE:
	    case PIXWIN_DEPTH_UNDER:
	  default:
	     /* Here we should complain about something */
	     return 1;
       }
    }
    return 0;
} /* pixwin_set */

#endif	using_pixwin_attr_interface
