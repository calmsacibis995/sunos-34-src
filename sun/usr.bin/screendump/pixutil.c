#ifndef lint
static	char sccsid[] = "@(#)pixutil.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Extended pixrect operations	-- dump pixrect to file
 *				-- load pixrect from file
 * 
 * Limitations:	The size of the color map is a compile-time parameter
 *		that cannot be determined from a display pixrect.  The
 *		file descriptor for a device pixrect is hidden, so the
 *		FBIOGYPE ioctl cannot be used.
 *		(sevans says you CAN determine colormap size from pixrect
 *		depth; we'll leave that for later.)
 *		
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <pixrect/pixrect_hs.h>
#include <sunwindow/rect.h>
#include <rasterfile.h>

#define BITS_BYTE	8
#define BITS_PADDING	16

#define CMAP_SIZE	256
#define CMAP_ENABLEALL	0xff

#define PIXERR		-1

struct	rect default_rect = {0, 0, 0, 0};

/*
 * Find the default color frame buffer in a system with
 * both black&white and color displays.
 * FOR NOW JUST RETURN /dev/cgone0.
 */
char *
find_defaultcolor()
{

	return ("/dev/cgone0");
}

/*
 * Dump a pixrect to the specified stream.
 */
pixdump(input_pr, input_rect, outfile, colormap)
	struct pixrect *input_pr;
	struct rect *input_rect;
	FILE *outfile;
	unsigned char **colormap;
{
	struct pixrect *copy_pr;
	struct mpr_data *copy_mprd;
	struct rasterfile rh;
	int bit_width, n, return_code, output_byte_count;
	unsigned char red[CMAP_SIZE], green[CMAP_SIZE], blue[CMAP_SIZE];
	unsigned char *colormap_array[3];

	/*
	 * The input pixrect may be a display or a memory pixrect.
	 * It may not be memory aligned, and the region selected
	 * may not be byte-sized multiples, so make a copy of the
	 * desired region into an aligned pixrect before writing
	 * the output file.
	 *
	 * If the desired rectangle is designated a NULL, then
	 * copy the entire input pixrect.
	 *
	 * If the colormap pointer is NULL, then get the color map
	 * from the input pixrect (it must be a display pixrect on
	 * a color display device.)  Otherwise, *colormap is an
	 * array of three pointers to the colormaps to dump.
	 */

	if (input_rect == NULL) {
		input_rect = &default_rect;
		input_rect->r_width = (int)input_pr->pr_width;
		input_rect->r_height = (int)input_pr->pr_height;
	}
	copy_pr = mem_create((int)input_rect->r_width,
	    (int)input_rect->r_height, input_pr->pr_depth);
	if (copy_pr == NULL)
		return (PIXERR);

	/*
	 * Lots of stuff below assumes that the byte array which implements
	 * the memory pixrect is contiguous in memory except for a possible
	 * one byte of padding at the end of each horizontal strip of pixels,
	 * so verify this assumption.
	 */
	copy_mprd = ((struct mpr_data *) copy_pr->pr_data);
	bit_width = round_up(((copy_pr->pr_width) * (copy_pr->pr_depth)),
			     BITS_PADDING);
	if ((copy_mprd->md_linebytes) != (bit_width / BITS_BYTE))
		return (PIXERR);

	if (pr_rop(copy_pr, 0, 0, (int)input_rect->r_width,
	    (int)input_rect->r_height, PIX_SRC, input_pr,
	    (int)input_rect->r_left, (int)input_rect->r_top) == PIXERR)
		return (PIXERR);

	/*
	 * Initialize rasterfile header .
	 */
	rh.ras_depth = (copy_pr->pr_depth);
	rh.ras_width = (copy_pr->pr_width);
	rh.ras_height = (copy_pr->pr_height);
	rh.ras_magic = RAS_MAGIC;
	rh.ras_encoding = 0;
	rh.ras_type = 0;
	rh.ras_maptype = 0;
	rh.ras_maplength = 0;

	/*
	 * Read color map if a color device pixrect and no colormap provided. 
	 */
	if (((copy_pr->pr_depth) != 1) && (colormap == NULL)) {
		if (pr_getcolormap(input_pr, 0, CMAP_SIZE, red, green, blue)
		    == PIXERR)
			return (PIXERR);
		colormap = colormap_array;
		colormap_array [0] = red;
		colormap_array [1] = green;
		colormap_array [2] = blue;
		rh.ras_maptype = 1;
		rh.ras_maplength = CMAP_SIZE * 3;
	}

	/*
	 * Mask out pixel elements that are not "visible"
	 * according to the bit plane write enable mask
	 * for the pixrect, if it is a color pixrect.  This
	 * keeps invisible planes out of the rasterfile.
	 */
	if ((copy_pr->pr_depth) != 1) {
		int	cplanes;

		if (pr_getattributes(copy_pr, &cplanes) == PIXERR)
			return (PIXERR);
		if (cplanes != CMAP_ENABLEALL) {

			/* Not implemented yet! */

		}
	}


	/*
	 * Output rasterfile header.
	 */
	if (fwrite((char *)&rh, 1, sizeof(rh), outfile) != sizeof(rh))
		return (PIXERR);

	/*
	 * Output color map if a color pixrect.
	 */
	if ((copy_pr->pr_depth) != 1)
		if (fwrite(colormap[0], 1, CMAP_SIZE, outfile) != CMAP_SIZE ||
		    fwrite(colormap[1], 1, CMAP_SIZE, outfile) != CMAP_SIZE ||
		    fwrite(colormap[2], 1, CMAP_SIZE, outfile) != CMAP_SIZE)
			return (PIXERR);

	/*
	 * Output pixel data values.
	 */
	output_byte_count = (bit_width * rh.ras_height) / BITS_BYTE;
	if (fwrite(((char *)(copy_mprd->md_image)), 1, output_byte_count,
	    outfile) != output_byte_count)
		return (PIXERR);
	if (pr_destroy(copy_pr) == PIXERR)
		return (PIXERR);
	return (0);
}

/*
 * Load a pixrect from the specified stream.
 */
struct pixrect *
pixload(input_file, colormap)
	FILE *input_file;
	unsigned char **colormap;
{
	struct pixrect *mem_pr;
	struct rasterfile rh;
	int n, input_byte_count, pixrect_buffer_size;
	unsigned char mapvector[CMAP_SIZE];
	unsigned char *colormap_array[3];
	char *input_buffer, *output_buffer;

	/*
	 * Read in the rasterfile header.
	 */
	if (fread((char *)&rh, 1, sizeof(rh), input_file) != sizeof(rh))
		return (NULL);
	if (rh.ras_magic != RAS_MAGIC)
		return (NULL);
	if (rh.ras_depth == 1) {
		/*
		 * Input data contains a monochrome raster.
		 */
		if (rh.ras_maplength != 0)
			return (NULL);
	} else {
		/*
		 * Input data contains a color raster.
		 * Read in the color map data.  Use the
		 * input colormap vectors if provided.
		 */
		if (rh.ras_maptype != 1)
			return (NULL);
		if (colormap == NULL) {
			colormap = colormap_array;
			colormap_array[0] = mapvector;
			colormap_array[1] = mapvector;
			colormap_array[2] = mapvector;
		}
		n = rh.ras_maplength/3;
		if (fread(colormap[0], 1, n, input_file) != n ||
		    fread(colormap[1], 1, n, input_file) != n ||
		    fread(colormap[2], 1, n, input_file) != n)
			return (NULL);
	}

	/*
	 * Create a memory pixrect and load the actual image data.
	 */
	mem_pr =  mem_create(rh.ras_width, rh.ras_height, rh.ras_depth);
	if (mem_pr == NULL)
		return (NULL);
	input_buffer = output_buffer =
		((char *) ((struct mpr_data *)(mem_pr->pr_data))->md_image);
	pixrect_buffer_size =
		(round_up((rh.ras_width * rh.ras_depth), BITS_PADDING) *
						rh.ras_height) / BITS_BYTE;
	if (rh.ras_encoding == 0)
		input_byte_count = pixrect_buffer_size;
	else if (rh.ras_encoding == 1) {
		if (fread(input_buffer, 1, sizeof (int), input_file) !=
		    sizeof (int)) {
			pr_destroy(mem_pr);
			return (NULL);
		}
		input_byte_count = makeint((unsigned char *) input_buffer);
		input_buffer = input_buffer +
				(pixrect_buffer_size - 1) - input_byte_count;
		for (n = sizeof (int) - 1; n >= 0; n--)
			*(input_buffer + n) = *(output_buffer + n);
		input_buffer += sizeof(int);
		input_byte_count -= sizeof(int);
	} else {
		pr_destroy(mem_pr);
		return (NULL);
	}
	if (fread(input_buffer, 1, input_byte_count, input_file) !=
	    input_byte_count) {
		pr_destroy(mem_pr);
		return (NULL);
	}
	return (mem_pr);
}

int
round_up(number, multiple)
	int	number, multiple;
{

	return (((number + multiple - 1) / multiple) * multiple);
}

int
makeint(chars)
	unsigned char	*chars;
{
	int	i;

	i  = (((int) *chars)   << 24);
	i += (((int) *++chars) << 16);
	i += (((int) *++chars) << 8 );
	i +=  ((int) *++chars);
	return (i);
}
