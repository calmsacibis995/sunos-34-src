#ifndef lint
static	char sccsid[] = "@(#)panel_seln.c 1.7 87/01/07 Copyr 1984 Sun Micro";
#endif

/***********************************************************************/
/*                            panel_seln.c                             */
/*              Copyright (c) 1984 by Sun Microsystems, Inc.           */
/***********************************************************************/

#include "panel_impl.h"
#include <sunwindow/sun.h>
#include <suntool/selection_attributes.h>

extern void	(*panel_seln_inform_proc)(),
		(*panel_seln_destroy_proc)();

static void	panel_seln_destroy_info(),
		panel_seln_function(),
		panel_seln_get();

static void	panel_seln_report_event(), check_cache();

		
static Seln_result	panel_seln_request();

/* Register with the service */
void
panel_seln_init(panel)
register panel_handle	panel;
{
   /* this is so we only try to contact
    * the selection service once.
    */
   static	no_selection_service;	 /* Defaults to FALSE */

   register panel_selection_handle primary = panel_seln(panel, SELN_PRIMARY);
   register panel_selection_handle secondary = panel_seln(panel, SELN_SECONDARY);
   register panel_selection_handle caret = panel_seln(panel, SELN_CARET);
   register panel_selection_handle shelf = panel_seln(panel, SELN_SHELF);

   /* don't register with the selection service
    * unless we are using the notifier (i.e. panel_begin() was
    * used instead of panel_create().
    */
   if (!using_notifier(panel) || no_selection_service)
      return;
         
   panel->seln_client = 
      seln_create(panel_seln_function, panel_seln_request, 
      	(char *)(LINT_CAST(panel)));

   if (!panel->seln_client) {
      no_selection_service = TRUE;
      return;
   }

   panel_seln_destroy_proc = panel_seln_destroy_info;
   panel_seln_inform_proc = (void(*)()) panel_seln_report_event;
   
   primary->rank = SELN_PRIMARY;
   primary->is_null = TRUE;
   primary->ip = (panel_item_handle) 0;
   
   secondary->rank = SELN_SECONDARY;
   secondary->is_null = TRUE;
   secondary->ip = (panel_item_handle) 0; 

   caret->rank = SELN_CARET;
   caret->is_null = TRUE;
   caret->ip = (panel_item_handle) 0; 

   shelf->rank = SELN_SHELF;
   shelf->is_null = TRUE;
   shelf->ip = (panel_item_handle) 0; 
}


/* Inquire about the holder of a selection */
Seln_holder
panel_seln_inquire(panel, rank)
panel_handle	panel;
Seln_rank	rank;
{
   Seln_holder	holder;
   
   /* always ask the service, even if we
    * have not setup contact before (i.e. no text items).
    * This could happen if some other item has PANEL_ACCEPT_KEYSTROKE
    * on.
   if (!panel->seln_client)
      holder.rank = SELN_UNKNOWN;
   else
   */
      holder = seln_inquire(rank);
   return holder;
}


static void
panel_seln_report_event(panel, event)
panel_handle	panel;
Event		*event;
{
   seln_report_event(panel->seln_client, event);

   if (!panel->seln_client)
      return;

   check_cache(panel, SELN_PRIMARY);
   check_cache(panel, SELN_SECONDARY);
   check_cache(panel, SELN_CARET);
}

static void
check_cache(panel, rank)
register panel_handle	panel;
register Seln_rank	rank;
{
   Seln_holder	holder;

   if (panel_seln(panel, rank)->ip) {
      holder = seln_inquire(rank);
      if (!seln_holder_same_client(&holder, (char *)(LINT_CAST(panel))))
         panel_seln_cancel(panel, rank);
   }
}



