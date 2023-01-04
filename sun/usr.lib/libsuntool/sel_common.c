#ifndef lint
static	char sccsid[] = "@(#)sel_common.c 1.4 87/02/24";
#endif

#include <sunwindow/rect.h>		/* LINT_CAST definition */
#include "selection_impl.h"

/*
 *	sel_common.c:	routines which appear in both the service and the 
 *			client library.  (See header of sel_clnt.c for
 *			discussion of service / client terminology in
 *			this package.)
 *
 *	 (C) Copyright 1986 Sun Microsystems, Inc.
 */


/*	initialized data goes into sharable (read-only) text segment
 */

Seln_function_buffer	seln_null_function =	{
    SELN_FN_ERROR, SELN_UNKNOWN,
    SELN_NULL_HOLDER, SELN_NULL_HOLDER,
    SELN_NULL_HOLDER, SELN_NULL_HOLDER
};
Seln_holder		seln_null_holder =	{
    SELN_NULL_HOLDER
};
Seln_request            seln_null_request =	{
    0, {0, 0}, 0, SELN_UNKNOWN, SELN_FAILED, 0, 0
};


/*	Basic information about how this process communicates: 
 */

Seln_access	 seln_my_access;	/* How requesters contact us	*/
int		 seln_my_tcp_socket;
int		 seln_transient_tcp_socket;
int		 seln_my_udp_socket;
SVCXPRT		*seln_my_tcp_transport;	/* How we answer		*/
SVCXPRT		*seln_my_udp_transport;

static int	 seln_my_open_tcps;	/* mask of fds that are open	*/

Seln_client_node clients;

/*
 *	External procedures (called by client code)
 *
 *			First, some predicates
 *
 *
 *	Holder_same_client:
 *		TRUE if argument access struct identifies this client
 *		(valid only in the client's address space)
 */
int
seln_holder_same_client(holder, client_data)
    register Seln_holder *holder;
    register char        *client_data;
{

    return (holder != (Seln_holder *) NULL		&&
	    holder->access.pid == seln_my_access.pid	&&
	    ((Seln_client_node *) (LINT_CAST(holder->access.client)))->client_data
		    == client_data);
}

/*
 * Holder_same_process:
 *	Return TRUE if argument access struct identifies same process 
 */
int
seln_holder_same_process(holder)
    register Seln_holder *holder;
{
    return (holder != (Seln_holder *) NULL &&
	    holder->access.pid == seln_my_access.pid);
}

/*
 * Same_holder: Return TRUE if 2 holders are the same (incl. rank & state)
 */
int
seln_same_holder(h1, h2)
register Seln_holder *h1, *h2;
{
    if (h1 == (Seln_holder *) NULL || h2 == (Seln_holder *) NULL) {
	return FALSE;
    }
    return (h1->rank == h2->rank && h1->state == h2->state &&
	     seln_equal_access(&h1->access, &h2->access));
}

/*
 *	Return TRUE if two holder structs agree in all elements, else FALSE
 */
int
seln_equal_access(first, second)
    register Seln_access *first;
    register Seln_access *second;
{

    if (first == (Seln_access *) NULL || second == (Seln_access *) NULL) {
	return FALSE;
    }
    return (first->pid == second->pid					&&
	    first->program == second->program				&&
	    first->client == second->client				&&
	    seln_equal_addrs(&first->tcp_address, &second->tcp_address)	&&
	    seln_equal_addrs(&first->udp_address, &second->udp_address));
}

/*  Secondary_made:  true if this buffer indicates a secondary selection
 *	has been made since the function key went down (whether or not
 *	one exists now).
 */
int
seln_secondary_made(buffer)
    Seln_function_buffer  *buffer;
{
    return (buffer->secondary.access.pid != 0);
}

/*  Secondary_exists:  true if this buffer indicates a secondary selection
 *	existed at the time the function key went up.
 */
int
seln_secondary_exists(buffer)
    Seln_function_buffer  *buffer;
{
    return (buffer->secondary.state != SELN_NONE);
}

/*
 *	package-internal routines:
 *
 */

