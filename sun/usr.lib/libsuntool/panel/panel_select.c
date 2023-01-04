#ifndef lint
static  char sccsid[] = "@(#)panel_select.c 1.7 87/01/07 Copyr 1984 Sun Micro";
#endif

/*****************************************************************************/
/*                          panel_select.c                                   */
/*            Copyright (c) 1984 by Sun Microsystems, Inc.                   */
/*****************************************************************************/

#include "panel_impl.h"
#include <suntool/window.h>
#include <suntool/selection_svc.h>

static panel_item_handle panel_finditem();

#define	CTRL_D_KEY	'\004'
#define	CTRL_G_KEY	'\007'

/*****************************************************************************/
/* panel_use_event                                                           */
/* called from both notifier-based panel_notify_event() and 		     */
/*   old panel_selected().   						     */
/*****************************************************************************/

/*VARARGS2*/
Notify_value
panel_use_event(panel, event, sb)
register panel_handle	panel;
register Event		*event;
Scrollbar		sb;
{
   short		is_down, id;

   /* ignore drag-in to subwindow */
   if ((event_id(event) == LOC_DRAG) && 
       (panel->mouse_state == PANEL_NONE_DOWN))
       return NOTIFY_IGNORED;

   /* ignore button-up after drag-in */
   if ((panel->mouse_state == PANEL_NONE_DOWN) && event_is_up(event)) {
      switch (event_id(event)) {
	 case MS_LEFT:
	 case MS_MIDDLE:
	 case MS_RIGHT:
            return NOTIFY_IGNORED;

	 default:
	    break;
      }
   }

   /* make an event adjusted for the current viewing window */
   if (event_id(event) != LOC_WINEXIT)
      (void) panel_event((Panel) panel, event);

   switch (event_id(event)) {
      case TOP_KEY:
      case OPEN_KEY:
         /* Here we perform the default interface tool actions */
         if (panel->tool) {
            extern Notify_value tool_input();
            return tool_input(panel->tool, event, (Notify_arg)0, NOTIFY_SAFE);
         }
         break;
      
      case PUT_KEY:
      case GET_KEY:
      case DELETE_KEY:
      case FIND_KEY:
         panel_seln_inform(panel, event);
         break;

      case CTRL_D_KEY:
	 /* map to DELETE down/up */
	 event_set_id(event, DELETE_KEY);
	 event_set_down(event);
         panel_seln_inform(panel, event);

	 event_set_up(event);
         panel_seln_inform(panel, event);

	 /* restore */
	 event_set_id(event, CTRL_D_KEY);
	 event_set_down(event);
	 break;

      case CTRL_G_KEY:
	 /* map to GET down/up */
	 event_set_id(event, GET_KEY);
	 event_set_down(event);
         panel_seln_inform(panel, event);

	 event_set_up(event);
         panel_seln_inform(panel, event);

	 /* restore */
	 event_set_id(event, CTRL_G_KEY);
	 event_set_down(event);
	 break;

      case SCROLL_REQUEST:
	 panel_scroll(panel, sb);
         break;

      case SCROLL_EXIT:
	 /* check the state of the mouse */
	 panel->button_status.left_down = 
	     win_get_vuid_value(panel->windowfd, MS_LEFT);
	 panel->button_status.middle_down = 
	     win_get_vuid_value(panel->windowfd, MS_MIDDLE);

	 /* don't keep track of the right button
	 panel->button_status.right_down = 
	     win_get_vuid_value(panel->windowfd, MS_RIGHT);
	 */
	 break;

      case KBD_REQUEST: {
	Seln_holder 		seln_holder;
	panel_item_handle 	new;
	int			take_focus;

	/* accept the keyboard focus if the user
	 * is not clicking on any item and we have a caret item or the panel
	 * wants key strokes,
	 * or if the user is clicking on a text item or item that wants key
	 * strokes
	 * and
	 * the user is not holding down a function key.
	 */

	/* find out who's under the locator */
	new = panel_finditem(panel, event);

	if (new)
	    take_focus = (new->item_type == PANEL_TEXT_ITEM) || wants_key(new);
	else 
	    take_focus = panel->caret || wants_key(panel);
	if (take_focus) {
	    /* find the rank of the selection */
	    seln_holder = panel_seln_inquire(panel, SELN_UNSPECIFIED);
 	    take_focus = (seln_holder.rank != SELN_SECONDARY);
	}
	if (!take_focus)
	    /* Refuse to take the keyboard focus */
	    (void)win_refuse_kbd_focus(panel->windowfd);
	/* else take the kbd focus by doing nothing */

      }

      default: 
         break;
   }
         
   /* make sure the caret is off before anything else is done */
   /* only turn off if not LOC_MOVE, to avoid caret flashing w/scrollbars */
   if (event_id(event) != LOC_MOVE)
      panel_caret_on(panel, FALSE);

   /* remember the event info */
   is_down = event_is_down(event);
   id = event_id(event);

   if (using_wrapper(panel))
      window_event_proc((Window)(LINT_CAST(panel)), event, sb);
   else
      (void)panel_default_event(panel, event);

   /* now update the state */
   switch (id) {
      case MS_LEFT:
	 panel->button_status.left_down = is_down;
	 break;

      case MS_MIDDLE:
	 panel->button_status.middle_down = is_down;
	 break;

      /* Don't keep track of right button
      case MS_RIGHT:
	 panel->button_status.right_down = is_down;
	 break;
      */

      case LOC_WINENTER:
      case LOC_WINEXIT:
	 /* erase our knowledge of the button state */
	 panel->button_status.left_down = FALSE;
	 panel->button_status.middle_down = FALSE;
	 panel->button_status.right_down = FALSE;
	 break;
	 
      default:
	 /* does not alter button state */
	 break;
   }
   if (panel->button_status.left_down + panel->button_status.middle_down + 
       panel->button_status.right_down > 1)
	 panel->mouse_state = PANEL_CHORD_DOWN;
   else if (panel->button_status.left_down)
      panel->mouse_state = PANEL_LEFT_DOWN;
   else if (panel->button_status.middle_down)
      panel->mouse_state = PANEL_MIDDLE_DOWN;
   else if (panel->button_status.right_down)
      panel->mouse_state = PANEL_RIGHT_DOWN;
   else
      panel->mouse_state = PANEL_NONE_DOWN;

   /* make sure the caret is on before we leave the subwindow */
   if (panel->mouse_state == PANEL_NONE_DOWN)
      panel_caret_on(panel, TRUE);

   return NOTIFY_DONE;
}

