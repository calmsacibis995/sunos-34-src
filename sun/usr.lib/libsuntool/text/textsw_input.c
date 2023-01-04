#ifndef lint
static  char sccsid[] = "@(#)textsw_input.c 1.6 87/04/20";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * User input interpreter for text subwindows.
 */

#include "primal.h"
#include "textsw_impl.h"
#include <errno.h>
#include <sundev/kbd.h>
	/* Only needed for SHIFTMASK and CTRLMASK */
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/win_struct.h>

extern int		errno;

extern Key_map_handle	textsw_do_filter();
pkg_private void	textsw_init_timer();

pkg_private int
textsw_flush_caches(view, flags)
	register Textsw_view	view;
	register int		flags;
{
	register Textsw_folio	textsw = FOLIO_FOR_VIEW(view);
	register int		count;
	register int		end_clear = (flags & TFC_SEL);

	count = (textsw->func_state & TXTSW_FUNC_FILTER)
		? 0
		: (textsw->to_insert_next_free - textsw->to_insert);
	if (flags & TFC_DO_PD) {
	    if ((count > 0) || ((flags & TFC_PD_IFF_INSERT) == 0)) {
		(void) textsw_do_pending_delete(view, EV_SEL_PRIMARY,
						end_clear|TFC_INSERT);
		end_clear = 0;
	    }
	}
	if (end_clear) {
	    if ((count > 0) || ((flags & TFC_SEL_IFF_INSERT) == 0)) {
		(void) textsw_set_selection(
			    VIEW_REP_TO_ABS(view),
			    ES_INFINITY, ES_INFINITY, EV_SEL_PRIMARY);
	    }
	}
	if (flags & TFC_INSERT) {
	    if (count > 0) {
		/*   WARNING!  The cache pointers must be updated BEFORE
		 * calling textsw_do_input, so that if the client is being
		 * notified of edits and it calls textsw_get, it will not
		 * trigger an infinite recursion of textsw_get calling
		 * textsw_flush_caches calling textsw_do_input calling
		 * the client calling textsw_get calling ...
		 */
		textsw->to_insert_next_free = textsw->to_insert;
		(void) textsw_do_input(view, textsw->to_insert, count);
	    }
	}
}

pkg_private void
textsw_read_only_msg(textsw, locx, locy)
	Textsw_folio	textsw;
	int		locx, locy;
{
	textsw_post_error((Textsw_opaque)textsw, locx, locy,
			  "The text is read_only and cannot be edited",
			  " - operation aborted.");
}

pkg_private int
textsw_note_event_shifts(textsw, ie)
	register Textsw_folio		 textsw;
	register struct inputevent	*ie;
{
	int				 result = 0;

	if (ie->ie_shiftmask & SHIFTMASK)
	    textsw->state |= TXTSW_SHIFT_DOWN;
	else {
#ifdef VT_100_HACK
	    if (textsw->state & TXTSW_SHIFT_DOWN) {
		/* Hack for VT-100 keyboard until PIT is available. */
		result = 1;
	    }
#endif
	    textsw->state &= ~TXTSW_SHIFT_DOWN;
	}
	if (ie->ie_shiftmask & CTRLMASK)
	    textsw->state |= TXTSW_CONTROL_DOWN;
	else
	    textsw->state &= ~TXTSW_CONTROL_DOWN;
	return(result);
}

