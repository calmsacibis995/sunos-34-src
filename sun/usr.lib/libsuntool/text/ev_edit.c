#ifndef lint
static  char sccsid[] = "@(#)ev_edit.c 1.5 87/01/07";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Initialization and finalization of entity views.
 */

#include "primal.h"

#include "ev_impl.h"
#include <sys/time.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <pixrect/pixfont.h>
#include <suntool/tool.h>

extern void			ev_notify();
extern void			ev_clear_from_margins();
extern void			ev_check_insert_visibility();
extern int			ev_clear_rect();
extern struct ei_process_result	ev_display_internal();
extern struct rect		ev_rect_for_line();

int
ev_span(views, position, first, last_plus_one, group_spec)
	register Ev_chain	 views;
	Es_index		 position,
				*first,
				*last_plus_one;
	int			 group_spec;
{
	struct ei_span_result		span_result;
	char				buf[EV_BUFSIZE];
	struct es_buf_object		esbuf;
	esbuf.esh = views->esh;
	esbuf.buf = buf;
	esbuf.sizeof_buf = sizeof(buf);
	esbuf.first = esbuf.last_plus_one = 0;
	span_result = ei_span_of_group(
	    views->eih, &esbuf, group_spec, position);
	*first = span_result.first;
	*last_plus_one = span_result.last_plus_one;
}

Es_index
ev_line_start(view, position)
	register Ev_handle	view;
	register Es_index	position;
{
	Es_index			dummy, result;
	register Ev_impl_line_seq	seq = (Ev_impl_line_seq)
						view->line_table.seq;
	if (position >= seq[0]) {
	    /* Optimization: First try the view's line_table */
	    int		lt_index = ft_bounding_index(
					&view->line_table, position);
	    if (lt_index < view->line_table.last_plus_one - 1)
		return(seq[lt_index]);
	    /* -1 is required to avoid mapping all large positions to the
	     *   hidden extra line entry.
	     */
	}
	ev_span(view->view_chain, position, &result, &dummy,
		EI_SPAN_LINE | EI_SPAN_LEFT_ONLY);
	if (result == ES_CANNOT_SET) {
	    result = position;
	}
	return(result);
}

Es_index
ev_get_insert(views)
	Ev_chain	 views;
{
	return((EV_CHAIN_PRIVATE(views))->insert_pos);
}

Es_index
ev_set_insert(views, position)
	Ev_chain	 views;
	Es_index	 position;
{
	Es_handle		esh = views->esh;
	Ev_chain_pd_handle	private = EV_CHAIN_PRIVATE(views);
	Es_index		result;

	result = es_set_position(esh, position);
	if (result != ES_CANNOT_SET) {
		private->insert_pos = result;
	}
	return(result);
}

static void
ev_update_lt_after_edit(table, before_edit, delta)
	register Ev_line_table	*table;
	Es_index		 before_edit;
	register long int	 delta;
{
/*
 * Modifies the entries in table as follows:
 *   delta > 0 => (before_edit..EOS) incremented by delta
 *   delta < 0 => (before_edit+delta..before_edit] mapped to before_edit+delta,
 *		  (before_edit..EOS) decremented by ABS(delta).
 */
	register		 lt_index;

	if (delta == 0)
	    return;
	if (delta > 0) {
	    /*
	     * Only entries greater than before_edit are affected.  In
	     *   particular, entries at the original insertion position do not
	     *   extend.
	     */
	    if (before_edit < table->seq[0]) {
		ft_add_delta(*table, 0, delta);
	    } else {
		lt_index = ft_bounding_index(table, before_edit);
		if (lt_index < table->last_plus_one)
		    ft_add_delta(*table, lt_index+1, delta);
	    }
	} else {
	    /*
	     * Entries in the deleted range are set to the range's beginning.
	     * Entries greater than before_edit are simply decremented.
	     */
	    ft_set_esi_span(
		*table, before_edit+delta+1, before_edit,
		before_edit+delta, 0);
	    /* For now, let the compiler do all the cross jumping */
	    if (before_edit-1 < table->seq[0]) {
		ft_add_delta(*table, 0, delta);
	    } else {
		lt_index = ft_bounding_index(table, before_edit-1);
		if (lt_index < table->last_plus_one)
		     ft_add_delta(*table, lt_index+1, delta);
	    }
	}
}