extern int	svc_fds;
static void
seln_adjust_fds(saved)
    int			saved;
{
    register int	opened;
    register int	closed;
    register int	fd;

    opened = svc_fds & ~saved;
    fd = 0;
    while (opened != 0) {
	if (opened & 1) {
	    (void) notify_set_input_func((Notify_client)seln_my_tcp_transport,
					 seln_listener, fd);
	    seln_my_open_tcps |= 1 << fd;
	}
	fd++;
	opened >>= 1;
    }
    closed = saved & ~svc_fds;
    fd = 0;
    while (closed != 0) {
	if (closed & 1) {
	    (void) notify_set_input_func((Notify_client)seln_my_tcp_transport,
					 NOTIFY_FUNC_NULL, fd);
	    seln_my_open_tcps &= ~(1 << fd);
	}
	fd++;
	closed >>= 1;
    }
}

Seln_result
seln_send_yield(rank, holder)
    Seln_rank             rank;
    Seln_holder          *holder;
{
    register CLIENT      *client;
    int                   sock = RPC_ANYSOCK;
    enum clnt_stat        rpc_result;
    Seln_request          buffer;
    Seln_replier_data     replier;
    char                **requestp;

    buffer.replier = 0;
    buffer.requester.consume = 0;
    buffer.requester.context = 0;
    buffer.addressee = holder->access.client;
    buffer.rank = rank;
    buffer.status = SELN_SUCCESS;
    buffer.buf_size = 3 * sizeof (char *);
    requestp = (char **) (LINT_CAST(buffer.data));
    *requestp++ = (char *) SELN_REQ_YIELD;
    *requestp++ = 0;
    *requestp++ = 0;

    if (seln_holder_same_process(holder)) {
	buffer.replier = &replier;
	replier.client_data =
	    ((Seln_client_node *)(LINT_CAST(holder->access.client)))->client_data;
	replier.rank = holder->rank;
	replier.context = 0;
	replier.request_pointer = (char **) (LINT_CAST(buffer.data));
	if (seln_get_reply_buffer(&buffer) != SELN_SUCCESS) {
	    return SELN_FAILED;
	}
    } else {
	if ((client = clntudp_bufcreate(&holder->access.udp_address,
				        holder->access.program, SELN_VERSION,
				        seln_std_timeout, &sock,
				        SELN_RPC_BUFSIZE, SELN_RPC_BUFSIZE))
		== (CLIENT *) NULL) {
	    return SELN_FAILED;
	}
	rpc_result = clnt_call(client, SELN_CLNT_REQUEST,
			       xdr_seln_request, &buffer,
			       xdr_seln_reply, &buffer,
			       seln_std_timeout);
	clnt_destroy(client);
	(void)close(sock);
	if (rpc_result == RPC_TIMEDOUT &&
	    seln_std_timeout.tv_sec == 0 &&
	    seln_std_timeout.tv_usec == 0) {
	    return SELN_SUCCESS;
	}
	if (rpc_result != RPC_SUCCESS) {
	    return SELN_FAILED;
	}
    }
    requestp = (char **) (LINT_CAST(buffer.data));
    if (*requestp++ != (char *) SELN_REQ_YIELD) {
	return SELN_FAILED;
    }
    return (Seln_result) *requestp;
}

/*	seln_listener is called by (window system) notifier
 */
/* ARGSUSED */
Notify_value
seln_listener(handle, fd)
    caddr_t         handle;
    int             fd;
{
    int             saved_fds;

    saved_fds = svc_fds;
    svc_getreq(1 << fd);		/* it calls the rpc dispatcher	 */
    if (fd == seln_my_tcp_socket || seln_my_open_tcps & (1 << fd))
	seln_adjust_fds(saved_fds);
    return NOTIFY_DONE;
}

/*
 *	Call from service to tell us a function needs to be executed
 */
void
seln_do_function(transport)
    SVCXPRT                   *transport;
{
    Seln_function_buffer       buffer;
    register Seln_client_node *client;

    if (!svc_getargs(transport, xdr_seln_function, &buffer)) {
	svcerr_decode(transport);
	return;
    }
    switch (buffer.addressee_rank) {
      case SELN_CARET:
	client = (Seln_client_node *) (LINT_CAST(buffer.caret.access.client));
	break;
      case SELN_PRIMARY:
	client = (Seln_client_node *) (LINT_CAST(buffer.primary.access.client));
	break;
      case SELN_SECONDARY:
	client = (Seln_client_node *) (LINT_CAST(buffer.secondary.access.client));
	break;
      case SELN_SHELF:
	client = (Seln_client_node *) (LINT_CAST(buffer.shelf.access.client));
	break;
      default:
	svcerr_decode(transport);
	return;
    }
    if (seln_verify_clnt_alive(client) == 0)  {
        /* the client is destroyed already, so just register error */
	svcerr_auth(transport,AUTH_REJECTEDCRED);
	return;
    }
    client->ops.do_function(client->client_data, &buffer);
}