extern int
textsw_process_event(abstract, ie, arg)
	Textsw		 abstract;
	register Event	*ie;
	Notify_arg	 arg;
{
	extern void			 textsw_resize();
	extern void			 textsw_repaint();
	extern void			 textsw_scroll();
	extern void			 textsw_update_scrollbars();
	int				 caret_was_up;
	int				 result = TEXTSW_PE_USED;
	register Textsw_view		 view = VIEW_ABS_TO_REP(abstract);
	register Textsw_folio		 textsw = FOLIO_FOR_VIEW(view);

	caret_was_up = textsw->caret_state & TXTSW_CARET_ON;
	/* Watch out for caret turds */
	if ((ie->ie_code == LOC_MOVE
	||   ie->ie_code == LOC_STILL
	||   ie->ie_code == LOC_WINENTER
	||   ie->ie_code == LOC_WINEXIT
	||   ie->ie_code == LOC_RGNENTER
	||   ie->ie_code == LOC_RGNEXIT
	||   ie->ie_code == SCROLL_ENTER
	||   ie->ie_code == SCROLL_EXIT)
	&&  !TXTSW_IS_BUSY(textsw)) {
	    /* leave caret up */
	} else {
	    textsw_take_down_caret(textsw);
	}
	switch (textsw_note_event_shifts(textsw, ie)) {
#ifdef VT_100_HACK
	  case 1:
	    if (textsw->func_state & TXTSW_FUNC_GET) {
		textsw_end_get(view);
	    }
#endif
	  default:
	    break;
	}
	if (ie->ie_code == TXTSW_STOP) {
	    (void) textsw_abort(textsw);
#ifdef GPROF
	} else if (ie->ie_code == KEY_RIGHT(13)) {
	    if (win_inputposevent(ie)) {
		if (textsw->state & TXTSW_SHIFT_DOWN) {
		    moncontrol(0);
		}
	    } else {
		if ((textsw->state & TXTSW_SHIFT_DOWN) == 0) {
		    moncontrol(1);
		}
	    }
	} else if (ie->ie_code == KEY_RIGHT(15)) {
	    if (win_inputposevent(ie)) {
	    } else {
		moncontrol(1);
		textsw_gprofed_routine(view, ie);
		moncontrol(0);
	    }
#endif
	/*
	 * Check Resize/Repaint early to avoid skipping due to 2nd-ary Seln. 
	 */
	} else if (ie->ie_code == WIN_RESIZE) {
	    textsw_resize(view);
	} else if (ie->ie_code == WIN_REPAINT) {
	    textsw_repaint(view);
	    /* if caret was up and we took it down, put it back */
	    if (caret_was_up
	    && (textsw->caret_state & TXTSW_CARET_ON) == 0) {
		textsw_remove_timer(textsw);
		textsw_timer_expired(textsw, 0);
	    }
	/*
	 * Check for possible scrollbar actions
	 */
	} else if (ie->ie_code == SCROLL_REQUEST) {
	    textsw_scroll((Scrollbar)arg);
	} else if (ie->ie_code == SCROLL_ENTER) {
	    textsw_update_scrollbars(textsw, view);
	/*
	 * Selection tracking and function keys.
	 *   track_selection() always called if tracking.  It consumes
	 * the event unless function key up while tracking secondary
	 * selection, which stops tracking and then does function.
	 */
	} else if ((textsw->track_state &
		    (TXTSW_TRACK_ADJUST|TXTSW_TRACK_POINT)) &&
		   track_selection(view, ie) ) {
	} else if (ie->ie_code == TXTSW_AGAIN) {
	    if (win_inputposevent(ie)) {
		textsw_begin_again(view);
	    } else if (textsw->func_state & TXTSW_FUNC_AGAIN) {
		textsw_end_again(view, ie->ie_locx, ie->ie_locy);
	    }
#ifdef VT_100_HACK
	} else if (ie->ie_code == TXTSW_AGAIN_ALT) {
	    /* Bug in releases through K3 only generates down, no up */
	    if (win_inputposevent(ie)) {
		textsw_begin_again(view);
		textsw_end_again(view, ie->ie_locx, ie->ie_locy);
	    } else if (textsw->func_state & TXTSW_FUNC_AGAIN) {
	    }
#endif
	} else if (ie->ie_code == TXTSW_UNDO) {
	    if (TXTSW_IS_READ_ONLY(textsw))
		goto Read_Only;
	    if (win_inputposevent(ie)) {
		textsw_begin_undo(view);
	    } else if (textsw->func_state & TXTSW_FUNC_UNDO) {
		textsw_end_undo(view);
	    }
#ifdef VT_100_HACK
	} else if (ie->ie_code == TXTSW_UNDO_ALT) {
	    /* Bug in releases through K3 only generates down, no up */
	    if (win_inputposevent(ie)) {
		textsw_begin_undo(view);
		textsw_end_undo(view);
	    } else if (textsw->func_state & TXTSW_FUNC_UNDO) {
	    }
#endif
	} else if ((ie->ie_code == TXTSW_TOP) ||
		   (ie->ie_code == TXTSW_OPEN)) {
	    textsw_notify(view, TEXTSW_ACTION_TOOL_MGR, ie, 0);
	} else if (ie->ie_code == TXTSW_FIND) {
	    if (win_inputposevent(ie)) {
		(void) textsw_begin_find(view);
		textsw->func_x = ie->ie_locx;
		textsw->func_y = ie->ie_locy;
		textsw->func_view = view;
	    } else {
		(void) textsw_end_find(view, ie->ie_locx, ie->ie_locy);
	    }
	} else if (ie->ie_code == TXTSW_POINT) {
	    if (win_inputposevent(ie)) {
#ifdef VT_100_HACK
		if ((textsw->state & TXTSW_SHIFT_DOWN) &&
		    (textsw->func_state & TXTSW_FUNC_ALL) == 0) {
		    /* Hack for VT-100 keyboard until PIT is available. */
		    (void) textsw_begin_get(view);
		    textsw->func_x = ie->ie_locx;
		    textsw->func_y = ie->ie_locy;
		    textsw->func_view = view;
		} else
#endif
		(void) start_selection_tracking(view, ie);
	    }
	    /* Discard negative events that get to here, as state is wrong. */
	} else if (ie->ie_code == TXTSW_ADJUST) {
	    if (win_inputposevent(ie)) {
		(void) start_selection_tracking(view, ie);
	    }
	    /* Discard negative events that get to here, as state is wrong. */
	} else if (ie->ie_code == LOC_WINENTER) {
	    extern void	textsw_sync_with_seln_svc();
	    textsw_sync_with_seln_svc(textsw);
	} else if (ie->ie_code == LOC_WINEXIT || ie->ie_code == KBD_DONE) {
	    textsw_may_win_exit(textsw);
	/*
	 * Menus
	 */
	} else if (ie->ie_code == TXTSW_MENU) {
	    if (win_inputposevent(ie)) {
		extern textsw_do_menu();
		(void) textsw_flush_caches(view, TFC_STD);
		(void) textsw_do_menu(view, ie);
	    }
	/*
	 * Function-key editing
	 */
	} else if (ie->ie_code == TXTSW_DELETE) {
	    if (win_inputposevent(ie)) {
		textsw_begin_delete(view);
		textsw->func_x = ie->ie_locx;
		textsw->func_y = ie->ie_locy;
		textsw->func_view = view;
	    } else {
		result |= textsw_end_delete(view);
	    }
	} else if (ie->ie_code == TXTSW_GET) {
	    if (win_inputposevent(ie)) {
		(void) textsw_begin_get(view);
		textsw->func_x = ie->ie_locx;
		textsw->func_y = ie->ie_locy;
		textsw->func_view = view;
	    } else {
		result |= textsw_end_get(view);
	    }
	} else if (ie->ie_code == TXTSW_PUT) {
	    if (win_inputposevent(ie)) {
		(void) textsw_begin_put(view, TRUE);
		textsw->func_x = ie->ie_locx;
		textsw->func_y = ie->ie_locy;
		textsw->func_view = view;
	    } else {
		result |= textsw_end_put(view);
	    }
	} else if (ie->ie_code == TXTSW_CAPS_LOCK) {
	    if (TXTSW_IS_READ_ONLY(textsw))
		goto Read_Only;
	    if (win_inputposevent(ie)) {
	    } else {
		textsw->state ^= TXTSW_CAPS_LOCK_ON;
		textsw_notify(view, TEXTSW_ACTION_CAPS_LOCK,
			       (textsw->state & TXTSW_CAPS_LOCK_ON), 0);
	    }
	/*
	 * Type-in
	 */
	} else if (textsw->track_state & TXTSW_TRACK_SECONDARY) {
	    /* No type-in processing during secondary (function) selections */
	} else if (ie->ie_code <= ASCII_LAST) {
	    unsigned	edit_unit;
	    if (textsw->func_state & TXTSW_FUNC_FILTER) {
		if (textsw->to_insert_next_free <
		    textsw->to_insert + sizeof(textsw->to_insert)) {
		    *textsw->to_insert_next_free++ = (char) ie->ie_code;
		}
	    } else {
		if (ie->ie_code == textsw->edit_bk_char) {
		    edit_unit = EV_EDIT_CHAR;
		} else if (ie->ie_code == textsw->edit_bk_word) {
		    edit_unit = EV_EDIT_WORD;
		} else if (ie->ie_code == textsw->edit_bk_line) {
		    edit_unit = EV_EDIT_LINE;
		} else edit_unit = 0;
		if (edit_unit != 0) {
		    if (TXTSW_IS_READ_ONLY(textsw))
			goto Read_Only;
		    (void) textsw_flush_caches(view,
			TFC_INSERT|TFC_PD_IFF_INSERT|TFC_DO_PD|TFC_SEL);
		    (void) textsw_do_edit(view, edit_unit,
				  (unsigned)((ie->ie_shiftmask & SHIFTMASK)
				  ? 0 : EV_EDIT_BACK));
		} else switch (ie->ie_code) { 
		  case TXTSW_DELETE_ALT:
		  case TXTSW_FIND_ALT:
		  case TXTSW_PASTE_ALT:
		    textsw->func_x = ie->ie_locx;
		    textsw->func_y = ie->ie_locy;
		    textsw->func_view = view;
		    if (ie->ie_code == TXTSW_DELETE_ALT) {
			result |= textsw_function_delete(view);
		    } else if (ie->ie_code == TXTSW_FIND_ALT) {
			(void) textsw_function_find(view,
						    ie->ie_locx, ie->ie_locy);
		    } else if (ie->ie_code == TXTSW_PASTE_ALT) {
			result |= textsw_function_get(view);
		    }
		    break;
		  case TXTSW_PUT_THEN_GET:
		    if (TXTSW_IS_READ_ONLY(textsw))
			goto Read_Only;
		    /* textsw_flush_caches(view, TFC_STD); */
		   (void)  textsw_put_then_get(view);
		    break;
		  case TXTSW_LOAD_FILE: {
		    char	*dummy;
		    if (textsw->state & TXTSW_NO_LOAD)
			goto Insert;
		    (void) textsw_flush_caches(view, TFC_STD);
		    if (0 == textsw_file_name(textsw, &dummy)) {
			if (TXTSW_IS_READ_ONLY(textsw))
			    goto Read_Only;
			goto Insert;
		    } else {
			(void) textsw_set_selection(abstract, 0,
				es_get_length(textsw->views->esh),
				EV_SEL_PRIMARY);
			if (textsw_load_selection(textsw, ie->ie_locx,
			    ie->ie_locy,
			    (int)(textsw->state & TXTSW_NO_CD)) == 0) {
			   textsw->state &= ~TXTSW_READ_ONLY_ESH;
			}
		    }
		    break;
		  }
		  case (short) '\r':	/* Fall through */
		  case (short) '\n':
		    if ((textsw->state & TXTSW_CONTROL_DOWN) &&
			(win_get_vuid_value(WIN_FD_FOR_VIEW(view),
					    'm') == 0) &&
			(win_get_vuid_value(WIN_FD_FOR_VIEW(view),
					    'j') == 0) &&
			(win_get_vuid_value(WIN_FD_FOR_VIEW(view),
					    'M') == 0) &&
			(win_get_vuid_value(WIN_FD_FOR_VIEW(view),
					    'J') == 0) ) {
			(void) textsw_do_next_field(view);
		    } else {
			if (TXTSW_IS_READ_ONLY(textsw))
			    goto Read_Only;
			(void) textsw_do_newline(view);
		    }
		    break;
		  default:
		    if (TXTSW_IS_READ_ONLY(textsw))
			goto Read_Only;
		    if (textsw->state & TXTSW_CAPS_LOCK_ON) {
			if ((char)ie->ie_code >= 'a' &&
			    (char)ie->ie_code <= 'z')
			    ie->ie_code += 'A' - 'a';
		    }
Insert:
		    *textsw->to_insert_next_free++ = (char) ie->ie_code;
		    if (textsw->to_insert_next_free ==
			textsw->to_insert + sizeof(textsw->to_insert)) {
			(void) textsw_flush_caches(view, TFC_STD);
		    }
		    break;
		}
	    }
	/*
	 * User filters
	 */
	} else if (textsw_do_filter(view, ie)) {
	/*
	 * Miscellaneous
	 */
	} else {
	    result &= ~TEXTSW_PE_USED;
	}
	/*
	 * Cleanup
	 */
	if ((textsw->state & TXTSW_EDITED) == 0)
	    (void) textsw_possibly_edited_now_notify(textsw);
Done:
	if (TXTSW_IS_BUSY(textsw))
	    result |= TEXTSW_PE_BUSY;
	return(result);
Read_Only:
	result |= TEXTSW_PE_READ_ONLY;
	goto Done;
}

