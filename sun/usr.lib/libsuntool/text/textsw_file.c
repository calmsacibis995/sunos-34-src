#ifndef lint
static  char sccsid[] = "@(#)textsw_file.c 1.7 87/03/17";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * File load/save/store utilities for text subwindows.
 */ 
#include "primal.h"

#include "textsw_impl.h"
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <errno.h>
#include <suntool/expand_name.h>

extern char	*strcat();
extern char	*strncat();
extern char	*getwd();
extern int	 errno, sys_nerr;
extern char	*sys_errlist[];

static int		textsw_change_directory();
pkg_private void	textsw_display(), textsw_display_view_margins();
pkg_private void	textsw_input_before();
pkg_private void	textsw_init_undo(), textsw_replace_esh();
pkg_private Es_index	textsw_input_after();
extern Es_status	es_copy();

static unsigned tmtn_counter;
static
textsw_make_temp_name(in_here)
	char	*in_here;
/* *in_here must be at least MAXNAMLEN characters long */
{
	/* BUG ALERT!  Should be able to specify directory other than
	 *   /tmp.  However, if that directory is not /tmp it should
	 *   be a local (not NFS) directory and will make finding files
	 *   that need to be replayed after crash harder - assuming we
	 *   ever implement replay.
	 */
	in_here[0] = '\0';
	(void) sprintf(in_here, "%s/Text%d.%d",
		       "/tmp", getpid(), tmtn_counter++);
}

#define ES_BACKUP_FAILED	ES_CLIENT_STATUS(0)
#define ES_BACKUP_OUT_OF_SPACE	ES_CLIENT_STATUS(1)
#define ES_CANNOT_GET_NAME	ES_CLIENT_STATUS(2)
#define ES_CANNOT_OPEN_OUTPUT	ES_CLIENT_STATUS(3)
#define ES_CANNOT_OVERWRITE	ES_CLIENT_STATUS(4)
#define ES_DID_NOT_CHECKPOINT	ES_CLIENT_STATUS(5)
#define ES_PIECE_FAIL		ES_CLIENT_STATUS(6)
#define ES_SHORT_READ		ES_CLIENT_STATUS(7)
#define ES_UNKNOWN_ERROR	ES_CLIENT_STATUS(8)
#define ES_USE_SAVE		ES_CLIENT_STATUS(9)

#define TXTSW_LRSS_MASK		0x00000007

pkg_private Es_handle
textsw_create_file_ps(folio, name, scratch_name, status)
	Textsw_folio		  folio;
	char			 *name, *scratch_name;
	Es_status		 *status;
/* *scratch_name must be at least MAXNAMLEN characters long, and is modified */
{
	extern Es_handle	  es_file_create();
	register Es_handle	  original_esh, scratch_esh, piece_esh;

	original_esh = es_file_create(name, 0, status);
	if (!original_esh)
	    return(ES_NULL);
	textsw_make_temp_name(scratch_name);
	scratch_esh = es_file_create(scratch_name,
				     ES_OPT_APPEND|ES_OPT_OVERWRITE,
				     status);
	if (!scratch_esh) {
	    es_destroy(original_esh);
	    return(ES_NULL);
	}
	(void) es_set(scratch_esh, ES_FILE_MODE, 0600, 0);
	piece_esh =
	    folio->es_create(folio->client_data, original_esh, scratch_esh);
	if (piece_esh) {
	    *status = ES_SUCCESS;
	} else {
	    es_destroy(original_esh);
	    es_destroy(scratch_esh);
	    *status = ES_PIECE_FAIL;
	}
	return(piece_esh);
}

#define	TXTSW_LFI_CLEAR_SELECTIONS	0x1

pkg_private Es_status
textsw_load_file_internal(
    textsw, name, scratch_name, piece_esh, start_at, flags)
	register Textsw_folio	 textsw;
	char			*name, *scratch_name;
	Es_handle		*piece_esh;
	Es_index		 start_at;
	int			 flags;
