#ifndef lint
static char sccsid[] = "@(#)clear_colormap.c 1.1 86/09/25 SMI"; /* from UCB 4.1 10/01/80 */
#endif

/*
 * clear_colormap - clear the colormap
 * load me with -lpixrect
 */

#include <stdio.h>
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>

main() 
{
	struct pixrect *screen;
	unsigned char rmap[256], gmap[256], bmap[256];
	int planes = 255;

    	if ((screen = pr_open("/dev/fb")) == NULL)
		exit(1);
    	if (screen != 0) {
        	pr_putattributes(screen, &planes);
        	rmap[0] = gmap[0] = bmap[0] = 255;
        	rmap[1] = gmap[1] = bmap[1] = 0;
        	pr_putcolormap(screen, 0, 2, rmap, gmap, bmap);
        	pr_rop(screen, 0, 0, screen->pr_width, screen->pr_height,
		    PIX_CLR, 0, 0, 0);
        }
}
