#ifndef lint
static	char sccsid[] = "@(#)convert.0.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Standard rasterfile encode/decode filter for ras_type == RT_OLD.
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <pixrect/pixrect_hs.h>

#define FALSE		0
#define TRUE		!FALSE

#define	SERROR(err_message)						\
	{fprintf(stderr, "%s: %s\n", Progname, err_message); exit(1);}

#define ERR_FUSEAGE	"No command line switches or options are supported"
#define ERR_RASTYPE	"Input raster has incorrect ras_type"
#define ERR_MAPTYPE	"Input raster has incorrect ras_maptype"
#define MY_RAS_TYPE	RT_OLD

extern int		pr_load_colormap(), pr_load_header();
extern struct pixrect	*pr_dump_init(), *pr_load_std_image();

char	*Progname;

main(argc, argv)
	int	argc;
	char	**argv;
{
	extern char		*malloc();
	FILE			*infile = stdin,
				*outfile = stdout;
	struct pixrect		*input_pr, *output_pr;
	struct rasterfile	rh;
	colormap_t		colormap;

	/*
	 * Process argc/argv.
	 */
	Progname = argv[0];
	if (argc != 1)
		SERROR(ERR_FUSEAGE);
	/*
	 * Load the rasterfile header and the colormap (if there is one).
	 * Also perform a few sanity checks on the rasterfile.
	 */
	if (pr_load_header(infile, &rh) == PIX_ERR)
		SERROR(PR_IO_ERR_RASREAD);
	switch (colormap.type = rh.ras_maptype) {
	case RMT_NONE:
		break;
	case RMT_EQUAL_RGB:
		colormap.length = rh.ras_maplength / 3;
		colormap.map[0] = (unsigned char *)malloc(colormap.length);
		colormap.map[1] = (unsigned char *)malloc(colormap.length);
		colormap.map[2] = (unsigned char *)malloc(colormap.length);
		if (pr_load_colormap(infile, &rh, &colormap) == PIX_ERR)
			SERROR(PR_IO_ERR_RASREAD);
		break;
	default:
		SERROR(ERR_MAPTYPE);
	}
	/*
	 * If the rasterfile is a standard type, use pr_load_std_image to
	 *   read the image.
	 * If the rasterfile is our (formerly standard) type, also use
	 *   pr_load_std_image.
	 * In either case, transform the input to the other type and output it.
	 */
	switch (rh.ras_type) {
	case RT_STANDARD:
		if ((input_pr = pr_load_std_image(infile, &rh)) == NULL)
			SERROR(PR_IO_ERR_RASREAD);
		output_pr = input_pr;
		rh.ras_length = 0;	/* Don't confuse old software */
		if (pr_dump(output_pr, outfile, &colormap, MY_RAS_TYPE, FALSE)
		    == PIX_ERR)
			SERROR(PR_IO_ERR_RASWRITE);
		break;
	case MY_RAS_TYPE:
		if ((input_pr = pr_load_std_image(infile, &rh)) == NULL)
			SERROR(PR_IO_ERR_RASREAD);
		output_pr = input_pr;
		if (pr_dump(output_pr, outfile, &colormap, RT_STANDARD, FALSE)
		    == PIX_ERR)
			SERROR(PR_IO_ERR_RASWRITE);
		break;
	default:
		SERROR(ERR_RASTYPE);
	}
	exit(0);
}