/* Acquire the selection and update state */
void
panel_seln_acquire(panel, ip, rank, is_null)
register panel_handle		panel;
Seln_rank			rank;
panel_item_handle		ip;
int				is_null;
{
   register panel_selection_handle	selection;
   
   if (!panel->seln_client)
      return;
      
   switch (rank) {
      case SELN_PRIMARY:
      case SELN_SECONDARY:
      case SELN_CARET:
	 selection = panel_seln(panel, rank);
	 /* if we already own the selection,
	  * don't ask the service for it.
	  */
	 if (ip && selection->ip == ip)
	    break;
	 /* otherwise fall through ... */

      default:
         rank = seln_acquire(panel->seln_client, rank);
         switch (rank) {
            case SELN_PRIMARY:
            case SELN_SECONDARY:
            case SELN_CARET:
            case SELN_SHELF:
	       selection = panel_seln(panel, rank);
	       break;

            default:
	       return;
         }
	 break;
   }
      
   /* if we already have the selection & it's not
    * null, don't do anything.
    */
   if (ip && selection->ip == ip && !selection->is_null)
      return;

   /* if there was an old selection, de-hilite it */   
   if (selection->ip)
      panel_seln_dehilite(selection);
   
   /* record the selection & hilite it if it's not null */
   selection->ip = ip;
   selection->is_null = is_null;
   if (!is_null)
      panel_seln_hilite(selection);
}

/*
 * Clear out the current selection. 
 */
void
panel_seln_cancel(panel, rank)
panel_handle	panel;
Seln_rank	rank;
{
    panel_selection_handle	selection = panel_seln(panel, rank);

    if (!panel->seln_client || !selection->ip)
	return;

   /* de-hilite the selection */
   panel_seln_dehilite(selection);
   selection->ip = (panel_item_handle) 0;
   (void)seln_done(panel->seln_client, rank);
}

/* de-hilite selection */
void
panel_seln_dehilite(selection)
panel_selection_handle	selection;
{
   if (selection->is_null)
      return;
      
   panel_seln_hilite(selection);
   selection->is_null = TRUE;
}


/* Destroy myself as a selection client */
static void
panel_seln_destroy_info(panel)
register panel_handle	panel;
{
   if (!panel->seln_client)
      return;
      
   /* cancel PRIMARY and SECONDARY to
    * get rid of possible highlighting
    */
   panel_seln_cancel(panel, SELN_PRIMARY);
   panel_seln_cancel(panel, SELN_SECONDARY);
   if (panel->shelf) {
      free(panel->shelf);
      panel->shelf = (char *) 0;
   }
   seln_destroy(panel->seln_client);
}

/* Callback routines */

/* A function key has gone up -- do something. */
static void
panel_seln_function(panel, buffer)
register panel_handle		 panel;
register Seln_function_buffer	*buffer;
{
   Seln_holder			*holder;
   Event			event;
   panel_selection_handle	selection;
   char				*selection_string;
   
   if (!panel->caret)
      return;
      

   switch (seln_figure_response(buffer, &holder)) {
      case SELN_IGNORE:
	 break;
            
      case SELN_REQUEST:
         panel_seln_get(panel, holder, buffer->addressee_rank);
	 break;

      case SELN_SHELVE:
        if (panel->shelf)
	   free(panel->shelf);
#ifdef notdef
        panel->shelf = panel_strsave(panel_get_value(panel->caret));
#else
	/* shelve the requested selection, not the
	 * caret selection.
	 */
	selection = panel_seln(panel, buffer->addressee_rank);
	if (selection->is_null || !selection->ip)
	   selection_string = "";
	else
	    selection_string = panel_get_value((Panel_item) selection->ip);
        panel->shelf = panel_strsave(selection_string ? selection_string : "");
#endif notdef
        panel_seln_acquire(panel, (panel_item_handle) 0, SELN_SHELF, TRUE);
        break;

      case SELN_FIND:
	 (void) seln_ask(holder,
	 	SELN_REQ_COMMIT_PENDING_DELETE,
	 	SELN_REQ_YIELD, 0,
	 	0);
	 break;

      case SELN_DELETE:
        if (panel->shelf)
	   free(panel->shelf);
	selection = panel_seln(panel, buffer->addressee_rank);
#ifdef notdef
	selection_string = selection->ip ? panel_get_value(selection->ip) : "";
#else
	/* only delete the hilited selection */
	if (selection->is_null || !selection->ip)
	   selection_string = "";
	else
	    selection_string = panel_get_value((Panel_item) selection->ip);
#endif notdef
        panel->shelf = panel_strsave(selection_string ? selection_string : "");
        panel_seln_acquire(panel, (panel_item_handle) 0, SELN_SHELF, TRUE);

	if (selection->is_null || !selection->ip)
	   break;

        /* send the item a delete-line event */
        event_set_down(&event);
        event_set_id(&event, (short) panel->edit_bk_line);
	(void)panel_handle_event((Panel_item) selection->ip, &event);
        break;

      default:
	/* ignore anything else */
	break;
    }
}
 

/* Respond to a request about my
 * selections.
 */
