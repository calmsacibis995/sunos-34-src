#ifndef lint
static  char sccsid[] = "@(#)cmapshared.c 1.1 86/09/25 SMI";
#endif

/*
 * Copyright 1985, Sun Microsystems, Inc.
 */

#include <sys/file.h>
#include <errno.h>
#include        <sunwindow/window_hs.h>
#include        <sunwindow/cms.h>


/*
 * _core_sharedcmap accesses all windows on a specified device and if they
 * have a colormap of specified name, returns the colormap.  An error(0) return
 * results if no windows have the named colormap or there are no windows.
 */

static	int windownotactive;
static	int (*wmgr_errorcached)();

_core_sharedcmap( wfd, name, dcms, dcmap)
	int wfd;
	char *name;
	struct  colormapseg *dcms;
	struct  cms_map *dcmap;
{
	char	winname[WIN_NAMESIZE];
	int	testwfd, winnumber, satisfied = 0;
	struct	screen screen, newscreen;
	struct  colormapseg scms;
	struct  cms_map cmap;
	extern	int errno;

	/*
	 * Get the base screen
	 */
	win_screenget( wfd, &screen);
	cmap.cm_red = cmap.cm_green = cmap.cm_blue = 0;
	/*
	 * Find window numbers associated with wfd's device.
	 */
	for (winnumber = 0;1;winnumber++) {
		int	screenget_error();
		extern  int win_errorhandler();
		/*
		 * Open window.
		 */
		win_numbertoname(winnumber, winname);
		if ((testwfd = open(winname, O_RDONLY, 0)) < 0) {
			if (errno == ENXIO || errno == ENOENT)
				break;	/* Last window passed. */
			goto badopen;
		}
		/*
		 * Get window's device name (if active).
		 */
		wmgr_errorcached = (int (*)())win_errorhandler(screenget_error);
		windownotactive = 0;
		win_screenget(testwfd, &newscreen);
		if (windownotactive) {
			close(testwfd);
			continue;
		}
		(void) win_errorhandler(wmgr_errorcached);
		/*
		 * Is this window on the same screen
		 */
		if (strncmp(screen.scr_fbname, newscreen.scr_fbname,
			SCR_NAMESIZE) == 0) {
			/*
			 * Does it have the named colormap
			 */
			win_getcms( testwfd, &scms, &cmap);
			if (strncmp(name, scms.cms_name, CMS_NAMESIZE) == 0) {
				win_getcms( testwfd, &scms, dcmap);
				if (dcms) *dcms = scms;
				satisfied = 1;
				close(testwfd);
				break;
			}
		}
		close(testwfd);
	}
	return( satisfied);
	badopen: return(0);
}
screenget_error(errnum, winopnum)
        int     errnum, winopnum;
{
        switch (errnum) {
        case 0:
                return;
        case -1:
		if (errno == ESPIPE) {
			windownotactive = 1;
			return;
		}
	}
	wmgr_errorcached(errnum, winopnum);
	return;
}

#include <sunwindow/cms_rgb.h>
/*----------------------------------------------------------------------*/
_core_standardcmap( name, cms, cmap)
	char *name;
	struct  colormapseg *cms;
	struct  cms_map *cmap;
{
	if (strncmp(name, "rgb", CMS_NAMESIZE) == 0) {
		if ( cms)
			cms->cms_size = CMS_RGBSIZE;
		if ( cmap) {
			cms_rgbsetup(cmap->cm_red,cmap->cm_green,cmap->cm_blue);
		}
		return( 1);
	}
	return( 0);
}