/* Meaning of returned ei_span_result.flags>>16 is:
 *	0 success
 *	1 illegal edit_action
 *	2 unable to set insert
 */
extern struct ei_span_result
ev_span_for_edit(views, edit_action)
	Ev_chain	 views;
	int		 edit_action;
{
	Ev_chain_pd_handle	private = EV_CHAIN_PRIVATE(views);
	struct ei_span_result	span_result;
	int			group_spec = 0;
	char			buf[EV_BUFSIZE];
	struct es_buf_object	esbuf;

	switch (edit_action) {
	    case EV_EDIT_BACK_CHAR: {
		group_spec = EI_SPAN_CHAR | EI_SPAN_LEFT_ONLY;
		break;
	    }
	    case EV_EDIT_BACK_WORD: {
		group_spec = EI_SPAN_WORD | EI_SPAN_LEFT_ONLY;
		break;
	    }
	    case EV_EDIT_BACK_LINE: {
		group_spec = EI_SPAN_LINE | EI_SPAN_LEFT_ONLY;
		break;
	    }
	    case EV_EDIT_CHAR: {
		group_spec = EI_SPAN_CHAR | EI_SPAN_RIGHT_ONLY;
		break;
	    }
	    case EV_EDIT_WORD: {
		group_spec = EI_SPAN_WORD | EI_SPAN_RIGHT_ONLY;
		break;
	    }
	    case EV_EDIT_LINE: {
		group_spec = EI_SPAN_LINE | EI_SPAN_RIGHT_ONLY;
		break;
	    }
	    default: {
		span_result.flags = 1<<16;
		goto Return;
	    }
	}
	esbuf.esh = views->esh;
	esbuf.buf = buf;
	esbuf.sizeof_buf = sizeof(buf);
	esbuf.first = esbuf.last_plus_one = 0;
	span_result = ei_span_of_group(
	    views->eih, &esbuf, group_spec, private->insert_pos);
	if (span_result.first == ES_CANNOT_SET) {
	    span_result.flags = 2<<16;
	    goto Return;
	}
	if (((group_spec & EI_SPAN_CLASS_MASK) == EI_SPAN_WORD) &&
	    (span_result.flags & EI_SPAN_NOT_IN_CLASS) &&
	    (span_result.flags & EI_SPAN_HIT_NEXT_LEVEL) == 0) {
	    /*
	     * On a FORWARD/BACK_WORD, skip over preceding/trailing white
	     *   space and delete the preceding word.
	     */
	    struct ei_span_result	span2_result;
	    span2_result = ei_span_of_group(
		views->eih, &esbuf, group_spec,
		(group_spec & EI_SPAN_LEFT_ONLY)
		    ? span_result.first : span_result.last_plus_one);
	    if (span2_result.first != ES_CANNOT_SET) {
		if (group_spec & EI_SPAN_LEFT_ONLY)
		    span_result.first = span2_result.first;
		else
		    span_result.last_plus_one = span2_result.last_plus_one;
	    }
	}
Return:
	return(span_result);
}

ev_delete_span(views, first, last_plus_one, delta)
	Ev_chain		 views;
	register Es_index	 first, last_plus_one;
	Es_index		*delta;
{
	Es_handle		 esh = views->esh;
	register Ev_chain_pd_handle
				 private = EV_CHAIN_PRIVATE(views);
	register Es_index	 old_length = es_get_length(esh);
	Es_index		 new_insert_pos,
				 private_insert_pos = private->insert_pos;
	int			 result, used;

	/* Since *delta depends on last_plus_one, normalize ES_INFINITY */
	if (last_plus_one > old_length) {
	    last_plus_one = old_length;
	}
	/* See if operation makes sense */
	if (old_length == 0) {
	    result = 1;
	    goto Return;
	}
	/* We cannot assume where the esh is positioned, so position first. */
	if (first != es_set_position(esh, first)) {
	    result = 2;
	    goto Return;
	}
	new_insert_pos = es_replace(esh, last_plus_one, 0, 0, &used);
	if (new_insert_pos == ES_CANNOT_SET) {
	    result = 3;
	    goto Return;
	}
	*delta = first - last_plus_one;
	private->insert_pos = new_insert_pos;
		/* Above assignment required to make following call work! */
	ev_update_after_edit(
		views, last_plus_one, *delta, old_length, first);
	if (first < private_insert_pos) {
	    if (last_plus_one < private_insert_pos)
		private->insert_pos = private_insert_pos + *delta;
	    else
		/* Don't optimize out in case kludge above vanishes. */
		private->insert_pos = new_insert_pos;
	} else
	    private->insert_pos = private_insert_pos;
	if (private->notify_level & EV_NOTIFY_EDIT_DELETE) {
	    ev_notify(views->first_view,
		      EV_ACTION_EDIT, first, old_length, first, last_plus_one,
			0,
		      0);
	}
	result = 0;
Return:
	return(result);
}