/*
 *	Respond to a request from another process
 */
void
seln_do_reply(transport)
    SVCXPRT              *transport;
{
    Seln_request          request;
    Seln_request          reply;
    Seln_replier_data     replier;

    if (!svc_getargs(transport, xdr_seln_request, &request)) {
	svcerr_decode(transport);
	return;
    }
    (void)seln_init_reply(&request, &reply, &replier);
    if (seln_verify_clnt_alive(request.addressee) == 0)  {
        /* the client is destroyed already, so just regiester error */
	svcerr_auth(transport,AUTH_REJECTEDCRED);
	return;
    }
    if (!svc_sendreply(transport, xdr_seln_reply, 
    		(char *)(LINT_CAST(&reply)))) {
	/* failure usually is refusal of requester to accept continuation  */
	*replier.response_pointer = (char *)SELN_REQ_END_REQUEST;
	((Seln_client_node *) (LINT_CAST((request.addressee))))->ops.do_request(
		SELN_REQ_END_REQUEST, &replier, 0);
    }
}

Seln_result
seln_init_reply(request, reply, replier)
    Seln_request         *request;
    Seln_request         *reply;
    Seln_replier_data    *replier;
{
    *reply = *request;
    reply->status = SELN_SUCCESS;
    reply->requester = request->requester;
    reply->replier = replier;
    replier->client_data =
		((Seln_client_node *) (LINT_CAST((request->addressee))))->client_data;
    replier->rank = request->rank;
    replier->context = 0;
    replier->request_pointer = (char **)(LINT_CAST(request->data));    
}

Seln_result
seln_get_reply_buffer(buffer)
    Seln_request         *buffer;
{
    Seln_client_node     *client;
    int                   buf_len;
    char                 *request;
    char                 *limit;

    client = (Seln_client_node *) (LINT_CAST(buffer->addressee));
    limit = buffer->data + SELN_BUFSIZE - 2*sizeof(Seln_attribute);
    buffer->replier->response_pointer = (char **)(LINT_CAST(buffer->data));
    while ((request = *buffer->replier->request_pointer++) != 0) {
	if (buffer->status != SELN_CONTINUED) {
	    *buffer->replier->response_pointer++ = request;
	}
	buf_len = limit  - (char *) buffer->replier->response_pointer;
	switch (client->ops.do_request(request, buffer->replier, buf_len)) {
	  case SELN_SUCCESS:		/*  go round again	*/
	    buffer->status = SELN_SUCCESS;
	    break;
	  case SELN_CONTINUED:		/*  1 buffer filled	*/
	    buffer->buf_size = (char *) buffer->replier->response_pointer
				- buffer->data;
	    *buffer->replier->response_pointer++ = 0;
	    buffer->replier->response_pointer = (char **) (LINT_CAST(buffer->data));
	    buffer->replier->request_pointer -= 1;
	    buffer->status = SELN_CONTINUED;
	    return SELN_SUCCESS;
	  case SELN_UNRECOGNIZED:
	    buffer->replier->response_pointer[-1] = (char *) SELN_REQ_UNKNOWN;
	    *buffer->replier->response_pointer++ = request;
	    buffer->status = SELN_SUCCESS;
	    break;
	  case SELN_DIDNT_HAVE:
	    buffer->replier->response_pointer[-1] = 0;
	    buffer->status = SELN_DIDNT_HAVE;
	    return SELN_SUCCESS;
	  default:			/*  failure - quit	 */
	    buffer->replier->response_pointer[-1] = 0;
	    buffer->status = SELN_FAILED;
	    return SELN_FAILED;
	}
	buffer->replier->request_pointer =
	    attr_skip_value((unsigned)request, buffer->replier->request_pointer);
    }
    (void)client->ops.do_request(SELN_REQ_END_REQUEST, buffer->replier, 0);
    buffer->status = SELN_SUCCESS;
    *buffer->replier->response_pointer++ = 0;
    buffer->buf_size = (char *) buffer->replier->response_pointer
			- buffer->data;
    return SELN_SUCCESS;
}


