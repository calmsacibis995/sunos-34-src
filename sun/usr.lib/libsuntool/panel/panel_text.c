#ifndef lint
static  char sccsid[] = "@(#)panel_text.c 1.4 87/01/07 Copyr 1984 Sun Micro";
#endif

/***********************************************************************/
/*                            panel_text.c                             */
/*              Copyright (c) 1985 by Sun Microsystems, Inc.           */
/***********************************************************************/

#include "panel_impl.h"
#include <suntool/menu.h>
#include <sunwindow/defaults.h>
#include <sundev/kbd.h>

static caddr_t panel_getcaret(), panel_setcaret();

char *malloc();

/***********************************************************************/
/* field-overflow pointer                                    		*/
/***********************************************************************/

static short left_arrow_image[] = {
#include <images/panel_left_arrow.pr>
};
static mpr_static(panel_left_arrow_pr, 8, 8, 1, left_arrow_image);


#define textdp(ip) ((text_data *)LINT_CAST((ip)->data))

static	begin_preview(), cancel_preview(), accept_preview(),
	accept_menu(),
	accept_key(),
	paint(),
	destroy(),
	set_attr();
static caddr_t get_attr();
static Panel_item remove();
static Panel_item restore();

static void	text_caret_invert();
static void	text_caret_on();
static void	paint_value();

extern void 	(*panel_caret_on_proc)(), 
		(*panel_caret_invert_proc)();


static struct panel_ops ops  =  {
   panel_default_handle_event,		/* handle_event() */
   begin_preview,			/* begin_preview() */
   panel_nullproc,			/* update_preview() */
   cancel_preview,			/* cancel_preview() */
   accept_preview,			/* accept_preview() */
   accept_menu,				/* accept_menu() */
   accept_key,				/* accept_key() */
   paint,				/* paint() */
   destroy,				/* destroy() */
   get_attr,				/* get_attr() */
   set_attr,				/* set_attr() */
   remove,				/* remove() */
   restore,				/* restore() */
   panel_nullproc			/* layout() */
};

/***********************************************************************/
/*			data area		                       */
/***********************************************************************/

typedef struct text_data {		
   Panel_setting       notify_level;   /* NONE, SPECIFIED, NON_PRINTABLE, ALL*/
   panel_item_handle   orig_caret;     /* original item with the caret */
   int                 underline;      /* TRUE, FALSE (not implemented) */
   int                 flags;
   char               *value;
   Pixfont            *font;
   char                mask;
   int                 caret_offset;   /* x offset from value rect */
   int                 first_char;     /* first displayed character */
   char               *terminators;
   int                 stored_length;
   int                 display_length;
   char                edit_bk_char;
   char                edit_bk_word;
   char                edit_bk_line;
   panel_item_handle   next;
}  text_data;


#define SELECTING_ITEM		0x00000001
#define HASCARET		0x00000002