/* *scratch_name must be at least MAXNAMLEN characters long, and is modified */
{
	Es_status		status;

	*piece_esh = textsw_create_file_ps(textsw, name,
					   scratch_name, &status);
	if (status == ES_SUCCESS) {
	    if (flags & TXTSW_LFI_CLEAR_SELECTIONS) {
		Textsw	abstract = VIEW_REP_TO_ABS(textsw->first_view);

		(void) textsw_set_selection(abstract, ES_INFINITY, ES_INFINITY,
				     EV_SEL_PRIMARY);
		(void) textsw_set_selection(abstract, ES_INFINITY, ES_INFINITY,
				     EV_SEL_SECONDARY);
	    }
	    textsw_replace_esh(textsw, *piece_esh);
	    if (start_at != ES_CANNOT_SET) {
		(void) ev_set(textsw->views->first_view,
			      EV_FOR_ALL_VIEWS,
			      EV_DISPLAY_LEVEL, EV_DISPLAY_NONE,
			      EV_DISPLAY_START, start_at,
			      EV_DISPLAY_LEVEL, EV_DISPLAY,
			      0);
		textsw_notify(textsw->first_view,
			      TEXTSW_ACTION_LOADED_FILE, name, 0);
		textsw_update_scrollbars(textsw, TEXTSW_VIEW_NULL);	      
	    }
	    
	}
	return(status);
}

pkg_private void
textsw_destroy_esh(textsw, esh)
	register Textsw_folio	textsw;
	register Es_handle	esh;
{
	Es_handle		original_esh, scratch_esh;
	
	if (textsw->views->esh == esh)
	    textsw->views->esh = ES_NULL;
	if (original_esh = (Es_handle)LINT_CAST(es_get(esh, ES_PS_ORIGINAL))) {
	    textsw_give_shelf_to_svc(textsw);
	    scratch_esh = (Es_handle)LINT_CAST(es_get(esh, ES_PS_SCRATCH));
	    es_destroy(original_esh);
	    if (scratch_esh) es_destroy(scratch_esh);
	}
	es_destroy(esh);
}

pkg_private void
textsw_replace_esh(textsw, new_esh)
	register Textsw_folio	textsw;
	Es_handle		new_esh;
/* Caller is repsonsible for actually repainting the views. */
{
	Es_handle		save_esh = textsw->views->esh;
	int			undo_count = textsw->undo_count;

	(void) ev_set(textsw->views->first_view,
		      EV_DISPLAY_LEVEL, EV_DISPLAY_NONE,
		      EV_CHAIN_ESH, new_esh,
		      0);
	textsw->state &= ~TXTSW_EDITED;
	textsw_destroy_esh(textsw, save_esh);
	/* Following two calls are inefficient textsw_re-init_undo. */
	textsw_init_undo(textsw, 0);
	textsw_init_undo(textsw, undo_count);
	textsw->state &= ~TXTSW_READ_ONLY_ESH;
	if (textsw->notify_level & TEXTSW_NOTIFY_SCROLL) {
	    register Textsw_view	view;
	    FORALL_TEXT_VIEWS(textsw, view) {
		textsw_display_view_margins(view, RECT_NULL);
	    }
	}
}

pkg_private Es_handle
textsw_create_mem_ps(folio, initial_greeting)
	Textsw_folio		 folio;
	char			*initial_greeting;
{
	Es_handle		 es_mem_create();
	register Es_handle	 original, scratch;
	Es_handle		 ps_esh = ES_NULL;

	if (initial_greeting == NULL)
	    initial_greeting = "";
	original = es_mem_create((unsigned)strlen(initial_greeting),
				 initial_greeting);
	if (original == ES_NULL)
	    goto Return;
	scratch = es_mem_create(folio->es_mem_maximum, "");
	if (scratch == ES_NULL) {
	    es_destroy(original);
	    goto Return;
	}
	ps_esh = folio->es_create(folio->client_data, original, scratch);
	if (ps_esh == ES_NULL) {
	    es_destroy(scratch);
	    es_destroy(original);
	    goto Return;
	}
Return:
	return(ps_esh);
}

/* Returns 0 iff load succeeded (can do cd instead of load). */
pkg_private int
textsw_load_selection(folio, locx, locy, no_cd)
	register Textsw_folio	folio;
	register int		locx, locy;
	int			no_cd;
{
	char			filename[MAXNAMLEN];
	register int		result;

	if (textsw_get_selection_as_filename(
			folio, filename, sizeof(filename), locx, locy)) {
	    return(-10);
	}
	result = no_cd ? -2
		 : textsw_change_directory(folio, filename, TRUE, locx, locy);
	if (result == -2) {
	    result = textsw_load_file(VIEW_REP_TO_ABS(folio->first_view),
				      filename, TRUE, locx, locy);
	    if (result == 0) {
		(void) textsw_set_insert(folio, 0L);
	    }
	}
	return(result);
}

pkg_private char *
textsw_full_pathname(name)
    register char	*name;
{
    char		 pathname[MAXPATHLEN];
    register char	*full_pathname;
    
    if (name == 0)
        return(name);
    if (*name == '/') {
        if ((full_pathname = malloc((unsigned)(1+strlen(name)))) == 0)
            return (0);
        (void) strcpy(full_pathname, name);
        return(full_pathname);
    }
    if (getwd(pathname) == 0)
        return (0);
    if ((full_pathname =
        malloc((unsigned)(2+strlen(pathname)+strlen(name)))) == 0)
        return(0);
    (void) strcpy(full_pathname, pathname);
    (void) strcat(full_pathname, "/");
    (void) strcat(full_pathname, name);
    return(full_pathname);
}

