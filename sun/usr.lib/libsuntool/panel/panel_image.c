#ifndef lint
static	char sccsid[] = "@(#)panel_image.c 1.4 87/01/07 Copyr 1984 Sun Micro";
#endif

/*               Copyright (c) 1986 by Sun Microsystems, Inc.                */

#include "panel_impl.h"

static void outline_button();


/*****************************************************************************/
/* panel_button_image                                                        */
/* panel_button_image creates a button pixrect from the characters in        */
/* 'string'.  'width' is the desired width of the button (in characters).    */
/* If 'font' is NULL, the font in 'panel' is used.                           */
/*****************************************************************************/

Pixrect *
panel_button_image(client_object, string, width, font)
Panel		  client_object;
char		  *string;
int		   width;
Pixfont	  	  *font;
{
   panel_handle    object = PANEL_CAST(client_object);
   struct pr_prpos where;	/* where to write the string */
   struct pr_size  size;	/* size of the pixrect */
   panel_handle    panel;

   /* make sure we were really passed a panel, not an item */
   if (is_panel(object)) 
      panel = object;
   else if (is_item(object)) 
      panel = ((panel_item_handle) object)->panel;
   else 
      return NULL;

   if (!font)
      font = panel->font;
      
   size = pf_textwidth(strlen(string), font, string);

   width = panel_col_to_x(font, width);

   if (width < size.x)
      width = size.x;

   where.pr = mem_create(width + 12, size.y + 4, 1);
   if (!where.pr)
      return (NULL);

   where.pos.x = 6 + (width - size.x) / 2;
   where.pos.y = 2 + panel_fonthome(font);

   (void)pf_text(where, PIX_SRC, font, string);
   
   outline_button(where.pr);

   return (where.pr);
}


static void
outline_button(pr)
register Pixrect *pr;
/* outline_button draws an outline of a button in pr.
*/
{
   int	x_left		= 0;
   int	x_right		= pr->pr_width - 1;
   int	y_top		= 0;
   int	y_bottom	= pr->pr_height - 1;
   int	x1		= 3;
   int	x2		= x_right - 3;
   int	y1		= 3;
   int	y2		= y_bottom - 3;

   /* horizontal lines */
   (void)pr_vector(pr, x1, y_top, x2, y_top, PIX_SRC, 1);
   (void)pr_vector(pr, x1, y_top + 1, x2, y_top + 1, PIX_SRC, 1);

   (void)pr_vector(pr, x1, y_bottom, x2, y_bottom, PIX_SRC, 1);
   (void)pr_vector(pr, x1, y_bottom - 1, x2, y_bottom - 1, PIX_SRC, 1);

   /* vertical lines */
   (void)pr_vector(pr, x_left, y1, x_left, y2, PIX_SRC, 1);
   (void)pr_vector(pr, x_left + 1, y1, x_left + 1, y2, PIX_SRC, 1);

   (void)pr_vector(pr, x_right, y1, x_right, y2, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 1, y1, x_right - 1, y2, PIX_SRC, 1);

   /* left corners */
   (void)pr_vector(pr, x_left, y1, x1, y_top, PIX_SRC, 1);
   (void)pr_vector(pr, x_left + 1, y1, x1 + 1, y_top, PIX_SRC, 1);
   (void)pr_vector(pr, x_left + 1, y1 + 1, x1 + 2, y_top, PIX_SRC, 1);

   (void)pr_vector(pr, x_left, y2, x1, y_bottom, PIX_SRC, 1);
   (void)pr_vector(pr, x_left + 1, y2, x1 + 1, y_bottom, PIX_SRC, 1);
   (void)pr_vector(pr, x_left + 1, y2 - 1, x1 + 2, y_bottom, PIX_SRC, 1);

   /* right corners */
   (void)pr_vector(pr, x_right, y1, x2, y_top, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 1, y1, x2 - 1, y_top, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 1, y1 + 1, x2 - 2, y_top, PIX_SRC, 1);

   (void)pr_vector(pr, x_right, y2, x2, y_bottom, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 1, y2, x2 - 1, y_bottom, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 1, y2 - 1, x2 - 2, y_bottom, PIX_SRC, 1);
}
