#ifndef lint
static	char sccsid[] = "@(#)screenload.c 1.5 87/01/08 SMI";
#endif

/*
 * Copyright 1984, 1986 by Sun Microsystems, Inc.
 */

/*
 * Load the frame buffer with image from rasterfile
 *
 * LIMITATIONS:
 *		This program has not been tested with color depths
 *		other than 8 bits or color maps with other than
 *		256 entries, because no Sun hardware or software
 *		supports that capability.
 */

#include "screendump.h"
#include <sys/file.h>

#define WARN_MESSAGE_PAUSE	4

static short rootimage[] = {
	0x8888, 0x8888,
	0x8888, 0x8888,
	0x2222, 0x2222,
	0x2222, 0x2222
};

mpr_static(_screenload_root_pr, 32, 4, 1, rootimage);

static Pixrect *find_color_display();

#ifndef	MERGE
#define	screenload_main	main
#endif

screenload_main(argc, argv)
	int argc;
	char *argv[];
{
	extern long strtol();

	int c, errflg = 0;

	int fill = 1, fill_color = ~0;
	Pixrect *fill_pr = (Pixrect *) 0;
	char *display_device = (char *) 0;
	int pause_flag = 0;
	int warn_flag = 0;

	FILE *input_file = stdin;
	colormap_t colormap;
	Pixrect *input_pr, *disp_pr;
	int dx, dy;

	/*
	 * Process command-line arguments.
	 */
	Progname = basename(argv[0]);

	opterr = 0;

	while ((c = getopt(argc, argv, "bdf:gh:i:npw")) != EOF)
		switch (c) {

		case 'b':
			fill_pr = (Pixrect *) 0;
			fill_color = ~0;
			break;

		case 'd':
			warn_flag = 1;
			break;

		case 'f':
			display_device = optarg;
			break;

		case 'g':
			fill_pr = &_screenload_root_pr;
			break;

		case 'h': {
			int h;
			short *image;

			if ((h = (int) strtol(optarg, (char **) 0, 16)) <= 0) {
				errflg++;
				break;
			}

			if (!(image = (short *) 
				malloc((unsigned) h * sizeof (short))) ||
				!(fill_pr = mem_point(16, h, 1, image)))
				error("out of memory");
			
			while (h-- && optind < argc) 
				*image++ = (short) strtol(argv[optind++],
					(char **) 0, 16);

			if (h > 0)
				errflg++;

			}
			break;

		case 'i':
			fill_color = atoi(optarg);
			break;

		case 'n':
			fill = 0;
			break;

		case 'p':
			pause_flag = 1;
			break;

		case 'w':
			fill_pr = (Pixrect *) 0;
			fill_color = 0;
			break;

		case '?':
			errflg++;
			break;
		}

	if (errflg) 
		error((char *) 0, 
"Usage:\n\
%s [-dp] [-f display] [-i color] [-bgw] [-h count data ...] [file]",
			Progname);

	if (optind < argc &&
		(input_file = fopen(argv[optind], "r")) == NULL) 
		error("%s %s", PR_IO_ERR_INFILE, argv[optind]);

	/* load the image from the file */
	colormap.type = RMT_NONE;
	if ((input_pr = pr_load(input_file, &colormap)) == NULL)
		error(PR_IO_ERR_RASREAD);

	(void) fclose(input_file);

	/* open display device */
	if (!display_device) {
		disp_pr = pr_open(display_device = "/dev/fb");

		if (disp_pr && 
			disp_pr->pr_depth < input_pr->pr_depth) {
			(void) pr_close(disp_pr);
			if (!(disp_pr = find_color_display()))
				error(PR_IO_ERR_NOCDISPLAY);
		}
	}
	else
		disp_pr = pr_open(display_device);

	if (!disp_pr) 
		error("%s %s", PR_IO_ERR_DISPLAY, display_device);

	/* It would be nice to dither here. */
	if (disp_pr->pr_depth < input_pr->pr_depth)
		error(PR_IO_ERR_CDEPTH);

	/* 
	 * If the input file has a colormap, try to write it to the
	 * display device.  We could probably do something more
	 * intelligent for a monochrome file displayed on a color device.
	 */
	if (colormap.type == RMT_EQUAL_RGB &&
		pr_putcolormap(disp_pr, 0, colormap.length, 
			colormap.map[0], colormap.map[1], colormap.map[2]))
		error(PR_IO_ERR_PIXRECT);

	/*
	 * Transfer the memory pixrect data to the display pixrect.
	 * Clip or fill the background as necessary for mismatched
	 * image/display dimensions.
	 */
	dx = disp_pr->pr_size.x - input_pr->pr_size.x;
	dy = disp_pr->pr_size.y - input_pr->pr_size.y;

	if (warn_flag && (dx != 0 || dy != 0)) {
		(void) fprintf(stderr, 
		"%s: warning - displaying %d x %d image on %d x %d screen\n",
			Progname,
			input_pr->pr_size.x, input_pr->pr_size.y, 
			disp_pr->pr_size.x, disp_pr->pr_size.y);
			sleep(WARN_MESSAGE_PAUSE);
	}

	dx = dx < 0 ? 0 : dx / 2;
	dy = dy < 0 ? 0 : dy / 2;

	if (fill && (dx > 0 || dy > 0))
		(void) pr_replrop(disp_pr, 0, 0, 
			disp_pr->pr_size.x, disp_pr->pr_size.y, 
			PIX_SRC | PIX_COLOR(fill_color) | PIX_DONTCLIP,
			fill_pr, 0, 0);
			
	if (pr_rop(disp_pr, dx, dy, 
		input_pr->pr_size.x, input_pr->pr_size.y,
		PIX_SRC | PIX_COLOR(fill_color), 
		input_pr, 0, 0))
		error(PR_IO_ERR_PIXRECT);

	/*
	 * If pause option was given, wait for a newline to be typed on 
	 * standard input before exiting.
	 */
	if (pause_flag)
		while ((c = getchar()) != EOF && c != '\n')
			/* nothing */ ;

	exit(0);
}


static Pixrect *
find_color_display()
{
	static char *color_list[] = {
		"/dev/cgfive0",
		"/dev/cgthree0",
		"/dev/gpone0a",
		"/dev/cgtwo0",
		"/dev/cgfour0",
		"/dev/cgone0",
		0
	};

	register char **p;
	register Pixrect *pr = 0;
	int fd;

	for (p = color_list; *p; p++) 
		/* 
		 * Try to open the file first to avoid the dumb messages
		 * pr_open prints.
		 */
		if ((fd = open(*p, O_RDWR)) >= 0) {
			(void) close(fd);
			if (pr = pr_open(*p))
				return pr;
		}

	return 0;
}