/* ARGSUSED */
pkg_private int
textsw_format_load_error(msg, status, filename, scratch_name)
	char		*msg;
	Es_status	 status;
	char		*filename;
	char		*scratch_name;	/* Currently unused */
{
	char		*full_pathname;

	switch (status) {
	  case ES_PIECE_FAIL:
	    (void) sprintf(msg, "Cannot create piece stream.");
	    break;
	  case ES_SUCCESS:
	    /* Caller is being lazy! */
	    break;
	  default:
	    full_pathname = textsw_full_pathname(filename);
	    (void) sprintf(msg, "Cannot load; ");
	    (void) es_file_append_error(msg, full_pathname, status);
	    free(full_pathname);
	    break;
	}
}

/* Returns 0 iff load succeeded. */
extern int
textsw_load_file(abstract, filename, reset_views, locx, locy)
	Textsw		 abstract;
	char		*filename;
	int		 reset_views;
	int		 locx, locy;
{
	char		 msg_buf[MAXNAMLEN+100];
	char		 scratch_name[MAXNAMLEN];
	Es_status	 status;
	Es_handle	 new_esh;
	Es_index	 start_at;
	Textsw_view	 view = VIEW_ABS_TO_REP(abstract);
	Textsw_folio	 textsw = FOLIO_FOR_VIEW(view);
	
	start_at = (reset_views) ? 0 : ES_CANNOT_SET;
	status = textsw_load_file_internal(
			textsw, filename, scratch_name, &new_esh, start_at,
			TXTSW_LFI_CLEAR_SELECTIONS);
	if (status == ES_SUCCESS) {
	    if (start_at == ES_CANNOT_SET)
		textsw_notify((Textsw_view)textsw,	/* Cast for lint */
			      TEXTSW_ACTION_LOADED_FILE, filename, 0);
	} else {
	    (void) textsw_format_load_error(msg_buf, status, filename, scratch_name);
	    textsw_post_error((Textsw_opaque)textsw, locx, locy,
			      msg_buf, (char *)0);
	}
	return(status);
}

#define RELOAD		1
#define NO_RELOAD	0
static Es_status
textsw_save_store_common(folio, output_name, reload)
	register Textsw_folio	 folio;
	char			*output_name;
	int			 reload;
{
	char			 scratch_name[MAXNAMLEN];
	Es_handle		 new_esh;
	register Es_handle	 output;
	Es_status		 result;
	Es_index		 length;

	output = es_file_create(output_name, ES_OPT_APPEND, &result);
	if (!output)
	    /* BUG ALERT!  For now, don't look at result. */
	    return(ES_CANNOT_OPEN_OUTPUT);
	length = es_get_length(folio->views->esh);
	result = es_copy(folio->views->esh, output, TRUE);
	if (result == ES_SUCCESS) {
	    es_destroy(output);
	    if (folio->checkpoint_name) {
		if (unlink(folio->checkpoint_name) == -1) {
		    perror("removing checkpoint file:");
		}
		free(folio->checkpoint_name);
		folio->checkpoint_name = NULL;
	    }
	    if (reload) {
		result = textsw_load_file_internal(
		    folio, output_name, scratch_name, &new_esh,
		    ES_CANNOT_SET, 0);
		if ((result == ES_SUCCESS) &&
		    (length != es_get_length(new_esh))) {
		    /* Added a newline - repaint to fix line tables */
		    textsw_display((Textsw)folio->first_view);
		}
	    }
	}
	return(result);
}

extern Es_status
textsw_process_save_error(abstract, error_buf, status, locx, locy)
	Textsw			 abstract;
	char			*error_buf;
	Es_status	 	 status;
	int			 locx, locy;
{
	char			*msg;
	Textsw_view		 view = VIEW_ABS_TO_REP(abstract);

	switch (status) {
	  case ES_BACKUP_FAILED:
	    msg = "Save aborted because cannot backup file: ";
	    goto PostError;
	  case ES_BACKUP_OUT_OF_SPACE:
	    msg = "Save aborted because no space for backup file: ";
	    goto PostError;
	  case ES_CANNOT_OPEN_OUTPUT:
	    msg = "Save aborted because cannot re-write file: ";
	    goto PostError;
	  case ES_CANNOT_GET_NAME:
	    msg = "Save aborted because forgot the name of the file: ";
	    goto PostError;
	  case ES_UNKNOWN_ERROR:	/* Fall through */
	  default:
	    goto InternalError;
	}
InternalError:
	msg = "Save failed due to INTERNAL ERROR.";
PostError:
	textsw_post_error((Textsw_opaque)FOLIO_FOR_VIEW(view), locx, locy,
			  msg, error_buf);
	return(ES_UNKNOWN_ERROR);
}

