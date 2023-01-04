#ifndef lint
static  char sccsid[] = "@(#)es_file.c 1.6 87/01/07";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Entity stream implementation for disk files.
 *
 * Considerations for file consistency:
 *	Sun Unix 3.X (and BSD 4.X):
 * For a local file system, write(2) does not report success unless there is
 * space available on the disk for the data and write(2) claims that space.
 * For a NFS file system, write(2) returns as soon as it has transferred all
 * of the user data into kernel buffers.  There need not be enough space for
 * that data on the remote disk, and only a successful fsync(2) guarantees
 * that the data is on the remote disk.  If the fsync(2) fails, there is no
 * indication of which data did not make it to the disk!
 *	Sun Unix 4.X (the VM-rewrite):
 * With mapped files, the local file system may have the same problem as the
 * NFS file system.
 *	AT&T System V R ?
 * There is a notion of "synchronous" files, where the write(2) does not
 * report success until the data is on the disk.
 *	stdio
 * fwrite(3) is not guarantee to write(2) unless the stream is unbuffered.
 * fflush(3) forces the write(2), but does not provide enough information to
 * caller for it to figure out what did not get written.  Worse yet, fseek(3)
 * can call fflush(3) as a side-effect, and does not even report an error
 * if the fflush(3) fails!
 * To get around the delayed nature of the calls to write(2), we force
 * WRITE_BUF_LEN > BUFSIZE, forcing all full-buffer fwrite(3) calls to call
 * write(2) immediately.  (We don't just want to make the stream unbuffered
 * because we need the buffering for reading).  Since the only remaining
 * partial calls to fwrite(3) should be in es_commit and es_destroy, this
 * reduces the number of places that have to be very careful about disk
 * consistency to entity_stream shutdown and es_replace callers.
 */

#include <strings.h>
#include <varargs.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <stdio.h>
#include "primal.h"
#include "entity_stream.h"

extern int	 	 errno, sys_nerr;
extern char		*sys_errlist[];
extern char		*malloc(), *sprintf();
extern long		 lseek();

static Es_status	es_file_commit();
static Es_handle	es_file_destroy();
static caddr_t		es_file_get();
static Es_index		es_file_get_length();
static Es_index		es_file_get_position();
static Es_index		es_file_set_position();
static Es_index		es_file_read();
static Es_index		es_file_replace();
static int		es_file_set();

static struct es_ops es_file_ops = {
	es_file_commit,
	es_file_destroy,
	es_file_get,
	es_file_get_length,
	es_file_get_position,
	es_file_set_position,
	es_file_read,
	es_file_replace,
	es_file_set
};

struct private_data {
	Es_status	 status;
	char		*name;
#ifndef BACKUP_AT_HEAD_OF_LINK
	char		*true_name;	 /* Non-null iff name was sym link */
#endif
	unsigned	 flags, options;
        Es_index	 length, pos;
	FILE		*file;
	unsigned 	 write_buf_used; /* Cache for replace's, see below */
	char		*write_buf;
};
typedef struct private_data *Es_file_data;
/*
 *   For a common use by the textsw, stdio interacts poorly with ps_impl.c,
 * because fseek fflush's the writable scratch file a lot.  To get around
 * this, if write_buf is non-NULL, the last write_buf_used characters in the
 * es_file_stream are actually located in write_buf.  Note that this means
 * that positions [length-write_buf_used..length) are valid for the stream,
 * but not for the stdio FILE!
 */
#define	WRITE_BUF_LEN	(BUFSIZ*2)
	/* Bits for flags */
#define COMMIT_DONE	0x00000001

#define	ABS_TO_REP(esh)	(Es_file_data)LINT_CAST(esh->data)

