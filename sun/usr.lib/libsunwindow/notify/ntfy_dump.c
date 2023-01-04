#ifndef lint
static  char sccsid[] = "@(#)ntfy_dump.c 1.3 87/01/09 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Ntfy_dump.c - Calls to dump notifier state.
 */

#include "ntfy.h"
#include "ndet.h"
#include "ndis.h"
#include <stdio.h>		/* For output */

typedef	struct ntfy_dump_data {
	Notify_dump_type	type;
	Notify_client		nclient;
	FILE			*file;
	NTFY_CLIENT		*client_latest;
} Ntfy_dump_data;

static	NTFY_ENUM ntfy_dump();

extern void
notify_dump(nclient, type, file)
	Notify_client nclient;
	Notify_dump_type type;
	FILE *file;
{
	Ntfy_dump_data data;

	/* Set up enumeration record */
	data.nclient = nclient;
	if (file == (FILE *)1)
		file = stdout;
	if (file == (FILE *)2)
		file = stderr;
	data.file = file;

	if (type == NOTIFY_ALL || type == NOTIFY_DETECT) {
		(void) fprintf(file, "DETECTOR CONDITIONS:\n");
		data.type = NOTIFY_DETECT;
		data.client_latest = NTFY_CLIENT_NULL;
		(void) ntfy_enum_conditions(ndet_clients, ntfy_dump,
		    (NTFY_ENUM_DATA)&data);
	}
	if (type == NOTIFY_ALL || type == NOTIFY_DISPATCH) {
		(void) fprintf(file, "DISPATCH CONDITIONS:\n");
		data.type = NOTIFY_DISPATCH;
		data.client_latest = NTFY_CLIENT_NULL;
		(void) ntfy_enum_conditions(ndis_clients, ntfy_dump,
		    (NTFY_ENUM_DATA)&data);
	}
	return;
}

static NTFY_ENUM
ntfy_dump(client, cond, context)
	register NTFY_CLIENT *client;
	register NTFY_CONDITION *cond;
	NTFY_ENUM_DATA context;
{
	register Ntfy_dump_data *data = (Ntfy_dump_data *)context;

	if (data->nclient && client->nclient != data->nclient)
		return (NTFY_ENUM_NEXT);
	if (data->client_latest != client) {
		(void) fprintf(data->file, "Client handle %X, prioritizer %X",
		    client->nclient, client->prioritizer);
		if (data->type == NOTIFY_DISPATCH &&
		    client->flags & NCLT_EVENT_PROCESSING)
			(void) fprintf(data->file,
			    " (in middle of dispatch):\n");
		else
			(void) fprintf(data->file, ":\n");
		data->client_latest = client;
	}
	(void) fprintf(data->file, "\t");
	switch (cond->type) {
	case NTFY_INPUT:
		(void) fprintf(data->file,
		    "input pending on fd %D", cond->data.fd);
		break;
	case NTFY_OUTPUT:
		(void) fprintf(data->file, "output completed on fd %D",
		    cond->data.fd);
		break;
	case NTFY_EXCEPTION:
		(void) fprintf(data->file, "exception occured on fd %D",
		    cond->data.fd);
		break;
	case NTFY_SYNC_SIGNAL:
		(void) fprintf(data->file, "signal (synchronous) %D",
		    cond->data.signal);
		break;
	case NTFY_ASYNC_SIGNAL:
		(void) fprintf(data->file, "signal (asynchronous) %D",
		    cond->data.signal);
		break;
	case NTFY_REAL_ITIMER:
		(void) fprintf(data->file, "interval timer (real time) ");
		if (data->type == NOTIFY_DETECT) {
			(void) fprintf(data->file, "waiting (%X)",
			    cond->data.ntfy_itimer);
		} else
			(void) fprintf(data->file, "expired");
		break;
	case NTFY_VIRTUAL_ITIMER:
		(void) fprintf(data->file, "interval timer (virtual time) ");
		if (data->type == NOTIFY_DETECT) {
			(void) fprintf(data->file, "waiting (%X)",
			    cond->data.ntfy_itimer);
		} else
			(void) fprintf(data->file, "expired");
		break;
	case NTFY_WAIT3:
		if (data->type == NOTIFY_DETECT)
			(void) fprintf(data->file, "wait3 pid %D",
			    cond->data.pid);
		else
			(void) fprintf(data->file, "wait3 pid %D",
			    cond->data.wait3->pid);
		break;
	case NTFY_SAFE_EVENT:
		(void) fprintf(data->file, "event (safe) %X", cond->data.event);
		break;
	case NTFY_IMMEDIATE_EVENT:
		(void) fprintf(data->file, "event (immediate) %X",
		    cond->data.event);
		break;
	case NTFY_DESTROY:
		(void) fprintf(data->file, "destroy status %X",
		    cond->data.status);
		break;
	default:
		(void) fprintf(data->file, "UNKNOWN %X", cond->data.an_u_int);
		break;
	}
	/* Copy function list, if appropriate */
	if (cond->func_count > 1) {
		(void) fprintf(data->file, "\n\t\tfunctions: %X %X %X %X",
		    cond->callout.functions[0],
		    cond->callout.functions[1],
		    cond->callout.functions[2],
		    cond->callout.functions[3],
		    cond->callout.functions[4]);
		(void) fprintf(data->file,
		    "\n\t\tfunc count %D, func next %D\n",
		    cond->func_count, cond->func_next);
	} else
		(void) fprintf(data->file, ", func: %X\n",
		    cond->callout.function);
	if (data->type == NOTIFY_DISPATCH) {
		if (cond->arg && cond->release)
			(void) fprintf(data->file,
			    "\targ: %X, release func: %X\n",
			    cond->arg, cond->release);
		else if (cond->arg)
			(void) fprintf(data->file, "\targ: %X\n", cond->arg);
		else if (cond->release)
			(void) fprintf(data->file, "\trelease func: %X\n",
			    cond->release);
	}
	return(NTFY_ENUM_NEXT);
}