/* ARGSUSED */
static Es_status
textsw_save_internal(textsw, error_buf, locx, locy)
	register Textsw_folio	 textsw;
	char			*error_buf;
	int			 locx, locy;	/* Currently unused */
{
	extern Es_handle	 es_file_make_backup();
	char			 original_name[MAXNAMLEN], *name;
	register char		*msg;
	Es_handle		 backup, original = ES_NULL;
	int			 status;
	Es_status		 es_status;

	if (textsw_file_name(textsw, &name) != 0)
	    return(ES_CANNOT_GET_NAME);
	(void) strcpy(original_name, name);
	original = (Es_handle)LINT_CAST(es_get(textsw->views->esh,
						ES_PS_ORIGINAL));
	if (!original) {
	    msg = "es_ps_original";
	    goto Return_Error_Status;
	}
	if ((backup = es_file_make_backup(original, "%s%%", &es_status))
	    == ES_NULL) {
	    return(((es_status == ES_CHECK_ERRNO) && (errno = ENOSPC))
		   ? ES_BACKUP_OUT_OF_SPACE
		   : ES_BACKUP_FAILED);
	}
	(void) es_set(textsw->views->esh,
	       ES_STATUS_PTR, &es_status,
	       ES_PS_ORIGINAL, backup,
	       0);
	if (es_status != ES_SUCCESS) {
	    (void) es_destroy(backup);
	    status = (int)es_status;
	    msg = "ps_replace_original";
	    goto Return_Error_Status;
	}
	switch (status =
		textsw_save_store_common(textsw, original_name, RELOAD)) {
	  case ES_SUCCESS:
	    (void) es_destroy(original);
	    textsw_notify(textsw->first_view,
			  TEXTSW_ACTION_LOADED_FILE, original_name, 0);
	    return(ES_SUCCESS);
	  case ES_CANNOT_OPEN_OUTPUT:
	    if (errno == EACCES)
		goto Return_Error;
	    msg = "es_file_create";
	    goto Return_Error_Status;
	  default:
	    msg = "textsw_save_store_common";
	    break;
	}
Return_Error_Status:
	(void) sprintf(error_buf, "  %s; status = 0x%x", msg, status);
	status = ES_UNKNOWN_ERROR;
Return_Error:
	if (original)
	    (void) es_set(textsw->views->esh,
			  ES_STATUS_PTR, &es_status,
			  ES_PS_ORIGINAL, original,
			  0);
	return(status);
}

extern Es_status
textsw_save(abstract, locx, locy)
	Textsw		abstract;
	int		locx, locy;
{
	char		error_buf[MAXNAMLEN+100];
	Es_status	status;
	Textsw_view	view = VIEW_ABS_TO_REP(abstract);

	error_buf[0] = '\0';
	status = textsw_save_internal(FOLIO_FOR_VIEW(view), error_buf,
				      locx, locy);
	if (status != ES_SUCCESS)
	    status = textsw_process_save_error(
				abstract, error_buf, status, locx, locy);
	return(status);
}

static Es_status
textsw_get_from_fd(view, fd)
	register Textsw_view	view;
	int			fd;
{
	Textsw_folio		folio = FOLIO_FOR_VIEW(view);
	int			record;
	Es_index		old_insert_pos, old_length;
	register long		count;
	char			buf[2096];

	(void) textsw_flush_caches(view, TFC_PD_SEL);		/* Changes length! */
	textsw_input_before(view, &old_insert_pos, &old_length);
	for (;;) {
	    count = read(fd, buf, sizeof(buf)-1);
	    if (count == 0)
		break;
	    if (count < 0) {
		return(ES_UNKNOWN_ERROR);
	    }
	    buf[count] = '\0';
	    (void) textsw_input_partial(view, buf, count);
	}
	record = (TXTSW_DO_AGAIN(folio) &&
		  ((folio->func_state & TXTSW_FUNC_AGAIN) == 0) );
	(void) textsw_input_after(view, old_insert_pos, old_length, record);
	return(ES_SUCCESS);
}