static void
ev_update_fingers_after_edit(ft, insert, delta)
	register Ev_finger_table	*ft;
	register Es_index		 insert;
	register int			 delta;
/* This routine differs from the similar ev_update_lt_after_edit in that it
 * makes use of the extra Ev_finger_info fields in order to potentially
 * adjust entries at the insert point.
 */
{
	register Ev_finger_handle	 fingers;
	register int			 lt_index;
	register Es_index		 range_min;
	Ev_mark_object			 mark;
	opaque_t			 client_data;

	ev_update_lt_after_edit(ft, insert, delta);
	/* Correct entries that move_at_insert in views->fingers */
	lt_index = ft_bounding_index(ft, insert);
	if (lt_index < ft->last_plus_one) {
	    fingers = (Ev_finger_handle)ft->seq;
	    if (delta > 0) {
		while (fingers[lt_index].pos == insert) {
		    if (EV_MARK_MOVE_AT_INSERT(fingers[lt_index].info)) {
			fingers[lt_index].pos += delta;
			if (lt_index-- > 0)
			    continue;
		    }
		    break;
		}
	    } else {
		/* Search backwards through fingers whose
		 * pos == insert+delta for a finger with
		 * move_at_insert == 0;
		 * If one is found, continue searching backward
		 * through all fingers with pos == insert+delta.
		 * Any with move_at_insert != 0 must be removed
		 * and re-inserted to make sure they follow all
		 * entries of the same pos with move_at_insert == 0.
		 */
		range_min = insert + delta;
		while (lt_index > 0
		&& fingers[lt_index].pos == range_min) {
		    if (!EV_MARK_MOVE_AT_INSERT(fingers[lt_index].info))
			break;
		    lt_index--;
		}
		while (lt_index > 0
		&& fingers[lt_index].pos == range_min) {
		    if (EV_MARK_MOVE_AT_INSERT(fingers[lt_index].info)) {
			mark = fingers[lt_index].info;
			client_data = fingers[lt_index].client_data;
			ev_remove_finger(ft, mark);
			ev_add_finger(ft, range_min, client_data, &mark);
		    }
		    lt_index--;
		}
	    }
	}
}

static void
ev_update_view_lt_after_edit(line_seq, start, stop_plus_one, minimum, delta)
	register Ev_impl_line_seq	 line_seq;
	int				 start;
	register int			 stop_plus_one;
	register Es_index		 minimum;
	register int			 delta;
/* This routine differs from the similar ev_update_lt_after_edit in that it
 * can introduce positions of ES_CANNOT_SET (which are transient and not
 * valid in a normal finger table).
 */
{
	register int			 i;

	for (i = start; i < stop_plus_one; i++) {
	    if (line_seq[i] < minimum) {
		line_seq[i] = ES_CANNOT_SET;
	    } else if (line_seq[i] != ES_INFINITY)
		line_seq[i] += delta;
	}
}