static Key_map_handle
find_key_map(textsw, ie)
	register Textsw_folio	 textsw;
	register Event		*ie;
{
	register Key_map_handle	 current_key = textsw->key_maps;

	while (current_key) {
	    if (current_key->event_code == ie->ie_code) {
		break;
	    }
	    current_key = current_key->next;
	}
	return(current_key);
}

pkg_private Key_map_handle
textsw_do_filter(view, ie)
	register Textsw_view	 view;
	register Event		*ie;
{
	register Textsw_folio	 textsw = FOLIO_FOR_VIEW(view);
	register Key_map_handle	 result = find_key_map(textsw, ie);

	if (result == 0)
	    goto Return;
	if (win_inputposevent(ie)) {
	    switch (result->type) {
	      case TXTSW_KEY_SMART_FILTER:
	      case TXTSW_KEY_FILTER:
		(void) textsw_flush_caches(view, TFC_STD);
		textsw->func_state |= TXTSW_FUNC_FILTER;
		result = 0;
		break;
	    }
	    goto Return;
	}
	switch (result->type) {
	  case TXTSW_KEY_SMART_FILTER:
	  case  TXTSW_KEY_FILTER: {
	    extern int	textsw_call_smart_filter();
	    extern int	textsw_call_filter();
	    int		again_state, filter_result;

	    again_state = textsw->func_state & TXTSW_FUNC_AGAIN;
	    (void) textsw_record_filter(textsw, ie);
	    textsw->func_state |= TXTSW_FUNC_AGAIN;
	    if (result->type == TXTSW_KEY_SMART_FILTER) {
		filter_result = textsw_call_smart_filter(view, ie,
		    (char **)LINT_CAST(result->maps_to));
	    } else {
		filter_result = textsw_call_filter(view,
		    (char **)LINT_CAST(result->maps_to));
	    }
	    switch (filter_result) {
	      case 0:
	      default:
		break;
	      case 1: {
		char	msg[300];
		if (errno == ENOENT) {
		    (void) sprintf(msg, "Cannot locate filter '%s'.",
			    ((char **)LINT_CAST(result->maps_to))[0]);
		} else {
		    (void) sprintf(msg, "Unexpected problem with filter '%s'.",
			    ((char **)LINT_CAST(result->maps_to))[0]);
		}
		textsw_post_error((Textsw_opaque)textsw, ie->ie_locx,
		    ie->ie_locy, msg, NULL);
		
		break;
	      }
	    }
	    textsw->func_state &= ~TXTSW_FUNC_FILTER;
	    textsw->to_insert_next_free = textsw->to_insert;
	    if (again_state == 0)
		textsw->func_state &= ~TXTSW_FUNC_AGAIN;
	    result = 0;
	  }
	}
Return:
	return(result);
}