Panel_item
panel_text(ip, avlist)
register panel_item_handle	ip;
Attr_avlist			avlist;
{
   register text_data     *dp;
   char            	  *def_str; 

   /* set the caret functions for panel_public.c and panel_select.c */
   panel_caret_on_proc = text_caret_on;
   panel_caret_invert_proc = text_caret_invert;

   if (!ip->panel->seln_client)
      panel_seln_init(ip->panel);
   
   if (!(dp = (text_data *) LINT_CAST(calloc(1, sizeof(text_data)))))
      return(NULL);
   
   ip->ops               = &ops;
   ip->data              = (caddr_t) dp;
   ip->item_type         = PANEL_TEXT_ITEM;
   if (ip->notify == panel_nullproc)
      ip->notify = (int (*)()) panel_text_notify; 

   dp->notify_level      = PANEL_SPECIFIED;
   dp->font              = ip->panel->font;
   dp->mask              = '\0';
   dp->terminators       = panel_strsave("\n\r\t");
   dp->stored_length     = 80;
   dp->display_length    = 80;
   
   if (ip->panel->edit_bk_char == NULL) {
       def_str = defaults_get_string("/Text/Edit_back_char", "\177", (int *)NULL);
       ip->panel->edit_bk_char = def_str[0];
       def_str = defaults_get_string("/Text/Edit_back_word", "\027", (int *)NULL);
       ip->panel->edit_bk_word = def_str[0];
       def_str = defaults_get_string("/Text/Edit_back_line", "\025", (int *)NULL);
       ip->panel->edit_bk_line = def_str[0];
   }
   dp->edit_bk_char = ip->panel->edit_bk_char;
   dp->edit_bk_word = ip->panel->edit_bk_word;
   dp->edit_bk_line = ip->panel->edit_bk_line;
   
   dp->value = (char *) calloc(1, (u_int) (dp->stored_length + 1));
   if (!dp->value)
      return(NULL);

   /* All other dp-> fields are initialized
    * to zero by calloc().
    */

   ip->value_rect.r_width = panel_col_to_x(dp->font, dp->display_length);
   ip->value_rect.r_height = dp->font->pf_defaultsize.y;

   /* set the user specified attributes */
   if (!set_attr(ip, avlist))
      return NULL;;

   /* append to the panel list of items */
   (void) panel_append(ip);

   /* only insert in caret list if not hidden already */
   if (!hidden(ip))
      insert(ip);

   return (Panel_item) ip;
}


static int
set_attr(ip, avlist)
register panel_item_handle	ip;
register Attr_avlist		avlist;
{
   register text_data		*dp = textdp(ip);
   register Panel_attribute	attr;  
   char				*new_value = NULL;
   short			value_changed = FALSE;
   extern char			*realloc();
   extern char			*strncpy();

   while(attr = (Panel_attribute)*avlist++) {
      switch (attr) {
	 case PANEL_VALUE:
	    new_value = (char *)*avlist++;
	    break;

	 case PANEL_VALUE_FONT:
	    dp->font = (Pixfont *) LINT_CAST(*avlist++);
	    value_changed = TRUE;
	    break;

	 case PANEL_VALUE_UNDERLINED:
	    dp->underline = (int)*avlist++; 
	    break;

	 case PANEL_NOTIFY_LEVEL:
	    dp->notify_level = (Panel_setting) *avlist++;
	    break;

	 case PANEL_NOTIFY_STRING:
	    dp->terminators = panel_strsave((char *)*avlist++);
	    break;

	 case PANEL_VALUE_STORED_LENGTH:
	    dp->stored_length = (int)*avlist++;
	    dp->value = (char *) realloc(dp->value, (u_int) (dp->stored_length + 1));
	    break;

	 case PANEL_VALUE_DISPLAY_LENGTH:
	    dp->display_length = (int)*avlist++;
	    value_changed = TRUE;
	    break;

	 case PANEL_MASK_CHAR:
	    dp->mask = (char)*avlist++;
	    break;

         default:
            avlist = attr_skip(attr, avlist);
            break;

      }
   }

   if (new_value) {
      char *old_value = dp->value;

      dp->value = (char *) calloc(1, (u_int) (dp->stored_length + 1));
      (void) strncpy(dp->value, new_value, dp->stored_length);

      free(old_value);
   }

   /* update the value & items rect
    * if the width or height of the value has changed.
    */
   if (value_changed) {
      ip->value_rect.r_width = panel_col_to_x(dp->font, dp->display_length);
      ip->value_rect.r_height = dp->font->pf_defaultsize.y;
      ip->rect = panel_enclosing_rect(&ip->label_rect, &ip->value_rect);
   }

   return 1;
}


/***********************************************************************/
/* get_attr                                                            */
/* returns the current value of an attribute for the text item.        */
/***********************************************************************/