static int
ev_scroll_view_down_after_edit(view, min_insert_pos, delta, to, from)
	register Ev_handle	 view;
	Es_index		 min_insert_pos;
	int			 delta;
	register int		 to, from;
{
	Rect			 to_rect, from_rect;
	int			 slide_rc;
	Ev_chain_pd_handle	 private = EV_CHAIN_PRIVATE(view->view_chain);

	slide_rc = ev_slide_lines(view, to, from, &to_rect, &from_rect);
	/*  First: fix up the line table and associated display.  */
	switch (slide_rc) {
	  case 2:
	  case 0: {
	    ev_update_view_lt_after_edit(
	        (Ev_impl_line_seq)view->line_table.seq, to,
		view->line_table.last_plus_one, min_insert_pos, delta);
	    if ((private->notify_level & EV_NOTIFY_SCROLL) ||
		(private->glyph_count)) {
		ev_clear_from_margins(view, &from_rect, &to_rect);
	    }
	    if (slide_rc == 2) {
		ev_display_fixup(view);
	    }
	    if (private->notify_level & EV_NOTIFY_SCROLL)
		ev_notify(view, EV_ACTION_SCROLL, &from_rect, &to_rect, 0);
	    break;
	  }
	  default:
	    break;
	}
	return(0);
}

static int
ev_scroll_view_up_after_edit(view, min_insert_pos, delta, to, from)
	register Ev_handle	 view;
	Es_index		 min_insert_pos;  /* start of deletion */
	int			 delta;		  /* amount deleted */
	register int		 to, from;	  /* indices into line_table */
{
	Rect			 to_rect, from_rect;
	register int		 fixup_start;
	int			 slide_rc;
	Ev_chain_pd_handle	 private = EV_CHAIN_PRIVATE(view->view_chain);

	slide_rc = ev_slide_lines(view, to, from, &to_rect, &from_rect);
	/*  First: fix up the line table and associated display.  */
	switch (slide_rc) {
	  case 2:
	  case 0: {
	    fixup_start = view->line_table.last_plus_one - 1 - (from - to);
	    ev_update_view_lt_after_edit(
	        (Ev_impl_line_seq) view->line_table.seq,
	        to, fixup_start+1, min_insert_pos,
	        delta);
	    if ((private->notify_level & EV_NOTIFY_SCROLL) ||
		(private->glyph_count)) {
		ev_clear_from_margins(view, &from_rect, &to_rect);
	    }
	    ev_display_starting_at_line(view, fixup_start);
	    if (slide_rc == 2) {
		ev_display_fixup(view);
	    }
	    if (private->notify_level & EV_NOTIFY_SCROLL)
		ev_notify(view, EV_ACTION_SCROLL, &from_rect, &to_rect, 0);
	    break;
	  }
	  default:
	    break;
	}
	return(0);
}

ev_update_after_edit(views, last_plus_one, delta, old_length, min_insert_pos)
	register Ev_chain	 views;
	Es_index		 last_plus_one;
	int			 delta;
	Es_index		 old_length,
				 min_insert_pos;