static int
textsw_do_next_field(view)
	register Textsw_view	view;
{
	Es_index		ev_get_insert();
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	register Es_index	length;    /* Can be changed by cache flush */
	int			lower_context;

	if (!TXTSW_IS_READ_ONLY(folio))
	    (void) textsw_flush_caches(view, TFC_INSERT|TFC_PD_IFF_INSERT|TFC_DO_PD);
	length = es_get_length(folio->views->esh);
	if (length == 0)
	    return;
	if (!TXTSW_IS_READ_ONLY(folio) &&
	    (ev_get_insert(folio->views) != length)) {
	    (void) textsw_set_insert(folio, length);
	}
	lower_context = (int)LINT_CAST(
			ev_get(folio->views, EV_CHAIN_LOWER_CONTEXT) );
	(void) textsw_normalize_internal(view, length, length, 0, lower_context,
		TXTSW_NI_AT_BOTTOM|TXTSW_NI_NOT_IF_IN_VIEW|TXTSW_NI_MARK);
}

static int
textsw_do_newline(view)
	register Textsw_view	view;
{
	Es_index		ev_get_insert();
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	int			delta;
	Es_index		first = ev_get_insert(folio->views),
				last_plus_one,
				previous;

	(void) textsw_flush_caches(view, TFC_INSERT|TFC_PD_SEL);
	if (folio->state & TXTSW_AUTO_INDENT)
	    first = ev_get_insert(folio->views);
	view->state |= TXTSW_UPDATE_SCROLLBAR;
	delta = textsw_do_input(view, "\n", 1);
	if (folio->state & TXTSW_AUTO_INDENT) {
	    previous = first;
	    (void) textsw_find_pattern(folio, &previous, &last_plus_one,
				"\n", 1, EV_FIND_BACKWARD);
	    if (previous != ES_CANNOT_SET) {
		char			 buf[100];
		struct es_buf_object	 esbuf;
		register char		*c;

		esbuf.esh = folio->views->esh;
		esbuf.buf = buf;
		esbuf.sizeof_buf = sizeof(buf);
		if (es_make_buf_include_index(&esbuf, previous, 0) == 0) {
		    if (AN_ERROR(buf[0] != '\n')) {
		    } else {
			for (c = buf+1; c < buf+sizeof(buf); c++) {
			   switch (*c) {
			      case '\t':
			      case ' ':
				break;
			      default:
				goto Did_Scan;
			   }
			}
Did_Scan:
			if (c != buf+1) {
			    delta += textsw_do_input(view, buf+1,
						     (int) (c-buf-1));
			}
		    }
		}
	    }
	}
	return(delta);
}