extern int
es_file_append_error(error_buf, file_name, status)
	char		*error_buf, *file_name;
	Es_status	 status;
{
/* Messages appended to error_buf have no trailing newline */
	if (error_buf == 0)
	    return;			/* Caller is fouled up. */
	if (status & ES_CLIENT_STATUS(0)) {
	    (void) sprintf(error_buf+strlen(error_buf),
			   "INTERNAL error for file '%s', status is %X",
			   file_name, status);
	    return;
	}
	switch (ES_BASE_STATUS(status)) {
	  case ES_SUCCESS:
	    break;			/* Caller is REALLY lazy! */
	  case ES_CHECK_ERRNO:
	    switch (errno) {
	      case ENOENT:
		(void) sprintf(error_buf+strlen(error_buf),
				"'%s' does not exist", file_name);
		break;
	      case EACCES:
		(void) sprintf(error_buf+strlen(error_buf),
				"not permitted to access '%s'", file_name);
		break;
	      case EISDIR:
		(void) sprintf(error_buf+strlen(error_buf),
				"'%s' is not a file of ASCII text", file_name);
		break;
	      case ELOOP:
		(void) sprintf(error_buf+strlen(error_buf),
				"too many symbolic links from '%s'",
				file_name);
		break;
	      case ENOMEM:
		(void) strcat(error_buf, "alloc failure");
		break;
	      default:
		if (errno <= 0 || errno >= sys_nerr)
		    goto Default;
		(void) sprintf(error_buf+strlen(error_buf),
				"file '%s': ", file_name, sys_errlist[errno]);
		break;
	    }
	    break;
	  case ES_INVALID_HANDLE:
	    (void) strcat(error_buf, "invalid es_handle");
	    break;
	  case ES_SEEK_FAILED:
	    (void) strcat(error_buf, "seek failed");
	    break;
	  case ES_FLUSH_FAILED:
	  case ES_FSYNC_FAILED:
	  case ES_SHORT_WRITE:
	    (void) sprintf(error_buf+strlen(error_buf),
			   "out of space for file '%s'", file_name);
	    break;
	  default:
Default:
	    (void) sprintf(error_buf+strlen(error_buf),
			   "cannot read file '%s'", file_name);
	}
}

Es_handle
es_file_create(name, options, status)
	char			*name;
	int			 options;
	Es_status		*status;
{
	extern char		*calloc(), *malloc();
	extern			 fstat();
	Es_handle		 esh = NEW(Es_object);
	register Es_file_data	 private = NEW(struct private_data);
	char			*fopen_option;
	struct stat		 buf;
	Es_status		 dummy_status;
#ifndef BACKUP_AT_HEAD_OF_LINK
	char			*temp_name, true_name[MAXNAMLEN];
	int			 link_count, true_name_len;
#endif

	if (status == 0)
	    status = &dummy_status;
	*status = ES_CHECK_ERRNO;
	errno = 0;
	if ((esh == NULL) || (private == NULL))
	    goto AllocFailed;
#ifndef BACKUP_AT_HEAD_OF_LINK
	/* Chase the symbolic link if 'name' is one. */
	for (temp_name = name, link_count = 0;
	     (link_count < MAXSYMLINKS) &&
	     (-1 != (true_name_len =
		readlink(temp_name, true_name, sizeof(true_name)) ));
	    temp_name = true_name, link_count++) {
	    true_name[true_name_len] = '\0';
	}
	if (link_count == MAXSYMLINKS) {
	    errno = ELOOP;
	    goto Done;
	}
	if (temp_name == name) {
	    private->true_name = NULL;
	} else
	    private->true_name = strdup(true_name);
#endif
	fopen_option = (options & ES_OPT_APPEND) ? "w+" : "r";
	private->file = fopen(name, fopen_option);
	if (private->file == NULL) {
            goto Done;
	}
	private->flags = 0;
	private->options = options;
	if ((private->options & ES_OPT_APPEND) == 0) {
	    if (fstat(fileno(private->file), &buf) == -1)
		goto Done;
	    if ((buf.st_mode & S_IFMT) != S_IFREG) {
		errno = EISDIR;
		goto Done;
	    }
	    private->length = buf.st_size;
	}
	private->name = strdup(name);
	private->write_buf_used = 0;
	private->write_buf =
	    (options & ES_OPT_APPEND) ? malloc(WRITE_BUF_LEN) : NULL;
	esh->ops = &es_file_ops;
	esh->data = (caddr_t)private;
	*status = private->status = ES_SUCCESS;
	return(esh);

AllocFailed:
	errno = ENOMEM;
Done:
	if (esh) {
	    free((char *)esh); esh = ES_NULL;
	}
	if (private) {
            if (private->file)
		(void) fclose(private->file);
	    free((char *)private); private = (Es_file_data)0;
	}
	return(esh);
}