panel_default_event(panel, event)
register panel_handle	panel;
register Event		*event;
{
   panel_item_handle	new;

   /* find out who's under the locator */
   new = panel_finditem(panel, event);

   /* use the panel if not over some item */
   if (!new)
      new = (panel_item_handle) panel;

   /* cancel the old item if needed */
   if (new != panel->current) {
      (void)panel_cancel((Panel_item) panel->current, event);
      panel->current = new;
      /* Map the event to an object-enter event:
       * LOC_MOVE -> PANEL_EVENT_MOVE_IN,
       * LOC_DRAG -> PANEL_EVENT_DRAG_IN.
       */
       if (event_id(event) == LOC_MOVE)
	 event_id(event) = PANEL_EVENT_MOVE_IN;
       else if (event_id(event) == LOC_DRAG)
	 event_id(event) = PANEL_EVENT_DRAG_IN;
   }

   /* Here we offer keyboard events first to the
    * item under the locator (which may be the panel),
    * then to the item with the caret.
    */
   if (event_is_ascii(event) || event_is_key_left(event))
      if (wants_key(new))
	 /* tell the item about the key event */
	 (void)panel_handle_event((Panel_item) new, event);
      else if (panel->caret)
	 /* give the key event to the caret item */
	 (void)panel_handle_event((Panel_item) panel->caret, event);
      else
	 /* nobody wants the key event */;
   else
      /* tell the item about the non-key event */
      (void)panel_handle_event((Panel_item) new, event);
       
   /* no current item if window-exit */
   if (event_id(event) == LOC_WINEXIT)
      panel->current = NULL;
}

panel_handle_event(client_object, event)
Panel_item	client_object;	/* could be a Panel */
Event		*event;
{
   panel_item_handle	object = PANEL_ITEM_CAST(client_object);

   if (!object)
      return;

   (*object->ops->handle_event)(object, event);
}