pkg_private Es_index
textsw_get_saved_insert(textsw)
	register Textsw_folio	textsw;
{
	Ev_finger_handle	saved_insert_finger;

	saved_insert_finger = ev_find_finger(
			&textsw->views->fingers, textsw->save_insert);
	return(saved_insert_finger ? saved_insert_finger->pos : ES_INFINITY);
}

pkg_private int
textsw_clear_pending_func_state(textsw)
	register Textsw_folio	textsw;
{
	if (!EV_MARK_IS_NULL(&textsw->save_insert)) {
	    if (textsw->func_state & TXTSW_FUNC_PUT) {
		Es_index	old_insert = textsw_get_saved_insert(textsw);
		if AN_ERROR(old_insert == ES_INFINITY) {
		} else {
		    (void) textsw_set_insert(textsw, old_insert);
		}
	    } else
		ASSUME(textsw->func_state & TXTSW_FUNC_GET);
	    (void) ev_remove_finger(&textsw->views->fingers, textsw->save_insert);
	    (void) ev_init_mark(&textsw->save_insert);
	}
	if (textsw->func_state & TXTSW_FUNC_FILTER) {
	    textsw->to_insert_next_free = textsw->to_insert;
	}
	textsw->func_state &= ~(TXTSW_FUNC_ALL | TXTSW_FUNC_EXECUTE);
}