/* last_plus_one:  end of inserted/deleted span
 * delta:	   if positive, amount inserted; else, amount deleted
 * old_length:	   length before change
 * min_insert_pos: beginning of inserted/deleted span
 */
{
  /* Much of this routine's complexity is due to the following two
   *   cases being the possible situation at "end of stream":
   *	... entities ... '\n'
   *  vs
   *	... entities ... '\n'
   *	x
   * In the first case, the entire last line has to be processed before it is
   *   realized that in fact the new entities belong on the next line.  Relying
   *   on the fact that the following line table entry is EOS fails because of
   *   the second case, where such reliance would skip the single-entity line.
   */
	Es_handle		  esh = views->esh;
	Ev_chain_pd_handle	  private = EV_CHAIN_PRIVATE(views);
	int			  break_action, lpo_lt_index;
	int			  lpo_is_line_start, min_is_line_start;
	int			  update_view_lt = TRUE;
	register Ev_handle	  view;
	register int		  lt_index;
	register Ev_impl_line_seq line_seq;

	private->edit_number++;
	/* Update all tables that index into esh (excepting the view
	 *   line_tables) before trying to repaint.
	 */
	ev_update_lt_after_edit(&private->op_bdry, last_plus_one, delta);
	ev_update_fingers_after_edit(&views->fingers, last_plus_one, delta);
	FORALLVIEWS(views, view) {
	    line_seq = (Ev_impl_line_seq) view->line_table.seq;
	    break_action = 0;
	    lt_index = ft_bounding_index(&view->line_table, min_insert_pos);
	    if (delta < 0) {
		/*
		 * Delete
		 */
		if (lt_index == view->line_table.last_plus_one) {
		    /* Note: the test below is >=, rather than >, to catch
		     * the case where the user does an edit_char/word with the
		     * insert exactly at the first displayed character.
		     */
		    if (min_is_line_start = (last_plus_one >= line_seq[0])) {
			line_seq[lt_index = 0] = ES_INFINITY;
			/* Above invalidates cache used by next call. */
			line_seq[lt_index] =
			    ev_line_start(view, min_insert_pos);
		    }
		} else
		    min_is_line_start = (min_insert_pos == line_seq[lt_index]);
		if (min_is_line_start) {
		    lpo_lt_index = ft_bounding_index(&view->line_table,
						     last_plus_one);
		    lpo_is_line_start =
			((lpo_lt_index != view->line_table.last_plus_one) &&
			 (last_plus_one == line_seq[lpo_lt_index]));
		    if (lpo_is_line_start) {
			/* Deleted complete lines: try RasterOp fixup */
			switch (ev_scroll_view_up_after_edit(
			    view, min_insert_pos, delta,
			    lt_index, lpo_lt_index)) {
			  case 1:
			    break;	/* Have to do it the hard way. */
			  default:
			    goto NextView;
			}
		    }
		}
	    } else {
		/*
		 * Insert
		 */
		if (lt_index < view->line_table.last_plus_one-1)
		    min_is_line_start = (min_insert_pos == line_seq[lt_index]);
		if (min_insert_pos == old_length) {
		    /* End of stream is special */
		    if (min_insert_pos > view->line_table.seq[0] &&
			min_insert_pos > 0) {
			lt_index = ft_bounding_index(
			    &view->line_table, min_insert_pos-1);
		    } else {
			ev_display_view(view);
			goto NextView;
		    }
		}
	    }
	    if (lt_index < view->line_table.last_plus_one-1) {
		Rect				rect;
		Es_index			desired_successor, new_length;
		struct ei_process_result	result;

		/*
		 * Following test is only one guaranteed to work when calls to
		 *   ev_display_internal use stop_plus_one that is ES_INFINITY
		 *   only some of the time.
		 */
#define REACHED_END(result_formal)					\
	((result_formal).last_plus_one == new_length)

		/* Compute the line_seq[lt_index+1] value that would imply that
		 *   the first affected line is the ONLY affected line.
		 */
		desired_successor = line_seq[lt_index+1] + delta;
		if (desired_successor < private->insert_pos)
		    desired_successor = ES_INFINITY;
		/* We begin by updating the first affected line's display.  */
		rect = ev_rect_for_line(view, lt_index);
		new_length = es_get_length(esh);
		result.last_plus_one =
		    es_set_position(esh, line_seq[lt_index]);
		if (!min_is_line_start) {
		    Ev_pos_info		*cache;
		    Ev_pd_handle	 view_private = EV_PRIVATE(view);
		    cache = &view_private->cached_insert_info;
		    /* Try using (barely(?) invalid) cached insert info */
		    if ((cache->edit_number > 0) &&
		        (cache->pos == min_insert_pos) &&
		        (cache->edit_number == private->edit_number-1) &&
		        (cache->index_of_first == EV_VIEW_FIRST(view)) ) {
			result.break_reason = EI_PR_BUF_EMPTIED;
			result.pos.x = cache->pr_pos.x;
			lt_index = cache->lt_index;
			ASSUME(lt_index != EV_NOT_IN_VIEW_LT_INDEX);
			if (delta < 0) {
			    result.last_plus_one = min_insert_pos;
			    result.bounds.r_top =
				cache->pr_pos.y+1 - rect.r_height;
			    result.bounds.r_height = rect.r_height;
			}
		    } else {
			result = ev_display_internal(
			    view, &rect, lt_index, min_insert_pos,
			    EI_OP_MEASURE, EV_Q_DORBREAK);
		    }
		    if (result.break_reason & EI_PR_BUF_EMPTIED) {
			rect.r_width -= result.pos.x - rect.r_left;
			rect.r_left = result.pos.x;
			if (delta < 0 && REACHED_END(result)) {
			    /*
			     * On a delete, clean up trash on display below
			     *    the last valid line in the view.
			     */
			    ev_clear_to_bottom(view, &rect,
						rect_bottom(&rect)+1, 1);
			    goto NextView;
			}
			es_set_position(esh, min_insert_pos);
		    } else if (result.break_reason & EI_PR_HIT_RIGHT) {
			/* Must be insert off the right edge of view. */
			lt_index++;
			goto FollowingLines;
		    } else if (result.break_reason & EI_PR_NEWLINE) {
			/* Must be EOS insert after a newline. */
			lt_index++;
			goto FollowingLines;
		    } else {		/* Something awful happened! */
			ev_display_view(view);
			goto NextView;
		    }
		} else if (line_seq[lt_index] > 0 && delta > 0) {
		    /* If a newline is inserted at line start,
		     * and line start was previously caused by wrap,
		     * increment line_seq[lt_index], result.last_plus_one
		     * and es_set_position.
		     */
		    Es_index	span_first, span_lpo;
		    
		    ev_span(views, line_seq[lt_index]-1,
		            &span_first, &span_lpo,
		            EI_SPAN_LINE);
		    if (span_lpo-1 == line_seq[lt_index]) {
			/* line_seq[lt_index]-1 != newline; if it were,
			 * span_lpo-1 would be it.
			 * line_seq[lt_index] == newline because
			 * == span_lpo-1.
			 * Therefore, we inserted a newline at a wrap point.
			 */
			result.last_plus_one = ++line_seq[lt_index];
		    }
		    es_set_position(esh, line_seq[lt_index]);
		}
		if (delta < 0 && REACHED_END(result)) {
		    /*
		     * On a delete, clean up trash that may be left on
		     *   display below the last valid line in the view.
		     */
		    line_seq[lt_index] = result.last_plus_one;
		    ft_set(view->line_table, lt_index+1,
			view->line_table.last_plus_one, ES_INFINITY, (char *)0);
		    ev_clear_to_bottom(view, &rect, rect_bottom(&rect)+1, 1);
		    goto NextView;
		}
		ev_clear_rect(view, &rect);
		result = ev_display_internal(
		    view, &rect, lt_index, ES_INFINITY, EI_OP_CLEARED_RECT,
		    EV_QUIT|EV_Q_DORBREAK);
		lt_index++;
		if (result.break_reason & EI_PR_NEWLINE) {
		    line_seq[lt_index] = ++result.last_plus_one;
		} else if (result.break_reason & EI_PR_HIT_RIGHT) {
		    /* BUG ALERT: must handle wrap here! */
		}
		if (REACHED_END(result)) {
		    if (delta < 0) {
			/* One more clean up after delete. */
			for(lt_index++;
			    lt_index < view->line_table.last_plus_one;
			    lt_index++)
			{
			    if (line_seq[lt_index] == ES_INFINITY)
				break;
			    line_seq[lt_index] = ES_INFINITY;
			}
			ev_clear_to_bottom(view, &rect,
			    rect_bottom(&result.bounds)+1, 0);
		    }
		    goto NextView;	/* line_table already fixed up */
		}
FollowingLines:
		if (lt_index+1 == view->line_table.last_plus_one) {
		    goto NextView;
		}
		if (line_seq[lt_index] == desired_successor) {
		    ft_add_delta(view->line_table, lt_index+1, delta);
		    goto NextView;
		}
		if (delta < 0) {
		    /* On delete look ahead to see if any of the remaining
		     *   lines in the view are still valid.
		     * Note: the line table has NOT been updated yet.
		     */
		    desired_successor = line_seq[lt_index] - delta;
		    lpo_lt_index = ft_bounding_index(&view->line_table,
						     desired_successor);
		    lpo_is_line_start =
			(desired_successor == line_seq[lpo_lt_index]);
		    if (lpo_is_line_start) {
			/* Try RasterOp fixup */
			switch (ev_scroll_view_up_after_edit(
			    view, min_insert_pos, delta,
			    lt_index, lpo_lt_index)) {
			  case 1:
			    break;	/* Have to do it the hard way. */
			  default:
			    goto NextView;
			}
		    }
		}
		rect = ev_rect_for_line(view, lt_index);
		if ((delta > 0) &&
		     (lt_index+2 < view->line_table.last_plus_one)) {
		    /* On insert measure ahead to see if any of the remaining
		     *   lines in the view are still valid.
		     * Note: the line table has NOT been updated yet.
		     */
		    es_set_position(esh, line_seq[lt_index]);
		    result = ev_display_internal(
			view, &rect, lt_index, ES_INFINITY,
			EI_OP_MEASURE, EV_QUIT);
		    if ((result.break_reason & EI_PR_NEWLINE) &&
			(desired_successor == result.last_plus_one+1)) {
			/* Try RasterOp fixup */
			switch (ev_scroll_view_down_after_edit(
			    view, min_insert_pos, delta,
			    lt_index+1, lt_index)) {
			  case 1:
			    break;	/* Have to do it the hard way. */
			  default:
			    update_view_lt = FALSE;
			    break;	/* Cleanup this line */
			}
		    }
		    /* Prepare for either cleanup or full repaint. */
		    rect = ev_rect_for_line(view, lt_index);
		    result.last_plus_one = line_seq[lt_index];
		}
		/* Now update the display of the following lines.
		 * Arrange line_table so that early stop can be correctly
		 *   detected.
		 */
		if (update_view_lt) {
		    ev_update_view_lt_after_edit(
			line_seq, lt_index+1, view->line_table.last_plus_one,
			private->insert_pos, delta);
		}
		rect.r_height = view->rect.r_height -
			(rect.r_top - view->rect.r_top);
		es_set_position(esh, result.last_plus_one);
		break_action = EV_Q_LTMATCH;
		result = ev_display_internal(
		    view, &rect, lt_index, ES_INFINITY,
		    EI_OP_CLEAR_INTERIOR|EI_OP_CLEAR_BACK,
		    break_action);
		if (REACHED_END(result)) {
		    if (delta < 0)
			/* One more clean up after delete. */
			ev_clear_to_bottom(view, &rect,
			    rect_bottom(&result.bounds)+1, 0);
		}
	    } else {
		/*
		 * Although the edit had no visible effect on the view, we
		 *   still need to update its line_table to reflect the edit.
		 */
		ev_update_lt_after_edit(
			&view->line_table, last_plus_one, delta);
	    }
NextView: {}
	}
	ASSUME(allock());
}