panel_begin_preview(client_object, event)
Panel_item	client_object;	/* could be a Panel */
Event		*event;
{
   panel_item_handle	object = PANEL_ITEM_CAST(client_object);

   if (!object)
      return;

   (*object->ops->begin_preview)(object, event);
}

panel_update_preview(client_object, event)
Panel_item	client_object;	/* could be a Panel */
Event		*event;
{
   panel_item_handle	object = PANEL_ITEM_CAST(client_object);

   if (!object)
      return;

   (*object->ops->update_preview)(object, event);
}

panel_accept_preview(client_object, event)
Panel_item	client_object;	/* could be a Panel */
Event			*event;
{
   panel_item_handle	object = PANEL_ITEM_CAST(client_object);

   if (!object)
      return;

   (*object->ops->accept_preview)(object, event);
}

panel_cancel_preview(client_object, event)
Panel_item	client_object;	/* could be a Panel */
Event		*event;
{
   panel_item_handle	object = PANEL_ITEM_CAST(client_object);

   if (!object)
      return;

   (*object->ops->cancel_preview)(object, event);
}

panel_accept_menu(client_object, event)
Panel_item	client_object;	/* could be a Panel */
Event		*event;
{
   panel_item_handle	object = PANEL_ITEM_CAST(client_object);

   if (!object || !show_menu(object))
      return;

   (*object->ops->accept_menu)(object, event);
}

panel_accept_key(client_object, event)
Panel_item	client_object;	/* could be a Panel */
Event			*event;
{
   panel_item_handle	object = PANEL_ITEM_CAST(client_object);

   if (!object)
      return;

   (*object->ops->accept_key)(object, event);
}


panel_cancel(client_object, event)
Panel_item	client_object;	/* could be a Panel */
Event		*event;
{
   Event		cancel_event;

   if (!client_object)
      return;

   cancel_event = *event;
   event_id(&cancel_event) = PANEL_EVENT_CANCEL;
   (void)panel_handle_event(client_object, &cancel_event);
}


panel_default_handle_event(client_object, event)
Panel_item	client_object;	/* could be a Panel */
register Event	*event;
{
   Panel_setting 	current_state;
   
   current_state = (Panel_setting) panel_get(client_object, PANEL_MOUSE_STATE);

   switch (event_id(event)) {
      case MS_LEFT:		/* left button up or down */
	 if (event_is_up(event))
	    (void)panel_accept_preview(client_object, event);
	 else
	    (void)panel_begin_preview(client_object, event);
	 break;

      case MS_MIDDLE:		/* middle button up or down */
	 break;

      case MS_RIGHT:		/* right button up or down */
	 if (event_is_down(event) && current_state == PANEL_NONE_DOWN)
	    (void)panel_accept_menu(client_object, event);
	 break;

      case PANEL_EVENT_DRAG_IN:		/* drag into item rect */
	 if (current_state == PANEL_LEFT_DOWN)
	    (void)panel_begin_preview(client_object, event);
	 break;

      case LOC_DRAG:	/* left, middle, or right drag */
	 if (current_state == PANEL_LEFT_DOWN)
	    (void)panel_update_preview(client_object, event);
	 break;

      case LOC_WINEXIT:	/* exit from panel */
      case PANEL_EVENT_CANCEL:		/* exit from item rectangle */
	 if (current_state == PANEL_LEFT_DOWN)
	    (void)panel_cancel_preview(client_object, event);
	 break;

      default:			/* some other event */
	 if (event_is_ascii(event) || event_is_key_left(event))
	    (void)panel_accept_key(client_object, event);
	 break;
   }
}

static panel_item_handle
panel_finditem(panel, event)
panel_handle	 	panel;
Event	*event;
{
   register panel_item_handle	ip = panel->current;
   register int	   		x = event_x(event);
   register int			y = event_y(event);
   
   if (!panel->items)
      return NULL;

   if (ip && is_item(ip) && !hidden(ip) && rect_includespoint(&ip->rect, x, y))
	 return ip;

   for (ip = hidden(panel->items) ?
	     panel_successor(panel->items) : panel->items;
	ip && !rect_includespoint(&ip->rect, x, y);
	ip = panel_successor(ip));

   return ip;
}