/*	==========================================================
 *
 *		Input mask initialization and setting.
 *
 *	==========================================================
 */

static struct inputmask	basemask_kbd, mousebuttonmask_kbd;
static struct inputmask	basemask_pick, mousebuttonmask_pick;
static int		masks_have_been_initialized;	/* Defaults to FALSE */

static setupmasks()
{
	register struct inputmask	*mask;
	register int			i;

	/*
	 * Set up the standard kbd mask.
	 */
	mask = &basemask_kbd;
	(void) input_imnull(mask);
	mask->im_flags |= IM_ASCII | IM_NEGEVENT;
	for (i=1; i<17; i++) {
	    win_setinputcodebit(mask, KEY_LEFT(i));
	    win_setinputcodebit(mask, KEY_TOP(i));
	    win_setinputcodebit(mask, KEY_RIGHT(i));
	}
	/* Unset TOP and OPEN because will look for them in pick mask */
	win_unsetinputcodebit(mask, TXTSW_TOP);
	win_unsetinputcodebit(mask, TXTSW_OPEN);
	win_setinputcodebit(mask, KBD_USE);
	win_setinputcodebit(mask, KBD_DONE);
#ifdef VT_100_HACK
	win_setinputcodebit(mask, SHIFT_LEFT);		/* Pick up the shift */
	win_setinputcodebit(mask, SHIFT_RIGHT);		/*   keys for VT-100 */
	win_setinputcodebit(mask, SHIFT_LOCK);		/*   compatibility */
#endif
	/*
	 * Set up the standard pick mask.
	 */
	mask = &basemask_pick;
	(void) input_imnull(mask);
	win_setinputcodebit(mask, WIN_STOP);
	win_setinputcodebit(mask, LOC_WINENTER);
	win_setinputcodebit(mask, LOC_WINEXIT);
	win_setinputcodebit(mask, TXTSW_POINT);
	win_setinputcodebit(mask, TXTSW_ADJUST);
	win_setinputcodebit(mask, TXTSW_MENU);
	win_setinputcodebit(mask, TXTSW_TOP);
	win_setinputcodebit(mask, TXTSW_OPEN);
	win_setinputcodebit(mask, KBD_REQUEST);
	win_setinputcodebit(mask, LOC_MOVE);	/* NOTE: New */
	win_setinputcodebit(mask, LOC_STILL);	/* NOTE: Detect shift up??? */
	mask->im_flags |= IM_NEGEVENT;
	/* Make "mouse" mask use base mask */
	mousebuttonmask_kbd = basemask_kbd;
	mousebuttonmask_pick = basemask_pick;
	masks_have_been_initialized = TRUE;
}

pkg_private int
textsw_set_base_mask(fd)
	int	fd;
{
	if (masks_have_been_initialized == FALSE) {
	    setupmasks();
	}
	(void) win_set_kbd_mask(fd, &basemask_kbd);
	(void) win_set_pick_mask(fd, &basemask_pick);
}

pkg_private int
textsw_set_mouse_button_mask(fd)
	int	fd;
{
	if (masks_have_been_initialized == FALSE) {
	    setupmasks();
	}
	(void) win_set_kbd_mask(fd, &mousebuttonmask_kbd);
	(void)win_set_pick_mask(fd, &mousebuttonmask_pick);
}

/*	==========================================================
 *
 *		Functions invoked by function keys.
 *
 *	==========================================================
 */