extern void
ev_make_visible(view, position, lower_context, auto_scroll_by, delta)
	Ev_handle		view;
	Es_index		position;
	int			lower_context;
	int			auto_scroll_by;
	int			delta;
{
	extern int		ev_xy_in_view();
	Ev_impl_line_seq	line_seq;
	int			top_of_lc;
	int			lt_index;
	struct rect		rect;

	line_seq = (Ev_impl_line_seq) view->line_table.seq;
	top_of_lc = max(1,
	    view->line_table.last_plus_one - 1 - lower_context);
	/* Following test works even if line_seq[] == ES_INFINITY.
	 * The test catches the common cases and saves the expensive
	 * call on ev_xy_in_view().
	 */
	if (position < line_seq[top_of_lc])
	    return;
	switch (ev_xy_in_view(view, position, &lt_index, &rect)) {
	  case EV_XY_BELOW:
	    /* BUG ALERT: The following heuristic must be replaced! */
	    delta = min(delta, position - line_seq[top_of_lc]);
	    if (delta < (50 * view->line_table.last_plus_one)
	    &&  lower_context < view->line_table.last_plus_one - 1
	    &&  auto_scroll_by< view->line_table.last_plus_one - 1)
	    {
		Es_index		old_top = line_seq[0], new_top;
		new_top = ev_scroll_lines(view,
			min(view->line_table.last_plus_one - 1,
			max(1, 
			max(lower_context, auto_scroll_by) 
			+ (delta/50))));
		line_seq = (Ev_impl_line_seq) view->line_table.seq;
		while ((old_top != new_top) &&
		       (position >= line_seq[top_of_lc])) {
		    old_top = new_top;
		    new_top = ev_scroll_lines(view,
			(position - line_seq[top_of_lc]) > 150 ? 2 : 1);
		}
	    } else {
		ev_set_start(view, ev_line_start(view, position));
	    }
	    break;
	  case EV_XY_RIGHT:
	  case EV_XY_VISIBLE:
	    /* only scroll if at least 1 newline was inserted */
	    if ((EV_PRIVATE(view))->cached_insert_info.lt_index != lt_index)
	    {
		Es_index	new_top;

		/* We know lt_index >= top_of_lc */
		new_top = ev_scroll_lines(view,
		    min(lt_index,
		    max(lt_index - top_of_lc + 1,
		    auto_scroll_by)));
		ASSUME(new_top >= 0);
	    }
	    break;
	  default:
	    break;
	}
}

