#ifndef lint
static  char sccsid[] = "@(#)es_util.c 1.3 87/01/07";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Utilities for use with entity streams.
 */

#include "primal.h"
#include "entity_stream.h"

/* Caller must make sure that esbuf->last_plus_one is the current position
 *   in the entity stream.
 */
es_advance_buf(esbuf)
	Es_buf_handle	esbuf;
{
	int			read = 0;
	register Es_index	next = esbuf->last_plus_one;
	while (read == 0) {
	    esbuf->first = next;
	    next = es_read(esbuf->esh, esbuf->sizeof_buf, esbuf->buf, &read);
	    esbuf->last_plus_one = esbuf->first + read;
	    if READ_AT_EOF(esbuf->first, next, read) {
		return(1);
	    }
	}
	return(0);
}

Es_index
es_backup_buf(esbuf)
	Es_buf_handle	esbuf;
{
	/* Backing the buffer up is complicated by the fact that
	 *   we don't know the size (hopefully 0) of the gap
	 *   before the current beginning of the buffer.
	 */
	register Es_index	old_first = esbuf->first;
	register Es_index	esi = old_first-1;

	while (es_make_buf_include_index(esbuf, esi, esbuf->sizeof_buf-1)) {
	    if (old_first != esbuf->first) {
		/* esi must be immediately after a gap, so take
		 *   what is available before the gap.
		 */
		esi = esbuf->last_plus_one-1;
		goto Return;
	    }
	    /* The gap is bigger than the buffer.
	     * Try moving closer to the start of the stream.
	     */
	    if (esi == 0) {
		esi = ES_CANNOT_SET;
		goto Return;
	    }
	    if (esi < esbuf->sizeof_buf-1)
		esi = 0;
	    else
		esi -= esbuf->sizeof_buf-1;
	}
Return:
	return(esi);
}

/* esbuf->first and ->last_plus_one are only adjusted when entities are
 *   actually read.  This makes it easier for callers to figure out how to
 *   correct after failure to re-position a buffer.
 * Returns 0 iff it managed to align the buffer as requested.
 */
es_make_buf_include_index(esbuf, index, desired_prior_count)
	register Es_buf_handle	esbuf;
	Es_index		index;
	int			desired_prior_count;
{
	register Es_index	last_plus_one, next;
	int			read;

	last_plus_one = (desired_prior_count > index) ? 0
			: index - desired_prior_count;
	es_set_position(esbuf->esh, last_plus_one);
	FOREVER {
	    next = es_read(esbuf->esh, esbuf->sizeof_buf, esbuf->buf, &read);
	    if READ_AT_EOF(last_plus_one, next, read)
		return(1);
	    esbuf->first = last_plus_one;
	    esbuf->last_plus_one = last_plus_one+read;
	    if (next > index) {
		if (last_plus_one+read < index) {
		    /* The desired index is in a "hole" in the entity stream's
		     *   entities.
		     */
		    return(1);
		} else
		    return(0);
	    }
	    last_plus_one = next;
	}
}

extern Es_status
es_copy(from, to, newline_must_terminate)
	register Es_handle	from, to;
	int			newline_must_terminate;
{
#define BUFSIZE 2096
	char			buf[BUFSIZE+4];
	register Es_status	result;
	int			read, write;
	Es_index		new_pos, pos;

	pos = es_set_position(from, 0);
	write = 0;
	FOREVER {
	    new_pos = es_read(from, BUFSIZE, buf, &read);
	    if (read > 0) {
		(void) es_replace(to, ES_INFINITY, read, buf, &write);
		if (write < read) {
		    result = ES_SHORT_WRITE;
		    return(result);
		}
	    } else if (pos == new_pos)
		break;
	    pos = new_pos;
	}
	if (newline_must_terminate && (
	    (write <= 0) || (write > sizeof(buf)) ||
	    (buf[write-1] != '\n') )) {
	    buf[0] = '\n';
	    (void) es_replace(to, ES_INFINITY, 1, buf, &write);
	    if (write < 1) {
		result = ES_SHORT_WRITE;
		return(result);
	    }
	}
	result = es_commit(to);
	return(result);
#undef BUFSIZE
}