pkg_private void
textsw_begin_function(view, function)
	Textsw_view		view;
	unsigned		function;
{
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);

	(void) textsw_flush_caches(view, TFC_STD);
	if ((folio->state & TXTSW_CONTROL_DOWN) &&
	    !TXTSW_IS_READ_ONLY(folio))
	    folio->state |= TXTSW_PENDING_DELETE;
	folio->track_state |= TXTSW_TRACK_SECONDARY;
	folio->func_state |= function|TXTSW_FUNC_EXECUTE;
	textsw_init_timer(folio);
	if AN_ERROR(folio->func_state & TXTSW_FUNC_SVC_SAW(function))
	    /* Following covers up inconsistent state with Seln. Svc. */
	    folio->func_state &= ~TXTSW_FUNC_SVC_SAW(function);
}

pkg_private void
textsw_init_timer(folio)
	Textsw_folio	folio;
{
	/*
	 * Make last_point/_adjust/_ie_time close (but not too close) to
	 *   current time to avoid overflow in tests for multi-click.
	 */
	folio->last_point.tv_sec -= 1000;
	folio->last_adjust = folio->last_point;
	folio->last_ie_time = folio->last_point;
};
	

pkg_private void
textsw_end_function(view, function)
	Textsw_view		view;
	unsigned		function;
{
	pkg_private void	textsw_end_selection_function();
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);

	folio->state &= ~TXTSW_PENDING_DELETE;
	folio->track_state &= ~TXTSW_TRACK_SECONDARY;
	folio->func_state &=
	    ~(function | TXTSW_FUNC_SVC_SAW(function) | TXTSW_FUNC_EXECUTE);
	textsw_end_selection_function(folio);
}

static
textsw_begin_again(view)
	Textsw_view	 view;
{
	textsw_begin_function(view, TXTSW_FUNC_AGAIN);
}

static
textsw_end_again(view, x, y)
	Textsw_view	 view;
	int			 x, y;
{
	(void) textsw_do_again(view, x, y);
	textsw_end_function(view, TXTSW_FUNC_AGAIN);
}

pkg_private int
textsw_again(view, x, y)
	Textsw_view	 view;
	int		 x, y;
{
	textsw_begin_again(view);
	textsw_end_again(view, x, y);
}

static
textsw_begin_delete(view)
	Textsw_view	view;
{
	register Textsw_folio	textsw = FOLIO_FOR_VIEW(view);

	textsw_begin_function(view, TXTSW_FUNC_DELETE);
	if (!TXTSW_IS_READ_ONLY(textsw))
	    textsw->state |= TXTSW_PENDING_DELETE;
	    /* Force pending-delete feedback as it is implicit in DELETE */
	(void) textsw_inform_seln_svc(textsw, TXTSW_FUNC_DELETE, TRUE);
}

pkg_private int
textsw_end_delete(view)
	Textsw_view	view;
{
	extern void		textsw_init_selection_object();
	extern void		textsw_clear_secondary_selection();
	Textsw_selection_object	selection;
	int			result = 0;
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);

	(void) textsw_inform_seln_svc(folio, TXTSW_FUNC_DELETE, FALSE);
	if ((folio->func_state & TXTSW_FUNC_DELETE) == 0)
	    return(0);
	if ((folio->func_state & TXTSW_FUNC_EXECUTE) == 0)
	    goto Done;
	textsw_init_selection_object(folio, &selection, "", 0, FALSE);
	if (TFS_IS_ERROR(textsw_func_selection(folio, &selection, 0)))
	    goto Done;
	if (selection.type & TFS_IS_SELF) {
	    switch(textsw_adjust_delete_span(folio, &selection.first,
					     &selection.last_plus_one)) {
	      case TEXTSW_PE_READ_ONLY:
		textsw_clear_secondary_selection(folio, EV_SEL_SECONDARY);
		result = TEXTSW_PE_READ_ONLY;
		break;
	      case TXTSW_PE_EMPTY_INTERVAL:
		break;
	      case TXTSW_PE_ADJUSTED:
		(void) textsw_set_selection(VIEW_REP_TO_ABS(folio->first_view),
				     ES_INFINITY, ES_INFINITY, selection.type);
		/* Fall through to delete remaining span */
	      default:
		(void) textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
					(caddr_t)TEXTSW_INFINITY-1);
		(void) textsw_delete_span(
			view, selection.first, selection.last_plus_one,
			(unsigned)((selection.type & EV_SEL_PRIMARY)
			    ? TXTSW_DS_RECORD|TXTSW_DS_SHELVE
			    : TXTSW_DS_SHELVE));
		(void) textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
					(caddr_t)TEXTSW_INFINITY-1);
		break;
	    }
	}