pkg_private int
textsw_cd(textsw, locx, locy)
	Textsw_folio	 textsw;
	int		 locx, locy;
{
	char		 buf[MAXNAMLEN];

	if (0 == textsw_get_selection_as_filename(
			textsw, buf, sizeof(buf), locx, locy)) {
	    (void) textsw_change_directory(textsw, buf, FALSE, locx, locy);
	}
	return;
}

pkg_private int
textsw_file_stuff(view, locx, locy)
	Textsw_view	 view;
	int		 locx, locy;
{
	Textsw_folio	 folio = FOLIO_FOR_VIEW(view);
	int		 fd;
	char		 buf[MAXNAMLEN], msg[MAXNAMLEN+100], *sys_msg;
	Es_status	 status;
	int		 cannot_open = 0;

	if (0 == textsw_get_selection_as_filename(
			folio, buf, sizeof(buf), locx, locy)) {
	    if ((fd = open(buf, 0)) < 0) {
		cannot_open = (fd == -1);
		goto InternalError;
	    };
	    errno = 0;
	    status = textsw_get_from_fd(view, fd);
	    (void) close(fd);
	    if (status != ES_SUCCESS)
		goto InternalError;
	}
	return;
InternalError:
	if (cannot_open) {
	   char		*full_pathname;
	   
	   full_pathname = textsw_full_pathname(buf);
	   (void) sprintf(msg, "'%s': ", full_pathname);
	   free(full_pathname);
	} else 
	   (void) sprintf(msg, "%s", "Stuff from File failed due to INTERNAL ERROR: ");
	sys_msg = (errno > 0 && errno < sys_nerr) ? sys_errlist[errno] : NULL;
	textsw_post_error((Textsw_opaque)folio, locx, locy, msg, sys_msg);
}

pkg_private Es_status
textsw_store_init(textsw, filename)
	Textsw_folio	 textsw;
	char		*filename;
{
	struct stat	 stat_buf;

	if (stat(filename, &stat_buf) == 0) {
	    Es_handle	 original = (Es_handle)LINT_CAST(
	    			  es_get(textsw->views->esh, ES_PS_ORIGINAL));
	    if AN_ERROR(original == ES_NULL) {
		return(ES_CANNOT_GET_NAME);
	    }
	    switch ((Es_enum)es_get(original, ES_TYPE)) {
	      case ES_TYPE_FILE:
		if (es_file_copy_status(original, filename) != 0)
		    return(ES_USE_SAVE);
		/* else fall through */
	      default:
		if ((stat_buf.st_size > 0) &&
		    (textsw->state & TXTSW_CONFIRM_OVERWRITE))
		    return(ES_CANNOT_OVERWRITE);
		break;
	    }
	} else if (errno != ENOENT) {
	    return(ES_CANNOT_OPEN_OUTPUT);
	}
	return(ES_SUCCESS);
}

/* ARGSUSED */
pkg_private Es_status
textsw_process_store_error(textsw, filename, status, locx, locy)
	Textsw_folio	 textsw;
	char		*filename;	/* Currently unused */
	Es_status	 status;
	int		 locx, locy;
{
	char		*msg;
	Event		 event;

	switch (status) {
	  case ES_SUCCESS:
	    LINT_IGNORE(ASSUME(0));
	    return(ES_UNKNOWN_ERROR);
	  case ES_CANNOT_GET_NAME:
	    msg = "Cannot remember the name of the file: Store aborted.";
	    goto PostError;
	  case ES_CANNOT_OPEN_OUTPUT:
	    msg = "Problems accessing specified file: Store aborted.";
	    goto PostError;
	  case ES_CANNOT_OVERWRITE:
	    (void) textsw_menu_prompt(textsw->first_view, &event, locx, locy,
		"The file already exists and has data in it.  Click the left \
button to confirm the Store, the right button to Abort.");
	    return(event.ie_code == MS_LEFT ? ES_SUCCESS : ES_UNKNOWN_ERROR);
	  case ES_FLUSH_FAILED:
	  case ES_FSYNC_FAILED:
	  case ES_SHORT_WRITE:
	    msg = "File system full: Store aborted.";
	    goto PostError;
	  case ES_USE_SAVE:
	    msg = "Cannot Store onto the current file, use Save instead.";
	    goto PostError;
    	  case ES_UNKNOWN_ERROR:	/* Fall through */
	  default:
	    goto InternalError;
	}
InternalError:
	msg = "Store failed due to INTERNAL ERROR.";
PostError:
	textsw_post_error((Textsw_opaque)textsw, locx, locy, msg, (char *)0);
	return(ES_UNKNOWN_ERROR);
}