/* ARGSUSED */
static caddr_t
es_file_get(esh, attribute, va_alist)
	Es_handle		esh;
	Es_attribute		attribute;
	va_dcl
{
	register Es_file_data	private = ABS_TO_REP(esh);
#ifndef lint
	va_list			args;
#endif
	switch (attribute) {
	  case ES_NAME:
	    return((caddr_t)(private->name));
	  case ES_STATUS:
	    return((caddr_t)(private->status));
	  case ES_SIZE_OF_ENTITY:
	    return((caddr_t)sizeof(char));
	  case ES_TYPE:
	    return((caddr_t)ES_TYPE_FILE);
	  default:
	    return(0);
	}
}

static int
es_file_set(esh, attrs)
	Es_handle	esh;
	Attr_avlist	attrs;
{
	register Es_file_data	 private = ABS_TO_REP(esh);
	Es_status		 status_dummy = ES_SUCCESS;
	register Es_status	*status = &status_dummy;

	for (; *attrs && (*status == ES_SUCCESS); attrs = attr_next(attrs)) {
	    switch ((Es_attribute)*attrs) {
	      case ES_FILE_MODE:
		if (fchmod(fileno(private->file), (int)attrs[1]) == -1)
		    *status = private->status = ES_CHECK_ERRNO;
		break;
	      case ES_STATUS:
		private->status = (Es_status)attrs[1];
		break;
	      case ES_STATUS_PTR:
		status = (Es_status *)LINT_CAST(attrs[1]);
		*status = status_dummy;
		break;
	      default:
		*status = ES_INVALID_ATTRIBUTE;
		break;
	    }
	}
	return((*status == ES_SUCCESS));
}

/* ARGSUSED */
static int
es_file_fseek(private, pos, caller)
        register Es_file_data	 private;
        Es_index		 pos;
        char			*caller;
{
	clearerr(private->file);
	if (fseek(private->file, pos, 0) == -1) {
	    private->status = ES_SEEK_FAILED;
#ifdef DEBUG
	    (void) fprintf(stderr, "Bad fseek in %s to position %d\n",
			   caller, pos);
#endif
	    return(0);
	} else if ferror(private->file) {
	    private->status = ES_FLUSH_FAILED;
#ifdef DEBUG
	    (void) fprintf(stderr, "fflush failed during fseek in %s\n",
			   caller);
	    LINT_IGNORE(ASSUME(0));
#endif
	    return(0);
	} else {
	    return(1);
	}
}

static int
es_file_flush_cache(esh, force_to_disk)
        Es_handle	esh;
        int		force_to_disk;
/*
 * Caller must guarantee:
 *	((private->write_buf != NULL) && (private->write_buf_used > 0))
 * Return values <= 0 indicate various errors.
 */
{
	register Es_file_data	private = ABS_TO_REP(esh);
	register int		written;
	int			length_in_file;

	length_in_file = private->length - private->write_buf_used;
        if ((written = fseek(private->file, (long)length_in_file, 0))
            == -1) {
            private->status = ES_SEEK_FAILED;
#ifdef DEBUG
	    (void) fprintf(stderr,
			   "Bad fseek in es_file_flush_cache to position %d\n",
			   length_in_file);
#endif
        } else {
	    written = fwrite(private->write_buf, sizeof(char),
			     (int)private->write_buf_used, private->file);
	    if (written) {
		private->write_buf_used -= written;
		if (private->write_buf_used > 0) {
		    /* Partial write: update private->write_buf contents. */
		    bcopy(private->write_buf+written, private->write_buf,
			  (int)private->write_buf_used);
		    written = -2;
		    private->status = ES_SHORT_WRITE;
		} else if (force_to_disk) {
		    if (fflush(private->file) == EOF) {
			private->status = ES_FLUSH_FAILED;
			/* BUG ALERT!  Who knows what state stdio has left
			 *	the file in?
			 */
			written = -3;
		    } else if (fsync(fileno(private->file)) == -1) {
			private->status = ES_FSYNC_FAILED;
			/* BUG ALERT!  Who knows what state stdio and the
			 *	kernel have left the file in?
			 */
			written = -4;
		    }
		}
	    } else {
		private->status = ES_SHORT_WRITE;
	    }
	}
	return(written);
}
 
static Es_status
es_file_commit(esh)
        Es_handle esh;
{
	register Es_file_data	private = ABS_TO_REP(esh);

	if ((private->write_buf) && (private->write_buf_used > 0)) {
	    if (es_file_flush_cache(esh, TRUE) <= 0)
		return(private->status);
	}
	private->flags |= COMMIT_DONE;
	return(ES_SUCCESS);
}
 