static caddr_t
get_attr(ip, which_attr)
panel_item_handle		ip;
register Panel_attribute	which_attr;
{
   register text_data 	*dp = textdp(ip);

   switch (which_attr) {
      case PANEL_VALUE:
	 return (caddr_t) dp->value;

      case PANEL_VALUE_STORED_LENGTH:
	 return (caddr_t) dp->stored_length;

      case PANEL_VALUE_DISPLAY_LENGTH:
	 return (caddr_t) dp->display_length;

      case PANEL_NOTIFY_LEVEL:
	 return (caddr_t) dp->notify_level;

      case PANEL_NOTIFY_STRING :
	 return (caddr_t) dp->terminators;

      default:
	 return panel_get_generic(ip, which_attr);
   }
} 

/***********************************************************************/
/* remove                                                              */
/***********************************************************************/

static Panel_item
remove(ip)
register panel_item_handle ip;
{
   register panel_handle	panel = ip->panel;
   register panel_item_handle 	prev_item, next_item;
   short			had_caret_seln = FALSE;

   next_item = textdp(ip)->next;

   /* if already removed then ignore remove() */
   if (!next_item)
      return NULL;

   /* cancel the selection if this item
    * owns it.
    */
   if (panel_seln(panel, SELN_PRIMARY)->ip == ip)
       panel_seln_cancel(panel, SELN_PRIMARY); 

   if (panel_seln(panel, SELN_SECONDARY)->ip == ip)
       panel_seln_cancel(panel, SELN_SECONDARY); 

   if (panel_seln(panel, SELN_CARET)->ip == ip) {
       had_caret_seln = TRUE;
       panel_seln_cancel(panel, SELN_CARET); 
   }

   /* if ip is the only text item, then remove caret for panel */
   if (next_item == ip) {
      textdp(ip)->next   = NULL;
      textdp(ip)->flags &= ~HASCARET;
      panel->caret      = NULL;
      return NULL;
   }

   /* find the next item */
   for (prev_item = next_item;
	textdp(prev_item)->next != ip;
	prev_item = textdp(prev_item)->next
       );

   /* unlink ip from the list */
   textdp(prev_item)->next = next_item;
   textdp(ip)->next        = NULL;

   /* reset the caret */
   if (panel->caret == ip)  {
      textdp(ip)->flags &= ~HASCARET;
      panel->caret = next_item;
      textdp(next_item)->flags |= HASCARET;
      if (had_caret_seln)
	 panel_seln(panel, SELN_CARET)->ip = panel->caret;
   }

   return (Panel_item) ip;
}


/*************************************************************************/
/* restore                                                               */
/* this code assumes that the caller has already set ip to be not hidden */
/*************************************************************************/

static Panel_item
restore(ip)
register panel_item_handle	ip;
{
   register panel_item_handle	prev_item, next_item;
   register text_data		*dp, *prev_dp;

   /* if not removed then ignore restore() */
   if (textdp(ip)->next)
      return NULL;

   /* find next non_hidden text item following ip in the circular list */
   /* of caret items (could be ip itself ) */
   next_item = ip;
   do {
      next_item = panel_successor(next_item);     /* find next unhidden item */
      if (!next_item) next_item = ip->panel->items;  /* wrap to start of list   */ 
   } while ( (next_item->item_type != PANEL_TEXT_ITEM) || hidden(next_item) ); 

   /* now find the previous text item of next_item in the caret list */
   prev_item = next_item;
   prev_dp   = textdp(prev_item);
   /* if next_item is different from ip, then find the previous item */
   if (next_item != ip) 
      while (prev_dp->next != next_item)  {
	 prev_item = prev_dp->next;
	 prev_dp   = textdp(prev_item);
      }
   /* prev_item now points to the (circular) 	 */
   /* predecessor of ip; if ip is sole text item,*/
   /* prev_item == next_item == ip.		 */

   /* link ip into the caret list */
   dp            = textdp(ip);
   dp->next      = next_item; 
   prev_dp->next = ip;

   /* deselect the previous caret item*/
   deselect(ip->panel);

   /* give the caret to this item */
   ip->panel->caret = ip;
   dp->flags |= HASCARET;
   /* if we have the caret selection, change the item */
   if (panel_seln(ip->panel, SELN_CARET)->ip)
      panel_seln(ip->panel, SELN_CARET)->ip = ip;
   /* note that the caret will be drawn when the item is painted */ 

   return (Panel_item) ip;
}