/*
 *  Package-internal utilities
 *
 *	Return TRUE if two sockaddr_in structs agree in all elements 
 */
int
seln_equal_addrs(a, b)
    struct sockaddr_in *a, *b;
{
    return (a->sin_family == b->sin_family  &&
	    a->sin_port == b->sin_port	    &&
	    bcmp((char *)(LINT_CAST(&a->sin_addr)), 
	    	 (char *)(LINT_CAST(&b->sin_addr)), sizeof(a->sin_addr)) == 0);
}

/*
 *	Initialize the TCP connection (used fro requests by any holder
 *	    (includes service, for file-selections).
 */
Seln_result
seln_get_tcp(prognum, dispatcher)
    int                   prognum;
    void		(*dispatcher)();
{

    if ((seln_my_tcp_transport = svctcp_create(RPC_ANYSOCK,
    					       sizeof(Seln_request),
    					       sizeof(Seln_request)))
	== (SVCXPRT *) NULL) {
	complain("Couldn't get a TCP transport for requesters to talk to us");
	return SELN_FAILED;
    }
    seln_my_tcp_socket = seln_my_tcp_transport->xp_sock;
    get_myaddress(&seln_my_access.tcp_address);
    seln_my_access.tcp_address.sin_port =  seln_my_tcp_transport->xp_port;
    if (!svc_register(seln_my_tcp_transport, prognum,
		      SELN_VERSION, dispatcher, NO_PMAP)) {
	complain("Couldn't register transport for requesters to talk to us");
	svc_destroy(seln_my_tcp_transport);
	(void)close(seln_my_tcp_socket);
	return SELN_FAILED;
    }
    (void) notify_set_input_func((Notify_client) seln_my_tcp_transport,
				 seln_listener, seln_my_tcp_socket);
    return SELN_SUCCESS;
}

/*
 *	routines & data for keeping track of the state of the function keys
 */
 
Seln_functions_state functions;

void
seln_function_clear(function)
    register Seln_function function;
{
    register u_int        i, j;

    i = (u_int)function / (sizeof(functions.data[0]) * 8);
    if (i < SELN_FUNCTION_WORD_COUNT)  {
	j = (u_int)function % (sizeof(functions.data[0]) * 8);
	functions.data[i] &= ~(1 << j);
    }
}

void
seln_function_set(function)
    register Seln_function function;
{
    register u_int        i, j;

    i = (u_int)function / (sizeof(functions.data[0]) * 8);
    if (i < SELN_FUNCTION_WORD_COUNT)  {
	j = (u_int)function % (sizeof(functions.data[0]) * 8);
	functions.data[i] |= (1 << j);
    }
}

int
seln_function_state(function)
    register Seln_function function;
{
    register u_int        i, j;

    i = (u_int)function / (sizeof(functions.data[0]) * 8);
    if (i < SELN_FUNCTION_WORD_COUNT)  {
	j = (u_int)function % (sizeof(functions.data[0]) * 8);
	return ((functions.data[i] & (1 << j)) != 0);
    } else {
	return 0;
    }
}

int
seln_function_pending()
{
    register u_int        i;

    for (i = 0; i < SELN_FUNCTION_WORD_COUNT; i++) {
	if (functions.data[i] != 0)
	    return TRUE;
    }
    return FALSE;
}

void
seln_function_reset()
{
    register u_int        i;

    for (i = 0; i < SELN_FUNCTION_WORD_COUNT; i++) {
	functions.data[i] = 0;
    }
}

Seln_functions_state
seln_functions_all()
{
    return functions;
}


/* Given client, seln_verify_clnt_alive returns 1 if the client is alive
 * 0 if dead.
 */
static 
seln_verify_clnt_alive(client)
Seln_client_node *client;
{
    
    Seln_client_node *temp;
    
    for (temp = &clients; temp != NULL; temp=temp->next){
        /* first check if the client address is the same */
        if (temp == client)  {
	    /* now check if the serial id number is the same.
	     * if so, then it's the same client; otherwise, the original
	     * client is probably dead, and a new client owns the same
	     * vm address
	     */
	     if (temp->client_num == client->client_num)  {
	         return(1);
	     }   else  {
	         return(0);
	     }
	}
    }
    return(0);
}