extern Es_status
textsw_store_file(abstract, filename, locx, locy)
	Textsw			 abstract;
	char			*filename;
	int			 locx, locy;
{
	register Es_status	 status;
	Textsw_view		 view = VIEW_ABS_TO_REP(abstract);
	register Textsw_folio	 textsw = FOLIO_FOR_VIEW(view);

	status = textsw_store_init(textsw, filename);
	switch (status) {
	  case ES_SUCCESS:
	    break;
	  case ES_USE_SAVE:
	    if (textsw->state & TXTSW_STORE_SELF_IS_SAVE) {
		return(textsw_save(abstract, locx, locy));
	    }
	    /* Fall through */
	  default:
	    status = textsw_process_store_error(
				textsw, filename, status, locx, locy);
	    break;
	}
	if (status == ES_SUCCESS) {
	    status = textsw_save_store_common(textsw, filename,
	    			(textsw->state & TXTSW_STORE_CHANGES_FILE)
	    			? TRUE : FALSE);
	    if (status == ES_SUCCESS) {
		if (textsw->state & TXTSW_STORE_CHANGES_FILE)
		    textsw_notify(textsw->first_view,
		    		  TEXTSW_ACTION_LOADED_FILE, filename, 0);
	    } else {
		status = textsw_process_store_error(
				textsw, filename, status, locx, locy);
	    }
	}
	return(status);
}

pkg_private Es_status
textsw_store_to_selection(textsw, locx, locy)
	Textsw_folio		textsw;
	int			locx, locy;
{
	char			filename[MAXNAMLEN];

	if (textsw_get_selection_as_filename(
		textsw, filename, sizeof(filename), locx, locy))
	    return(ES_CANNOT_GET_NAME);
	return (textsw_store_file(VIEW_REP_TO_ABS(textsw->first_view),
				  filename, locx, locy));
}

/* ARGSUSED */
extern void
textsw_reset(abstract, locx, locy)
	Textsw		 abstract;
	int		 locx, locy;	/* Currently ignored */
{
	static Es_status textsw_checkpoint_internal();
	Es_handle	 piece_esh;
	char		*name, save_name[MAXNAMLEN], scratch_name[MAXNAMLEN];
	int		 status;
	Textsw_folio	 folio = FOLIO_FOR_VIEW(VIEW_ABS_TO_REP(abstract));

	if (textsw_has_been_modified(abstract) &&
	    (status = textsw_file_name(folio, &name)) == 0) {
	    /* Have edited a file, so reset to the file, not memory. */
	    /* First take a checkpoint, so recovery is possible. */
	    if (folio->checkpoint_frequency > 0) {
		if (textsw_checkpoint_internal(folio) == ES_SUCCESS) {
		    folio->checkpoint_number++;
		}
	    }
	    (void) strcpy(save_name, name);
	    status = textsw_load_file_internal(folio, save_name, scratch_name,
			&piece_esh, 0L, TXTSW_LFI_CLEAR_SELECTIONS);
			/*
			 * It would be nice to preserve the old positioning of
			 *   the views, but all of the material in a view may
			 *   be either new or significantly rearranged.
			 * One possiblity is to get the piece_stream to find
			 *   the nearest original stream position and use that
			 *   if we add yet another hack into ps_impl.c!
			 */
	    if (status == ES_SUCCESS)
		goto Return;
	    /* BUG ALERT: a few diagnostics might be appreciated. */
	}
	piece_esh = textsw_create_mem_ps(folio, (char *)0);
	if (piece_esh != ES_NULL) {
	    (void) textsw_set_selection(abstract, ES_INFINITY, ES_INFINITY,
				 EV_SEL_PRIMARY);
	    (void) textsw_set_selection(abstract, ES_INFINITY, ES_INFINITY,
				 EV_SEL_SECONDARY);
	    textsw_replace_esh(folio, piece_esh);
	    (void) ev_set(folio->views->first_view,
			  EV_FOR_ALL_VIEWS,
			  EV_DISPLAY_LEVEL, EV_DISPLAY_NONE,
			  EV_DISPLAY_START, 0,
			  EV_DISPLAY_LEVEL, EV_DISPLAY,
			  0);
			  
	    textsw_update_scrollbars(folio, TEXTSW_VIEW_NULL);
	    
	    textsw_notify(folio->first_view, TEXTSW_ACTION_USING_MEMORY, 0);
	}
Return:
	(void) textsw_set_insert(folio, 0L);
}

pkg_private int
textsw_filename_is_all_blanks(filename)
	char		*filename;
{
	int		i = 0;
	
	while ((filename[i] == ' ') || (filename[i] == '\t')|| (filename[i] == '\n'))
		i++;
	return(filename[i] == '\0');	
}