/***********************************************************************/
/* insert                                                              */
/***********************************************************************/

static
insert(ip)
register panel_item_handle	ip;
/* insert inserts ip into the caret list for ip->panel.
*/
{
   register panel_item_handle	 head;
   register text_data 	        *dp;

   head = ip->panel->caret;
   if (head == NULL)  {
      ip->panel->caret = ip;
      dp = textdp(ip);
      dp->flags |= HASCARET;
      head = ip;
   } else {
      /* find the last caret item */
      for (dp = textdp(head); dp->next != head; dp = textdp(dp->next));

      /* link after the last */
      dp->next = ip;
   }
   /* point back to the head of the list */
   dp = textdp(ip);
   dp->next = head;
}

/***********************************************************************/
/* destroy                                                             */
/***********************************************************************/

static
destroy(dp)
register text_data *dp;
{
  free(dp->value);
  free(dp->terminators);
  free((char *) dp);
}



/***********************************************************************/
/* paint                                                               */
/***********************************************************************/

static
paint(ip)
register panel_item_handle ip;
{
   (void)panel_paint_image(ip->panel, &ip->label, &ip->label_rect);
   paint_text(ip);
}


/***********************************************************************/
/* paint_text                                                          */
/***********************************************************************/

static
paint_text(ip)
panel_item_handle	 ip;
{
   register text_data	*dp = textdp(ip);

   /* compute the caret position */
   update_caret_offset(ip);

   /* don't paint the text if masked */
   if (dp->mask != ' ')
      paint_value(ip);
}


static void
paint_value(ip)
register panel_item_handle	ip;
/* paint_value clears the value rect for ip and paints the string value
   clipped to the left of the rect.
*/
{
   register text_data	*dp	= textdp(ip);
   register panel_handle panel	= ip->panel;
   register int		 x	= ip->value_rect.r_left;
   register int		 y	= ip->value_rect.r_top;

   /* clear the value rect */
   (void)panel_clear(panel, &ip->value_rect);

   /* draw the left clip arrow if needed */
   if (dp->first_char) {
      /* center the arrow vertically in the value rect */
      (void)panel_pw_write(panel, x, 
	 y + (ip->value_rect.r_height - panel_left_arrow_pr.pr_height) / 2, 
	 panel_left_arrow_pr.pr_width, panel_left_arrow_pr.pr_height, 
	 PIX_SRC, &panel_left_arrow_pr, 0, 0);
      x += panel_left_arrow_pr.pr_width;
   }
   /* draw the text */
   if (dp->mask == '\0') /* not masked */
      (void)panel_pw_text(panel, x, y + panel_fonthome(dp->font), PIX_SRC, dp->font, 
                    &dp->value[dp->first_char]);
   else { /* masked */
      char *buf;
      int length, i;
      length = strlen(&dp->value[dp->first_char]);
      buf = malloc((u_int) (length+1));
      for (i=0; i < length; i++)
	 buf[i] = dp->mask;
      buf[i] = '\0';
      (void)panel_pw_text(panel, x, y + panel_fonthome(dp->font), PIX_SRC, dp->font, 
                    buf);
      free(buf);
   }

   /* re-hilite if this is a selection item */
   if (ip == panel_seln(panel, SELN_PRIMARY)->ip && 
       !panel_seln(panel, SELN_PRIMARY)->is_null)
      panel_seln_hilite(panel_seln(panel, SELN_PRIMARY));
   else if (ip == panel_seln(panel, SELN_SECONDARY)->ip && 
            !panel_seln(panel, SELN_SECONDARY)->is_null)
      panel_seln_hilite(panel_seln(panel, SELN_SECONDARY));
}


/***********************************************************************/
/* paint_caret                                                         */
/***********************************************************************/

