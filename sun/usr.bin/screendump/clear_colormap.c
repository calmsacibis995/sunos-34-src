#ifndef lint
static char sccsid[] = "@(#)clear_colormap.c 1.4 87/01/08 SMI";
#endif

/*
 * clear_colormap - clear the colormap
 */

#include "screendump.h"
#include <pixrect/pr_planegroups.h>

#ifndef	MERGE
#define	clear_colormap_main	main
#endif

clear_colormap_main(argc, argv)
	int	argc;
	char	*argv[];
{
	int c;
	Pixrect *pr = NULL;
	static u_char rgb[2] = { 255, 0 };
	char groups[PIXPG_OVERLAY + 1];

	char *dev = "/dev/fb";
	int clear = 1, overlay = 1;

	Progname = basename(argv[0]);

	opterr = 0;

	while ((c = getopt(argc, argv, "d:f:no")) != EOF)
		switch (c) {
		case 'd':
		case 'f':
			dev = optarg;
			break;
		case 'n':
			clear = 0;
			break;
		case 'o':
			overlay = 0;
			break;
		case '?':
			error((char *) 0, "Usage: %s [-f display] [-no]",
				Progname);
		}

	if (!(pr = pr_open(dev))) 
		error("%s %s", PR_IO_ERR_DISPLAY, dev);

	/* reset colormap colors 0 and 1 */
	(void) pr_putcolormap(pr, 0, 2, rgb, rgb, rgb);

	/* clear frame buffer */
	if (clear) 
		(void) pr_rop(pr, 0, 0, pr->pr_size.x, pr->pr_size.y,
			PIX_CLR, (Pixrect *) 0, 0, 0);

	(void) pr_available_plane_groups(pr, sizeof groups, groups);

	if (groups[PIXPG_OVERLAY] && groups[PIXPG_OVERLAY_ENABLE]) {
		/* reset overlay plane colormap */
		pr_set_plane_group(pr, PIXPG_OVERLAY);
		(void) pr_putcolormap(pr, 0, 2, rgb, rgb, rgb);

		/* clear overlay plane */
		if (clear) 
			(void) pr_rop(pr, 0, 0, 
				pr->pr_size.x, pr->pr_size.y,
				PIX_CLR, (Pixrect *) 0, 0, 0);

		/* set overlay enable plane */
		if (overlay) {
			pr_set_plane_group(pr, PIXPG_OVERLAY_ENABLE);
			(void) pr_rop(pr, 0, 0, pr->pr_size.x, pr->pr_size.y,
					PIX_SET, (Pixrect *) 0, 0, 0);
		}
	}

	exit(0);
}