extern void
ev_scroll_if_old_insert_visible(views, insert_pos, delta)
	register Ev_chain	views;
	register Es_index	insert_pos;
	register int		delta;
{
	register Ev_handle	view;
	register Ev_pd_handle	private;
	Ev_chain_pd_handle	chain = EV_CHAIN_PRIVATE(views);

	if (delta <= 0)
	    /* BUG ALERT!  We are not handling upper_context auto_scroll. */
	    return;
	/* Scroll views which had old_insert_pos visible, but not new. */
	FORALLVIEWS(views, view) {
	    private = EV_PRIVATE(view);
	    if ((private->state & EV_VS_INSERT_WAS_IN_VIEW) == 0)
		continue /* FORALLVIEWS */;
	    ev_make_visible(view, insert_pos,
	    		    chain->lower_context, chain->auto_scroll_by,
	    		    delta);
	}
}

extern void
ev_input_before(views)
	register Ev_chain	 views;
{
	Ev_chain_pd_handle	 private = EV_CHAIN_PRIVATE(views);

	if (private->lower_context != EV_NO_CONTEXT) {
	    ev_check_insert_visibility(views);
	}
}

extern int
ev_input_partial(views, buf, buf_len)
	register Ev_chain	 views;
	char			*buf;
	long int		 buf_len;
{
	register Ev_chain_pd_handle
				 private = EV_CHAIN_PRIVATE(views);
	register Es_index	 new_insert_pos, old_insert_pos;
	int			 used;
	
	/* We cannot assume where the esh is positioned, so position first. */
	old_insert_pos = es_set_position(views->esh, private->insert_pos);
	if (old_insert_pos != private->insert_pos) {
	    return(ES_CANNOT_SET);
	}
	new_insert_pos = es_replace(views->esh, old_insert_pos, buf_len, buf,
				    &used);
	if (new_insert_pos == ES_CANNOT_SET || used != buf_len) {
	    return(ES_CANNOT_SET);
	}
	private->insert_pos = new_insert_pos;
	return(0);
}

extern int
ev_input_after(views, old_insert_pos, old_length)
	register Ev_chain	 views;
	Es_index		 old_insert_pos, old_length;
{
	Ev_chain_pd_handle	 private = EV_CHAIN_PRIVATE(views);
	Es_index		 delta = private->insert_pos - old_insert_pos;

	/* Update all of the views' data structures */
	ev_update_after_edit(
		views, old_insert_pos, delta, old_length, old_insert_pos);
	if (private->lower_context != EV_NO_CONTEXT) {
	    ev_scroll_if_old_insert_visible(views, private->insert_pos,
				 	    delta);
	}
	return(delta);
}

#ifdef EXAMPLE
extern int
ev_process_input(views, buf, buf_len)
	register Ev_chain	 views;
	char			*buf;
	int			 buf_len;
{
	Ev_chain_pd_handle	 private = EV_CHAIN_PRIVATE(views);
	Es_index		 old_length = es_get_length(views->esh);
	Es_index	 	 old_insert_pos = private->insert_pos;
	int			 delta;

	ev_input_before(views);
	delta = ev_input_partial(views, buf, buf_len);
	if (delta != ES_CANNOT_SET)
	    delta = ev_input_after(views, old_insert_pos, old_length);
	return(delta);
}
#endif