static
paint_caret(ip, op)
panel_item_handle	ip;
int		    	op;
{
   register panel_handle	 panel = ip->panel;
   register text_data 		*dp = textdp(ip);
   register int                  x, max_x;

   if (((op == PIX_SET && panel->caret_on) ||
        (op == PIX_CLR && !panel->caret_on)))
      return;

   panel->caret_on = panel->caret_on ? FALSE : TRUE;
   op = PIX_SRC ^ PIX_DST;

   /* paint the caret after the offset & above descender */
   x = ip->value_rect.r_left + dp->caret_offset -
       dp->font->pf_defaultsize.x / 2;
   max_x = view_width(panel) + panel->h_offset;
   (void)panel_pw_write(panel, 
            (x > max_x - 7) ? (max_x - 3) : x,
            ip->value_rect.r_top + dp->font->pf_defaultsize.y - 4,
            7, 7, op, panel->caret_pr, 0, 0);
}


/***********************************************************************/
/* ops vector routines                                                 */
/***********************************************************************/

/* ARGSUSED */
static
begin_preview(ip, event)
register panel_item_handle	ip;
Event 				*event;
{
   text_data			*dp = textdp(ip);
   register panel_handle	 panel = ip->panel;
   Seln_holder	 		 holder;
   
   /* don't move the caret if this is a
    * secondary selection.
    */
   holder = panel_seln_inquire(panel, SELN_UNSPECIFIED);
   if (panel->caret != ip) {
      if (holder.rank == SELN_SECONDARY) {
         panel_seln_acquire(panel, ip, SELN_SECONDARY, FALSE);  
         return;
      }
   }
   
   /* Make this item the selection.
    * If it already had the caret, hilite the text. 
    * This avoids annoying selection feedback when the user
    * simply wants to move the caret.
    */
#ifdef notdef
   panel_seln_acquire(panel, ip, holder.rank, panel->caret != ip);  
#else
   /* always hilite the selection */
   panel_seln_acquire(panel, ip, holder.rank, FALSE);  
#endif notdef

   /* If we are getting the PRIMARY selection, make
    * sure we have the CARET selection also.
    */
   if (holder.rank == SELN_PRIMARY)
       panel_seln_acquire(panel, ip, SELN_CARET, TRUE);  
      	
   dp->flags |= SELECTING_ITEM;
   dp->orig_caret = ip->panel->caret;
   (void) panel_setcaret(panel, ip);
}



/***********************************************************************/
/* cancel_preview                                                      */
/***********************************************************************/

/* ARGSUSED */
static
cancel_preview(ip, event)
panel_item_handle	ip;
Event 			*event;
{
   register text_data 		*dp = textdp(ip);
   register panel_handle	 panel = ip->panel;
   
   if (dp->flags & SELECTING_ITEM) {
      deselect(panel);
      panel->caret = dp->orig_caret;
      textdp(dp->orig_caret)->flags |= HASCARET;
      dp->orig_caret = (panel_item_handle) NULL;
      dp->flags &= ~SELECTING_ITEM;
   }
   if (ip == panel_seln(panel, SELN_PRIMARY)->ip)
      panel_seln_cancel(panel, SELN_PRIMARY);
   if (ip == panel_seln(panel, SELN_SECONDARY)->ip)
      panel_seln_cancel(panel, SELN_SECONDARY);
   if (ip == panel_seln(panel, SELN_CARET)->ip)
      panel_seln_cancel(panel, SELN_CARET);
}


/***********************************************************************/
/* accept_preview                                                      */
/***********************************************************************/

/* ARGSUSED */
static
accept_preview(ip, event)
panel_item_handle	ip;
Event 			*event;
{
   register text_data		*dp		= textdp(ip);
   register panel_handle	panel		= ip->panel;
   Seln_holder	 		holder;
  
   if (!(dp->flags & SELECTING_ITEM))
      return;
   
   dp->orig_caret = (panel_item_handle) NULL;
   dp->flags &= ~SELECTING_ITEM;
   /* Ask for kbd focus if this is a primary selection */
   holder = panel_seln_inquire(panel, SELN_UNSPECIFIED);
   if (holder.rank == SELN_PRIMARY)
      (void)win_set_kbd_focus(panel->windowfd, win_fdtonumber(panel->windowfd));
}