/* Returns 0 iff a selection exists and it is matched by exactly one name. */
/* ARGSUSED */
pkg_private int
textsw_expand_filename(textsw, buf, sizeof_buf, locx, locy)
	Textsw_folio	 textsw;
	char		*buf;
	int		 sizeof_buf;	/* BUG ALERT!  Being ignored. */
	int		 locx, locy;
{
	struct namelist	*nl;
	char		*msg, *msg_extra = 0;

	nl = expand_name(buf);
	if ((buf[0] == '\0') || (nl == NONAMES)) {
	    msg = "Unable to expand specified pattern: ";
	    msg_extra = buf;
	    goto PostError;
	}
	
	if (textsw_filename_is_all_blanks(buf)) {
	   msg = "Filname contains only blank or tab characters.  Please input a valid filename.";
	   goto PostError;
	}
	/*
	 * Below here we have dynamically allocated memory pointed at by nl
	 *   that we must make sure gets deallocated.
	 */
	if (nl->count == 0) {
	    msg = "No files match specified pattern: ";
	    msg_extra = buf;
	} else if (nl->count > 1) {
	    msg = "Too many files match specified pattern: ";
	    msg_extra = buf;
	    goto PostError;
	} else
	    (void) strcpy(buf, nl->names[0]);
	free_namelist(nl);
	if (msg_extra != 0)
	    goto PostError;
	return(0);
PostError:
	textsw_post_error((Textsw_opaque)textsw, locx, locy, msg, msg_extra);
	return(1);
}

/* Returns 0 iff there is a selection, and it is matched by exactly one name. */
pkg_private int
textsw_get_selection_as_filename(textsw, buf, sizeof_buf, locx, locy)
	Textsw_folio		 textsw;
	char			*buf;
	int			 sizeof_buf, locx, locy;
{
	char			*msg;

	if (!textsw_get_selection_as_string(textsw, EV_SEL_PRIMARY,
					    buf, sizeof_buf)) {
	    msg = "After removing this message, please select the desired filename and choose this menu option again.";
	    goto PostError;
	}
	return(textsw_expand_filename(textsw, buf, sizeof_buf, locx, locy));
PostError:
	textsw_post_error((Textsw_opaque)textsw, locx, locy, msg, (char *)0);
	return(1);
}

pkg_private int
textsw_possibly_edited_now_notify(textsw)
	Textsw_folio	textsw;
{
	char		*name;

	if (textsw_has_been_modified(VIEW_REP_TO_ABS(textsw->first_view))) {
	    if (textsw_file_name(textsw, &name) == 0) {
		textsw_notify(textsw->first_view,
			      TEXTSW_ACTION_EDITED_FILE, name, 0);
	    }
	    textsw->state |= TXTSW_EDITED;
	}
}

extern int
textsw_has_been_modified(abstract)
	Textsw		abstract;
{
	Textsw_folio	folio = FOLIO_FOR_VIEW(VIEW_ABS_TO_REP(abstract));

	if (folio->state & TXTSW_INITIALIZED) {
	    return((int)es_get(folio->views->esh, ES_HAS_EDITS));
	}
	return(0);
}

pkg_private int
textsw_file_name(textsw, name)
	Textsw_folio	  textsw;
	char		**name;
/* Returns 0 iff editing a file and name could be successfully acquired. */
{
	Es_handle	  original;

	original = (Es_handle)LINT_CAST(
		   es_get(textsw->views->esh, ES_PS_ORIGINAL));
	if (original == 0)
	    return(1);
	if ((Es_enum)es_get(original, ES_TYPE) != ES_TYPE_FILE)
	    return(2);
	if ((*name = (char *)es_get(original, ES_NAME)) == NULL)
	    return(3);
	if (name[0] == '\0')
	    return(4);
	return(0);
}

extern int
textsw_append_file_name(abstract, name)
	Textsw		 abstract;
	char		*name;
/* Returns 0 iff editing a file and name could be successfully acquired. */
{
	Textsw_folio	 textsw = FOLIO_FOR_VIEW(VIEW_ABS_TO_REP(abstract));
	char		*internal_name;
	int		 result;

	result = textsw_file_name(textsw, &internal_name);
	if (result == 0)
	    (void) strcat(name, internal_name);
	return(result);
}