static Seln_result
panel_seln_request(attr, context, max_length)
Seln_attribute			attr;
register Seln_replier_data	*context;
int				max_length;
{
    register panel_handle	 panel	= (panel_handle) LINT_CAST(context->client_data);
    register panel_selection_handle selection;
    register char		*selection_string = (char *) 0;
    Seln_result			result;
    
    switch (context->rank) {
      case SELN_PRIMARY:
      case SELN_SECONDARY:
        selection = panel_seln(panel, context->rank);
	if (selection->ip)
	   selection_string = selection->is_null ?
	      "" : panel_get_value((Panel_item) selection->ip);
	break;

      case SELN_SHELF:
	selection_string = panel->shelf;
	break;

      default:
	break;
    }
    
    switch (attr) {
      case SELN_REQ_BYTESIZE:
	if (!selection_string)
	    return SELN_DIDNT_HAVE;

	*context->response_pointer++ = (caddr_t) strlen(selection_string);
	return SELN_SUCCESS;
	    
      case SELN_REQ_CONTENTS_ASCII: {
	char		*temp	= (char *) context->response_pointer;
	int 		count;

	if (!selection_string)
	    return SELN_DIDNT_HAVE;

        count = strlen(selection_string);
	if (count <= max_length) {
	    bcopy(selection_string, temp, count);
	    temp += count;
	    while ((unsigned) temp % sizeof (*context->response_pointer))
	       *temp++ = '\0';
	    context->response_pointer = (char **) LINT_CAST(temp);
	    *context->response_pointer++ = 0;
	    return SELN_SUCCESS;
	}
	return SELN_FAILED;
      }

      case SELN_REQ_YIELD:
	result = SELN_FAILED;
	switch (context->rank) {
	    case SELN_SHELF:
	       if (panel->shelf) {
		  result = SELN_SUCCESS;
		  free(panel->shelf);
		  panel->shelf = 0;
                  (void)seln_done(panel->seln_client, SELN_SHELF);
	       }
	       break;

	    default:
	       if (panel_seln(panel, context->rank)->ip) {
	          panel_seln_cancel(panel, (Seln_rank) context->rank);
	          result = SELN_SUCCESS;
	       }
	       break;
	}
	*context->response_pointer++ = (caddr_t) result;
	return result;
	    
      default:
	return SELN_UNRECOGNIZED;
    }
}


/* Selection utilities */

/* Get the selection from holder and put it in the
 * text item that owns the selection rank.
 */
static void
panel_seln_get(panel, holder, rank)
panel_handle		panel;
Seln_holder		*holder;
Seln_rank		rank;
{
   Seln_request			*buffer;
   register Attr_avlist		avlist;
   register char		*cp;
   Event			event;
   int				num_chars;
   panel_selection_handle	selection = panel_seln(panel, rank);
   panel_item_handle	 	ip = selection->ip;
  
   if (!panel->seln_client)
      return;
    
   /* if the request is too large,
    * drop it on the floor.
    */
   if (holder->rank == SELN_SECONDARY)
       buffer = seln_ask(holder, 
			 SELN_REQ_BYTESIZE, 0,
			 SELN_REQ_CONTENTS_ASCII, 0,
			 SELN_REQ_COMMIT_PENDING_DELETE,
			 SELN_REQ_YIELD, 0,
			 0);
   else
       buffer = seln_ask(holder, 
			 SELN_REQ_BYTESIZE, 0,
			 SELN_REQ_CONTENTS_ASCII, 0,
			 SELN_REQ_COMMIT_PENDING_DELETE,
			 0);

   if (buffer->status == SELN_FAILED)
      return;
   
   avlist = (Attr_avlist) LINT_CAST(buffer->data);
   
   if ((Seln_attribute) *avlist++ != SELN_REQ_BYTESIZE)
      return;
      
   num_chars = (int) *avlist++;

   if ((Seln_attribute) *avlist++ != SELN_REQ_CONTENTS_ASCII)
      return;
      
   cp = (char *) avlist;
   
   /* make sure the string is null terminated
    * in the last byte of the selection.
    */
   cp[num_chars] = '\0';
   
   panel_seln_dehilite(selection);
   
   /* initialize the event */
   event_set_down(&event);
   
   while (*cp) {
      event_id(&event) = (short) *cp++;
      (void)panel_handle_event((Panel_item) ip, &event);
   }
}