static
accept_menu(ip, event)
panel_item_handle	ip;
Event 			*event;
{
   struct menuitem *mi;

   /* Make sure the menu title reflects the label. */
   panel_sync_menu_title(ip);
   
   if (mi = panel_menu_display(ip, event)) {
      if ((panel_item_handle)LINT_CAST(panel_getcaret(ip->panel)) != ip)
	 (void) panel_setcaret(ip->panel, ip);
      event_id(event) = (short)mi->mi_data; 
      accept_key(ip, event);
   }
}


/***********************************************************************/
/* accept_key                                                          */
/***********************************************************************/

static
accept_key(ip, event)
register panel_item_handle	ip;
register Event 			*event;
{
   panel_handle		panel 		= ip->panel;
   register text_data	*dp 		= textdp(ip);
   short    		code		= event_id(event);
   int			has_caret	= panel->caret == ip;
   int      		notify_desired	= FALSE;
   Panel_setting	notify_rtn_code;
   int			ok_to_insert;
   panel_selection_handle selection	= panel_seln(panel, SELN_CARET);
   extern char		*index();
   
   /* get the caret selection */
   if (panel->seln_client && !selection->ip)
      if (event_is_down(event) &&
         (event_is_ascii(event) || event_id(event) == GET_KEY))
         panel_seln_acquire(panel, ip, SELN_CARET, TRUE);  

   /* not interested in function keys, 
    * except for acquiring the caret selection.
    */
   if (event_is_key_left(event))
      return;
      

   /* de-hilite */
   if (ip == panel_seln(panel, SELN_PRIMARY)->ip)
      panel_seln_dehilite(panel_seln(panel, SELN_PRIMARY));
   else if (ip == panel_seln(panel, SELN_SECONDARY)->ip)
      panel_seln_dehilite(panel_seln(panel, SELN_SECONDARY));
   
   switch (dp->notify_level) {
      case PANEL_ALL:
	 notify_desired = TRUE;
	 break;
      case PANEL_SPECIFIED:
	 notify_desired = index(dp->terminators, code) != 0;
	 break;
      case PANEL_NON_PRINTABLE:
	 notify_desired = (code < ' ' || code > '~');
	 break;
      case PANEL_NONE:
	 notify_desired = FALSE;
	 break;
   }
   if (notify_desired) {
      notify_rtn_code = (Panel_setting) (*ip->notify)(ip, event);
      ok_to_insert = notify_rtn_code == PANEL_INSERT;
   }
   else
      ok_to_insert = (code >= ' ' && code <= '~') ;

   /* turn off the caret */
   if (has_caret)
      paint_caret(ip, PIX_CLR);

   /* do something with the character */
   update_value(ip, code, ok_to_insert);

   if (has_caret)
      paint_caret(ip, PIX_SET);

   if (notify_desired && has_caret)
      switch (notify_rtn_code) {
	 case PANEL_NEXT:
            (void) panel_advance_caret((Panel) panel);
	    break;

	 case PANEL_PREVIOUS:
	    (void) panel_backup_caret((Panel) panel);
	    break;

	 default:
	    break;
      }
}