static Es_handle
es_file_destroy(esh)
        Es_handle esh;
{
        register Es_file_data private = ABS_TO_REP(esh);

	if (private->write_buf) {
#ifdef DEBUG
	    if ((private->write_buf_used > 0) &&
		(private->flags & COMMIT_DONE)) {
		/* Caller should have called es_commit in order to guarantee
		 * appropriate recovery in case of errors.
		 */
		take_breakpoint();
	    }
#endif
	    free((char *)private->write_buf);
	}
	(void) fclose(private->file);
	if ((private->options & ES_OPT_APPEND) &&
	    (private->flags & COMMIT_DONE) == 0) {
	    (void) unlink(private->name);
	}
	free((char *)esh);
	free(private->name);
	free((char *)private);
	return(NULL);
}

static Es_index
es_file_get_length(esh)
        Es_handle esh;  
{
	register Es_file_data private = ABS_TO_REP(esh);
        return(private->length);
}
 
static Es_index
es_file_get_position(esh)
        Es_handle esh;  
{
        register Es_file_data private = ABS_TO_REP(esh);
        return(private->pos);
}
 
static Es_index
es_file_set_position(esh, pos)
        Es_handle esh;  
{
        register Es_file_data private = ABS_TO_REP(esh);
	if (pos > private->length)
		pos = private->length;
	private->pos = pos;
	return(private->pos);
} 
 
static Es_index
es_file_read(esh, count, buf, count_read)
        Es_handle		 esh;
	register int		 count;
	int			*count_read;
	caddr_t			 buf;
{
	register Es_file_data	 private = ABS_TO_REP(esh);
	register int		 length_in_file, to_read;
	char			*write_buf_offset;

	length_in_file = private->length - private->write_buf_used;
	/* The read can begin at one of three locations, depending on
	 * whether private->pos <, ==, or > length_in_file (most of the
	 * handling of > is needed for ==).
	 */
	if ((private->write_buf == NULL) ||
	    (private->pos + count <= length_in_file)) {
	    to_read = count;
	} else if (private->pos < length_in_file) {
	    to_read = length_in_file - private->pos;
	    write_buf_offset = private->write_buf;
	} else {
	    to_read = 0;
	    write_buf_offset = private->write_buf +
			       (private->pos-length_in_file);
	    *count_read = 0;
	}
	/* Get the characters that are in the file from the file */
	if (to_read > 0) {
	    if (fseek(private->file, private->pos, 0) == -1) {
#ifdef DEBUG
		(void) fprintf(stderr,
				"Bad fseek in es_file_read to position %d\n",
				private->pos);
#endif
		*count_read = 0;
		goto Return;
	    } else {
		*count_read = fread(buf, sizeof(char), count, private->file);
		private->pos += *count_read;
	    }
	}
	/* Get any remaining characters from the write_buf */
	if (to_read < count) {
	    to_read = count - to_read;
	    if (to_read > private->write_buf_used -
	    		  (private->pos-length_in_file))
		to_read = private->write_buf_used -
	    		  (private->pos-length_in_file);
	    bcopy(write_buf_offset, buf, to_read);
	    *count_read += to_read;
	    private->pos += to_read;
	}
Return:
	return(private->pos);
}

typedef enum {esfr_truncate, esfr_overwrite, esfr_insert} Esfr_mode;
 