Done:
	textsw_end_function(view, TXTSW_FUNC_DELETE);
	textsw_update_scrollbars(folio, view);
	return(result);
}

pkg_private int
textsw_function_delete(view)
	Textsw_view	view;
{
	int		result;

	textsw_begin_delete(view);
	result = textsw_end_delete(view);
	return(result);
}

static
textsw_begin_undo(view)
	Textsw_view	view;
{
	textsw_begin_function(view, TXTSW_FUNC_UNDO);
	(void) textsw_flush_caches(view, TFC_SEL);
}

static
textsw_end_undo(view)
	Textsw_view	view;
{
	textsw_do_undo(view);
	textsw_end_function(view, TXTSW_FUNC_UNDO);
	textsw_update_scrollbars(FOLIO_FOR_VIEW(view), view);
}

static
textsw_undo_notify(folio, start, delta)
	register Textsw_folio	folio;
	register Es_index	start, delta;
{
	extern void		textsw_notify_replaced();
	extern Es_index		ev_get_insert(),
				ev_set_insert();
	register Ev_chain	chain = folio->views;
	register Es_index	old_length =
				    es_get_length(chain->esh) - delta;
	Es_index		old_insert;

	if (folio->notify_level & TEXTSW_NOTIFY_EDIT)
	    old_insert = ev_get_insert(chain);
	(void) ev_set_insert(chain, (delta > 0) ? start+delta : start);
	(void) ev_update_after_edit(chain,
			     (delta > 0) ? start+delta : start-delta,
			     delta, old_length, start);
	if (folio->notify_level & TEXTSW_NOTIFY_EDIT) {
	    textsw_notify_replaced((Textsw_opaque)folio->first_view, 
			  old_insert, old_length,
			  (delta > 0) ? start : start+delta,
			  (delta > 0) ? start+delta : start,
			  (delta > 0) ? delta : 0);
	}
	(void) textsw_checkpoint(folio);
}

static
textsw_do_undo(view)
	Textsw_view			view;
{
	extern Ev_finger_handle		ev_add_finger();
	extern int			ev_remove_finger();
	extern Es_index			ev_get_insert(),
					textsw_set_insert();
	register Textsw_folio		folio = FOLIO_FOR_VIEW(view);
	register Ev_finger_handle	saved_insert_finger;
	register Ev_chain		views = folio->views;
	Ev_mark_object			save_insert;

	if (!TXTSW_DO_UNDO(folio))
	    return;
	if (folio->undo[0] == es_get(views->esh, ES_UNDO_MARK)) {
	    /* Undo followed immediately by another undo.
	     * Note: textsw_set_internal guarantees that folio->undo_count != 1
	     */
	    bcopy((caddr_t)&folio->undo[1], (caddr_t)&folio->undo[0],
		  (int)((folio->undo_count-2)*sizeof(folio->undo[0])));
	    folio->undo[folio->undo_count-1] = ES_NULL_UNDO_MARK;
	}
	if (folio->undo[0] == ES_NULL_UNDO_MARK)
	    return;
	/* Undo the changes to the piece source. */
	(void) ev_add_finger(&views->fingers, ev_get_insert(views), 0,
			     &save_insert);
	(void) es_set(views->esh,
	       ES_UNDO_NOTIFY_PAIR, textsw_undo_notify, (caddr_t)folio,
	       ES_UNDO_MARK, folio->undo[0],
	       0);
	saved_insert_finger =
	    ev_find_finger(&views->fingers, save_insert);
	if AN_ERROR(saved_insert_finger == 0) {
	} else {
	    (void) textsw_set_insert(folio, saved_insert_finger->pos);
	    (void) ev_remove_finger(&views->fingers, save_insert);
	}
	/* Get the new mark. */
	folio->undo[0] = es_get(views->esh, ES_UNDO_MARK);
	/* Check to see if this has undone all edits to the folio. */
	if (textsw_has_been_modified(VIEW_REP_TO_ABS(folio->first_view))
	    == 0) {
	    char	*name;
	    if (textsw_file_name(folio, &name) == 0) {
		textsw_notify(view, TEXTSW_ACTION_LOADED_FILE, name, 0);
	    }
	    folio->state &= ~TXTSW_EDITED;
	}
}

extern int
textsw_undo(textsw)
	Textsw_folio	 textsw;
{
	textsw_begin_undo(textsw->first_view);
	textsw_end_undo(textsw->first_view);
}