static
update_value(ip, code, ok_to_insert)
panel_item_handle	ip;
register int		code;
int			ok_to_insert;
/* update_value updates the value of ip according to code.  If code
   is an edit character, the appropriate edit action is taken.
   Otherwise, if ok_to_insert is true, code is appended to ip->value.
*/
{
   register text_data	*dp = textdp(ip);
   register char	*sp;			/* string value */
   int		 	orig_len, new_len;	/* before & after lengths */
   register int	 	i;			/* counter */
   register int		 x;			/* left of insert/del point */
   int		 	was_clipped;		/* TRUE if value was clipped */
   int		 	orig_offset;		/* before caret offset */

   sp = dp->value;
   orig_len = strlen(sp);

   if (code == dp->edit_bk_char) {		/* backspace */
      if (*sp)
	 sp[orig_len - 1] = '\0';
   }
   else if (code == dp->edit_bk_word) {	/* backword */
      /* skip back past blanks */
      for(i = orig_len - 1; i >= 0 && sp[i] == ' '; i--);
      for(; i >= 0 && sp[i] != ' '; i--);
      sp[i + 1] = '\0';
   }
   else if (code == dp->edit_bk_line) 		/* backline */
      sp[0] = '\0';
   else if (ok_to_insert) {			/* insert */
      if (orig_len < dp->stored_length) {	/* there is room */
	 sp[orig_len] = (char) code;
	 sp[orig_len + 1] = '\0';
      } else					/* no more room */
	 blink_value(ip);
   }

   /* determine the new caret offset */
   was_clipped = dp->first_char != 0;
   orig_offset = dp->caret_offset;
   update_caret_offset(ip);

   /* update the display if not masked */
   if (dp->mask != ' ') {
      /* compute the position of the caret */
      x = ip->value_rect.r_left + dp->caret_offset;

      new_len = strlen(sp);

      /* erase deleted characters that were displayed */
      if (new_len < orig_len) {
	 /* repaint the whole value if needed */
	 if (was_clipped || dp->first_char)
	    paint_value(ip);
	 else
	    /* clear the deleted characters */
	    (void)panel_pw_writebackground(ip->panel, x, ip->value_rect.r_top,
	  	               orig_offset - dp->caret_offset,
		               ip->value_rect.r_height, PIX_CLR);
      } else if (new_len > orig_len) {
	 /* repaint the whole value if it doesn't fit */
	 if (new_len > dp->display_length)
	    paint_value(ip);
	 else
	    /* write the new character to the left of the caret */
            if (dp->mask == '\0') /* not masked */
	       (void)panel_pw_text(ip->panel, x - dp->font->pf_defaultsize.x,
		       ip->value_rect.r_top + panel_fonthome(dp->font),
		       PIX_SRC, dp->font, &sp[new_len - 1]);
	    else { /* masked */
	       char buf[2];
	       buf[0] = dp->mask;
	       buf[1] = '\0';
	       (void)panel_pw_text(ip->panel, x - dp->font->pf_defaultsize.x,
		       ip->value_rect.r_top + panel_fonthome(dp->font),
		       PIX_SRC, dp->font, buf);
	    }
      }
   }
   
}


static
update_caret_offset(ip)
panel_item_handle	ip;
/* update_caret_offset computes the caret x offset for ip.
*/
{
   register text_data	*dp = textdp(ip);
   int			clip_len, full_len;
   struct pr_size	size;

   /* no offset if masked completely */
   if (dp->mask == ' ') {
      dp->caret_offset = 0;
      dp->first_char = 0;
      return;
   }

   full_len = strlen(dp->value);
   clip_len = full_len > dp->display_length ? 
	      dp->display_length - 1 : full_len;
   size = pf_textwidth(clip_len, dp->font, dp->value);

   /* clip at the left if needed */
   dp->first_char = full_len - clip_len;
   dp->caret_offset = size.x;
   /* account for the left arrow if clipped */
   if (dp->first_char)
      dp->caret_offset += panel_left_arrow_pr.pr_width;
}

   
static
blink_value(ip)
panel_item_handle	ip;
/* blink_value blinks the value rect of ip.
*/
{
   int	i;	/* counter */

   /* invert the value rect */
   (void)panel_invert(ip->panel, &ip->value_rect);

   /* wait a while */
   for (i = 1; i < 5000; i++);

   /* un-invert the value rect */
   (void)panel_invert(ip->panel, &ip->value_rect);
}


/***********************************************************************/
/*                         caret routines                              */
/***********************************************************************/