static Es_index
es_file_replace(esh, last_plus_one, count, buf, count_used) 
        Es_handle		 esh;
        register int		 count;
        int			*count_used, last_plus_one;
        caddr_t			 buf;  
{ 
	register Es_file_data	 private = ABS_TO_REP(esh);
	register Esfr_mode	 mode;
	int			 added, length_in_file;

	/* Ensure that the operation is consistent with the options. */
	if ((private->options & ES_OPT_APPEND) == 0) {
	    private->status = ES_INCONSISTENT_POS;
#ifdef DEBUG
	    (void) fprintf(stderr, "es_file_replace: read-only stream\n");
#endif
	    goto Error_Return;
	}
	if (private->pos < private->length) {
	    if (last_plus_one < private->length) {
		mode = esfr_overwrite;
	    } else {
		mode = esfr_truncate;
		*count_used = 0;
		if (count != 0) {
		    private->status = ES_INVALID_ARGUMENTS;
#ifdef DEBUG
	    (void) fprintf(stderr,
			  "es_file_replace: non-zero (%d) count in truncate\n",
			  count);
#endif
		    goto Error_Return;
		}
	    }
	    if ((private->options & ES_OPT_OVERWRITE) == 0 ||
	        ((mode == esfr_overwrite) &&
	         (count != last_plus_one-private->pos))) {
		private->status = ES_INVALID_ARGUMENTS;
#ifdef DEBUG
		(void) fprintf(stderr, "%s last_plus_one is %d, len is %d\n",
				"es_file_replace position error:",
				last_plus_one, private->length); 
#endif
		goto Error_Return;
	    }
	} else {
	    mode = esfr_insert;
	}
	
	/* Do the replace */
	if (private->write_buf != NULL) {
	    length_in_file = private->length - private->write_buf_used;
	    added = (mode == esfr_insert) ? count : 0;
	    if ((private->pos < length_in_file) ||
		(added + private->write_buf_used > WRITE_BUF_LEN)) {
		/* Overwriting chars already in file OR too many chars
		 * to fit in cache => cannot use cache.
		 */
		if (private->write_buf_used > 0) {
		    if (mode == esfr_truncate) {
			private->write_buf_used = 0;
		    } else if (es_file_flush_cache(esh, FALSE) <= 0) {
			goto Error_Return;
		    }
		}
	    } else {
		if (mode == esfr_truncate) {
		    private->write_buf_used = private->pos - length_in_file;
		} else {
		    bcopy(buf,
			  private->write_buf+(private->pos-length_in_file),
			  count);
		    private->write_buf_used += added;
		    *count_used = count;
		}
		goto Return;
	    }
	}
	if (es_file_fseek(private, private->pos, "es_file_replace")) {
	    if (count > 0) {
		*count_used = fwrite(buf, sizeof(char),
				     count, private->file);
		if (*count_used < count) {
		    private->status = ES_SHORT_WRITE;
		    goto Error_Return;
		}
	    }
	} else {
	    goto Error_Return;
	}
Return:
	private->pos += *count_used;
	if (mode != esfr_overwrite) private->length = private->pos;
	return(private->pos); 

Error_Return:
	return(ES_CANNOT_SET); 
} 

extern int
es_file_copy_status(esh, to)
	Es_handle	 esh;
	char		*to;
{
	Es_file_data	 private = ABS_TO_REP(esh);
	int		 dummy;

	return(copy_status(to, fileno(private->file), &dummy));
}

extern Es_handle
es_file_make_backup(esh, backup_pattern, status)
	register Es_handle	 esh;
	char			*backup_pattern;
	Es_status		*status;
/* Currently backup_pattern must be of the form "%s<suffix>" */
{
	register Es_file_data	 private;
	char			 backup_name[MAXNAMLEN];
	int			 fd, len, retrying = FALSE;
	Es_status		 dummy_status;
	Es_handle		 result;

	if (status == 0)
	    status = &dummy_status;
	if ((esh == NULL) || (esh->ops != &es_file_ops)) {
	    *status = ES_INVALID_HANDLE;
	    return(NULL);
	}
	*status = ES_CHECK_ERRNO;
	errno = 0;
	private = ABS_TO_REP(esh);
#ifdef BACKUP_AT_HEAD_OF_LINK
	(void) sprintf(backup_name, backup_pattern, private->name);
#else
	(void) sprintf(backup_name, backup_pattern,
			(private->true_name) ? private->true_name
					     : private->name);
#endif
	fd = fileno(private->file);
	len = lseek(fd, 0L, 1);
	if (lseek(fd, 0L, 0) != 0)
	    goto Lseek_Failed;
Retry:
	if (copy_fd(private->name, backup_name, fd) != 0) {
	    if ((!retrying) && (errno == EACCES)) {
		/* It may be that the backup_name is already taken by a file
		 * that cannot be overwritten, so try to remove it first.
		 */
		if (unlink(backup_name) == 0) {
		    retrying = TRUE;
		    goto Retry;
		}
	    }
	    return(NULL);
	}
	if (lseek(fd, (long)len, 0) != len)
	    goto Lseek_Failed;
	result = es_file_create(backup_name, 0, status);
	*status = ES_SUCCESS;
	return(result);

Lseek_Failed:
	*status = ES_SEEK_FAILED;
	return(NULL);
}