pkg_private void
textsw_post_error(folio_or_view, locx, locy, msg1, msg2)
	Textsw_opaque	 folio_or_view;
	int		 locx, locy;
	char		*msg1, *msg2;
{
	char		 buf[MAXNAMLEN+1000],
			*trailer = "  (Click any button to remove msg.)";
	int		 size_to_use = sizeof(buf)-sizeof(*trailer);
	Event		 event;
	Textsw_view	 view;

	buf[0] = '\0';
	(void) strncat(buf, msg1, size_to_use);
	if (msg2) {
	    int	len = strlen(buf);
	    if (len < size_to_use) {
		(void) strncat(buf, msg2, size_to_use-len);
	    }
	}
	(void) strcat(buf, trailer);
	view = VIEW_FROM_FOLIO_OR_VIEW(folio_or_view);
	(void) textsw_menu_prompt(view, &event, locx, locy, buf);
}


/*	===================================================================
 *
 *	Misc. file system manipulation utilities
 *
 *	===================================================================
 */

/* Returns 0 iff change directory succeeded. */
static int
textsw_change_directory(textsw, filename, might_not_be_dir, locx, locy)
	Textsw_folio	 textsw;
	char		*filename;
	int		 might_not_be_dir;
	int		 locx, locy;
{
	char		*sys_msg;
	char		*full_pathname;
	char		 msg[MAXNAMLEN+100];
	struct stat	 stat_buf;
	int		 result = 0;

	errno = 0;
	if (stat(filename, &stat_buf) < 0) {
	    result = -1;
	    goto Error;
	}
	if ((stat_buf.st_mode&S_IFMT) != S_IFDIR) {
	    if (might_not_be_dir)
		return(-2);
	}
	if (chdir(filename) < 0) {
	    result = errno;
	    goto Error;
	}
	textsw_notify((Textsw_view)textsw,	/* Cast is for lint */
		      TEXTSW_ACTION_CHANGED_DIRECTORY, filename, 0);
	return(result);

Error:
	full_pathname = textsw_full_pathname(filename);
	(void) sprintf(msg, "Cannot %s '%s': ",
		       (might_not_be_dir ? "access file" : "cd to directory"),
		       full_pathname);
	free(full_pathname);
	sys_msg = (errno > 0 && errno < sys_nerr) ? sys_errlist[errno] : NULL;
	textsw_post_error((Textsw_opaque)textsw, locx, locy, msg, sys_msg);
	return(result);
}

pkg_private Es_status
textsw_checkpoint_internal(folio)
	Textsw_folio	folio;
{
	Es_handle		 cp_file;
	Es_status		 result;
	
	if (!folio->checkpoint_name) {
	    char	*name;
	    if (textsw_file_name(folio, &name) != 0)
		return(ES_CANNOT_GET_NAME);
	    if ((folio->checkpoint_name = (char *)malloc(MAXNAMLEN)) == 0)
		return(ES_CANNOT_GET_NAME);
	    (void) sprintf(folio->checkpoint_name, "%s%%%%", name);
	}

	cp_file = es_file_create(folio->checkpoint_name,
				 ES_OPT_APPEND, &result);
	if (!cp_file) {
	    /* BUG ALERT!  For now, don't look at result. */
	    return(ES_CANNOT_OPEN_OUTPUT);
	}
	result = es_copy(folio->views->esh, cp_file, TRUE);
	es_destroy(cp_file);

	return(result);
}


/*
 *  If the number of edits since the last checkpoint has exceeded the
 *  checkpoint frequency, take a checkpoint.
 *  Return ES_SUCCESS if we do the checkpoint.
 */
pkg_private Es_status
textsw_checkpoint(folio)
	Textsw_folio	folio;
{
	int		edit_number = (int)LINT_CAST(
					ev_get((Ev_handle)folio->views,
						  EV_CHAIN_EDIT_NUMBER));
	Es_status	result = ES_DID_NOT_CHECKPOINT;

	if (folio->state & TXTSW_IN_NOTIFY_PROC ||
	    folio->checkpoint_frequency <= 0)
	    return(result);
	if ((edit_number / folio->checkpoint_frequency)
	                > folio->checkpoint_number) {
	    /* do the checkpoint */
	    result = textsw_checkpoint_internal(folio);
	    if (result == ES_SUCCESS) {
	        folio->checkpoint_number++;
	    }
	}
	return(result);
}

#if defined(DEBUG) && !defined(lint)
static char           header[] = "fd      dev: #, type    inode\n";
static void
debug_dump_fds(stream)
	FILE                 *stream;
{
	register int          fd;
	struct stat           s;

	if (stream == 0)
	    stream = stderr;
	fprintf(stream, header);
	for (fd = 0; fd < 32; fd++) {
            if (fstat(fd, &s) != -1) {
        	fprintf(stream, "%2d\t%6d\t%4d\t%5d\n",
        		fd, s.st_dev, s.st_rdev, s.st_ino);
            }
	}
}
#endif