static
deselect(panel)
panel_handle	panel;
{
   panel_item_handle 	old_item = (panel_item_handle) panel->caret;
   text_data       	*old_dp;

   if (old_item != NULL)  {
      old_dp = textdp(old_item);
      old_dp->flags &= ~HASCARET;
      paint_caret(old_item, PIX_CLR);
   }
}



static caddr_t
panel_getcaret(panel)
panel_handle panel;
{
   return (caddr_t) panel->caret;
}

static caddr_t
panel_setcaret(panel,ip)
panel_handle panel; 
panel_item_handle ip;
{
   text_data  *dp = textdp(ip);

   if (ip == NULL || hidden(ip))
      return NULL;

   deselect(panel);
   panel->caret = ip;
   if (panel_seln(panel, SELN_CARET)->ip)
      panel_seln(panel, SELN_CARET)->ip = ip;
   dp->flags |= HASCARET;
   paint_caret(ip, PIX_SET);
   return (caddr_t) ip;
}

Panel_item
panel_advance_caret(client_panel)
Panel_item	client_panel;
{
   panel_handle		panel = PANEL_CAST(client_panel);
   panel_item_handle 	old_item;
   text_data 		*old_dp;

   old_item = panel->caret;

   if (!old_item)
      return(NULL);

   old_dp = textdp(old_item);
   (void) panel_setcaret(panel,  old_dp->next);

   return (Panel_item) panel->caret;
}

Panel_item
panel_backup_caret(client_panel)
Panel_item	client_panel;
{
   panel_handle			panel = PANEL_CAST(client_panel);
   register panel_item_handle	old_ip;
   register panel_item_handle	new_ip,pre_ip;

   old_ip = panel->caret;

   if (!old_ip)
      return(NULL); 

   /* find previous item by going forward in linked list of text items... */
   pre_ip = old_ip;
   new_ip = textdp(old_ip)->next;
   while (new_ip != old_ip) {
      pre_ip = new_ip;
      new_ip = textdp(new_ip)->next;
   }

   (void) panel_setcaret(panel, pre_ip);

   return (Panel_item) panel->caret;
}


static void
text_caret_invert(panel)
register panel_handle panel;
/* text_caret_invert inverts the type-in caret.
*/
{
   if (!panel->caret)
      return;

   paint_caret(panel->caret, panel->caret_on ? PIX_CLR : PIX_SET);
} /* text_caret_invert */


static void
text_caret_on(panel, on)
panel_handle 	panel;
int 		on;
/* text_caret_on paints the type-in caret if on is true; otherwise xor's it.
*/
{
   if (!panel->caret)
      return;

   paint_caret(panel->caret, on ? PIX_SET : PIX_CLR);
} /* text_caret_on */


/***********************************************************************/
/* panel_text_notify                                                   */
/***********************************************************************/

/* ARGSUSED */
Panel_setting
panel_text_notify(client_item, event)
Panel_item	client_item;
register Event	*event;
{
   switch (event_id(event)) {
      case '\n':
      case '\r':
      case '\t':
	 return (event_shift_is_down(event)) ? PANEL_PREVIOUS : PANEL_NEXT;

      default:
	 return (event_id(event) >= ' ' && event_id(event) <= '~') ? 
		 PANEL_INSERT : PANEL_NONE;
   }
}


/* Hilite selection according to 
 * its rank. 
 */
void
panel_seln_hilite(selection)
register panel_selection_handle	selection;
{
   register panel_item_handle	ip = selection->ip;
   int				caret_offset = textdp(ip)->caret_offset;
   Rect				rect;
   
   if (caret_offset == 0)
      return;
       
   rect = ip->value_rect;
   rect.r_width = caret_offset;
   switch (selection->rank) {
      case SELN_PRIMARY:
         (void)panel_invert(ip->panel, &rect);
         break;
         
      case SELN_SECONDARY:
         rect.r_top = rect_bottom(&rect) - 1;
         rect.r_height = 1;
         (void)panel_invert(ip->panel, &rect);
         break;
   }
}
